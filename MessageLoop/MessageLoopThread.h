// Author: WangZhan -> wangzhan.1985@gmail.com
#pragma once
#include "MessageLoop.h"
#include "WaitableEvent.h"

class MessageLoopThread
{
public:
    struct Options
    {
        Options() : messageLoopType(MessageLoop::Type_default), iStackSize(0) {}
        Options(MessageLoop::Type type, size_t iSize) : messageLoopType(type),
        iStackSize(iSize) {}

        MessageLoop::Type messageLoopType;
        size_t iStackSize;
    };

    explicit MessageLoopThread(const tstring &csThreadName);
    virtual ~MessageLoopThread();

    bool Start();
    bool StartWithOptions(const Options &options);
    void Stop();

    MessageLoop* GetMessageLoop() const { return m_pMessageLoop; }

    HANDLE GetThreadHandle() const { return m_hThreadHandle; }
    DWORD GetThreadID() const { return m_dwThreadID; }
    tstring GetThreadName() const { return m_csThreadName; }

protected:
    virtual void Initialize() {}
    virtual void ThreadMain();
    virtual void CleanUp() {}

    static DWORD WINAPI ThreadProc(void *pParam);

private:
    struct StartupData
    {
        explicit StartupData(const Options &op) : option(op), event(true, false) {}

        Options option;
        WaitableEvent event;
    };
    StartupData *m_pStartupData;

    bool m_bStarted;
    bool m_bStopping;

    tstring m_csThreadName;
    HANDLE m_hThreadHandle;
    DWORD m_dwThreadID;

    MessageLoop *m_pMessageLoop;
};
