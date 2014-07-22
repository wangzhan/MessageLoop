// Author: WangZhan -> wangzhan.1985@gmail.com
#include "stdafx.h"
#include "gtest/gtest.h"
#include "../MessageLoop/MessageLoopThread.h"

namespace
{
    class ToggleValueTask : public std::enable_shared_from_this<ToggleValueTask>
    {
    public:
        ToggleValueTask(bool *pValue) : m_pValue(pValue) {}
        ~ToggleValueTask() {}

        void Run()
        {
            *m_pValue = !*m_pValue;
        }

        bool GetValue() const { return *m_pValue; }

    private:
        bool *m_pValue;
    };

    class SleepTask
    {
    public:
        SleepTask(int iTimeOut) : m_iTimeOut(iTimeOut) {}
        ~SleepTask() {}

        void Run()
        {
            Sleep(m_iTimeOut);
        }

    private:
        int m_iTimeOut;
    };

    enum EventType
    {
        Type_Init,
        Type_ClearUp,
        Type_Destruct,
    };

    typedef std::vector<EventType> EVENT_VECTOR;
    typedef EVENT_VECTOR::iterator EVENT_VECTOR_ITER;

    class EventsThread : public MessageLoopThread
    {
    public:
        EventsThread(const tstring &csThreadName, EVENT_VECTOR *pEvents)
            : MessageLoopThread(csThreadName), m_pEvents(pEvents) {
            ASSERT(m_pEvents);
        }

        virtual ~EventsThread() 
        {
            // 先于基类析构函数中的调用
            Stop();
            m_pEvents->push_back(Type_Destruct);
        }

        virtual void Initialize() override { m_pEvents->push_back(Type_Init); }
        virtual void CleanUp() override { m_pEvents->push_back(Type_ClearUp); }

    private:
        EVENT_VECTOR *m_pEvents;
    };
}

TEST(MessageLoopThreadUnitTest, Restart)
{
    MessageLoopThread thread(_T("restart"));
    ASSERT_FALSE(thread.GetThreadID() > 0);
    ASSERT_FALSE(thread.GetMessageLoop() != NULL);

    ASSERT_TRUE(thread.Start());
    ASSERT_TRUE(thread.GetThreadID() > 0);
    ASSERT_TRUE(thread.GetMessageLoop() != NULL);

    thread.Stop();
    ASSERT_FALSE(thread.GetThreadID() > 0);
    ASSERT_FALSE(thread.GetMessageLoop() != NULL);

    ASSERT_TRUE(thread.Start());
    ASSERT_TRUE(thread.GetThreadID() > 0);
    ASSERT_TRUE(thread.GetMessageLoop() != NULL);

    thread.Stop();
    ASSERT_FALSE(thread.GetThreadID() > 0);
    ASSERT_FALSE(thread.GetMessageLoop() != NULL);

    ASSERT_TRUE(thread.Start());
    ASSERT_TRUE(thread.GetThreadID() > 0);
    ASSERT_TRUE(thread.GetMessageLoop() != NULL);

    thread.Stop();
    ASSERT_FALSE(thread.GetThreadID() > 0);
    ASSERT_FALSE(thread.GetMessageLoop() != NULL);
}

TEST(MessageLoopThreadUnitTest, ThreadName)
{
    tstring csThreadName = _T("threadname1");
    MessageLoopThread thread(csThreadName);
    ASSERT_STREQ(csThreadName.c_str(), thread.GetThreadName().c_str());
}

TEST(MessageLoopThreadUnitTest, ToggleValue)
{
    bool bValue = false;
    ToggleValueTask task(&bValue);
    TaskClosure taskClosure = CreateTaskClosure(&ToggleValueTask::Run, &task);
    MessageLoopThread thread(_T("togglevalue"));
    ASSERT_TRUE(thread.Start());

    thread.GetMessageLoop()->PostTask(taskClosure);

    // wait for the task to finish
    Sleep(3 * 1000);

    ASSERT_TRUE(bValue);
}

TEST(MessageLoopThreadUnitTest, ToggleValue2)
{
    bool bValue = false;
    std::shared_ptr<ToggleValueTask> task(new ToggleValueTask(&bValue));
    TaskClosure taskClosure = CreateTaskClosure(&ToggleValueTask::Run, task);
    MessageLoopThread thread(_T("togglevalue"));
    ASSERT_TRUE(thread.Start());

    thread.GetMessageLoop()->PostTask(taskClosure);

    // wait for the task to finish
    Sleep(3 * 1000);

    ASSERT_TRUE(bValue);
}

TEST(MessageLoopThreadUnitTest, ToggleValue3)
{
    bool bValue = false;
    TaskClosure taskClosure;
    {
        std::shared_ptr<ToggleValueTask> task(new ToggleValueTask(&bValue));
        taskClosure = CreateTaskClosure(&ToggleValueTask::Run, task);
    }
    
    MessageLoopThread thread(_T("togglevalue"));
    ASSERT_TRUE(thread.Start());

    thread.GetMessageLoop()->PostTask(taskClosure);

    // wait for the task to finish
    Sleep(3 * 1000);

    ASSERT_FALSE(bValue);
}

TEST(MessageLoopThreadUnitTest, MonitorEvent)
{
    EVENT_VECTOR vecEvents;
    {
        EventsThread thread(_T("monitorevent"), &vecEvents);
        ASSERT_TRUE(thread.Start());
    }

    ASSERT_EQ(3, vecEvents.size());
    ASSERT_EQ(Type_Init, vecEvents[0]);
    ASSERT_EQ(Type_ClearUp, vecEvents[1]);
    ASSERT_EQ(Type_Destruct, vecEvents[2]);
}

TEST(MessageLoopThreadUnitTest, PostTwoTask)
{
    bool bValue = false;
    ToggleValueTask task(&bValue);
    TaskClosure taskClosure1 = CreateTaskClosure(&ToggleValueTask::Run, &task);

    SleepTask task2(1000);
    TaskClosure taskClosure2 = CreateTaskClosure(&SleepTask::Run, &task2);

    MessageLoopThread thread(_T("togglevalue"));
    ASSERT_TRUE(thread.Start());

    thread.GetMessageLoop()->PostTask(taskClosure1);
    thread.GetMessageLoop()->PostTask(taskClosure2);

    // wait for the task to finish
    Sleep(5 * 1000);

    ASSERT_TRUE(bValue);
}
