// NotepadService.cpp
//
// Based on MS sample Svc.cpp:
// https://docs.microsoft.com/en-us/windows/win32/services/the-complete-service-sample
//

#include "stdafx.h"
#include "NotepadService.h"
#include "SessionNotification.h"

#ifdef _DEBUG
#define SERVICE_WAIT_INTERVAL 60*1000
static int fDebug = false;
#else
#define SERVICE_WAIT_INTERVAL INFINITE
#endif

SERVICE_STATUS          gSvcStatus = { 0 };
SERVICE_STATUS_HANDLE   gSvcStatusHandle = NULL;
HANDLE                  ghSvcStopEvent = NULL;

std::wstring strEditor = L"notepad.exe";

//
// Service control and handler routines
//

VOID SvcReportEvent(LPCWSTR pszMessage, bool isError)
{
	HANDLE hEventSrc = RegisterEventSource(NULL, APPLICATION_LOGNAME);
	if (hEventSrc)
	{
		WORD wType = isError ? EVENTLOG_ERROR_TYPE : EVENTLOG_INFORMATION_TYPE;
		DWORD dwEventId = 0;

		LPCWSTR lpszStrings[2];
		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = pszMessage;

		ReportEvent(hEventSrc,	// event log handle
			wType,				// event type
			0,                  // event category
			dwEventId,			// event identifier
			NULL,               // no security identifier
			2,                  // size of lpszStrings array
			0,                  // no binary data
			lpszStrings,        // array of strings
			NULL);              // no binary data

		DeregisterEventSource(hEventSrc);
	}
}

VOID SvcReportStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	gSvcStatus.dwControlsAccepted = 0;

	if (dwCurrentState == SERVICE_START_PENDING)
	{
		gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_SESSIONCHANGE;
	}
	else
	{
		if (dwCurrentState == SERVICE_RUNNING)
			gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_SESSIONCHANGE;
		gSvcStatus.dwControlsAccepted |= SERVICE_ACCEPT_STOP;
	}

	if (dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED)
	{
		gSvcStatus.dwCheckPoint = 0;
	}
	else
	{
		gSvcStatus.dwCheckPoint = dwCheckPoint++;
	}

	// Report the status of the service to the SCM.
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

VOID SvcInit(DWORD dwArgc, LPTSTR *lpszArgv)
{
	//   Be sure to periodically call SvcReportStatus() with 
	//   SERVICE_START_PENDING. If initialization fails, call
	//   SvcReportStatus with SERVICE_STOPPED.

	// Create an event. The control handler function, SvcCtrlHandler,
	// signals this event when it receives the stop control code.

	ghSvcStopEvent = CreateEvent(
		NULL,    // default security attributes
		TRUE,    // manual reset event
		FALSE,   // not signaled
		NULL);   // no name

	if (ghSvcStopEvent == NULL)
	{
		SvcReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	// Report running status when initialization is complete.

	SvcReportStatus(SERVICE_RUNNING, NO_ERROR, 0);

	for (;;)
	{
		// Check whether to stop the service.
		DWORD rc = WaitForSingleObject(ghSvcStopEvent, SERVICE_WAIT_INTERVAL);
		switch (rc)
		{
		case WAIT_TIMEOUT:
#ifdef _DEBUG
			if (fDebug)
			{
				LaunchProcessForUser(WTSGetActiveConsoleSessionId(), strEditor);
			}
#endif
			SvcReportEvent(L"Heartbeat", false);
			break;
		default:
			SvcReportEvent(L"Wait event failed", true);
			//fall through to exit
		case WAIT_OBJECT_0:
			SvcReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
			return;
		}
	}
}

DWORD WINAPI SvcCtrlHandlerEx(
	DWORD dwCtrl,
	DWORD dwEventType,
	LPVOID lpEventData,
	LPVOID lpContext
	)
{
#ifdef _DEBUG
	//log all notification events in debug build
	std::wstring eventType = L"Control=";
	eventType += std::to_wstring(dwCtrl);
	eventType += L" type=";
	eventType += std::to_wstring(dwEventType);
	SvcReportEvent(eventType.c_str(), false);
#endif

	DWORD dwStatus = NO_ERROR;
	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		SvcReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
		// Signal the service to stop.
		SetEvent(ghSvcStopEvent);
		SvcReportStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);
		break;
	case SERVICE_CONTROL_INTERROGATE:
		//always handle and return NO_ERROR
		SvcReportEvent(L"SERVICE_CONTROL_INTERROGATE", false);
		break;
	case SERVICE_CONTROL_SESSIONCHANGE:
		{
			//launch the editor for logon events. Also want to launch for the UNLOCK notification
			//which is the logon from a locked screen after a system restart
			CSessionNotification notify(dwEventType, lpEventData);
			if (notify.IsLogonEvent() ||
				notify.IsUnlockEvent())
			{
				LaunchProcessForUser(notify.GetSessionId(), strEditor);
			}
		}
		break;
	default:
		dwStatus = ERROR_CALL_NOT_IMPLEMENTED;
		break;
	}

	return dwStatus;
}

VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	// Register the handler function for the service

	DWORD context = 1;
	gSvcStatusHandle = RegisterServiceCtrlHandlerEx(
		SVCNAME,
		SvcCtrlHandlerEx,
		&context);

	if (!gSvcStatusHandle)
	{
		SvcReportEvent(TEXT("RegisterServiceCtrlHandler"), true);
		return;
	}

	// These SERVICE_STATUS members remain as set here

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	// Report initial status to the SCM

	SvcReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	// Perform service-specific initialization and work.

	SvcInit(dwArgc, lpszArgv);
}

//
// main - the service entry point
//
int _tmain(int argc, _TCHAR* argv[])
{
	if (argc > 1 && argv[1] != '\0')
	{
		//if a parameter is given, use this as the launch editor
		strEditor = argv[1];
	}

	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{ SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain },
		{ NULL, NULL }
	};

	// This call returns when the service has stopped. 
	// The process should simply terminate when the call returns.

	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		SvcReportEvent(L"StartServiceCtrlDispatcher", true);
	}
	return 0;
}

