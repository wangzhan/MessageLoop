#include "stdafx.h"
#include "WaitableEvent.h"


WaitableEvent::WaitableEvent(bool bManualReset, bool bInitialState)
{
    m_handle = CreateEvent(NULL, bManualReset, bInitialState, NULL);
}


WaitableEvent::~WaitableEvent()
{
    CloseHandle(m_handle);
}

void WaitableEvent::Reset()
{
    ResetEvent(m_handle);
}

void WaitableEvent::Signal()
{
    SetEvent(m_handle);
}

bool WaitableEvent::IsSignaled()
{
    return TimedWait(0);
}

void WaitableEvent::Wait()
{
    WaitForSingleObject(m_handle, INFINITE);
}

bool WaitableEvent::TimedWait(DWORD dwTime)
{
    DWORD dwRet = WaitForSingleObject(m_handle, dwTime);
    return dwRet == WAIT_OBJECT_0;
}
