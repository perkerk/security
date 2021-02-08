// UsbNotifications.cpp
// See https://docs.microsoft.com/en-us/windows/win32/devio/registering-for-device-notification
//
#include "stdafx.h"
#include <sstream>
#include <strsafe.h>
#include <Dbt.h>
#include <SetupAPI.h>
#include <devguid.h>

//Returns a string describing the logical volumes represented in the given unitmask field from PDEV_BROADCAST_VOLUME
void GetLogicalVolumeName(DWORD unitmask, std::wstring& description)
{
	std::wstringstream ss;
	int volumes = 0;
	for (int i = 0; i < 26; ++i)
	{
		if (unitmask & 1)
		{
			WCHAR drive = L'A' + i;
			if (volumes > 0)
			{
				ss << L',' << drive;
			}
			else
			{
				ss << drive;
			}
			++volumes;
		}
		unitmask >>= 1;
		if (!unitmask)
			break;
	}
	switch (volumes)
	{
	case 0:
		description = L"Volumes unknown";
		break;
	case 1:
		description = L"Volume " + ss.str();
		break;
	default:
		description = L"Volumes: " + ss.str();
		break;
	}
}

HDEVNOTIFY DoRegisterDeviceInterfaceToHwnd(
	IN GUID InterfaceClassGuid,
	IN HWND hWnd
	)
{
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = InterfaceClassGuid;

	return RegisterDeviceNotification(
		hWnd,                       // events recipient
		&NotificationFilter,        // type of device
		DEVICE_NOTIFY_WINDOW_HANDLE // type of recipient handle
		);
}

// Convenience method that calls DoRegisterDeviceInterfaceToHwnd for the GUID_DEVCLASS_WCEUSBS
// device notifications we're interested in
HDEVNOTIFY RegisterForUsbNotifications(HWND hWnd)
{
	return DoRegisterDeviceInterfaceToHwnd(GUID_DEVCLASS_WCEUSBS, hWnd);
}

// Handler for WM_DEVICECHANGE. This will set the action and deviceName strings for
// USB change events the user should be notified about
bool GetDeviceChangeMessage(WPARAM wParam, LPARAM lParam, std::wstring& action, std::wstring& deviceName)
{
	//
	// This is the actual message from the interface via Windows messaging.
	// This code includes some additional decoding for this particular device type
	// and some common validation checks.
	//
	// Note that not all devices utilize these optional parameters in the same
	// way. Refer to the extended information for your particular device type 
	// specified by your GUID.
	//
	switch (wParam)
	{
	case DBT_DEVICEARRIVAL:
		OutputDebugString(L"DBT_DEVICEARRIVAL\n");
		action = L"Device Inserted";
		break;
	case DBT_DEVICEREMOVECOMPLETE:
		OutputDebugString(L"DBT_DEVICEREMOVECOMPLETE\n");
		action = L"Device Removed";
		break;
	case DBT_DEVNODES_CHANGED:
		OutputDebugString(L"DBT_DEVNODES_CHANGED\n");
		break;
	default:
		OutputDebugString(L"DBT_ UNKNOWN EVENT\n");
		break;
	}

	if (!action.empty())
	{
		PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
		if (pHdr)
		{
			switch (pHdr->dbch_devicetype)
			{
			case DBT_DEVTYP_VOLUME:
			{
				PDEV_BROADCAST_VOLUME pVol = (PDEV_BROADCAST_VOLUME)lParam;
				GetLogicalVolumeName(pVol->dbcv_unitmask, deviceName);
				break;
			}
			case DBT_DEVTYP_DEVICEINTERFACE:
			{
				PDEV_BROADCAST_DEVICEINTERFACE pbdi = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
				deviceName = pbdi->dbcc_name;
				break;
			}
			case DBT_USERDEFINED:
			{
				_DEV_BROADCAST_USERDEFINED* pUserdef = (_DEV_BROADCAST_USERDEFINED*)lParam;
				//we know this is straight ASCII so just convert to WCHAR
				for (int i = 0; i < lstrlenA(pUserdef->dbud_szName); ++i)
				{
					deviceName += (WCHAR)pUserdef->dbud_szName[i];
				}
				break;
			}
			case DBT_DEVTYP_PORT:
			{
				PDEV_BROADCAST_PORT pPort = (PDEV_BROADCAST_PORT)lParam;
				deviceName = pPort->dbcp_name;
				break;
			}
			case DBT_DEVTYP_OEM:
				deviceName = L"Generic OEM Device";
			default:
				break;
			}
		}

		if (deviceName.empty())
			deviceName = L"Generic Device";
		return true;
	}
	
	return false;
}
