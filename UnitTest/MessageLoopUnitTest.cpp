#include "stdafx.h"
#include "gtest/gtest.h"
#include "../MessageLoop/MessageLoop.h"
#include "../MessageLoop/MessageLoopThread.h"


namespace
{
    class Foo
    {
    public:
        Foo() : m_iTestCount(0) {}
        ~Foo() {}

        void Test0() { ++m_iTestCount; }

        void Test1(int i) { m_iTestCount += i; }

        void Test1ConstRef(const CString &csValue)
        {
            ++m_iTestCount;
            m_csTestString.Append(csValue);
        }

        void Test1Ptr(CString *pValue)
        {
            ++m_iTestCount;
            m_csTestString.Append(*pValue);
        }

        void Test2Ptr(CString *pValue1, CString *pValue2)
        {
            ++m_iTestCount;
            m_csTestString.Append(*pValue1);
            m_csTestString.Append(*pValue2);
        }

        void TestMix(const CString &csVaule, CString *pValue)
        {
            ++m_iTestCount;
            m_csTestString.Append(csVaule);
            m_csTestString.Append(*pValue);
        }

        int TestCount() const { return m_iTestCount; }
        CString TestString() const { return m_csTestString; }

    private:
        int m_iTestCount;
        CString m_csTestString;
    };


    void RunTest_PostTask(MessageLoop::Type type)
    {
        MessageLoop loop(type);

        std::shared_ptr<Foo> pFoo(new Foo);
        CString a(_T("a")), b(_T("b")), c(_T("c")), d(_T("d")), e(_T("e")), f(_T("f"));
        MessageLoop::Current()->PostTask(CreateTaskClosure(&Foo::Test0, pFoo.get()));
        MessageLoop::Current()->PostTask(CreateTaskClosure(&Foo::Test1ConstRef, pFoo.get(), a));
        MessageLoop::Current()->PostTask(CreateTaskClosure(&Foo::Test1Ptr, pFoo.get(), &b));
        MessageLoop::Current()->PostTask(CreateTaskClosure(&Foo::Test1, pFoo.get(), 100));
        MessageLoop::Current()->PostTask(CreateTaskClosure(&Foo::Test2Ptr, pFoo.get(), &c, &d));
        MessageLoop::Current()->PostTask(CreateTaskClosure(&Foo::TestMix, pFoo.get(), e, &f));
        
        MessageLoop::Current()->QuitByTaskClosure();

        MessageLoop::Current()->Run();

        ASSERT_EQ(105, pFoo->TestCount());
        ASSERT_STREQ(_T("abcdef"), pFoo->TestString());
    }

    void SlowFunc(DWORD dwTimeOut, int *pQuitCount)
    {
        Sleep(dwTimeOut);
        if (--*pQuitCount == 0)
        {
            MessageLoop::Current()->Quit();
        }
    }

    void RecordRunTime(DWORD *pRunTime, int *pQuitCount)
    {
        *pRunTime = GetTickCount();
        SlowFunc(10, pQuitCount);
    }

    void RunTest_PostDelayedTask_Basic(MessageLoop::Type messageLoopType)
    {
        MessageLoop loop(messageLoopType);

        const DWORD dwTimeOut = 20;
        int iNumTasks = 1;
        DWORD dwRunTime = 0;

        MessageLoop::Current()->PostDelayedTask(
            CreateTaskClosure(RecordRunTime, &dwRunTime, &iNumTasks), dwTimeOut);

        DWORD dwStartTime = GetTickCount();
        MessageLoop::Current()->Run();
        DWORD dwEndTime = GetTickCount();

        ASSERT_EQ(0, iNumTasks);
        ASSERT_LT(dwTimeOut, dwEndTime - dwStartTime);
    }

    void RunTest_PostDelayedTask_InDelayOrder(MessageLoop::Type type)
    {
        MessageLoop loop(type);

        int iNumTasks = 2;
        DWORD dwRunTime1 = 0;
        DWORD dwRunTime2 = 0;

        MessageLoop::Current()->PostDelayedTask(
            CreateTaskClosure(RecordRunTime, &dwRunTime1, &iNumTasks), 200);

        MessageLoop::Current()->PostDelayedTask(
            CreateTaskClosure(RecordRunTime, &dwRunTime2, &iNumTasks), 10);

        MessageLoop::Current()->Run();

        ASSERT_EQ(0, iNumTasks);
        ASSERT_GT(dwRunTime1, dwRunTime2);
    }

    void RunTest_PostDelayedTask_InPostOrder(MessageLoop::Type type)
    {
        MessageLoop loop(type);

        const DWORD dwTimeOut = 100;
        int iNumTasks = 2;
        DWORD dwRunTime1 = 0;
        DWORD dwRunTime2 = 0;

        MessageLoop::Current()->PostDelayedTask(
            CreateTaskClosure(RecordRunTime, &dwRunTime1, &iNumTasks), dwTimeOut);

        MessageLoop::Current()->PostDelayedTask(
            CreateTaskClosure(RecordRunTime, &dwRunTime2, &iNumTasks), dwTimeOut);

        MessageLoop::Current()->Run();

        ASSERT_EQ(0, iNumTasks);
        ASSERT_LE(dwRunTime1, dwRunTime2);
    }

    void RunTest_PostDelayedTask_InPostOrder2(MessageLoop::Type type)
    {
        MessageLoop loop(type);

        const DWORD dwTimeOut = 50;
        int iNumTasks = 2;
        DWORD dwRunTime = 0;

        MessageLoop::Current()->PostTask(
            CreateTaskClosure(SlowFunc, dwTimeOut, &iNumTasks));

        MessageLoop::Current()->PostDelayedTask(
            CreateTaskClosure(RecordRunTime, &dwRunTime, &iNumTasks), 10);
        
        DWORD dwStartTime = GetTickCount();
        MessageLoop::Current()->Run();
        DWORD dwEndTime = GetTickCount();

        ASSERT_EQ(0, iNumTasks);
        ASSERT_LT(dwTimeOut, dwRunTime);
    }

    void RunTest_PostDelayedTask_SharedTime(MessageLoop::Type type)
    {
        MessageLoop loop(type);

        const DWORD dwTimeOut = 50;
        int iNumTasks = 1;
        DWORD dwRunTime1 = 0;
        DWORD dwRunTime2 = 0;

        MessageLoop::Current()->PostDelayedTask(
            CreateTaskClosure(RecordRunTime, &dwRunTime1, &iNumTasks), 1000000);

        MessageLoop::Current()->PostDelayedTask(
            CreateTaskClosure(RecordRunTime, &dwRunTime2, &iNumTasks), 10);

        DWORD dwStartTime = GetTickCount();
        MessageLoop::Current()->Run();
        DWORD dwEndTime = GetTickCount();

        ASSERT_EQ(0, iNumTasks);
        ASSERT_GT(50000, (int)(dwEndTime - dwStartTime));

        MessageLoop::Current()->RunAllPending();
        ASSERT_EQ(0, dwRunTime1);
        ASSERT_NE(0, dwRunTime2);
    }

    void SubPumpFunc()
    {
        MessageLoop::Current()->SetNestalbeTaskAllowed(true);
        MSG msg;
        while (GetMessage(&msg, NULL, NULL, NULL))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        MessageLoop::Current()->Quit();
    }

    void RunTest_PostDelayedTask_SharedTime_SubPump()
    {
        MessageLoop loop(MessageLoop::Type_UI);

        const DWORD dwTimeOut = 50;
        int iNumTasks = 1;
        DWORD dwRunTime = 0;

        MessageLoop::Current()->PostTask(CreateTaskClosure(SubPumpFunc));

        MessageLoop::Current()->PostDelayedTask(
            CreateTaskClosure(RecordRunTime, &dwRunTime, &iNumTasks), 1000000);

        MessageLoop::Current()->PostDelayedTask(CreateTaskClosure(PostQuitMessage, 0), 10);

        DWORD dwStartTime = GetTickCount();
        MessageLoop::Current()->Run();
        DWORD dwEndTime = GetTickCount();

        ASSERT_EQ(1, iNumTasks);
        ASSERT_GT(50000, int(dwEndTime - dwStartTime));

        MessageLoop::Current()->RunAllPending();
        ASSERT_EQ(0, dwRunTime);
    }

    void NestingFunc(int *pDepth)
    {
        if ((*pDepth) > 0)
        {
            --(*pDepth);
            MessageLoop::Current()->PostTask(CreateTaskClosure(NestingFunc, pDepth));

            MessageLoop::Current()->SetNestalbeTaskAllowed(true);
            MessageLoop::Current()->Run();
        }

        MessageLoop::Current()->Quit();
    }

    void RunTest_NestingFunc(MessageLoop::Type type)
    {
        MessageLoop loop(type);

        int iRunDepth = 100;
        MessageLoop::Current()->PostTask(CreateTaskClosure(NestingFunc, &iRunDepth));

        MessageLoop::Current()->Run();

        ASSERT_EQ(0, iRunDepth);
    }


    LPCTSTR g_lpszMessageBoxTitle = _T("MessageLoop Unit Test");

    enum TaskType
    {
        MESSAGEBOX,
        ENDDIALOG,
        RECURSIVE,
        TIMEDMESSAGELOOP,
        QUITMESSAGELOOP,
        ORDERERD,
        PUMPS,
        SLEEP
    };

    struct TaskItem
    {
        TaskItem(TaskType type, int iC, BOOL bS) : taskType(type), iCookie(iC), bStart(bS) {}

        bool operator == (const TaskItem &item) const
        {
            return (taskType == item.taskType) && (iCookie == item.iCookie) && (bStart == item.bStart);
        }

    public:
        TaskType taskType;
        int iCookie;
        BOOL bStart;
    };

    class TaskList
    {
    public:
        void RecordStart(TaskType type, int iCookie)
        {
            TaskItem item(type, iCookie, TRUE);
            m_vecTaskList.push_back(item);
        }

        void RecordEnd(TaskType type, int iCookie)
        {
            TaskItem item(type, iCookie, FALSE);
            m_vecTaskList.push_back(item);
        }

        TaskItem GetItem(int iIndex)
        {
            ASSERT(iIndex < (int)m_vecTaskList.size());
            return m_vecTaskList[iIndex];
        }

        int Size()
        {
            return (int)m_vecTaskList.size();
        }

    private:
        std::vector<TaskItem> m_vecTaskList;
    };

    void OrderFunc(TaskList *order, int iCookie)
    {
        order->RecordStart(ORDERERD, iCookie);
        order->RecordEnd(ORDERERD, iCookie);
    }

    void MessageBoxFunc(TaskList *order, int iCookie, BOOL bIsReentrant)
    {
        order->RecordStart(MESSAGEBOX, iCookie);
        if (bIsReentrant)
        {
            MessageLoop::Current()->SetNestalbeTaskAllowed(TRUE);
        }
        MessageBox(NULL, _T("Please wait ..."), g_lpszMessageBoxTitle, MB_OK);
        order->RecordEnd(MESSAGEBOX, iCookie);
    }

    void RecursiveFunc(TaskList *order, int iCookie, int iDepth, BOOL bIsReentrant)
    {
        order->RecordStart(RECURSIVE, iCookie);
        if (iDepth > 0)
        {
            if (bIsReentrant)
            {
                MessageLoop::Current()->SetNestalbeTaskAllowed(TRUE);
            }

            MessageLoop::Current()->PostTask(CreateTaskClosure(RecursiveFunc, order, iCookie,
                iDepth - 1, bIsReentrant));
        }
        order->RecordEnd(RECURSIVE, iCookie);
    }

    void RecursiveSlowFunc(TaskList *order, int iCookie, int iDepth, BOOL bIsReentrant)
    {
        RecursiveFunc(order, iCookie, iDepth, bIsReentrant);
        Sleep(10);
    }

    void EndDialogFunc(TaskList *order, int iCookie)
    {
        order->RecordStart(ENDDIALOG, iCookie);
        HWND hWnd = GetActiveWindow();
        if (hWnd != NULL)
        {
            ASSERT_NE(0, EndDialog(hWnd, IDCONTINUE));
            order->RecordEnd(ENDDIALOG, iCookie);
        }
    }

    void QuitFunc(TaskList *order, int iCookie)
    {
        order->RecordStart(QUITMESSAGELOOP, iCookie);
        MessageLoop::Current()->Quit();
        order->RecordEnd(QUITMESSAGELOOP, iCookie);
    }

    void SleepFunc(TaskList *order, int iCookie, DWORD dwMs)
    {
        order->RecordStart(SLEEP, iCookie);
        Sleep(dwMs);
        order->RecordEnd(SLEEP, iCookie);
    }

    void RecursiveFuncWin(MessageLoop *target, HANDLE event, BOOL bExpectWnd, TaskList *order, 
        BOOL bIsReentrant)
    {
        target->PostTask(CreateTaskClosure(RecursiveFunc, order, 1, 2, bIsReentrant));
        target->PostTask(CreateTaskClosure(MessageBoxFunc, order, 2, bIsReentrant));
        target->PostTask(CreateTaskClosure(RecursiveFunc, order, 3, 2, bIsReentrant));

        target->PostTask(CreateTaskClosure(EndDialogFunc, order, 4));
        target->PostTask(CreateTaskClosure(QuitFunc, order, 5));

        ASSERT_TRUE(!!SetEvent(event));

        while (bExpectWnd)
        {
            HWND hWnd = FindWindow(_T("#32770"), g_lpszMessageBoxTitle);
            if (hWnd)
            {
                for (;;)
                {
                    HWND hButton = FindWindowEx(hWnd, NULL, L"Button", NULL);
                    if (hButton)
                    {
                        ASSERT_EQ(0, SendMessage(hButton, WM_LBUTTONDOWN, 0, 0));
                        ASSERT_EQ(0, SendMessage(hButton, WM_LBUTTONUP, 0, 0));
                        break;
                    }
                }
                break;
            }
        }
    }

    void RunTest_Recursive1(MessageLoop::Type type)
    {
        MessageLoop loop(type);

        TaskList order;
        MessageLoop::Current()->PostTask(CreateTaskClosure(RecursiveFunc, &order, 1, 2, FALSE));
        MessageLoop::Current()->PostTask(CreateTaskClosure(RecursiveFunc, &order, 2, 2, FALSE));
        MessageLoop::Current()->PostTask(CreateTaskClosure(QuitFunc, &order, 3));

        MessageLoop::Current()->Run();

        ASSERT_EQ(14, order.Size());
        ASSERT_EQ(order.GetItem(0), TaskItem(RECURSIVE, 1, TRUE));
        ASSERT_EQ(order.GetItem(1), TaskItem(RECURSIVE, 1, FALSE));
        ASSERT_EQ(order.GetItem(2), TaskItem(RECURSIVE, 2, TRUE));
        ASSERT_EQ(order.GetItem(3), TaskItem(RECURSIVE, 2, FALSE));
        ASSERT_EQ(order.GetItem(4), TaskItem(QUITMESSAGELOOP, 3, TRUE));
        ASSERT_EQ(order.GetItem(5), TaskItem(QUITMESSAGELOOP, 3, FALSE));
        ASSERT_EQ(order.GetItem(6), TaskItem(RECURSIVE, 1, TRUE));
        ASSERT_EQ(order.GetItem(7), TaskItem(RECURSIVE, 1, FALSE));
        ASSERT_EQ(order.GetItem(8), TaskItem(RECURSIVE, 2, TRUE));
        ASSERT_EQ(order.GetItem(9), TaskItem(RECURSIVE, 2, FALSE));
        ASSERT_EQ(order.GetItem(10), TaskItem(RECURSIVE, 1, TRUE));
        ASSERT_EQ(order.GetItem(11), TaskItem(RECURSIVE, 1, FALSE));
        ASSERT_EQ(order.GetItem(12), TaskItem(RECURSIVE, 2, TRUE));
        ASSERT_EQ(order.GetItem(13), TaskItem(RECURSIVE, 2, FALSE));
    }

    void RunTest_Recursive2(MessageLoop::Type type)
    {
        MessageLoop loop(type);

        ASSERT_TRUE(MessageLoop::Current()->IsNestableTaskAllowed());
        TaskList order;
        MessageLoop::Current()->PostTask(CreateTaskClosure(RecursiveSlowFunc, &order, 1, 2, FALSE));
        MessageLoop::Current()->PostTask(CreateTaskClosure(RecursiveSlowFunc, &order, 2, 2, FALSE));
        MessageLoop::Current()->PostDelayedTask(CreateTaskClosure(OrderFunc, &order, 3), 5);
        MessageLoop::Current()->PostDelayedTask(CreateTaskClosure(QuitFunc, &order, 4), 5);

        MessageLoop::Current()->Run();

        ASSERT_EQ(16, order.Size());
        ASSERT_EQ(order.GetItem(0), TaskItem(RECURSIVE, 1, TRUE));
        ASSERT_EQ(order.GetItem(1), TaskItem(RECURSIVE, 1, FALSE));
        ASSERT_EQ(order.GetItem(2), TaskItem(RECURSIVE, 2, TRUE));
        ASSERT_EQ(order.GetItem(3), TaskItem(RECURSIVE, 2, FALSE));
        ASSERT_EQ(order.GetItem(4), TaskItem(RECURSIVE, 1, TRUE));
        ASSERT_EQ(order.GetItem(5), TaskItem(RECURSIVE, 1, FALSE));
        ASSERT_EQ(order.GetItem(6), TaskItem(ORDERERD, 3, TRUE));
        ASSERT_EQ(order.GetItem(7), TaskItem(ORDERERD, 3, FALSE));
        ASSERT_EQ(order.GetItem(8), TaskItem(RECURSIVE, 2, TRUE));
        ASSERT_EQ(order.GetItem(9), TaskItem(RECURSIVE, 2, FALSE));
        ASSERT_EQ(order.GetItem(10), TaskItem(QUITMESSAGELOOP, 4, TRUE));
        ASSERT_EQ(order.GetItem(11), TaskItem(QUITMESSAGELOOP, 4, FALSE));
        ASSERT_EQ(order.GetItem(12), TaskItem(RECURSIVE, 1, TRUE));
        ASSERT_EQ(order.GetItem(13), TaskItem(RECURSIVE, 1, FALSE));
        ASSERT_EQ(order.GetItem(14), TaskItem(RECURSIVE, 2, TRUE));
        ASSERT_EQ(order.GetItem(15), TaskItem(RECURSIVE, 2, FALSE));
    }

    void RunTest_Recursive3(MessageLoop::Type type)
    {
        MessageLoop loop(type);

        MessageLoopThread worker(L"worker");
        MessageLoopThread::Options options(type, 0);
        ASSERT_TRUE(worker.StartWithOptions(options));
        Sleep(10);      // 启动线程

        TaskList order;
        ScopedHandle event(CreateEvent(NULL, FALSE, FALSE, NULL));
        worker.GetMessageLoop()->PostTask(CreateTaskClosure(RecursiveFuncWin, MessageLoop::Current(),
            event.Get(), TRUE, &order, FALSE));

        WaitForSingleObject(event, INFINITE);
        MessageLoop::Current()->Run();

        ASSERT_EQ(order.Size(), 17);
        ASSERT_EQ(order.GetItem(0), TaskItem(RECURSIVE, 1, TRUE));
        ASSERT_EQ(order.GetItem(1), TaskItem(RECURSIVE, 1, FALSE));
        ASSERT_EQ(order.GetItem(2), TaskItem(MESSAGEBOX, 2, TRUE));
        ASSERT_EQ(order.GetItem(3), TaskItem(MESSAGEBOX, 2, FALSE));
        ASSERT_EQ(order.GetItem(4), TaskItem(RECURSIVE, 3, TRUE));
        ASSERT_EQ(order.GetItem(5), TaskItem(RECURSIVE, 3, FALSE));
        // When EndDialogFunc is processed, the window is already dismissed, hence no
        // "end" entry.
        ASSERT_EQ(order.GetItem(6), TaskItem(ENDDIALOG, 4, TRUE));
        ASSERT_EQ(order.GetItem(7), TaskItem(QUITMESSAGELOOP, 5, TRUE));
        ASSERT_EQ(order.GetItem(8), TaskItem(QUITMESSAGELOOP, 5, FALSE));
        ASSERT_EQ(order.GetItem(9), TaskItem(RECURSIVE, 1, TRUE));
        ASSERT_EQ(order.GetItem(10), TaskItem(RECURSIVE, 1, FALSE));
        ASSERT_EQ(order.GetItem(11), TaskItem(RECURSIVE, 3, TRUE));
        ASSERT_EQ(order.GetItem(12), TaskItem(RECURSIVE, 3, FALSE));
        ASSERT_EQ(order.GetItem(13), TaskItem(RECURSIVE, 1, TRUE));
        ASSERT_EQ(order.GetItem(14), TaskItem(RECURSIVE, 1, FALSE));
        ASSERT_EQ(order.GetItem(15), TaskItem(RECURSIVE, 3, TRUE));
        ASSERT_EQ(order.GetItem(16), TaskItem(RECURSIVE, 3, FALSE));
    }

    void RunTest_Recursive4(MessageLoop::Type type)
    {
        MessageLoop loop(type);

        MessageLoopThread worker(L"worker");
        MessageLoopThread::Options options(type, 0);
        ASSERT_TRUE(worker.StartWithOptions(options));
        Sleep(10);      // 启动线程

        TaskList order;
        ScopedHandle event(CreateEvent(NULL, FALSE, FALSE, NULL));
        worker.GetMessageLoop()->PostTask(CreateTaskClosure(RecursiveFuncWin, MessageLoop::Current(),
            event.Get(), FALSE, &order, TRUE));

        WaitForSingleObject(event, INFINITE);
        MessageLoop::Current()->Run();

        ASSERT_EQ(order.Size(), 18);
        ASSERT_EQ(order.GetItem(0), TaskItem(RECURSIVE, 1, TRUE));
        ASSERT_EQ(order.GetItem(1), TaskItem(RECURSIVE, 1, FALSE));
        ASSERT_EQ(order.GetItem(2), TaskItem(MESSAGEBOX, 2, TRUE));
        // Note that this executes in the MessageBox modal loop.
        ASSERT_EQ(order.GetItem(3), TaskItem(RECURSIVE, 3, TRUE));
        ASSERT_EQ(order.GetItem(4), TaskItem(RECURSIVE, 3, FALSE));
        ASSERT_EQ(order.GetItem(5), TaskItem(ENDDIALOG, 4, TRUE));
        ASSERT_EQ(order.GetItem(6), TaskItem(ENDDIALOG, 4, FALSE));
        ASSERT_EQ(order.GetItem(7), TaskItem(MESSAGEBOX, 2, FALSE));
        ASSERT_EQ(order.GetItem(12), TaskItem(RECURSIVE, 3, TRUE));
        ASSERT_EQ(order.GetItem(13), TaskItem(RECURSIVE, 3, FALSE));
        ASSERT_EQ(order.GetItem(14), TaskItem(RECURSIVE, 1, TRUE));
        ASSERT_EQ(order.GetItem(15), TaskItem(RECURSIVE, 1, FALSE));
        ASSERT_EQ(order.GetItem(16), TaskItem(RECURSIVE, 3, TRUE));
        ASSERT_EQ(order.GetItem(17), TaskItem(RECURSIVE, 3, FALSE));
    }

    void FuncThatPumps(TaskList *order, int iCookie)
    {
        order->RecordStart(PUMPS, iCookie);
        bool bOldState = MessageLoop::Current()->IsNestableTaskAllowed();
        MessageLoop::Current()->SetNestalbeTaskAllowed(true);
        MessageLoop::Current()->RunAllPending();
        MessageLoop::Current()->SetNestalbeTaskAllowed(bOldState);
        order->RecordEnd(PUMPS, iCookie);
    }

    // Tests that non nestable tasks run in FIFO if there are no nested loops.
    void RunTest_NonNestableWithNoNesting(MessageLoop::Type type) {
        MessageLoop loop(type);

        TaskList order;
        MessageLoop::Current()->PostNonNestableTask(CreateTaskClosure(OrderFunc, &order, 1));
        MessageLoop::Current()->PostTask(CreateTaskClosure(OrderFunc, &order, 2));
        MessageLoop::Current()->PostTask(CreateTaskClosure(&QuitFunc, &order, 3));
        MessageLoop::Current()->Run();

        ASSERT_EQ(6, order.Size());
        ASSERT_EQ(order.GetItem(0), TaskItem(ORDERERD, 1, TRUE));
        ASSERT_EQ(order.GetItem(1), TaskItem(ORDERERD, 1, FALSE));
        ASSERT_EQ(order.GetItem(2), TaskItem(ORDERERD, 2, TRUE));
        ASSERT_EQ(order.GetItem(3), TaskItem(ORDERERD, 2, FALSE));
        ASSERT_EQ(order.GetItem(4), TaskItem(QUITMESSAGELOOP, 3, TRUE));
        ASSERT_EQ(order.GetItem(5), TaskItem(QUITMESSAGELOOP, 3, FALSE));
    }

    // Tests that non nestable tasks don't run when there's code in the call stack.
    void RunTest_NonNestableInNestedLoop(MessageLoop::Type type, BOOL bUseDelayed) {
        MessageLoop loop(type);

        TaskList order;
        MessageLoop::Current()->PostTask(CreateTaskClosure(FuncThatPumps, &order, 1));
        if (bUseDelayed)
        {
            MessageLoop::Current()->PostNonNestableDelayedTask(
                CreateTaskClosure(OrderFunc, &order, 2), 1);
        }
        else
        {
            MessageLoop::Current()->PostNonNestableTask(CreateTaskClosure(OrderFunc, &order, 2));
        }

        MessageLoop::Current()->PostTask(CreateTaskClosure(OrderFunc, &order, 3));
        MessageLoop::Current()->PostTask(CreateTaskClosure(SleepFunc, &order, 4, 50));
        MessageLoop::Current()->PostTask(CreateTaskClosure(OrderFunc, &order, 5));
        if (bUseDelayed)
        {
            MessageLoop::Current()->PostNonNestableDelayedTask(
                CreateTaskClosure(&QuitFunc, &order, 6), 2);
        }
        else
        {
            MessageLoop::Current()->PostNonNestableTask(CreateTaskClosure(&QuitFunc, &order, 6));
        }

        MessageLoop::Current()->Run();

        ASSERT_EQ(12, order.Size());
        ASSERT_EQ(order.GetItem(0), TaskItem(PUMPS, 1, TRUE));
        ASSERT_EQ(order.GetItem(1), TaskItem(ORDERERD, 3, TRUE));
        ASSERT_EQ(order.GetItem(2), TaskItem(ORDERERD, 3, FALSE));
        ASSERT_EQ(order.GetItem(3), TaskItem(SLEEP, 4, TRUE));
        ASSERT_EQ(order.GetItem(4), TaskItem(SLEEP, 4, FALSE));
        ASSERT_EQ(order.GetItem(5), TaskItem(ORDERERD, 5, TRUE));
        ASSERT_EQ(order.GetItem(6), TaskItem(ORDERERD, 5, FALSE));
        ASSERT_EQ(order.GetItem(7), TaskItem(PUMPS, 1, FALSE));
        ASSERT_EQ(order.GetItem(8), TaskItem(ORDERERD, 2, TRUE));
        ASSERT_EQ(order.GetItem(9), TaskItem(ORDERERD, 2, FALSE));
        ASSERT_EQ(order.GetItem(10), TaskItem(QUITMESSAGELOOP, 6, TRUE));
        ASSERT_EQ(order.GetItem(11), TaskItem(QUITMESSAGELOOP, 6, FALSE));
    }

    class TestIOHandler : public MessageLoopForIO::IOHandler
    {
    public:
        TestIOHandler(LPCTSTR lpszName, HANDLE hSignal, BOOL bWait);

        virtual void OnIOCompleted(MessageLoopForIO::IOContext *pContext, DWORD dwBytesTranfered,
            DWORD dwError) override;

        void Init();
        void WaitForIO();

        OVERLAPPED* Context() { return &m_Context.overlapped; }
        DWORD Size() { return sizeof(m_Buffer); }

    private:
        char m_Buffer[48];
        MessageLoopForIO::IOContext m_Context;
        HANDLE m_hSignal;
        ScopedHandle m_hFile;
        BOOL m_bWait;
    };

    TestIOHandler::TestIOHandler(LPCTSTR lpszName, HANDLE hSignal, BOOL bWait)
        : m_hSignal(hSignal), m_bWait(bWait)
    {
        memset(m_Buffer, 0, sizeof(m_Buffer));
        memset(&m_Context, 0, sizeof(m_Context));
        m_Context.pHandler = this;

        m_hFile.Set(CreateFile(lpszName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED, NULL));
        ASSERT(m_hFile != 0);
    }

    void TestIOHandler::OnIOCompleted(MessageLoopForIO::IOContext *pContext, DWORD dwBytesTranfered,
        DWORD dwError)
    {
        ASSERT_TRUE(pContext == &m_Context);
        ASSERT_TRUE(!!SetEvent(m_hSignal));
    }

    void TestIOHandler::Init()
    {
        MessageLoopForIO* pIO = MessageLoopForIO::Current();
        MessageLoopForIO::Current()->RegisterIOHandler(m_hFile, this);

        DWORD dwRead = 0;
        ASSERT_FALSE(ReadFile(m_hFile, m_Buffer, Size(), &dwRead, Context()));
        ASSERT_EQ(ERROR_IO_PENDING, GetLastError());

        if (m_bWait)
        {
            WaitForIO();
        }
    }

    void TestIOHandler::WaitForIO()
    {
        ASSERT_TRUE(MessageLoopForIO::Current()->WaitForIOCompletion(300));
        ASSERT_TRUE(MessageLoopForIO::Current()->WaitForIOCompletion(400));
    }

    void RunTest_IOHandler() {
        ScopedHandle callbackSignal(CreateEvent(NULL, TRUE, FALSE, NULL));
        ASSERT_TRUE(callbackSignal != NULL);

        LPCTSTR lpszPipeName = L"\\\\.\\pipe\\iohandler_pipe";
        ScopedHandle server(CreateNamedPipe(lpszPipeName, PIPE_ACCESS_OUTBOUND, 0, 1, 0, 0, 0, NULL));
        ASSERT_TRUE(server != NULL);

        MessageLoopThread thread(L"IOHandler test");
        MessageLoopThread::Options options;
        options.messageLoopType = MessageLoop::Type_IO;
        ASSERT_TRUE(thread.StartWithOptions(options));

        MessageLoop* loop = thread.GetMessageLoop();
        ASSERT_TRUE(NULL != loop);

        TestIOHandler handler(lpszPipeName, callbackSignal, false);
        loop->PostTask(CreateTaskClosure(&TestIOHandler::Init, &handler));
        Sleep(100);  // Make sure the thread runs and sleeps for lack of work.

        const char buffer[] = "Hello there!";
        DWORD dwWritten;
        ASSERT_TRUE(!!WriteFile(server, buffer, sizeof(buffer), &dwWritten, NULL));

        DWORD dwResult = WaitForSingleObject(callbackSignal, 1000);
        ASSERT_EQ(WAIT_OBJECT_0, dwResult);

        thread.Stop();
    }

    void RunTest_WaitForIO() {
        ScopedHandle callbackSignal1(CreateEvent(NULL, TRUE, FALSE, NULL));
        ScopedHandle callbackSignal2(CreateEvent(NULL, TRUE, FALSE, NULL));
        ASSERT_TRUE(callbackSignal1 != NULL);
        ASSERT_TRUE(callbackSignal2 != NULL);

        LPCTSTR lpszPipeName1 = L"\\\\.\\pipe\\iohandler_pipe1";
        LPCTSTR lpszPipeName2 = L"\\\\.\\pipe\\iohandler_pipe2";
        ScopedHandle server1(
            CreateNamedPipe(lpszPipeName1, PIPE_ACCESS_OUTBOUND, 0, 1, 0, 0, 0, NULL));
        ScopedHandle server2(
            CreateNamedPipe(lpszPipeName2, PIPE_ACCESS_OUTBOUND, 0, 1, 0, 0, 0, NULL));
        ASSERT_TRUE(server1 != NULL);
        ASSERT_TRUE(server2 != NULL);

        MessageLoopThread thread(L"IOHandler test");
        MessageLoopThread::Options options;
        options.messageLoopType= MessageLoop::Type_IO;
        ASSERT_TRUE(thread.StartWithOptions(options));
        Sleep(10);

        MessageLoop* loop = thread.GetMessageLoop();
        ASSERT_TRUE(NULL != loop);

        TestIOHandler handler1(lpszPipeName1, callbackSignal1, false);
        TestIOHandler handler2(lpszPipeName2, callbackSignal2, true);
        loop->PostTask(CreateTaskClosure(&TestIOHandler::Init, &handler1));
        Sleep(100);  // Make sure the thread runs and sleeps for lack of work.
        loop->PostTask(CreateTaskClosure(&TestIOHandler::Init, &handler2));
        Sleep(100);

        // At this time handler1 is waiting to be called, and the thread is waiting
        // on the Init method of handler2, filtering only handler2 callbacks.

        const char buffer[] = "Hello there!";
        DWORD dwWritten = 0;
        ASSERT_TRUE(!!WriteFile(server1, buffer, sizeof(buffer), &dwWritten, NULL));
        Sleep(200);

        ASSERT_TRUE(!!WriteFile(server2, buffer, sizeof(buffer), &dwWritten, NULL));

        HANDLE objects[2] = { callbackSignal1, callbackSignal2 };
        DWORD dwResult = WaitForMultipleObjects(2, objects, TRUE, 1000);
        ASSERT_EQ(WAIT_OBJECT_0, dwResult);

        thread.Stop();
    }
}

TEST(MessageLoopTest, PostTask)
{
    RunTest_PostTask(MessageLoop::Type_default);
    RunTest_PostTask(MessageLoop::Type_UI);
    RunTest_PostTask(MessageLoop::Type_IO);
}

TEST(MessageLoopTest, PostDelayTask_InPostOrder)
{
    RunTest_PostDelayedTask_InPostOrder(MessageLoop::Type_default);
    RunTest_PostDelayedTask_InPostOrder(MessageLoop::Type_UI);
    RunTest_PostDelayedTask_InPostOrder(MessageLoop::Type_IO);
}

TEST(MessageLoopTest, PostDelayTask_InPostOrder2)
{
    RunTest_PostDelayedTask_InPostOrder2(MessageLoop::Type_default);
    RunTest_PostDelayedTask_InPostOrder2(MessageLoop::Type_UI);
    RunTest_PostDelayedTask_InPostOrder2(MessageLoop::Type_IO);
}

TEST(MessageLoopTest, PostDelayTask_SharedTime)
{
    RunTest_PostDelayedTask_SharedTime(MessageLoop::Type_default);
    RunTest_PostDelayedTask_SharedTime(MessageLoop::Type_UI);
    RunTest_PostDelayedTask_SharedTime(MessageLoop::Type_IO);
}

TEST(MessageLoopTest, PostDelayTask_Basic)
{
    RunTest_PostDelayedTask_Basic(MessageLoop::Type_default);
    RunTest_PostDelayedTask_Basic(MessageLoop::Type_UI);
    RunTest_PostDelayedTask_Basic(MessageLoop::Type_IO);
}

TEST(MessageLoopTest, PostDelayTask_InDelayedOrder)
{
    RunTest_PostDelayedTask_InDelayOrder(MessageLoop::Type_default);
    RunTest_PostDelayedTask_InDelayOrder(MessageLoop::Type_UI);
    RunTest_PostDelayedTask_InDelayOrder(MessageLoop::Type_IO);
}

TEST(MessageLoopTest, PostDelayTask_SharedTime_SubPump)
{
    RunTest_PostDelayedTask_SharedTime_SubPump();
}

TEST(MessageLoopTest, NestingFunc)
{
    RunTest_NestingFunc(MessageLoop::Type_default);
    RunTest_NestingFunc(MessageLoop::Type_UI);
    RunTest_NestingFunc(MessageLoop::Type_IO);
}

TEST(MessageLoopTest, Recursive1)
{
    RunTest_Recursive1(MessageLoop::Type_default);
    RunTest_Recursive1(MessageLoop::Type_UI);
    RunTest_Recursive1(MessageLoop::Type_IO);
}

TEST(MessageLoopTest, Recursive2)
{
    RunTest_Recursive2(MessageLoop::Type_default);
    RunTest_Recursive2(MessageLoop::Type_UI);
    RunTest_Recursive2(MessageLoop::Type_IO);
}

TEST(MessageLoopTest, Recursive3)
{
    RunTest_Recursive3(MessageLoop::Type_default);
    RunTest_Recursive3(MessageLoop::Type_UI);
    RunTest_Recursive3(MessageLoop::Type_IO);
}

TEST(MessageLoopTest, Recursive4)
{
    RunTest_Recursive4(MessageLoop::Type_UI);
}

TEST(MessageLoopTest, NonNestableWithNoNesting)
{
    RunTest_NonNestableWithNoNesting(MessageLoop::Type_default);
    RunTest_NonNestableWithNoNesting(MessageLoop::Type_UI);
    RunTest_NonNestableWithNoNesting(MessageLoop::Type_IO);
}

TEST(MessageLoopTest, NonNestableInNestedLoop_False)
{
    RunTest_NonNestableInNestedLoop(MessageLoop::Type_default, FALSE);
    RunTest_NonNestableInNestedLoop(MessageLoop::Type_UI, FALSE);
    RunTest_NonNestableInNestedLoop(MessageLoop::Type_IO, FALSE);
}

TEST(MessageLoopTest, NonNestableInNestedLoop_True)
{
    RunTest_NonNestableInNestedLoop(MessageLoop::Type_default, TRUE);
    RunTest_NonNestableInNestedLoop(MessageLoop::Type_UI, TRUE);
    RunTest_NonNestableInNestedLoop(MessageLoop::Type_IO, TRUE);
}

TEST(MessageLoopTest, IOHandler)
{
    RunTest_IOHandler();
}

TEST(MessageLoopTest, WaitForIO)
{
    RunTest_WaitForIO();
}
