#include "stdafx.h"
#include <UserEnv.h>
#include <wtsapi32.h>

#pragma comment(lib, "userenv")
#pragma comment(lib, "wtsapi32")

#define INTERACTIVE_WINDOW_STATION L"winsta0"
#define INTERACTIVE_WINDOW_DESKTOP L"winsta0\\default"

//
// class for managing aquiring and restore SE_DEBUG_NAME privilege for the current process
//
class CGetDebugPrivilege
{
private:
	HANDLE m_hProcessToken;
	TOKEN_PRIVILEGES m_restorePrivileges;
	DWORD m_cbRestore;
public:
	CGetDebugPrivilege() :
		m_hProcessToken(NULL),
		m_cbRestore(0)
	{
	}
	operator HANDLE() { return m_hProcessToken; }
	BOOL GetDebugPrivilege()
	{
		if (!OpenProcessToken(
			GetCurrentProcess(),
			TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_SESSIONID | TOKEN_READ | TOKEN_WRITE,
			&m_hProcessToken))
		{
			return FALSE;
		}

		TOKEN_PRIVILEGES tp;
		LUID luid;

		//get SE_DEBUG_NAME to access other processes
		if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
		{
			return false;
		}

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (!AdjustTokenPrivileges(m_hProcessToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &m_restorePrivileges, &m_cbRestore))
		{
			return FALSE;
		}

		return TRUE;
	}
	BOOL RestorePrivileges()
	{
		BOOL fSuccess = FALSE;
		if (m_hProcessToken)
		{
			if (m_cbRestore)
			{
				fSuccess = AdjustTokenPrivileges(m_hProcessToken, FALSE, &m_restorePrivileges, m_cbRestore, NULL, NULL);
				m_cbRestore = 0;
			}
		}
		return fSuccess;
	}
	~CGetDebugPrivilege()
	{
		RestorePrivileges();
		if (m_hProcessToken)
			CloseHandle(m_hProcessToken);
	}
};

// Creates a token to launch process in the context of the user logged into the given session
// https://docs.microsoft.com/en-us/windows/win32/secauthz/enabling-and-disabling-privileges-in-c--
HANDLE GetLaunchTokenForUser(DWORD dwSessionId)
{
	HANDLE hUserToken = NULL;
	HANDLE hImpersonationToken = NULL;

	if (!WTSQueryUserToken(dwSessionId, &hUserToken))
		goto done;

	if (!DuplicateTokenEx(hUserToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hImpersonationToken))
		goto done;

	if (!SetTokenInformation(hImpersonationToken, TokenSessionId, (void*)&dwSessionId, sizeof(DWORD)))
	{
		hImpersonationToken = NULL;
		goto done;
	}

done:
	if (hUserToken)
		CloseHandle(hUserToken);

	return hImpersonationToken;
}

/*
 Launches the given editor in the context of the just logged-on user.

 Note: This will not give the user process access to the HKEY_USERS registry

 See https ://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessasusera

 Alternatively, CreateProcessWithTokenW(token, LOGON_WITH_PROFILE, 0, lpCmd, CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT, lpEnvironment, 0, &StartupInfo, &processInfo)
 could be used for Vista+
*/
bool LaunchProcessForUser(DWORD dwSessionId, std::wstring& editor)
{
	CGetDebugPrivilege instance;
	if (!instance.GetDebugPrivilege())
		return false;

	bool fSuccess = false;

	HANDLE hToken = GetLaunchTokenForUser(dwSessionId);
	if (hToken)
	{
		DWORD dwCreationFlags = NORMAL_PRINT | CREATE_NEW_CONSOLE; //CREATE_NEW_CONSOLE is required or the process can hang

		LPVOID lpEnv = NULL;
		if (CreateEnvironmentBlock(&lpEnv, hToken, TRUE))
		{
			dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
		}

		PROCESS_INFORMATION pi = { 0 };
		STARTUPINFO si = { 0 };
		si.cb = sizeof(si);
		si.lpDesktop = INTERACTIVE_WINDOW_DESKTOP;

		//CreateProcessAsUser requires writable argument
		TCHAR buffer[MAX_PATH];
		StringCchCopy(buffer, ARRAYSIZE(buffer), editor.c_str());

		// Per docco: Before calling CreateProcessAsUser, you must change the discretionary access control list(DACL) of both the default interactive window station and the 
		// default desktop. The DACLs for the window station and desktop must grant access to the user or the logon session represented by the hToken parameter.
		//TODO 

		if (!CreateProcessAsUser(hToken, NULL, buffer, NULL, NULL, FALSE, dwCreationFlags, lpEnv, NULL, &si, &pi))
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			fSuccess = true;
		}

		if (lpEnv)
			DestroyEnvironmentBlock(lpEnv);
		CloseHandle(hToken);
	}
	return fSuccess;
}
