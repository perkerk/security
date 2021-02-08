#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>

#define ID_SYSTRAY_EXIT 2001
#define WM_SYSTRAY (WM_USER + 1)

class WinToastHandler;

class SysTray
{
private:
	NOTIFYICONDATA m_icon;
	WinToastHandler* m_toastHandler;

	void ShowPopupMenu();

public:
	SysTray(HWND hwnd, UINT uCallback, HICON icon);
	virtual ~SysTray();
	void RestoreIcon();
	void SetTooltip(const std::wstring &tooltip);
	void ShowToast(LPCWSTR title, LPCWSTR message);
	void HandleMessage(WPARAM wParam, LPARAM lParam);
};
