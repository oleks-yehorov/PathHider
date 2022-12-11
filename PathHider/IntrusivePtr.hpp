#pragma once

#include "Constants.h"
#include "Memory.h"
#include <fltkernel.h>

namespace KUtils
{

    template <class T> class intrusive_ptr
    {
      private:
        typedef intrusive_ptr this_type;

      public:
        typedef T element_type;

        intrusive_ptr() : m_p(0) {}

        intrusive_ptr(T* p, bool add_ref = true) : m_p(p)
        {
            if (m_p != 0 && add_ref)
                intrusive_ptr_add_ref(m_p);
        }

        intrusive_ptr(intrusive_ptr const& rhs) : m_p(rhs.m_p)
        {
            if (m_p != 0)
                intrusive_ptr_add_ref(m_p);
        }

        ~intrusive_ptr()
        {
            if (m_p != 0)
                intrusive_ptr_release(m_p);
        }

        intrusive_ptr& operator=(intrusive_ptr const& rhs)
        {
            this_type(rhs).swap(*this);
            return *this;
        }

        intrusive_ptr& operator=(T* rhs)
        {
            this_type(rhs).swap(*this);
            return *this;
        }

        T* get() const { return m_p; }

        T& operator*() const { return *m_p; }

        T* operator->() const { return m_p; }

        typedef T* this_type::*unspecified_bool_type;

        operator unspecified_bool_type() const { return m_p == 0 ? 0 : &this_type::m_p; }

        bool operator!() const { return m_p == 0; }

        void swap(intrusive_ptr& rhs)
        {
            T* tmp = m_p;
            m_p = rhs.m_p;
            rhs.m_p = tmp;
        }

      private:
        T* m_p;
    };

    template <class T, class U> inline bool operator==(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) { return a.get() == b.get(); }

    template <class T, class U> inline bool operator!=(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) { return a.get() != b.get(); }

    template <class T> inline bool operator==(intrusive_ptr<T> const& a, T* b) { return a.get() == b; }

    template <class T> inline bool operator!=(intrusive_ptr<T> const& a, T* b) { return a.get() != b; }

    template <class T> inline bool operator==(T* a, intrusive_ptr<T> const& b) { return a == b.get(); }

    template <class T> inline bool operator!=(T* a, intrusive_ptr<T> const& b) { return a != b.get(); }

    template <class T> inline bool operator<(intrusive_ptr<T> const& a, intrusive_ptr<T> const& b) { return a.get() < b.get(); }

    template <class T> void swap(intrusive_ptr<T>& lhs, intrusive_ptr<T>& rhs) { lhs.swap(rhs); }

    template <class T> T* get_pointer(intrusive_ptr<T> const& p) { return p.get(); }

    template <class T, class U> intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> const& p) { return static_cast<T*>(p.get()); }

    template <class T, class U> intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> const& p) { return const_cast<T*>(p.get()); }

    template <class T, class U> intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> const& p) { return dynamic_cast<T*>(p.get()); }

    class RefCountedBase
    {
      public:
        RefCountedBase() : m_refCount(0) {}
        virtual ~RefCountedBase() {}
        long volatile m_refCount;
    };

    template <class T> inline void intrusive_ptr_add_ref(T* pointer) { InterlockedIncrement((PLONG)&pointer->m_refCount); }

    template <class T> inline void intrusive_ptr_release(T* pointer)
    {
        if (InterlockedDecrement((PLONG)&pointer->m_refCount) == 0)
        {
            delete pointer;
        }
    }
}



