#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

std::string FormatErrorA(DWORD dwError, DWORD dwLanguageId);

inline std::string FormatError(DWORD status, DWORD dwLanguageId = 0)
{
    return FormatErrorA(status, dwLanguageId);
}

void wchar2str(const wchar_t* str, size_t strSize, std::vector<char>* res, UINT cpId);

inline void wchar2str_unthrowable(const wchar_t* str, size_t strSize, std::vector<char>* res, UINT cpId = CP_ACP)
{
    try
    {
        wchar2str(str, strSize, res, cpId);
    }
    catch (const std::exception&) {}//for capability
}

std::string wstring2string(const std::wstring& from);

void PsCreate(const std::wstring& executableName, const std::wstring& commandLine);
void PsCreateNoThrow(const std::wstring& executableName, const std::wstring& commandLine);
