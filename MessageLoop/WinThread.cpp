#include "stdafx.h"
#include "WinThread.h"


WinThread::WinThread(const tstring &csThreadName)
: m_csThreadName(csThreadName)
, m_bStarted(false)
, m_bStopping(false)
, m_dwThreadID(0)
, m_hThreadHandle(NULL)
{
}

WinThread::WinThread(const tstring &csThreadName, const Options &options)
: m_Options(options)
, m_csThreadName(csThreadName)
, m_bStarted(false)
, m_bStopping(false)
, m_dwThreadID(0)
, m_hThreadHandle(NULL)
{
}

WinThread::~WinThread()
{
    Stop();
}

bool WinThread::Start()
{
    m_hThreadHandle = CreateThread(NULL, m_Options.iStackSize, ThreadProc, this,
        0, &m_dwThreadID);
    m_bStarted = m_hThreadHandle != 0;
    return m_bStarted;
}

void WinThread::Stop()
{
    if (!m_bStarted || m_bStopping)
    {
        return;
    }

    m_bStopping = true;

    Join();

    CloseHandle(m_hThreadHandle);
    m_hThreadHandle = NULL;
    m_dwThreadID = 0;
    m_bStarted = false;
    m_bStopping = false;
}

void WinThread::Join()
{
    WaitForSingleObject(m_hThreadHandle, INFINITE);
}

DWORD WinThread::ThreadProc(void *pParam)
{
    if (pParam)
    {
        WinThread *pThis = (WinThread*)pParam;
        pThis->ThreadMain();
    }

    return 0;
}
