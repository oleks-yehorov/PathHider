#pragma once

#include <ntddk.h>

#include "Constants.h"

class UnicodeStringGuard final
{
  public:
    explicit UnicodeStringGuard(PUNICODE_STRING unicodeString,
                                ULONG tag = DRIVER_TAG)
        : m_unicodeString(unicodeString), m_tag(tag)
    {
    }

    ~UnicodeStringGuard()
    {
        if (m_unicodeString)
        {
            if (m_unicodeString->Buffer)
            {
                ExFreePoolWithTag(m_unicodeString->Buffer, m_tag);
            }
            m_unicodeString->Length = 0;
            m_unicodeString->MaximumLength = 0;
        }
    }

  private:
    PUNICODE_STRING m_unicodeString;
    ULONG m_tag;
};