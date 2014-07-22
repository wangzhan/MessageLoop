#include "stdafx.h"
#include "MessageLoop.h"
#include "TlsHelper.h"
#include "MessagePumpForUI.h"
#include "MessagePumpForIO.h"
#include "MessagePumpDefault.h"


static TlsHelper g_TlsMessageLoopInstanc;

MessageLoop::MessageLoop(Type type)
: m_Type(type)
, m_pState(NULL)
, m_iNextSequenceNum(0)
, m_bReentrantAllowed(true)
{
    if (m_Type == Type_UI)
    {
        m_pMessagePump.reset(new MessagePumpForUI);
    }
    else if (m_Type == Type_IO)
    {
        m_pMessagePump.reset(new MessagePumpForIO);
    }
    else
    {
        ZhanAssert(m_Type == Type_default);
        m_pMessagePump.reset(new MessagePumpDefault);
    }

    g_TlsMessageLoopInstanc.SetValue(this);
}


MessageLoop::~MessageLoop()
{
    g_TlsMessageLoopInstanc.SetValue(NULL);

    // try to delete
    for (int iIndex = 0; iIndex < 10; ++iIndex)
    {
        DeletePendingTasks();
        ReloadWorkQueue();
        if (!DeletePendingTasks())
        {
            break;
        }
    }

    for (DestructionObserverVectorIter iter = m_vecDestructionObserver.begin();
        iter != m_vecDestructionObserver.end(); ++iter)
    {
        (*iter)->WillDestructMessageLoop();
    }
}

bool MessageLoop::PendingTask::operator < (const PendingTask &task) const
{
    // 大根堆
    if (dwDelayedWorkTime != task.dwDelayedWorkTime)
    {
        return dwDelayedWorkTime > task.dwDelayedWorkTime;
    }

    return iSequenceNum > task.iSequenceNum;
}

MessageLoop* MessageLoop::Current()
{
    return reinterpret_cast<MessageLoop*>(g_TlsMessageLoopInstanc.GetValue());
}

void MessageLoop::AddDestructionObserver(DestructionObserver *pObserver)
{
    m_vecDestructionObserver.push_back(pObserver);
}

void MessageLoop::RemoveDestructionObserver(DestructionObserver *pObserver)
{
    for (DestructionObserverVectorIter iter = m_vecDestructionObserver.begin();
        iter != m_vecDestructionObserver.end(); ++iter)
    {
        if (pObserver == *iter)
        {
            m_vecDestructionObserver.erase(iter);
            break;
        }
    }
}

void MessageLoop::Run()
{
    AutoRunState state(this);
    m_pMessagePump->Run(this);
}

void MessageLoop::RunAllPending()
{
    AutoRunState state(this);
    m_pState->bShouldQuit = true;
    m_pMessagePump->Run(this);
}

void MessageLoop::QuitByTaskClosure()
{
    TaskClosure task = CreateTaskClosure(&MessageLoop::Quit, this);
    PostTask(task);
}

void MessageLoop::Quit()
{
    if (m_pState)
    {
        m_pState->bShouldQuit = true;
    }
}

void MessageLoop::QuitNow()
{
    if (m_pState)
    {
        m_pState->bShouldQuit = true;
        m_pMessagePump->Quit();
    }
}

void MessageLoop::SetNestalbeTaskAllowed(bool bNestable)
{
    if (m_bReentrantAllowed != bNestable)
    {
        m_bReentrantAllowed = bNestable;
        if (m_bReentrantAllowed)
        {
            // activate to do work if already stop for nestable loop
            m_pMessagePump->ScheduleWork();
        }
    }
}

bool MessageLoop::DoWork()
{
    if (!m_bReentrantAllowed)
    {
        return false;
    }

    for (;;)
    {
        ReloadWorkQueue();
        if (m_WorkQueue.empty())
        {
            break;
        }

        PendingTask task = m_WorkQueue.front();
        m_WorkQueue.pop();
        if (task.dwDelayedWorkTime)
        {
            AddTaskToDelayedQueue(task);
            if (m_DelayedWorkQueue.top().dwDelayedWorkTime == task.dwDelayedWorkTime)
            {
                m_pMessagePump->ScheduleDelayedWork(task.dwDelayedWorkTime);
            }
        }
        else
        {
            if (DeferOrRunPendingTask(task))
            {
                return true;
            }
        }
    }
    return false;
}

bool MessageLoop::DoDelayedWork(DWORD *pDelayedWorkTime)
{
    if (!m_bReentrantAllowed)
    {
        *pDelayedWorkTime = 0;
        return false;
    }

    for (;;)
    {
        if (m_DelayedWorkQueue.empty())
        {
            *pDelayedWorkTime = 0;
            break;
        }

        if (m_DelayedWorkQueue.top().dwDelayedWorkTime > GetTickCount())
        {
            *pDelayedWorkTime = m_DelayedWorkQueue.top().dwDelayedWorkTime;
            break;
        }

        PendingTask task = m_DelayedWorkQueue.top();
        m_DelayedWorkQueue.pop();
        if (DeferOrRunPendingTask(task))
        {
            if (m_DelayedWorkQueue.empty())
            {
                *pDelayedWorkTime = 0;
                break;
            }
            *pDelayedWorkTime = m_DelayedWorkQueue.top().dwDelayedWorkTime;
            return true;
        }
    }
    return false;
}

bool MessageLoop::DoIdleWork()
{
    if (!m_bReentrantAllowed || IsNested() || m_DeferredNonNestableWorkQueue.empty())
    {
        if (m_pState->bShouldQuit)
        {
            m_pMessagePump->Quit();
        }
        return false;
    }

    PendingTask task = m_DeferredNonNestableWorkQueue.front();
    m_DeferredNonNestableWorkQueue.pop();

    RunTask(task);
    return true;
}

void MessageLoop::PostTask(const TaskClosure &task)
{
    PendingTask pendingTask(task, 0);
    AddTaskToIncomingQueue(pendingTask);
}

void MessageLoop::PostDelayedTask(const TaskClosure &task, DWORD dwTimeout)
{
    PendingTask pendingTask(task, GetTickCount() + dwTimeout);
    AddTaskToIncomingQueue(pendingTask);
}

void MessageLoop::PostNonNestableTask(const TaskClosure &task)
{
    PendingTask pendingTask(task, 0, false);
    AddTaskToIncomingQueue(pendingTask);
}

void MessageLoop::PostNonNestableDelayedTask(const TaskClosure &task, DWORD dwTimeout)
{
    PendingTask pendingTask(task, GetTickCount() + dwTimeout, false);
    AddTaskToIncomingQueue(pendingTask);
}

bool MessageLoop::IsIdle() const
{
    AutoLock autoLock(m_lock);
    return m_IncomingQueue.empty();
}

void MessageLoop::AddTaskToIncomingQueue(const PendingTask &task)
{
    std::shared_ptr<MessagePump> pPump;
    {
        AutoLock autoLock(m_lock);
        bool bWasEmpty = m_IncomingQueue.empty();
        m_IncomingQueue.push(task);
        if (!bWasEmpty)
        {
            return;
        }

        // 增加pump引用计数，防止被队列中的任务析构了
        pPump = m_pMessagePump;
    }

    pPump->ScheduleWork();
}

void MessageLoop::AddTaskToDelayedQueue(PendingTask &task)
{
    task.iSequenceNum = m_iNextSequenceNum++;
    m_DelayedWorkQueue.push(task);
}

void MessageLoop::ReloadWorkQueue()
{
    if (!m_WorkQueue.empty())
    {
        return;
    }

    AutoLock autoLock(m_lock);
    if (m_IncomingQueue.empty())
    {
        return;
    }

    m_WorkQueue.swap(m_IncomingQueue);
}

bool MessageLoop::DeletePendingTasks()
{
    bool bDidWork = false;
    while (!m_WorkQueue.empty())
    {
        m_WorkQueue.pop();
        bDidWork = true;
    }

    while (!m_DelayedWorkQueue.empty())
    {
        m_DelayedWorkQueue.pop();
        bDidWork = true;
    }

    while (!m_DeferredNonNestableWorkQueue.empty())
    {
        m_DeferredNonNestableWorkQueue.pop();
        bDidWork = true;
    }

    return bDidWork;
}

bool MessageLoop::DeferOrRunPendingTask(const PendingTask &task)
{
    if (!task.bNestable && IsNested())
    {
        m_DeferredNonNestableWorkQueue.push(task);
        return false;
    }

    RunTask(task);
    return true;
}

void MessageLoop::RunTask(const PendingTask &task)
{
    m_bReentrantAllowed = false;

    task.task->Run();

    m_bReentrantAllowed = true;
}
