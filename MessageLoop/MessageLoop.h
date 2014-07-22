// Author: WangZhan -> wangzhan.1985@gmail.com
#pragma once
#include <memory>
#include <queue>
#include <vector>
#include "AutoLock.h"
#include "MessagePump.h"
#include "MessagePumpForIO.h"
#include "TaskClosure.h"


class MessageLoop : public MessagePump::Delegate
{
public:
    enum Type
    {
        Type_default,
        Type_UI,
        Type_IO
    };

    explicit MessageLoop(Type type = Type_default);
    virtual ~MessageLoop();

    // called by any thread
    static MessageLoop* Current();

    class DestructionObserver
    {
    public:
        virtual ~DestructionObserver() {}

        virtual void WillDestructMessageLoop() = 0;
    };

    typedef std::vector<DestructionObserver*> DestructionObserverVector;
    typedef DestructionObserverVector::iterator DestructionObserverVectorIter;

    void AddDestructionObserver(DestructionObserver *pObserver);
    void RemoveDestructionObserver(DestructionObserver *pObserver);

    // called by any thread
    void Run();
    void QuitByTaskClosure();

    void RunAllPending();
    void Quit();
    void QuitNow();

    virtual bool DoWork();
    virtual bool DoDelayedWork(DWORD *pDelayedWorkTime);
    virtual bool DoIdleWork();

    // called by any thread
    void PostTask(const TaskClosure &task);
    void PostDelayedTask(const TaskClosure &task, DWORD dwTimeout);
    void PostNonNestableTask(const TaskClosure &task);
    void PostNonNestableDelayedTask(const TaskClosure &task, DWORD dwTimeout);

    bool IsNested() const { return m_pState && m_pState->iRunDepth > 1; }
    bool IsIdle() const;
    bool IsRuning() const { return m_pState != NULL; }

    Type GetType() const { return m_Type; }

    void SetNestalbeTaskAllowed(bool bNestable);
    bool IsNestableTaskAllowed() const { return m_bReentrantAllowed; }

    void SetThreadName(const tstring &csName) { m_csThreadName = csName; }
    tstring GetThreadName() const { return m_csThreadName; }
    
protected:
    struct PendingTask
    {
        PendingTask(const TaskClosure &taskClosure, DWORD dwDelayedTime, bool bNest = true)
        : task(taskClosure)
        , dwDelayedWorkTime(dwDelayedTime)
        , bNestable(bNest)
        , iSequenceNum(0)
        {}
        
        bool operator < (const PendingTask &task) const;

        TaskClosure task;
        DWORD dwDelayedWorkTime;
        bool bNestable;
        int iSequenceNum;
    };

    void AddTaskToIncomingQueue(const PendingTask &task);
    void AddTaskToDelayedQueue(PendingTask &task);
    void ReloadWorkQueue();
    bool DeletePendingTasks();

    bool DeferOrRunPendingTask(const PendingTask &task);
    void RunTask(const PendingTask &task);

protected:
    typedef std::queue<PendingTask> TaskQueue;
    typedef std::priority_queue<PendingTask> PriorityTaskQueue;

    mutable LockGuard m_lock;

    TaskQueue m_IncomingQueue;
    TaskQueue m_WorkQueue;
    PriorityTaskQueue m_DelayedWorkQueue;
    TaskQueue m_DeferredNonNestableWorkQueue;

    std::shared_ptr<MessagePump> m_pMessagePump;

    tstring m_csThreadName;
    // task is not reentered by default
    bool m_bReentrantAllowed;
    int m_iNextSequenceNum;
    Type m_Type;

    DestructionObserverVector m_vecDestructionObserver;

    struct RunState
    {
        bool bShouldQuit;
        int iRunDepth;
    };
    RunState *m_pState;

    struct AutoRunState : public RunState
    {
        explicit AutoRunState(MessageLoop *pMsgLoop)
        : pMessageLoop(pMsgLoop)
        {
            pPreviouseState = pMessageLoop->m_pState;
            iRunDepth = pPreviouseState ? pPreviouseState->iRunDepth + 1 : 1;

            bShouldQuit = false;
            pMessageLoop->m_pState = this;
        }

        ~AutoRunState()
        {
            pMessageLoop->m_pState = pPreviouseState;
        }

        MessageLoop *pMessageLoop;
        RunState *pPreviouseState;
    };
};

class MessageLoopForUI : public MessageLoop
{
public:
    MessageLoopForUI() : MessageLoop(Type_UI) {}
    ~MessageLoopForUI() {}

    static MessageLoopForUI* Current() 
    {
        return static_cast<MessageLoopForUI*>(__super::Current());
    }
};

COMPILE_ZhanAssert(sizeof(MessageLoop) == sizeof(MessageLoopForUI),
    MessageLoopForUI_should_not_have_extra_member_variables);

class MessageLoopForIO : public MessageLoop
{
public:
    typedef MessagePumpForIO::IOHandler IOHandler;
    typedef MessagePumpForIO::IOContext IOContext;

    MessageLoopForIO() : MessageLoop(Type_IO) {}
    ~MessageLoopForIO() {}

    static MessageLoopForIO* Current()
    {
        return static_cast<MessageLoopForIO*>(__super::Current());
    }

    void RegisterIOHandler(HANDLE fileHandle, IOHandler *pHandler)
    {
        dynamic_cast<MessagePumpForIO*>(m_pMessagePump.get())->
            RegisterIOHandler(fileHandle, pHandler);
    }

    bool MessageLoopForIO::WaitForIOCompletion(DWORD timeout)
    {
        return dynamic_cast<MessagePumpForIO*>(m_pMessagePump.get())->
            WaitForIOCompletion(timeout);
    }
};

COMPILE_ZhanAssert(sizeof(MessageLoop) == sizeof(MessageLoopForIO),
    MessageLoopForIO_should_not_have_extra_member_variables);
