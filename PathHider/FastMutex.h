// ProjectFilter(KUtils)
#pragma once

#include <wdm.h>
namespace KUtils
{
    class FastMutex
    {
      public:
        void Init();

        _IRQL_raises_(APC_LEVEL) _IRQL_saves_global_(OldIrql, &m_mutex) void Lock();
        _IRQL_requires_(APC_LEVEL) _IRQL_restores_global_(OldIrql, &m_mutex) void Unlock();

      private:
        FAST_MUTEX m_mutex;
    };
} // namespace KUtils