#pragma once

#include "Constants.h"
#include "Memory.h"
#include <fltkernel.h>

template<class T> class intrusive_ptr
{
private:

    typedef intrusive_ptr this_type;

public:

    typedef T element_type;

    intrusive_ptr(): p_(0)
    {
    }

    intrusive_ptr(T * p, bool add_ref = true): p_(p)
    {
        if(p_ != 0 && add_ref) intrusive_ptr_add_ref(p_);
    }

    intrusive_ptr(intrusive_ptr const & rhs): p_(rhs.p_)
    {
        if(p_ != 0) intrusive_ptr_add_ref(p_);
    }

    ~intrusive_ptr()
    {
        if(p_ != 0) intrusive_ptr_release(p_);
    }

    intrusive_ptr & operator=(intrusive_ptr const & rhs)
    {
        this_type(rhs).swap(*this);
        return *this;
    }

    intrusive_ptr & operator=(T * rhs)
    {
        this_type(rhs).swap(*this);
        return *this;
    }

    T * get() const
    {
        return p_;
    }

    T & operator*() const
    {
        return *p_;
    }

    T * operator->() const
    {
        return p_;
    }

    typedef T * this_type::*unspecified_bool_type;

    operator unspecified_bool_type () const
    {
        return p_ == 0? 0: &this_type::p_;
    }

    bool operator! () const
    {
        return p_ == 0;
    }

    void swap(intrusive_ptr & rhs)
    {
        T * tmp = p_;
        p_ = rhs.p_;
        rhs.p_ = tmp;
    }

private:

    T * p_;
};

template<class T, class U> inline bool operator==(intrusive_ptr<T> const & a, intrusive_ptr<U> const & b)
{
    return a.get() == b.get();
}

template<class T, class U> inline bool operator!=(intrusive_ptr<T> const & a, intrusive_ptr<U> const & b)
{
    return a.get() != b.get();
}

template<class T> inline bool operator==(intrusive_ptr<T> const & a, T * b)
{
    return a.get() == b;
}

template<class T> inline bool operator!=(intrusive_ptr<T> const & a, T * b)
{
    return a.get() != b;
}

template<class T> inline bool operator==(T * a, intrusive_ptr<T> const & b)
{
    return a == b.get();
}

template<class T> inline bool operator!=(T * a, intrusive_ptr<T> const & b)
{
    return a != b.get();
}

template<class T> inline bool operator<(intrusive_ptr<T> const & a, intrusive_ptr<T> const & b)
{
    return a.get() < b.get();
}

template<class T> void swap(intrusive_ptr<T> & lhs, intrusive_ptr<T> & rhs)
{
    lhs.swap(rhs);
}

template<class T> T * get_pointer(intrusive_ptr<T> const & p)
{
    return p.get();
}

template<class T, class U> intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> const & p)
{
    return static_cast<T *>(p.get());
}

template<class T, class U> intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> const & p)
{
    return const_cast<T *>(p.get());
}

template<class T, class U> intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> const & p)
{
    return dynamic_cast<T *>(p.get());
}

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
        //pointer->~T();
        //ExFreePoolWithTag(pointer, DRIVER_TAG);
        delete pointer;
    }
}




