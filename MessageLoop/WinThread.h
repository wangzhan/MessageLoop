// Author: WangZhan -> wangzhan.1985@gmail.com
#pragma once


class WinThread
{
public:
    struct Options
    {
        Options() : iStackSize(0) {}
        Options(int iSize) : iStackSize(iSize) {}

        int iStackSize;
    };

    explicit WinThread(const tstring &csThreadName);
    WinThread(const tstring &csThreadName, const Options &options);
    virtual ~WinThread();

    bool Start();
    void Stop();

    DWORD GetThreadID() const { return m_dwThreadID; }
    HANDLE GetThreadHandle() const { return m_hThreadHandle; }
    tstring GetThreadName() const { return m_csThreadName; }

    bool IsStarted() const { return m_bStarted; }
    bool IsStopping() const { return m_bStopping; }

protected:
    void Join();

    static DWORD WINAPI ThreadProc(void *pParam);
    virtual void ThreadMain() = 0;

protected:
    Options m_Options;

    bool m_bStarted;
    bool m_bStopping;

    HANDLE m_hThreadHandle;
    DWORD m_dwThreadID;
    tstring m_csThreadName;
};
