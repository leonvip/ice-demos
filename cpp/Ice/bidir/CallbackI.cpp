// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************

#include <Ice/Ice.h>
#include <CallbackI.h>

using namespace std;
using namespace Ice;
using namespace Demo;

CallbackSenderI::CallbackSenderI(const Ice::CommunicatorPtr& communicator) :
    _communicator(communicator), _destroy(false)
{
}

void
CallbackSenderI::destroy()
{
    {
        IceUtil::Monitor<IceUtil::Mutex>::Lock lck(*this);

        cout << "destroying callback sender" << endl;
        _destroy = true;

        notify();
    }

    getThreadControl().join();
}

void
CallbackSenderI::addClient(const Identity& ident, const Current& current)
{
    IceUtil::Monitor<IceUtil::Mutex>::Lock lck(*this);

    cout << "adding client `" << _communicator->identityToString(ident) << "'"<< endl;

    CallbackReceiverPrx client = CallbackReceiverPrx::uncheckedCast(current.con->createProxy(ident));
    _clients.insert(client);
}

void
CallbackSenderI::run()
{
    int num = 0;
    bool destroyed = false;
    while(!destroyed)
    {
        std::set<Demo::CallbackReceiverPrx> clients;
        {
            IceUtil::Monitor<IceUtil::Mutex>::Lock lck(*this);
            timedWait(IceUtil::Time::seconds(2));

            if(_destroy)
            {
                destroyed = true;
                continue;
            }

            clients = _clients;
        }

        if(!clients.empty())
        {
            ++num;
            for(set<CallbackReceiverPrx>::iterator p = clients.begin(); p != clients.end(); ++p)
            {
                try
                {
                    (*p)->callback(num);
                }
                catch(const Exception& ex)
                {
                    cerr << "removing client `" << _communicator->identityToString((*p)->ice_getIdentity()) << "':\n"
                         << ex << endl;

                    IceUtil::Monitor<IceUtil::Mutex>::Lock lck(*this);
                    _clients.erase(*p);
                }
            }
        }
    }
}
