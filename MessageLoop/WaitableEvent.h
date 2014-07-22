// Author: WangZhan -> wangzhan.1985@gmail.com
#pragma once
#include "BasicTypes.h"

class WaitableEvent
{
public:
    WaitableEvent(bool bManualReset, bool bInitialState);
    ~WaitableEvent();

    void Reset();
    void Signal();

    bool IsSignaled();

    void Wait();

    // dwTime: ms
    bool TimedWait(DWORD dwTime);

private:
    HANDLE m_handle;

    DISALLOW_COPY_AND_ASSIGN(WaitableEvent);
};

