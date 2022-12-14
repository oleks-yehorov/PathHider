// ProjectFilter(KUtils)
#pragma once
//warning C28167: The function changes the IRQL and does not restore the IRQL before it exits. It should be annotated to reflect the change or the IRQL should be restored
#pragma warning(push)
#pragma warning(disable : 28167)
namespace KUtils
{
    template <typename TLock> struct AutoLock
    {
        AutoLock(TLock& lock) : m_lock(lock) { m_lock.Lock(); }

        ~AutoLock() { m_lock.Unlock(); }

      private:
        TLock& m_lock;
    };
} // namespace KUtils
#pragma warning(pop)