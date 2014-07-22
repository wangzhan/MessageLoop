#include "stdafx.h"
#include <tchar.h>
#include "MessagePumpForUI.h"

namespace
{
    LPCTSTR g_lpszWndClass = _T("MessageLoopWnd");
    const int g_MsgHaveWork = WM_USER + 1;
}

MessagePumpForUI::MessagePumpForUI()
: m_hMsgWnd(0)
{
    InitMessageWnd();
}


MessagePumpForUI::~MessagePumpForUI()
{
    ClearMessageWnd();
}

void MessagePumpForUI::InitMessageWnd()
{
    HINSTANCE hInst = GetModuleHandle(NULL);

    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.hInstance = hInst;
    wc.lpfnWndProc = WndProcThunk;
    wc.lpszClassName = g_lpszWndClass;
    RegisterClassEx(&wc);

    m_hMsgWnd = CreateWindow(g_lpszWndClass, 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, hInst, 0);
    ZhanAssert(m_hMsgWnd);
}

void MessagePumpForUI::ClearMessageWnd()
{
    DestroyWindow(m_hMsgWnd);
    UnregisterClass(g_lpszWndClass, GetModuleHandle(NULL));
}

void MessagePumpForUI::ScheduleWork()
{
    if (::InterlockedExchange(&m_lHaveWork, 1))
    {
        return;
    }

    PostMessage(m_hMsgWnd, g_MsgHaveWork, reinterpret_cast<WPARAM>(this), NULL);
}

void MessagePumpForUI::ScheduleDelayedWork(DWORD dwDelayedWorkTime)
{
    m_dwDelayedWorkTime = dwDelayedWorkTime;

    int iTimeout = GetCurrentDelay();

    // 防止出现负值
    if (iTimeout < USER_TIMER_MINIMUM)
    {
        iTimeout = USER_TIMER_MINIMUM;
    }

    SetTimer(m_hMsgWnd, reinterpret_cast<UINT>(this), iTimeout, NULL);
}

void MessagePumpForUI::DoRunLoop()
{
    for (;;)
    {
        bool bDidWork = ProcessNextWindowsMessage();
        if (m_pState->bShouldQuit)
        {
            break;
        }

        bDidWork |= m_pState->pDelegate->DoWork();
        if (m_pState->bShouldQuit)
        {
            break;
        }

        bDidWork |= m_pState->pDelegate->DoDelayedWork(&m_dwDelayedWorkTime);
        if (bDidWork && m_dwDelayedWorkTime == 0)
        {
            KillTimer(m_hMsgWnd, reinterpret_cast<UINT>(this));
        }

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

void MessagePumpForUI::WaitForWork()
{
    int iTimeout = GetCurrentDelay();
    if (iTimeout < 0)
    {
        iTimeout = INFINITE;
    }

    DWORD dwResult = MsgWaitForMultipleObjectsEx(0, NULL, iTimeout,
        QS_ALLINPUT, MWMO_INPUTAVAILABLE);

    if (dwResult == WAIT_OBJECT_0)
    {
        // 让子窗口处理特殊的WM_MOUSE事件
        MSG msg = { 0 };
        DWORD dwStatus = GetQueueStatus(QS_MOUSE);
        if (HIWORD(dwStatus) & QS_MOUSE &&
            !PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_NOREMOVE))
        {
            WaitMessage();
        }
    }
}

bool MessagePumpForUI::ProcessNextWindowsMessage()
{
    bool bDidWork = false;

    // 系统自动处理 Send 消息
    DWORD dwStatus = GetQueueStatus(QS_SENDMESSAGE);
    if (HIWORD(dwStatus) & QS_SENDMESSAGE)
    {
        bDidWork = true;
    }

    MSG msg = { 0 };
    if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
    {
        bDidWork |= ProcessMessageHelp(msg);
    }

    return bDidWork;
}

bool MessagePumpForUI::ProcessPumpReplacementMessage()
{
    ::InterlockedExchange(&m_lHaveWork, 0);

    MSG msg = { 0 };
    if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
    {
        return ProcessMessageHelp(msg);
    }

    return false;
}

bool MessagePumpForUI::ProcessMessageHelp(MSG &msg)
{
    if (msg.message == WM_QUIT)
    {
        m_pState->bShouldQuit = true;
        PostQuitMessage(static_cast<int>(msg.wParam));
        return false;
    }

    if (msg.message == g_MsgHaveWork)
    {
        return ProcessPumpReplacementMessage();
    }

    WillProcessMessage(msg);

    TranslateMessage(&msg);
    DispatchMessage(&msg);

    DidProcessMessage(msg);

    return true;
}

LRESULT MessagePumpForUI::WndProcThunk(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case g_MsgHaveWork:
        reinterpret_cast<MessagePumpForUI*>(wParam)->HandleWorkMessage();
        break;
    case WM_TIMER:
        reinterpret_cast<MessagePumpForUI*>(wParam)->HandleTimerMessage();
        break;
    default:
        break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

void MessagePumpForUI::HandleWorkMessage()
{
    if (m_pState == NULL)
    {
        ::InterlockedExchange(&m_lHaveWork, 0);
        return;
    }

    ProcessPumpReplacementMessage();

    ZhanAssert(m_pState->pDelegate);
    if (m_pState->pDelegate->DoWork())
    {
        ScheduleWork();
    }
}

void MessagePumpForUI::HandleTimerMessage()
{
    KillTimer(m_hMsgWnd, reinterpret_cast<UINT>(this));

    if (m_pState == NULL)
    {
        return;
    }

    ZhanAssert(m_pState->pDelegate);
    m_pState->pDelegate->DoDelayedWork(&m_dwDelayedWorkTime);

    ScheduleDelayedWork(m_dwDelayedWorkTime);
}
