#include "stdafx.h"
#include "MessageLoopThread.h"

MessageLoopThread::MessageLoopThread(const tstring &csThreadName)
: m_csThreadName(csThreadName)
, m_bStarted(false)
, m_bStopping(false)
, m_hThreadHandle(0)
, m_dwThreadID(0)
, m_pMessageLoop(NULL)
{
}

MessageLoopThread::~MessageLoopThread()
{
    Stop();
}

bool MessageLoopThread::Start()
{
    Options op;
    return StartWithOptions(op);
}

bool MessageLoopThread::StartWithOptions(const Options &options)
{
    if (m_bStarted)
    {
        return true;
    }

    StartupData data(options);
    m_pStartupData = &data;
    m_hThreadHandle = CreateThread(0, options.iStackSize, ThreadProc, this, 0,
        &m_dwThreadID);

    // 保证messagepumploop完成初始化
    m_pStartupData->event.Wait();
    m_pStartupData = NULL;

    m_bStarted = m_hThreadHandle != 0;
    return m_bStarted;
}

void MessageLoopThread::Stop()
{
    if (!m_bStarted || m_bStopping)
    {
        return;
    }

    m_bStopping = true;
    m_pMessageLoop->QuitByTaskClosure();
    
    WaitForSingleObject(m_hThreadHandle, INFINITE);
    CloseHandle(m_hThreadHandle);

    m_hThreadHandle = NULL;
    m_dwThreadID = 0;
    m_bStarted = false;
    m_bStopping = false;
}

void MessageLoopThread::ThreadMain()
{
    // try to use stack, static_cast of the object
    MessageLoop messageLoop(m_pStartupData->option.messageLoopType);
    m_pMessageLoop = &messageLoop;

    Initialize();

    m_pStartupData->event.Signal();

    m_pMessageLoop->Run();

    CleanUp();
    m_pMessageLoop = NULL;
}

DWORD MessageLoopThread::ThreadProc(void *pParam)
{
    if (pParam)
    {
        MessageLoopThread *pThread = (MessageLoopThread*)pParam;
        pThread->ThreadMain();
    }
    return 0;
}
