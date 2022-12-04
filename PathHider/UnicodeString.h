// ProjectFilter(KUtils)
#pragma once

#include <ntddk.h>

namespace KUtils
{
    class SimpleUString
    {
      public:
        SimpleUString();
        SimpleUString(const WCHAR& ch);
        SimpleUString(PWCHAR str);
        SimpleUString(PWCHAR str, USHORT byteLength);
        SimpleUString(PCUNICODE_STRING str);
        SimpleUString(const UNICODE_STRING& str);
        SimpleUString(const SimpleUString& str);

        ~SimpleUString();

        SimpleUString& operator=(const SimpleUString& str);

        UNICODE_STRING& GetUnicodeString();
        const UNICODE_STRING& GetUnicodeString() const;

        operator UNICODE_STRING*();
        operator const UNICODE_STRING*() const;

        bool IsValid() const;
        bool IsEmpty() const;
        void Empty();

        USHORT ByteLength() const;
        USHORT MaxByteLength() const;
        USHORT CharLength() const;
        USHORT MaxCharLength() const;

        PWCHAR Buffer();
        PCWCH Buffer() const;

        WCHAR At(USHORT charCount) const;
        WCHAR& At(USHORT charCount);

        WCHAR AtChar(USHORT charCount) const;
        WCHAR& AtChar(USHORT charCount);

        WCHAR AtByte(USHORT byteCount) const;
        WCHAR& AtByte(USHORT byteCount);

        WCHAR FirstChar() const;
        void TrimFirstChar();

        WCHAR LastChar() const;
        void TrimLastChar();

        // Comparison
        bool operator==(const SimpleUString& str) const;
        bool operator!=(const SimpleUString& str) const;
        bool operator<(const SimpleUString& str) const;
        bool operator>(const SimpleUString& str) const;
        bool operator<=(const SimpleUString& str) const;
        bool operator>=(const SimpleUString& str) const;

        // Comparison
        LONG Compare(const SimpleUString& str,
                     BOOLEAN caseInSensitive = FALSE) const;
        LONG CompareNoCase(const SimpleUString& str) const;

        LONG CompareByteCount(const SimpleUString& str,
                              USHORT byteCount,
                              BOOLEAN caseInSensitive = FALSE) const;
        LONG CompareByteCountNoCase(const SimpleUString& str,
                                    USHORT byteCount) const;

        LONG CompareCharCount(const SimpleUString& str,
                              USHORT charCount,
                              BOOLEAN caseInSensitive = FALSE) const;
        LONG CompareCharCountNoCase(const SimpleUString& str,
                                    USHORT charCount) const;

        // Convert the string to uppercase
        void ToUpper();

        // Find the first occurrence of character 'ch', starting at index
        // 'startChar'
        int Find(WCHAR ch, int startChar = 0) const;
        // Find the last occurrence of character 'ch'
        int ReverseFind(WCHAR ch) const;

        // Remove all trailing occurrences of character 'ch'
        void TrimRight(WCHAR ch);
        // Remove all leading occurrences of character 'ch'
        void TrimLeft(WCHAR ch);
        // Remove all leading and trailing occurrences of character 'ch'
        void Trim(WCHAR ch);

        // Remove leftmost characters
        void TrimLeftCharCount(USHORT charCount);
        void TrimLeftByteCount(USHORT byteCount);

        // Remove rightmost characters
        void TrimRightCharCount(USHORT charCount);
        void TrimRightByteCount(USHORT byteCount);

        // Leftmost characters
        void LeftCharCount(USHORT charCount);
        void LeftByteCount(USHORT byteCount);

        // Rightmost characters
        void RightCharCount(USHORT charCount);
        void RightByteCount(USHORT byteCount);

        // Make the substring starting at specified index with specified length
        void MidCharCount(USHORT firstChar, USHORT charCount);
        void MidByteCount(USHORT firstByte, USHORT byteCount);

      protected:
        UNICODE_STRING m_Str;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // UnicodeString

    class UnicodeString : public SimpleUString
    {
      public:
        UnicodeString(POOL_TYPE poolType = NonPagedPool);
        explicit UnicodeString(PCWCH str, POOL_TYPE poolType = NonPagedPool);
        explicit UnicodeString(PCWCH str,
                               USHORT byteLength,
                               POOL_TYPE poolType = NonPagedPool);
        explicit UnicodeString(const UNICODE_STRING& str,
                               POOL_TYPE poolType = NonPagedPool);
        explicit UnicodeString(const SimpleUString& str,
                               POOL_TYPE poolType = NonPagedPool);
        explicit UnicodeString(USHORT maxByteLength,
                               POOL_TYPE poolType = NonPagedPool);
        UnicodeString(const UnicodeString& str);

        ~UnicodeString();

        // Release ownership
        UNICODE_STRING Detach();

        void Realloc(USHORT newMaxByteLength);
        void Clear();

        UnicodeString& operator=(const SimpleUString& str);
        UnicodeString& operator=(const UnicodeString& str);
        UnicodeString& operator+=(const SimpleUString& str);

      protected:
        void Init(PCWCH str, USHORT length, USHORT maxLength);

      protected:
        POOL_TYPE m_PoolType;
        PVOID m_Buffer;
        USHORT m_BufferSize;
    };

    UnicodeString operator+(const SimpleUString& str1,
                            const SimpleUString& str2);
    UnicodeString operator+(const SimpleUString& str1, WCHAR ch);

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // inline

    inline SimpleUString::SimpleUString()
    {
        m_Str.Buffer = NULL;
        m_Str.Length = 0;
        m_Str.MaximumLength = 0;
    }

    inline SimpleUString::SimpleUString(const WCHAR& ch)
    {
        m_Str.Buffer = const_cast<PWSTR>(&ch);
        m_Str.MaximumLength = m_Str.Length = sizeof(WCHAR);
    }

    inline SimpleUString::SimpleUString(PWCHAR str)
    {
        RtlInitUnicodeString(&m_Str, str);
    }

    inline SimpleUString::SimpleUString(PWCHAR str, USHORT byteLength)
    {
        m_Str.Buffer = str;
        m_Str.MaximumLength = m_Str.Length = byteLength;
    }

    inline SimpleUString::SimpleUString(PCUNICODE_STRING str)
    {
        m_Str = *str;
        ASSERT(this->IsValid());
    }

    inline SimpleUString::SimpleUString(const UNICODE_STRING& str)
    {
        m_Str = str;
        ASSERT(this->IsValid());
    }

    inline SimpleUString::SimpleUString(const SimpleUString& str)
    {
        m_Str = str.GetUnicodeString();
        ASSERT(this->IsValid());
    }

    inline SimpleUString& SimpleUString::operator=(const SimpleUString& str)
    {
        if (this == &str)
        {
            return *this;
        }

        m_Str = str.m_Str;
        ASSERT(this->IsValid());

        return *this;
    }

    inline SimpleUString::~SimpleUString() {}

    inline UNICODE_STRING& SimpleUString::GetUnicodeString() { return m_Str; }

    inline const UNICODE_STRING& SimpleUString::GetUnicodeString() const
    {
        return m_Str;
    }

    inline SimpleUString::operator UNICODE_STRING*() { return &m_Str; }

    inline SimpleUString::operator const UNICODE_STRING*() const
    {
        return (const UNICODE_STRING*)&m_Str;
    }

    inline bool SimpleUString::IsValid() const
    {
        if (m_Str.Buffer == NULL)
        {
            return m_Str.Length == 0 && m_Str.MaximumLength == 0;
        }

        return m_Str.Length <= m_Str.MaximumLength;
    }

    inline bool SimpleUString::IsEmpty() const
    {
        return m_Str.Buffer == NULL || m_Str.Length < sizeof(WCHAR);
    }

    inline void SimpleUString::Empty()
    {
        m_Str.Buffer = NULL;
        m_Str.Length = 0;
        m_Str.MaximumLength = 0;
    }

    inline USHORT SimpleUString::ByteLength() const
    { 
        return m_Str.Length;
    }

    inline USHORT SimpleUString::MaxByteLength() const
    {
        return m_Str.MaximumLength;
    }

    inline USHORT SimpleUString::CharLength() const
    {
        return m_Str.Length/sizeof(WCHAR);
    }

    inline USHORT SimpleUString::MaxCharLength() const
    { 
        return m_Str.MaximumLength / sizeof(WCHAR);
    }

    inline PWCHAR SimpleUString::Buffer() { return m_Str.Buffer; }

    inline PCWCH SimpleUString::Buffer() const { return m_Str.Buffer; }

    inline WCHAR SimpleUString::At(USHORT charCount) const
    {
        NT_ASSERT(charCount < m_Str.MaximumLength);
        return m_Str.Buffer[charCount];
    }

    inline WCHAR& SimpleUString::At(USHORT charCount)
    {
        NT_ASSERT(charCount < m_Str.MaximumLength);
        return m_Str.Buffer[charCount];
    }

    inline WCHAR SimpleUString::AtChar(USHORT charCount) const
    {
        NT_ASSERT(charCount < m_Str.MaximumLength);
        return m_Str.Buffer[charCount];
    }

    inline WCHAR& SimpleUString::AtChar(USHORT charCount)
    {
        NT_ASSERT(charCount < m_Str.MaximumLength);
        return m_Str.Buffer[charCount];
    }

    inline WCHAR SimpleUString::AtByte(USHORT byteCount) const
    {
        return this->AtChar(byteCount / sizeof(WCHAR));
    }

    inline WCHAR& SimpleUString::AtByte(USHORT byteCount)
    {
        return this->AtChar(byteCount / sizeof(WCHAR));
    }

    inline WCHAR SimpleUString::FirstChar() const
    {
        NT_ASSERT(this->CharLength() > 0);
        return this->AtChar(0);
    }

    inline void SimpleUString::TrimFirstChar()
    {
        return this->TrimLeftCharCount(1);
    }

    inline WCHAR SimpleUString::LastChar() const
    {
        NT_ASSERT(this->CharLength() > 0);
        return this->AtChar(this->CharLength() - 1);
    }

    inline void SimpleUString::TrimLastChar()
    {
        return this->TrimRightCharCount(1);
    }

    inline LONG SimpleUString::Compare(const SimpleUString& str,
                                       BOOLEAN caseInSensitive) const
    {
        return RtlCompareUnicodeString(const_cast<PUNICODE_STRING>(&m_Str),
                                       const_cast<SimpleUString&>(str),
                                       caseInSensitive);
    }

    inline LONG SimpleUString::CompareNoCase(const SimpleUString& str) const
    {
        return RtlCompareUnicodeString(const_cast<PUNICODE_STRING>(&m_Str),
                                       const_cast<SimpleUString&>(str), TRUE);
    }

    inline LONG SimpleUString::CompareByteCount(const SimpleUString& str,
                                                USHORT byteCount,
                                                BOOLEAN caseInSensitive) const
    {
        SimpleUString tmp1(*this);
        SimpleUString tmp2(str);

        if (tmp1.ByteLength() > byteCount)
        {
            tmp1.LeftByteCount(byteCount);
        }

        if (tmp2.ByteLength() > byteCount)
        {
            tmp2.LeftByteCount(byteCount);
        }

        return tmp1.Compare(tmp2, caseInSensitive);
    }

    inline LONG SimpleUString::CompareByteCountNoCase(const SimpleUString& str,
                                                      USHORT byteCount) const
    {
        return this->CompareByteCount(str, byteCount, TRUE);
    }

    inline LONG SimpleUString::CompareCharCount(const SimpleUString& str,
                                                USHORT charCount,
                                                BOOLEAN caseInSensitive) const
    {
        return this->CompareByteCount(str, charCount * sizeof(WCHAR),
                                      caseInSensitive);
    }

    inline LONG SimpleUString::CompareCharCountNoCase(const SimpleUString& str,
                                                      USHORT charCount) const
    {
        return this->CompareCharCount(str, charCount, TRUE);
    }

    inline bool SimpleUString::operator==(const SimpleUString& str) const
    {
        return (this->Compare(str, FALSE) == 0);
    }

    inline bool SimpleUString::operator!=(const SimpleUString& str) const
    {
        return (this->Compare(str, FALSE) != 0);
    }

    inline bool SimpleUString::operator<(const SimpleUString& str) const
    {
        return (this->Compare(str, FALSE) < 0);
    }

    inline bool SimpleUString::operator>(const SimpleUString& str) const
    {
        return (this->Compare(str, FALSE) > 0);
    }

    inline bool SimpleUString::operator<=(const SimpleUString& str) const
    {
        return (this->Compare(str, FALSE) <= 0);
    }

    inline bool SimpleUString::operator>=(const SimpleUString& str) const
    {
        return (this->Compare(str, FALSE) >= 0);
    }

    inline void SimpleUString::ToUpper()
    {
        ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

        for (USHORT pos = 0; pos < this->CharLength(); ++pos)
        {
            this->AtChar(pos) = RtlUpcaseUnicodeChar(this->AtChar(pos));
        }
    }

    inline int SimpleUString::Find(WCHAR ch, int startChar) const
    {
        for (int pos = startChar; pos < this->CharLength(); ++pos)
        {
            if (this->AtChar(static_cast<USHORT>(pos)) == ch)
            {
                return pos;
            }
        }

        return -1;
    }

    inline int SimpleUString::ReverseFind(WCHAR ch) const
    {
        for (int pos = this->CharLength() - 1; pos >= 0; --pos)
        {
            if (this->AtChar(static_cast<USHORT>(pos)) == ch)
            {
                return pos;
            }
        }

        return -1;
    }

    inline void SimpleUString::TrimRight(WCHAR ch)
    {
        USHORT pos = this->CharLength() - 1;

        for (; pos >= 0; --pos)
        {
            if (this->AtChar(static_cast<USHORT>(pos)) != ch)
            {
                break;
            }
        }

        return this->LeftCharCount(pos + 1);
    }

    inline void SimpleUString::TrimLeft(WCHAR ch)
    {
        USHORT pos = 0;

        for (; pos < this->CharLength(); ++pos)
        {
            if (this->AtChar(static_cast<USHORT>(pos)) != ch)
            {
                break;
            }
        }

        return this->RightCharCount(this->CharLength() - pos);
    }

    inline void SimpleUString::Trim(WCHAR ch)
    {
        this->TrimRight(ch);
        this->TrimLeft(ch);
    }

    inline void SimpleUString::LeftCharCount(USHORT charCount)
    {
        return this->LeftByteCount(charCount * sizeof(WCHAR));
    }

    inline void SimpleUString::LeftByteCount(USHORT byteCount)
    {
        m_Str.Length = byteCount;
        ASSERT(this->IsValid());
    }

    inline void SimpleUString::RightCharCount(USHORT charCount)
    {
        return this->RightByteCount(charCount * sizeof(WCHAR));
    }

    inline void SimpleUString::RightByteCount(USHORT byteCount)
    {
        m_Str.Buffer += (m_Str.Length - byteCount) / sizeof(WCHAR);
        m_Str.MaximumLength = m_Str.Length = byteCount;
        ASSERT(this->IsValid());
    }

    inline void SimpleUString::TrimLeftCharCount(USHORT charCount)
    {
        return this->TrimLeftByteCount(charCount * sizeof(WCHAR));
    }

    inline void SimpleUString::TrimLeftByteCount(USHORT byteCount)
    {
        this->RightByteCount(this->ByteLength() - byteCount);
    }

    inline void SimpleUString::TrimRightCharCount(USHORT charCount)
    {
        return this->TrimRightByteCount(charCount * sizeof(WCHAR));
    }

    inline void SimpleUString::TrimRightByteCount(USHORT byteCount)
    {
        this->LeftByteCount(this->ByteLength() - byteCount);
    }

    inline void SimpleUString::MidCharCount(USHORT firstChar, USHORT charCount)
    {
        return this->MidByteCount(firstChar * sizeof(WCHAR),
                                  charCount * sizeof(WCHAR));
    }

    inline void SimpleUString::MidByteCount(USHORT firstByte, USHORT byteCount)
    {
        m_Str.Buffer += firstByte / sizeof(WCHAR);
        m_Str.MaximumLength = m_Str.Length = byteCount;
        ASSERT(this->IsValid());
    }

    inline UnicodeString operator+(const SimpleUString& str1, WCHAR ch)
    {
        return str1 + SimpleUString(ch);
    }

} // namespace KUtils