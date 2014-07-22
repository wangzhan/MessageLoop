// Author: WangZhan -> wangzhan.1985@gmail.com
#include "stdafx.h"
#include <memory>
#include "../MessageLoop/TaskClosure.h"
#include "gtest/gtest.h"
#include "../MessageLoop/DelegateWinThreadPool.h"

namespace
{
    class SetIntRunner : public DelegateWinThread::Delegate
    {
    public:
        SetIntRunner(int *pValue, int iExpectValue)
            : m_pValue(pValue), m_iExpectValue(iExpectValue) {}
        ~SetIntRunner() {}

        virtual void Run() override { *m_pValue = m_iExpectValue; }

        int GetValue() const { return *m_pValue; }

    private:
        int *m_pValue;
        int m_iExpectValue;
    };

    class WaitableEventRunner : public DelegateWinThread::Delegate
    {
    public:
        WaitableEventRunner(WaitableEvent *pEvent) : m_pEvent(pEvent)
        { ASSERT(!m_pEvent->IsSignaled()); }
        ~WaitableEventRunner() {}

        virtual void Run() override { m_pEvent->Signal(); }
        bool IsSignaled() { return m_pEvent->IsSignaled(); }

    private:
        WaitableEvent *m_pEvent;
    };

    class AutoIncrementRunner : public DelegateWinThread::Delegate
    {
    public:
        AutoIncrementRunner() : m_iValue(0) {}
        ~AutoIncrementRunner() {}

        virtual void Run() override
        {
            AutoLock autoLock(m_Lock);
            ++m_iValue;
        }

        int GetValue() const { return m_iValue; }

    private:
        int m_iValue;
        LockGuard m_Lock;
    };
}

TEST(WinThreadUnitTest, CreateAndJoin)
{
    int iStackInt = 0;
    SetIntRunner runner(&iStackInt, 7);
    DelegateWinThread thread(L"set_int_runner", &runner);

    ASSERT_FALSE(thread.IsStarted());
    ASSERT_FALSE(thread.IsStopping());
    ASSERT_EQ(0, runner.GetValue());

    thread.Start();

    ASSERT_TRUE(thread.IsStarted());
    ASSERT_FALSE(thread.IsStopping());

    thread.Stop();

    ASSERT_FALSE(thread.IsStarted());
    ASSERT_FALSE(thread.IsStopping());
    ASSERT_EQ(7, runner.GetValue());
}

TEST(WinThreadUnitTest, WaitForEvent)
{
    WaitableEvent waitEvent(true, false);
    WaitableEventRunner runner(&waitEvent);
    DelegateWinThread thread(L"wait_event_runner", &runner);

    ASSERT_FALSE(thread.IsStarted());
    ASSERT_FALSE(thread.IsStopping());
    ASSERT_FALSE(runner.IsSignaled());

    thread.Start();

    ASSERT_TRUE(thread.IsStarted());
    ASSERT_FALSE(thread.IsStopping());

    thread.Stop();

    ASSERT_FALSE(thread.IsStarted());
    ASSERT_FALSE(thread.IsStopping());
    ASSERT_TRUE(runner.IsSignaled());
}

TEST(WinThreadUnitTest, ThreadPool)
{
    int iStackInt = 0;
    SetIntRunner intRunner(&iStackInt, 8);
    WaitableEvent waitEvent(true, false);
    WaitableEventRunner eventRunner(&waitEvent);

    DelegateWinThreadPool pool(L"work_thread", 10);
    pool.AddWork(&intRunner);

    ASSERT_EQ(0, intRunner.GetValue());

    pool.Start();
    pool.AddWork(&eventRunner);

    pool.JoinAll();

    ASSERT_EQ(8, intRunner.GetValue());
    ASSERT_TRUE(eventRunner.IsSignaled());
}

TEST(WinThreadUnitTest, ThreadPool2)
{
    AutoIncrementRunner runner;

    DelegateWinThreadPool pool(L"work_thread", 10);
    for (int i = 0; i < 10; ++i)
    {
        pool.AddWork(&runner);
    }

    ASSERT_EQ(0, runner.GetValue());

    pool.Start();

    for (int i = 0; i < 10; ++i)
    {
        pool.AddWork(&runner);
    }

    pool.JoinAll();

    ASSERT_EQ(20, runner.GetValue());
}
