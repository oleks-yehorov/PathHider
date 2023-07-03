#include <Windows.h>
#include <fltUser.h>

#include <exception>
#include <iostream>
#include <filesystem>

#include "UserModeShared.h"

#pragma comment(lib, "fltlib")

void SendMessage(const PHMessage& message)
{
    //open communication port
    HANDLE port = INVALID_HANDLE_VALUE;
    HRESULT result = FilterConnectCommunicationPort(COMMUNICATION_PORT, NULL, NULL, NULL, NULL, &port);
    if (result == S_OK)
    {
        //send command
        DWORD bytesReturned = 0;
        result = FilterSendMessage(port, (LPVOID)&message, sizeof(message), nullptr, 0, &bytesReturned);
        if (result != S_OK)
        {
            std::cout << "FilterSendMessage failed with status: " << result << "/n";
        }
        //close communication port
        CloseHandle(port);
    }
    else
    {
        std::cout << "FilterConnectCommunicationPort failed with status: " << result << "/n";
    }
}

void UnhideAll()
{
    PHMessage message;
    message.m_action = PHAction::RemoveAllhiddenPaths;
    message.m_data = nullptr;
    SendMessage(message);
}

void HidePath(const std::wstring& path)
{
    //convert DOS path to NT path
    WCHAR  DeviceName[MAX_PATH] = L"";
    WCHAR DosDeviceName[3];
    DosDeviceName[0] = path[0];
    DosDeviceName[1] = path[1];
    DosDeviceName[2] = 0;
    auto length = QueryDosDeviceW(DosDeviceName, DeviceName, ARRAYSIZE(DeviceName));
    if (length == 0)//fail
    {
        auto error = GetLastError();
        std::cout << "QueryDosDeviceW failed with error: " << error << "/n";
        return;
    }
    //build command
    //make path + name
    std::filesystem::path fsPath(path);
    std::wstring fileObjectName = fsPath.filename().wstring();
    std::wstring devicePath = std::wstring(DeviceName).append(L"\\").append(fsPath.parent_path().relative_path().wstring());
    PHMessage message;
    message.m_action = PHAction::AddPathToHideAction;
    PHData data;
    data.m_path = (PWCHAR)devicePath.c_str();
    data.m_name = (PWCHAR)fileObjectName.c_str();
    message.m_data = &data;
    //send command
    SendMessage(message);
}
