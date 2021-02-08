#pragma once
#include <Windows.h>

void GetLogicalVolumeName(DWORD unitmask, std::wstring& description);
HDEVNOTIFY RegisterForUsbNotifications(HWND hWnd);
bool GetDeviceChangeMessage(WPARAM wParam, LPARAM lParam, std::wstring& action, std::wstring& deviceName);