// Author: WangZhan -> wangzhan.1985@gmail.com
#pragma once
#include <vector>
#include <queue>
#include "AutoLock.h"
#include "WaitableEvent.h"
#include "WinThread.h"


class DelegateWinThread : public WinThread
{
public:
    struct Delegate
    {
        virtual ~Delegate() {}
        virtual void Run() = 0;
    };

    DelegateWinThread(const tstring &csThreadName, Delegate *pDelegate)
        : WinThread(csThreadName), m_pDelegate(pDelegate)
    {
        ZhanAssert(m_pDelegate);
    }

    DelegateWinThread(const tstring &csThreadName, const Options &options,
        Delegate *pDelegate)
        : WinThread(csThreadName, options), m_pDelegate(pDelegate)
    {
        ZhanAssert(m_pDelegate);
    }

    ~DelegateWinThread() {}

    virtual void ThreadMain() { m_pDelegate->Run(); }

private:
    Delegate *m_pDelegate;
};


class DelegateWinThreadPool : public DelegateWinThread::Delegate
{
public:
    typedef DelegateWinThread::Delegate Delegate;

    DelegateWinThreadPool(const tstring &csThreadName, int iNumOfThreads);
    ~DelegateWinThreadPool();

    void Start();
    void JoinAll();

    virtual void Run();

    void AddWork(Delegate *pDelegate);

private:
    int m_iNumOfThreads;
    tstring m_csThreadName;

    WaitableEvent m_Event;
    LockGuard m_Lock;
    std::queue<Delegate*> m_WorkQueue;
    std::vector<DelegateWinThread*> m_vecDelegateThread;
};

