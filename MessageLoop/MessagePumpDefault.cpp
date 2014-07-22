#include "stdafx.h"
#include "MessagePumpDefault.h"


MessagePumpDefault::MessagePumpDefault()
: m_bShouldQuit(false)
, m_dwDelayedWorkTime(0)
, m_event(false, false)
{
}

MessagePumpDefault::~MessagePumpDefault()
{
}

void MessagePumpDefault::Run(Delegate *pDelegate)
{
    ZhanAssert(pDelegate);

    for (;;)
    {
        bool bDidWork = pDelegate->DoWork();
        if (m_bShouldQuit)
        {
            break;
        }

        bDidWork |= pDelegate->DoDelayedWork(&m_dwDelayedWorkTime);
        if (m_bShouldQuit)
        {
            break;
        }

        if (bDidWork)
        {
            continue;
        }

        bDidWork = pDelegate->DoIdleWork();
        if (m_bShouldQuit)
        {
            break;
        }

        if (bDidWork)
        {
            continue;
        }

        DWORD dwTimeout = 0;
        DWORD dwCur = GetTickCount();
        if (m_dwDelayedWorkTime && m_dwDelayedWorkTime > dwCur)
        {
            dwTimeout = m_dwDelayedWorkTime - dwCur;
        }

        if (dwTimeout == 0)
        {
            m_event.Wait();
        }
        else
        {
            m_event.TimedWait(dwTimeout);
        }
    }
    m_bShouldQuit = false;
}

void MessagePumpDefault::Quit()
{
    m_bShouldQuit = true;
}

void MessagePumpDefault::ScheduleWork()
{
    m_event.Signal();
}

void MessagePumpDefault::ScheduleDelayedWork(DWORD dwDelayedWorkTime)
{
    m_dwDelayedWorkTime = dwDelayedWorkTime;
}
