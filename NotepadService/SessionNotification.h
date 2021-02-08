#pragma once

#include <wtsapi32.h>

class CSessionNotification
{
private:
	DWORD m_dwEventType;
	PWTSSESSION_NOTIFICATION m_pData;
public:
	CSessionNotification(DWORD dwEventType, LPVOID lpEventData);
	DWORD GetSessionId()
	{
		return m_pData ? m_pData->dwSessionId : -1;
	}
	bool IsLogonEvent();
	bool IsUnlockEvent();
	virtual ~CSessionNotification() {}
};