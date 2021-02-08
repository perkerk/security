#pragma once

#define SVCNAME				L"Notepad Helper"
#define APPLICATION_LOGNAME L"Application"

VOID SvcReportEvent(LPCWSTR pszMessage, bool isError);
bool LaunchProcessForUser(DWORD dwSessionId, std::wstring& editor);

