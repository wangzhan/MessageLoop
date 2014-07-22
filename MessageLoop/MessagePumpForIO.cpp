#include "stdafx.h"
#include "MessagePumpForIO.h"


MessagePumpForIO::MessagePumpForIO()
{
    m_IOCompletionPort.Set(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1));
    ZhanAssert(m_IOCompletionPort);
}

MessagePumpForIO::~MessagePumpForIO()
{
}

void MessagePumpForIO::RegisterIOHandler(HANDLE fileHandle, IOHandler *pHandler)
{
    CreateIoCompletionPort(fileHandle, m_IOCompletionPort, reinterpret_cast<ULONG_PTR>(pHandler), 1);
}

void MessagePumpForIO::ScheduleWork()
{
    if (InterlockedExchange(&m_lHaveWork, 1))
    {
        return;
    }

    PostQueuedCompletionStatus(m_IOCompletionPort, 0, reinterpret_cast<ULONG_PTR>(this),
        reinterpret_cast<LPOVERLAPPED>(this));
}

void MessagePumpForIO::ScheduleDelayedWork(DWORD dwDelayedWorkTime)
{
    m_dwDelayedWorkTime = dwDelayedWorkTime;
}

void MessagePumpForIO::DoRunLoop()
{
    for (;;)
    {
        bool bDidWork = m_pState->pDelegate->DoWork();
        if (m_pState->bShouldQuit)
        {
            break;
        }

        bDidWork |= WaitForIOCompletion(0);
        if (m_pState->bShouldQuit)
        {
            break;
        }

        bDidWork |= m_pState->pDelegate->DoDelayedWork(&m_dwDelayedWorkTime);
        if (m_pState->bShouldQuit)
        {
            break;
        }

        if (bDidWork)
        {
            continue;
        }

        bDidWork = m_pState->pDelegate->DoIdleWork();
        if (m_pState->bShouldQuit)
        {
            break;
        }

        if (bDidWork)
        {
            continue;
        }

        WaitForWork();
    }
}

bool MessagePumpForIO::WaitForWork()
{
    int iTimeout = GetCurrentDelay();
    if (iTimeout < 0)
    {
        iTimeout = INFINITE;
    }

    return WaitForIOCompletion(iTimeout);
}

bool MessagePumpForIO::WaitForIOCompletion(DWORD dwTimeout)
{
    DWORD dwBytesTransfered = 0;
    ULONG_PTR pKey = NULL;
    LPOVERLAPPED pOverlapped = NULL;
    DWORD dwError = 0;
    bool bRes = !!GetQueuedCompletionStatus(m_IOCompletionPort, &dwBytesTransfered,
        &pKey, &pOverlapped, dwTimeout);
    if (!bRes)
    {
        if (!pOverlapped)
        {
            // outputdebugstring
            return false;
        }
    }

    if ((this == reinterpret_cast<MessagePumpForIO*>(pKey)) &&
        (this == reinterpret_cast<MessagePumpForIO*>(pOverlapped)))
    {
        // work msg
        InterlockedExchange(&m_lHaveWork, 0);
        return true;
    }

    // do event
    dwError = GetLastError();
    IOHandler *pIOHandler = reinterpret_cast<IOHandler*>(pKey);
    IOContext *pIOContext = reinterpret_cast<IOContext*>(pOverlapped);
    if (pIOContext->pHandler)
    {
        pIOHandler->OnIOCompleted(pIOContext, dwBytesTransfered, dwError);
    }
    else
    {
        delete pIOContext;
    }

    return true;
}
