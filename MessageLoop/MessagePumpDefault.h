// Author: WangZhan -> wangzhan.1985@gmail.com
#pragma once
#include "BasicTypes.h"
#include "MessagePump.h"
#include "WaitableEvent.h"

class MessagePumpDefault : public MessagePump
{
public:
    MessagePumpDefault();
    ~MessagePumpDefault();

    virtual void Run(Delegate *pDelegate);
    virtual void Quit();
    virtual void ScheduleWork();
    virtual void ScheduleDelayedWork(DWORD dwDelayedWorkTime);

private:
    bool m_bShouldQuit;
    DWORD m_dwDelayedWorkTime;
    WaitableEvent m_event;

    DISALLOW_COPY_AND_ASSIGN(MessagePumpDefault);
};

