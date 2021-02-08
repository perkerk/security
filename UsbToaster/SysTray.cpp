
#include "stdafx.h"
#include "SysTray.h"
#include "WinToast\wintoastlib.h"
using namespace WinToastLib;

class WinToastHandler : public IWinToastHandler {
public:
	WinToastHandler() {}
	virtual void toastActivated() const {}
	virtual void toastActivated(int actionIndex) const {}
	virtual void toastDismissed(WinToastDismissalReason state) const {}
	virtual void toastFailed() const {}
};

SysTray::SysTray(HWND hwnd, UINT uCallback, HICON icon)
{
	ZeroMemory(&m_icon, sizeof(m_icon));
	m_icon.cbSize = sizeof(NOTIFYICONDATA_V2_SIZE);
    m_icon.hWnd = hwnd;
    m_icon.uID = 0;
    m_icon.hIcon = icon;
	m_icon.uCallbackMessage = uCallback;
    m_icon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_icon.szTip[0] = 0;

    RestoreIcon();

	bool rc = WinToast::isCompatible();
	WinToast::instance()->setAppName(L"USB Toaster");
	const auto aumi = WinToast::configureAUMI(L"USB Toaster", L"USB Toaster", L"USB Toaster", L"0.1");
	WinToast::instance()->setAppUserModelId(aumi);
	rc = WinToast::instance()->initialize();

	m_toastHandler = new WinToastHandler();
}

SysTray::~SysTray()
{
	Shell_NotifyIcon(NIM_DELETE, &m_icon);
}

void SysTray::RestoreIcon()
{
	Shell_NotifyIcon(NIM_ADD, &m_icon);
}

void SysTray::SetTooltip(const std::wstring &tooltip)
{
    wcscpy_s(m_icon.szTip, 64, tooltip.c_str());
    Shell_NotifyIcon(NIM_MODIFY, &m_icon);
}

void SysTray::ShowToast(LPCWSTR title, LPCWSTR message)
{
	WinToastTemplate templ = WinToastTemplate(WinToastTemplate::Text02);
	templ.setTextField(title, WinToastTemplate::FirstLine);
	templ.setTextField(message, WinToastTemplate::SecondLine);
	WinToast::instance()->showToast(templ, m_toastHandler);
}

void SysTray::ShowPopupMenu()
{
	HWND hWnd = m_icon.hWnd;

	HMENU hPop = CreatePopupMenu();
	InsertMenu(hPop, 0, MF_BYPOSITION | MF_STRING, ID_SYSTRAY_EXIT, L"Exit");

	SetMenuDefaultItem(hPop, ID_SYSTRAY_EXIT, FALSE);
	SetFocus(hWnd);
	SendMessage(hWnd, WM_INITMENUPOPUP, (WPARAM)hPop, 0);

	POINT pt;
	GetCursorPos(&pt);
	WORD cmd = TrackPopupMenu(hPop, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
	SendMessage(hWnd, WM_COMMAND, cmd, 0);

	DestroyMenu(hPop);
}

void SysTray::HandleMessage(WPARAM wParam, LPARAM lParam)
{
	switch (lParam)
	{
	case WM_RBUTTONUP:
		ShowPopupMenu();
		break;
	default:
		break;
	}
}
