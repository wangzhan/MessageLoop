#pragma once
#include <windows.h>

class TlsHelper
{
public:
    TlsHelper()
    {
        m_dwTlsIndex = TlsAlloc();
    }

    ~TlsHelper()
    {
        TlsFree(m_dwTlsIndex);
    }

    void SetValue(void *pValue)
    {
        TlsSetValue(m_dwTlsIndex, pValue);
    }

    void* GetValue()
    {
        return TlsGetValue(m_dwTlsIndex);
    }

private:
    DWORD m_dwTlsIndex;
};
