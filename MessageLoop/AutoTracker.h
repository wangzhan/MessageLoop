#pragma once
#include <windows.h>
#include <memory>

class TrackerFlag
{
public:
    TrackerFlag() : m_lCancelTask(0) {}
    ~TrackerFlag() {}

    bool DoesCanceled()
    {
        return m_lCancelTask;
    }

    friend class AutoTracker;

private:
    long m_lCancelTask;
};

class AutoTracker
{
public:
    AutoTracker()
    {
        m_pFalg.reset(new TrackerFlag);
    }

    ~AutoTracker()
    {
        InterlockedExchange(&(m_pFalg->m_lCancelTask), 1);
    }

private:
    std::shared_ptr<TrackerFlag> m_pFalg;
};
