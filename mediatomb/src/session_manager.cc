/*  session_manager.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "uuid/uuid.h"
#include "session_manager.h"
#include "config_manager.h"
#include "tools.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

static Ref<SessionManager> inst;

Ref<SessionManager> SessionManager::getInstance()
{
    if (inst == nil)
    {
        mutex.lock();
        if (inst == nil)
        {
            try
            {
                inst = Ref<SessionManager>(new SessionManager());
            }
            catch (Exception e)
            {
                mutex.unlock();
                throw e;
            }
        }
        mutex.unlock();
    }
    return inst;
}

Session::Session(long timeout) : Dictionary()
{
    this->timeout = timeout;
    loggedIn = false;
    last_access = 0; /// \todo set to right value
    sessionID = nil;
}

Mutex SessionManager::mutex = new Mutex();

SessionManager::SessionManager() : Object()
{
    Ref<ConfigManager> configManager = ConfigManager::getInstance();
    Ref<Element> accountsEl = configManager->getElement(_("/server/ui/accounts"));
    if (accountsEl == nil)
    {
        log_debug("accounts tag not found in config\n");
        accounts = Ref<Dictionary>(new Dictionary());
    }
    else
    {
        accounts = configManager->createDictionaryFromNodeset(accountsEl, _("account"), _("user"), _("password"));
    }
    sessions = Ref<Array<Session> >(new Array<Session>());
}

Ref<Session> SessionManager::createSession(long timeout, String sessionID)
{
    Ref<Session> newSession(new Session(timeout));
    mutex.lock();
    if (sessionID == nil)
    {
        int count=0;
        do
        {
            sessionID = generate_random_id();
            if (count++ > 100)
                throw _Exception(_("There seems to be something wrong with the random numbers. I tried to get a unique id 100 times and failed."));
        }
        while(getSession(sessionID) != nil); // for the rare case, where we get a random id, that is already taken
    }
    else if (getSession(sessionID) != nil)
    {
        throw _Exception(_("Session id is already taken!"));
    }
    newSession->setID(sessionID);
    sessions->append(newSession);
    mutex.unlock();
    return newSession;
}

Ref<Session> SessionManager::getSession(String sessionID)
{
    mutex.lock();
    for (int i = 0; i < sessions->size(); i++)
    {
        Ref<Session> s = sessions->get(i);
        if (s->getID() == sessionID)
        {
            mutex.unlock();
            return s;
        }
    }
    mutex.unlock();
    return nil;
}

void SessionManager::removeSession(String sessionID)
{
    mutex.lock();
    for (int i = 0; i < sessions->size(); i++)
    {
        Ref<Session> s = sessions->get(i);
        if (s->getID() == sessionID)
        {
            sessions->remove(i);
            mutex.unlock();
            return;
        }
    }
    mutex.unlock();
}

String SessionManager::getUserPassword(String user)
{
    if (accounts == nil)
        return nil;
    return accounts->get(user);
}
