#include "stdafx.h"
#include "SessionNotification.h"

#pragma comment(lib, "wtsapi32")

//
// CSessionNotification class
//

CSessionNotification::CSessionNotification(DWORD dwEventType, LPVOID lpEventData) :
	m_dwEventType(dwEventType)
{
	m_pData = reinterpret_cast<WTSSESSION_NOTIFICATION*>(lpEventData);
}

bool CSessionNotification::IsLogonEvent()
{
	return m_dwEventType == WTS_SESSION_LOGON;
}

bool CSessionNotification::IsUnlockEvent()
{
	return m_dwEventType == WTS_SESSION_UNLOCK;
}


