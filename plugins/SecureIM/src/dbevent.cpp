#include "commonheaders.h"


void HistoryLog(HANDLE hContact, LPCSTR szText)
{
	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.szModule = GetContactProto(hContact);
	dbei.flags = DBEF_SENT|DBEF_READ;
	dbei.timestamp = time(NULL);
	dbei.eventType = EVENTTYPE_MESSAGE;
	dbei.cbBlob = (int)strlen(szText) + 1;
	dbei.pBlob = (PBYTE)szText;
	db_event_add(0, &dbei);
}


// EOF
