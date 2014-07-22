// Author: WangZhan -> wangzhan.1985@gmail.com
#pragma once
#include <vector>
#include "MessagePumpObserver.h"

class MessagePump
{
public:
    class Delegate
    {
    public:
        virtual ~Delegate() {}
        
        virtual bool DoWork() = 0;
        virtual bool DoDelayedWork(DWORD *pDelayedWorkTime) = 0;
        virtual bool DoIdleWork() = 0;
    };

    MessagePump();
    virtual ~MessagePump();

    virtual void Run(Delegate *pDelegate) = 0;
    virtual void Quit() = 0;

    // can be called by any thread
    virtual void ScheduleWork() = 0;

    // only be called by created thread
    virtual void ScheduleDelayedWork(DWORD dwDelayedWorkTime) = 0;
};


class MessagePumpForMsg : public MessagePump
{
public:
    MessagePumpForMsg();
    ~MessagePumpForMsg();

    void AddObserver(MessagePumpObserver *pObserver);
    void RemoveObserver(MessagePumpObserver *pObserver);

    void WillProcessMessage(MSG &msg);
    void DidProcessMessage(MSG &msg);

    virtual void Run(Delegate *pDelegate);
    virtual void Quit();

protected:
    struct RunState
    {
        Delegate *pDelegate;
        bool bShouldQuit;
        int iRunDepth;
    };

    virtual void DoRunLoop() = 0;
    int GetCurrentDelay();

    MESSAGE_PUMP_OBSERVER_VECT m_vecObservers;
    DWORD m_dwDelayedWorkTime;
    LONG m_lHaveWork;
    RunState *m_pState;
};
