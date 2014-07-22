#include "stdafx.h"
#include "DelegateWinThreadPool.h"


DelegateWinThreadPool::DelegateWinThreadPool(const tstring &csThreadName, int iNumOfThreads)
: m_csThreadName(csThreadName)
, m_iNumOfThreads(iNumOfThreads)
, m_Event(true, false)
{

}

DelegateWinThreadPool::~DelegateWinThreadPool()
{
}

void DelegateWinThreadPool::Start()
{
    for (int i = 0; i < m_iNumOfThreads; ++i)
    {
        DelegateWinThread *pThread = new DelegateWinThread(m_csThreadName, this);
        pThread->Start();
        m_vecDelegateThread.push_back(pThread);
    }
}

void DelegateWinThreadPool::JoinAll()
{
    for (int i = 0; i < m_iNumOfThreads; ++i)
    {
        AddWork(NULL);
    }

    for (auto iter = m_vecDelegateThread.begin(); iter != m_vecDelegateThread.end(); ++iter)
    {
        DelegateWinThread *pThread = *iter;
        pThread->Stop();
        delete pThread;
    }
}

void DelegateWinThreadPool::Run()
{
    for (;;)
    {
        m_Event.Wait();

        Delegate *pDelegate = NULL;
        {
            AutoLock autoLock(m_Lock);
            if (m_WorkQueue.empty())
            {
                m_Event.Reset();
                continue;
            }

            pDelegate = m_WorkQueue.front();
            m_WorkQueue.pop();
        }

        if (pDelegate == NULL)
        {
            break;
        }
        pDelegate->Run();
    }
}

void DelegateWinThreadPool::AddWork(Delegate *pDelegate)
{
    AutoLock autoLock(m_Lock);
    m_WorkQueue.push(pDelegate);

    if (!m_Event.IsSignaled())
    {
        m_Event.Signal();
    }
}
