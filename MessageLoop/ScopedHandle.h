// Author: WangZhan -> wangzhan.1985@gmail.com
#pragma once

class ScopedHandle
{
public:
    ScopedHandle() : m_handle(NULL) {}
    ScopedHandle(HANDLE handle) : m_handle(handle) {}
    ~ScopedHandle() 
    { 
        if (m_handle)
        {
            CloseHandle(m_handle);
            m_handle = NULL;
        }
    }

    operator HANDLE () { return m_handle; }

    void Set(HANDLE handle) { m_handle = handle; }
    HANDLE Get() const { return m_handle; }

private:
    HANDLE m_handle;
};
