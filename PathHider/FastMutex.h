// ProjectFilter(KUtils)
#pragma once

#include <wdm.h>
namespace KUtils
{
    class FastMutex
    {
      public:
        void Init();

        void Lock();
        void Unlock();

      private:
        FAST_MUTEX m_mutex;
    };
} // namespace KUtils