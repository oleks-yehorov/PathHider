#include "utils.h"
#include "hidePath.h"

#include "UserModeShared.h"

#include <winternl.h>
extern "C"
NTSTATUS WINAPI NtQueryDirectoryFile(
    _In_     HANDLE                 FileHandle,
    _In_opt_ HANDLE                 Event,
    _In_opt_ PIO_APC_ROUTINE        ApcRoutine,
    _In_opt_ PVOID                  ApcContext,
    _Out_    PIO_STATUS_BLOCK       IoStatusBlock,
    _Out_    PVOID                  FileInformation,
    _In_     ULONG                  Length,
    _In_     FILE_INFORMATION_CLASS FileInformationClass,
    _In_     BOOLEAN                ReturnSingleEntry,
    _In_opt_ PUNICODE_STRING        FileName,
    _In_     BOOLEAN                RestartScan
);

typedef struct _FILE_DIRECTORY_INFORMATION
{
    ULONG         NextEntryOffset;
    ULONG         FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG         FileAttributes;
    ULONG         FileNameLength;
    WCHAR         FileName[256];
} FILE_DIRECTORY_INFORMATION, * PFILE_DIRECTORY_INFORMATION;

struct HandleCloser 
{
    typedef HANDLE pointer;
    void operator()(HANDLE h) 
    {
        if (h != INVALID_HANDLE_VALUE) 
        {
            CloseHandle(h);
        }
    }
};

bool CheckPath(const std::wstring& path)
{
    std::filesystem::path fsPath(path);
    std::filesystem::path fsFolder = fsPath.parent_path();
    
    HANDLE hDirectory = CreateFileA(fsFolder.string().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hDirectory == INVALID_HANDLE_VALUE)
    {
        std::cout << "CreateFile failed," << GetLastError() << "\n";
        return false;
    }
    std::unique_ptr<HANDLE, HandleCloser> handleGuard(hDirectory);
        
    IO_STATUS_BLOCK iosb;
    FILE_DIRECTORY_INFORMATION fdi;
    std::filesystem::path fsFileObjectName = fsPath.filename();
    // Perform the first call, with ReturnSingleEntry set to FALSE.
    //memset(&fdi.FileName[0], 0, sizeof(fdi.FileName));
    auto status = NtQueryDirectoryFile(hDirectory, NULL, NULL, NULL, &iosb, &fdi, sizeof(fdi), FileDirectoryInformation, TRUE, NULL, TRUE);
    int count = 0;
    while (NT_SUCCESS(status))
    {
        ++count;
        std::wstring curName(&fdi.FileName[0], fdi.FileNameLength/sizeof(WCHAR));
        std::wcout << L"Entry : " << curName << " fsFileObjectName " << fsFileObjectName << L"\n";
        if (curName == fsFileObjectName)
            return true;
        //memset(&fdi.FileName[0], 0, sizeof(fdi.FileName));
        status = NtQueryDirectoryFile(hDirectory, NULL, NULL, NULL, &iosb, &fdi, sizeof(fdi), FileDirectoryInformation, TRUE, NULL, FALSE);
    }
    return false;
}

int main()
{
    const std::wstring testPath(L"C:\\Test\\test.txt");
    try
    {
        system("pause");

        //load driver
        PsCreateNoThrow(L"C:\\Windows\\System32\\fltmc.exe", L"fltmc.exe load pathHider");
        //check visible 
        bool expectTrue = CheckPath(testPath);
        std::cout << "Expect true is " << (expectTrue ? "True" : "False") << "\n";
        //hide path
        HidePath(testPath);
        //check hidden
        bool expectFalse = CheckPath(testPath);
        std::cout << "Expect false is " << (expectFalse ? "True" : "False") << "\n";
        //unload driver
        PsCreate(L"C:\\Windows\\System32\\fltmc.exe", L"fltmc.exe unload pathHider");
    }
    catch (const std::exception& ex)
    {
        std::wcout << ex.what();
        return -1;
    }
    return 0;
}
