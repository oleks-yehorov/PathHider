#include "utils.h"

std::string FormatErrorA(DWORD dwError, DWORD dwLanguageId)
{
    HLOCAL hlocal = NULL;   // Buffer that gets the error message string

    // Get the error code's textual description
    BOOL fOk = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER
        , NULL
        , dwError
        , dwLanguageId
        , reinterpret_cast<LPSTR>(&hlocal)
        , 0
        , NULL);

    if (!fOk)
    {
        // Is it a network-related error?
        HMODULE hDll = LoadLibraryExA("netmsg.dll"
            , NULL
            , DONT_RESOLVE_DLL_REFERENCES);

        if (hDll != NULL)
        {
            FormatMessageA(
                FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM
                , hDll
                , dwError
                , dwLanguageId
                , reinterpret_cast<LPSTR>(&hlocal)
                , 0
                , NULL);
            FreeLibrary(hDll);
        }
    }

    if (hlocal != NULL)
    {
        std::string sErr = reinterpret_cast<const char*>(LocalLock(hlocal));
        LocalUnlock(hlocal);
        LocalFree(hlocal);
        return sErr;
    }
    else
    {
        return "Can not retreive description for error: " + std::to_string(dwError);
    }
}

void wchar2str(const wchar_t* str, size_t strSize, std::vector<char>* res, UINT cpId)
{
    if (!strSize)
        return;
    res->resize(strSize + 1);//it is enough in 99%
    while (1)
    {
        int size = WideCharToMultiByte(cpId, 0, str, (int)strSize, &(*res)[0], (int)res->size(), 0, 0);
        if (size)
        {
            res->resize(size + 1);
            return;
        }
        DWORD error = ::GetLastError();
        if (error != ERROR_INSUFFICIENT_BUFFER)
            throw std::runtime_error(FormatError(error));
        size = WideCharToMultiByte(cpId, 0, str, (int)strSize, 0, 0, 0, 0);
        if (!size)
            throw std::runtime_error(FormatError(::GetLastError()));
        res->resize(size + 1);
    }
}

std::string wstring2string(const std::wstring& from)
{
    std::vector<char> res;
    wchar2str_unthrowable(from.c_str(), from.size(), &res);
    return res.empty() ? "" : &res[0];
}

void PsCreate(const std::wstring& executablePath, const std::wstring& commandLine)
{
    PROCESS_INFORMATION procinfo;
    STARTUPINFO stinfo = { 0 };
    ::GetStartupInfo(&stinfo);

    stinfo.hStdError = GetStdHandle(STD_OUTPUT_HANDLE);
    stinfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);


    if (!::CreateProcessW(executablePath.empty() ? nullptr : (LPWSTR)executablePath.c_str(), (LPWSTR)commandLine.c_str(), NULL, NULL, TRUE, NULL, NULL, NULL, &stinfo, &procinfo))
    {
        auto error = ::GetLastError();
        throw std::runtime_error(wstring2string(L"Cannot create process  '" + executablePath + L"' by command '" + commandLine + L"' Error code : ") + FormatError(error));
    }

    if (WAIT_OBJECT_0 != WaitForSingleObject(procinfo.hProcess, INFINITE))
    {
        throw std::runtime_error(wstring2string(L"Wait of sign process failed"));
    }

    DWORD exitCode = 0;

    if (!GetExitCodeProcess(procinfo.hProcess, &exitCode))
    {
        throw std::runtime_error(wstring2string(L"Cannot get result " + executablePath));
    }

    if (0 != exitCode)
    {
        throw std::runtime_error(wstring2string(L"Failed '" + executablePath + L"'!!"));
    }
}

void PsCreateNoThrow(const std::wstring& executablePath, const std::wstring& commandLine)
{
    try
    {
        PsCreate(executablePath, commandLine);
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what();
    }

}