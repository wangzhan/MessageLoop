// Author: WangZhan -> wangzhan.1985@gmail.com
#pragma once
#include "BasicTypes.h"
#include "MessagePump.h"


class MessagePumpForUI : public MessagePumpForMsg
{
public:
    MessagePumpForUI();
    ~MessagePumpForUI();

    virtual void ScheduleWork();
    virtual void ScheduleDelayedWork(DWORD dwDelayedWorkTime);

private:
    virtual void DoRunLoop();

    void InitMessageWnd();
    void ClearMessageWnd();

    bool ProcessNextWindowsMessage();
    // 处理have_work_msg时，先处理一个系统消息，防止have_work_msg阻塞消息循环
    bool ProcessPumpReplacementMessage();
    bool ProcessMessageHelp(MSG &msg);

    void HandleWorkMessage();
    void HandleTimerMessage();

    void WaitForWork();

    static LRESULT CALLBACK WndProcThunk(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    HWND m_hMsgWnd;

    DISALLOW_COPY_AND_ASSIGN(MessagePumpForUI);
};
