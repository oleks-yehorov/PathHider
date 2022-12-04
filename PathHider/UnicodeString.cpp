// ProjectFilter(KUtils)
#pragma once

#include "UnicodeString.h"
#include "Constants.h"
#include <limits.h>

// TODO Get rid of c-style casts
namespace KUtils
{
    UnicodeString::UnicodeString(POOL_TYPE poolType)
        : SimpleUString(), m_PoolType(poolType), m_Buffer(NULL), m_BufferSize(0)
    {
    }

    UnicodeString::UnicodeString(PCWCH str, POOL_TYPE poolType)
        : SimpleUString(), m_PoolType(poolType), m_Buffer(NULL), m_BufferSize(0)
    {
        if (str != NULL)
        {
            ULONG length = static_cast<ULONG>(wcslen(str) * sizeof(WCHAR));
            NT_ASSERT(length < USHRT_MAX);
            this->Init(str, (USHORT)length, (USHORT)length);
        }
    }

    UnicodeString::UnicodeString(PCWCH str,
                                 USHORT byteLength,
                                 POOL_TYPE poolType)
        : SimpleUString(), m_PoolType(poolType), m_Buffer(NULL), m_BufferSize(0)
    {
        if (str != NULL && byteLength != 0)
        {
            this->Init(str, byteLength, byteLength);
        }
    }

    UnicodeString::UnicodeString(const UNICODE_STRING& str, POOL_TYPE poolType)
        : SimpleUString(), m_PoolType(poolType), m_Buffer(NULL), m_BufferSize(0)
    {
        this->Init(str.Buffer, str.Length, str.MaximumLength);
    }

    UnicodeString::UnicodeString(const SimpleUString& str, POOL_TYPE poolType)
        : SimpleUString(), m_PoolType(poolType), m_Buffer(NULL), m_BufferSize(0)
    {
        this->Init(str.Buffer(), str.ByteLength(), str.MaxByteLength());
    }

    UnicodeString::UnicodeString(USHORT maxByteLength, POOL_TYPE poolType)
        : SimpleUString(), m_PoolType(poolType), m_Buffer(NULL), m_BufferSize(0)
    {
        if (maxByteLength > 0)
        {
            m_Buffer = ExAllocatePoolWithTag(m_PoolType, (ULONG)maxByteLength,
                                             DRIVER_TAG);
            if (!m_Buffer)
            {
                ExRaiseStatus(STATUS_NO_MEMORY);
            }
            m_BufferSize = maxByteLength;
            m_Str.Buffer = (PWSTR)m_Buffer;
            m_Str.Length = 0;
            m_Str.MaximumLength = maxByteLength;
        }
    }

    UnicodeString::UnicodeString(const UnicodeString& str)
        : SimpleUString(), m_PoolType(str.m_PoolType), m_Buffer(NULL),
          m_BufferSize(0)
    {
        this->Init(str.m_Str.Buffer, str.m_Str.Length, str.m_Str.MaximumLength);
    }

    void UnicodeString::Init(PCWCH str, USHORT length, USHORT maxByteLength)
    {
        ASSERT(m_Str.Buffer == NULL);
        ASSERT(m_Buffer == NULL);
        ASSERT(length <= maxByteLength);

        if (maxByteLength > 0)
        {
            m_Buffer = ExAllocatePoolWithTag(m_PoolType, (ULONG)maxByteLength,
                                             DRIVER_TAG);
            if (!m_Buffer)
            {
                ExRaiseStatus(STATUS_NO_MEMORY);
            }
            m_BufferSize = maxByteLength;
            m_Str.Buffer = (PWSTR)m_Buffer;
            RtlCopyMemory(m_Str.Buffer, str, length);
        }

        m_Str.Length = length;
        m_Str.MaximumLength = maxByteLength;
    }

    UnicodeString::~UnicodeString() { ExFreePoolWithTag(m_Buffer, DRIVER_TAG); }

    void UnicodeString::Clear()
    {
        m_Str.Length = 0;
        m_Str.MaximumLength = m_BufferSize;
        m_Str.Buffer = (PWSTR)m_Buffer;
    }

    UnicodeString& UnicodeString::operator=(const SimpleUString& str)
    {
        this->Clear();
        this->Realloc(str.ByteLength());

        RtlCopyMemory(m_Str.Buffer, str.Buffer(), str.ByteLength());
        m_Str.Length = str.ByteLength();

        return *this;
    }

    UnicodeString& UnicodeString::operator=(const UnicodeString& str)
    {
        if (this == &str)
        {
            return *this;
        }

        return UnicodeString::operator=((const SimpleUString&)str);
    }

    UnicodeString& UnicodeString::operator+=(const SimpleUString& str)
    {
        ASSERT(this->IsValid() && str.IsValid());

        if (!str.IsEmpty())
        {
            ULONG newMaxByteLength =
                ((m_Str.Length + str.ByteLength()) * 3 / 2) & ~1;
            if (newMaxByteLength > USHRT_MAX)
            {
                newMaxByteLength = USHRT_MAX;
            }

            USHORT newByteLength = m_Str.Length + str.ByteLength();
            if (newByteLength > newMaxByteLength)
            {
                newByteLength = (USHORT)newMaxByteLength;
            }

            this->Realloc((USHORT)newMaxByteLength);

            RtlCopyMemory(m_Str.Buffer + this->CharLength(), str.Buffer(),
                          newByteLength - m_Str.Length);

            m_Str.Length = newByteLength;
        }

        return *this;
    }

    void UnicodeString::Realloc(USHORT newMaxByteLength)
    {
        if (m_Str.MaximumLength < newMaxByteLength)
        {
            if (m_BufferSize < newMaxByteLength)
            {
                PVOID pNewBuffer = ExAllocatePoolWithTag(
                    m_PoolType, (ULONG)newMaxByteLength, DRIVER_TAG);
                if (!pNewBuffer)
                {
                    ExRaiseStatus(STATUS_NO_MEMORY);
                }
                if (m_Buffer != NULL)
                {
                    if (m_Str.Length != 0)
                    {
                        RtlCopyMemory(pNewBuffer, m_Str.Buffer, m_Str.Length);
                    }
                    ExFreePoolWithTag(m_Buffer, DRIVER_TAG);
                }

                m_Buffer = pNewBuffer;
                m_BufferSize = newMaxByteLength;
            }
            else
            {
                if (m_Str.Length != 0)
                {
                    RtlMoveMemory(m_Buffer, m_Str.Buffer, m_Str.Length);
                }
            }

            m_Str.Buffer = (PWSTR)m_Buffer;
            m_Str.MaximumLength = m_BufferSize;
        }
    }

    UNICODE_STRING UnicodeString::Detach()
    {
        UNICODE_STRING tmp = m_Str;

        m_Buffer = NULL;
        m_BufferSize = 0;
        this->Empty();

        return tmp;
    }

    UnicodeString operator+(const SimpleUString& str1,
                            const SimpleUString& str2)
    {
        UnicodeString newStr(str1);
        newStr += str2;
        return newStr;
    }

} // namespace KUtils
