#include "stdafx.h"
#include "MessagePump.h"

// MessagePump implementation
MessagePump::MessagePump()
{
}

MessagePump::~MessagePump()
{
}


// MessagePumpForMsg implementation
MessagePumpForMsg::MessagePumpForMsg()
: m_lHaveWork(0)
, m_pState(NULL)
, m_dwDelayedWorkTime(0)
{
}

MessagePumpForMsg::~MessagePumpForMsg()
{
}

void MessagePumpForMsg::AddObserver(MessagePumpObserver *pObserver)
{
    m_vecObservers.push_back(pObserver);
}

void MessagePumpForMsg::RemoveObserver(MessagePumpObserver *pObserver)
{
    for (MESSAGE_PUMP_OBSERVER_VECT_ITER iter = m_vecObservers.begin();
        iter != m_vecObservers.end(); ++iter)
    {
        if (*iter == pObserver)
        {
            m_vecObservers.erase(iter);
            break;
        }
    }
}

void MessagePumpForMsg::WillProcessMessage(MSG &msg)
{
    FOR_EACH_PUMP_OBSERVER(m_vecObservers, WillProcessMessage(msg));
}

void MessagePumpForMsg::DidProcessMessage(MSG &msg)
{
    FOR_EACH_PUMP_OBSERVER(m_vecObservers, DidProcessMessage(msg));
}

void MessagePumpForMsg::Run(Delegate *pDelegate)
{
    RunState state = RunState();
    state.pDelegate = pDelegate;
    state.bShouldQuit = false;
    state.iRunDepth = m_pState ? m_pState->iRunDepth + 1 : 1;

    RunState *pPreviousState = m_pState;
    m_pState = &state;

    DoRunLoop();

    m_pState = pPreviousState;
}

void MessagePumpForMsg::Quit()
{
    ZhanAssert(m_pState);
    m_pState->bShouldQuit = true;
}

int MessagePumpForMsg::GetCurrentDelay()
{
    if (m_dwDelayedWorkTime == 0)
    {
        return -1;
    }

    DWORD dwTimeout = m_dwDelayedWorkTime - GetTickCount();
    if (dwTimeout < 0)
    {
        dwTimeout = 0;
    }
    return (int)dwTimeout;
}
