#pragma once
#include <Windows.h>

BOOL AddAceToWindowStation(HWINSTA hwinsta, PSID psid);
BOOL AddAceToDesktop(HDESK hdesk, PSID psid);
BOOL GetLogonSID(HANDLE hToken, PSID *ppsid);
VOID FreeLogonSID(PSID *ppsid);