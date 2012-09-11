#include "Mra.h"
#include "MraChat.h"
#include "proto.h"



//static COLORREF crCols[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static LPWSTR lpwszStatuses[] = { L"Owners", L"Inviter", L"Visitors" };
#define MRA_CHAT_STATUS_OWNER	0
#define MRA_CHAT_STATUS_INVITER	1
#define MRA_CHAT_STATUS_VISITOR	2




void	MraChatSendPrivateMessage	(LPWSTR lpwszEMail);


void CMraProto::MraChatDllError()
{
	MessageBoxW(NULL, TranslateW(L"CHAT plugin is required for conferences. Install it before chatting"), m_tszUserName, (MB_OK|MB_ICONWARNING));
}

BOOL CMraProto::MraChatRegister()
{
	if ( !ServiceExists(MS_GC_REGISTER))
		return FALSE;

	GCREGISTER gcr = {0};
	gcr.cbSize = sizeof(GCREGISTER);
	gcr.dwFlags = GC_UNICODE;
	gcr.iMaxText = MRA_MAXLENOFMESSAGE;
	gcr.nColors = 0;
	gcr.ptszModuleDispName = m_tszUserName;
	gcr.pszModule = m_szModuleName;
	CallServiceSync(MS_GC_REGISTER, NULL, (LPARAM)&gcr);

	HookEvent(ME_GC_EVENT, &CMraProto::MraChatGcEventHook);
	return TRUE;
}


INT_PTR CMraProto::MraChatSessionNew(HANDLE hContact)
{
	INT_PTR iRet = 1;

	if (bChatExists)
	if (hContact)
	{
		GCSESSION gcw = {0};
		WCHAR wszEMail[MAX_EMAIL_LEN] = {0};

		gcw.cbSize = sizeof(GCSESSION);
		gcw.iType = GCW_CHATROOM;
		gcw.pszModule = m_szModuleName;
		gcw.ptszName = GetContactNameW(hContact);
		gcw.ptszID = (LPWSTR)wszEMail;
		gcw.ptszStatusbarText = L"status bar";
		gcw.dwFlags = GC_UNICODE;
		gcw.dwItemData = (DWORD)hContact;
		mraGetStaticStringW(hContact, "e-mail", wszEMail, SIZEOF(wszEMail), NULL);
		iRet = CallServiceSync(MS_GC_NEWSESSION, NULL, (LPARAM)&gcw);

		if (iRet == 0)
		{
			CHAR szEMail[MAX_EMAIL_LEN] = {0};
			size_t dwEMailSize;
			GCDEST gcd = {0};
			GCEVENT gce = {0};

			gcd.pszModule = m_szModuleName;
			gcd.ptszID = (LPWSTR)wszEMail;
			gcd.iType = GC_EVENT_ADDGROUP;

			gce.cbSize = sizeof(GCEVENT);
			gce.pDest = &gcd;
			gce.dwFlags = GC_UNICODE;
			for (size_t i = 0;i<SIZEOF(lpwszStatuses);i++)
			{
				gce.ptszStatus = TranslateW(lpwszStatuses[i]);
				CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);
			}

			gce.cbSize = sizeof(GCEVENT);
			gce.pDest = &gcd;
			gcd.iType = GC_EVENT_CONTROL;

			CallServiceSync(MS_GC_EVENT, SESSION_INITDONE, (LPARAM)&gce);
			CallServiceSync(MS_GC_EVENT, SESSION_ONLINE, (LPARAM)&gce);

			//MraChatSessionJoinUser(hContact, NULL, 0, 0);
			(*(DWORD*)wszEMail) = MULTICHAT_GET_MEMBERS;
			mraGetStaticStringA(hContact, "e-mail", szEMail, SIZEOF(szEMail), &dwEMailSize);
			MraMessageW(FALSE, NULL, 0, MESSAGE_FLAG_MULTICHAT, szEMail, dwEMailSize, NULL, 0, (LPBYTE)wszEMail, 4);
		}
	}
	return(iRet);
}


void CMraProto::MraChatSessionDestroy(HANDLE hContact)
{
	if (bChatExists)
	{
		GCDEST gcd = {0};
		GCEVENT gce = {0};
		WCHAR wszEMail[MAX_EMAIL_LEN] = {0};

		gcd.pszModule = m_szModuleName;
		gcd.iType = GC_EVENT_CONTROL;
		if (hContact)
		{
			gcd.ptszID = (LPWSTR)wszEMail;
			mraGetStaticStringW(hContact, "e-mail", wszEMail, SIZEOF(wszEMail), NULL);
		}
		gce.cbSize = sizeof(GCEVENT);
		gce.pDest = &gcd;
		gce.dwFlags = GC_UNICODE;

		CallServiceSync(MS_GC_EVENT, SESSION_TERMINATE, (LPARAM)&gce);
		CallServiceSync(MS_GC_EVENT, WINDOW_CLEARLOG, (LPARAM)&gce);
	}
}

INT_PTR CMraProto::MraChatSessionEventSendByHandle(HANDLE hContactChatSession, DWORD dwType, DWORD dwFlags, LPSTR lpszUID, size_t dwUIDSize, LPWSTR lpwszStatus, LPWSTR lpwszMessage, DWORD_PTR dwItemData, DWORD dwTime)
{
	INT_PTR iRet = 1;

	if (bChatExists)
	{
		GCDEST gcd = {0};
		GCEVENT gce = {0};
		WCHAR wszID[MAX_EMAIL_LEN] = {0}, wszUID[MAX_EMAIL_LEN] = {0}, wszNick[MAX_EMAIL_LEN] = {0};

		gcd.pszModule = m_szModuleName;
		if (hContactChatSession)
		{
			gcd.ptszID = (LPWSTR)wszID;
			mraGetStaticStringW(hContactChatSession, "e-mail", wszID, SIZEOF(wszID), NULL);
		}
		gcd.iType = dwType;

		gce.cbSize = sizeof(GCEVENT);
		gce.pDest = &gcd;
		gce.dwFlags = GC_UNICODE|dwFlags;
		gce.ptszUID = wszUID;
		gce.ptszStatus = lpwszStatus;
		gce.ptszText = lpwszMessage;
		gce.dwItemData = dwItemData;
		gce.time = dwTime;

		if (lpszUID && dwUIDSize) 
		{
			gce.bIsMe = IsEMailMy(lpszUID, dwUIDSize);
		}else {
			gce.bIsMe = TRUE;
		}

		if (gce.bIsMe)
		{
			mraGetStaticStringW(NULL, "e-mail", wszUID, SIZEOF(wszUID), NULL);
			mraGetStaticStringW(NULL, "Nick", wszNick, SIZEOF(wszNick), NULL);
			gce.ptszNick = wszNick;
		}else {
			HANDLE hContactSender;

			hContactSender = MraHContactFromEmail(lpszUID, dwUIDSize, FALSE, TRUE, NULL);
			MultiByteToWideChar(MRA_CODE_PAGE, 0, lpszUID, dwUIDSize, wszUID, SIZEOF(wszUID));
			if (hContactSender)
			{
				gce.ptszNick = GetContactNameW(hContactSender);
			}else {
				gce.ptszNick = wszUID;
			}
		}

		iRet = CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);
	}
return(iRet);
}

INT_PTR CMraProto::MraChatSessionInvite(HANDLE hContactChatSession, LPSTR lpszEMailInMultiChat, size_t dwEMailInMultiChatSize, DWORD dwTime)
{
	INT_PTR iRet = 1;

	if (hContactChatSession)
	{
		WCHAR wszEMailInMultiChat[MAX_EMAIL_LEN] = {0}, wszBuff[((MAX_EMAIL_LEN*2)+MAX_PATH)];

		MultiByteToWideChar(MRA_CODE_PAGE, 0, lpszEMailInMultiChat, dwEMailInMultiChatSize, wszEMailInMultiChat, SIZEOF(wszEMailInMultiChat));
		mir_sntprintf(wszBuff, SIZEOF(wszBuff), L"[%s]: %s", wszEMailInMultiChat, TranslateW(L"invite sender"));

		iRet = MraChatSessionEventSendByHandle(hContactChatSession, GC_EVENT_ACTION, GCEF_ADDTOLOG, lpszEMailInMultiChat, dwEMailInMultiChatSize, NULL, wszBuff, 0, dwTime);
	}
return(iRet);
}


INT_PTR CMraProto::MraChatSessionMembersAdd(HANDLE hContactChatSession, LPSTR lpszEMailInMultiChat, size_t dwEMailInMultiChatSize, DWORD dwTime)
{
	INT_PTR iRet = 1;

	if (hContactChatSession)
	{
		WCHAR wszEMailInMultiChat[MAX_EMAIL_LEN] = {0}, wszBuff[((MAX_EMAIL_LEN*2)+MAX_PATH)];

		MultiByteToWideChar(MRA_CODE_PAGE, 0, lpszEMailInMultiChat, dwEMailInMultiChatSize, wszEMailInMultiChat, SIZEOF(wszEMailInMultiChat));
		mir_sntprintf(wszBuff, SIZEOF(wszBuff), L"[%s]: %s", wszEMailInMultiChat, TranslateW(L"invite new members"));

		iRet = MraChatSessionEventSendByHandle(hContactChatSession, GC_EVENT_ACTION, GCEF_ADDTOLOG, lpszEMailInMultiChat, dwEMailInMultiChatSize, NULL, wszBuff, 0, dwTime);
	}
return(iRet);
}


INT_PTR CMraProto::MraChatSessionJoinUser(HANDLE hContactChatSession, LPSTR lpszEMailInMultiChat, size_t dwEMailInMultiChatSize, DWORD dwTime)
{
	INT_PTR iRet = 1;

	if (hContactChatSession)
	{
		iRet = MraChatSessionEventSendByHandle(hContactChatSession, GC_EVENT_JOIN, GCEF_ADDTOLOG, lpszEMailInMultiChat, dwEMailInMultiChatSize, lpwszStatuses[MRA_CHAT_STATUS_VISITOR], L"", 0, dwTime);
	}
return(iRet);
}


INT_PTR CMraProto::MraChatSessionLeftUser(HANDLE hContactChatSession, LPSTR lpszEMailInMultiChat, size_t dwEMailInMultiChatSize, DWORD dwTime)
{
	INT_PTR iRet = 1;

	if (hContactChatSession)
	{
		iRet = MraChatSessionEventSendByHandle(hContactChatSession, GC_EVENT_PART, GCEF_ADDTOLOG, lpszEMailInMultiChat, dwEMailInMultiChatSize, NULL, NULL, 0, dwTime);
	}
return(iRet);
}


INT_PTR CMraProto::MraChatSessionSetIviter(HANDLE hContactChatSession, LPSTR lpszEMailInMultiChat, size_t dwEMailInMultiChatSize)
{
	INT_PTR iRet = 1;

	if (hContactChatSession && lpszEMailInMultiChat && dwEMailInMultiChatSize)
	{
		//iRet = MraChatSessionEventSendByHandle(hContactChatSession, GC_EVENT_REMOVESTATUS, 0, lpszEMailInMultiChat, dwEMailInMultiChatSize, lpwszStatuses[MRA_CHAT_STATUS_VISITOR], NULL, 0, 0);
		iRet = MraChatSessionEventSendByHandle(hContactChatSession, GC_EVENT_ADDSTATUS, 0, lpszEMailInMultiChat, dwEMailInMultiChatSize, lpwszStatuses[MRA_CHAT_STATUS_INVITER], NULL, 0, 0);
	}
return(iRet);
}


INT_PTR CMraProto::MraChatSessionSetOwner(HANDLE hContactChatSession, LPSTR lpszEMailInMultiChat, size_t dwEMailInMultiChatSize)
{
	INT_PTR iRet = 1;

	if (hContactChatSession && lpszEMailInMultiChat && dwEMailInMultiChatSize)
	{
		iRet = MraChatSessionEventSendByHandle(hContactChatSession, GC_EVENT_ADDSTATUS, 0, lpszEMailInMultiChat, dwEMailInMultiChatSize, lpwszStatuses[MRA_CHAT_STATUS_OWNER], NULL, 0, 0);
	}
return(iRet);
}


INT_PTR CMraProto::MraChatSessionMessageAdd(HANDLE hContactChatSession, LPSTR lpszEMailInMultiChat, size_t dwEMailInMultiChatSize, LPWSTR lpwszMessage, size_t dwMessageSize, DWORD dwTime)
{
	INT_PTR iRet = 1;

	if (hContactChatSession)
	{
		LPWSTR lpwszMessageLocal;

		lpwszMessageLocal = (LPWSTR)mir_calloc((dwMessageSize*sizeof(WCHAR)));
		if (lpwszMessageLocal)
		{// we need zeros after text allways
			memmove((LPVOID)lpwszMessageLocal, lpwszMessage, (dwMessageSize*sizeof(WCHAR)));//gce.ptszText = lpwszMessage;
			
			iRet = MraChatSessionEventSendByHandle(hContactChatSession, GC_EVENT_MESSAGE, GCEF_ADDTOLOG, lpszEMailInMultiChat, dwEMailInMultiChatSize, NULL, lpwszMessageLocal, 0, dwTime);

			mir_free(lpwszMessageLocal);
		}
	}
return(iRet);
}

int CMraProto::MraChatGcEventHook(WPARAM, LPARAM lParam)
{
	int iRet = 0;

	if (lParam)
	if (bChatExists)
	{
		GCHOOK* gch = (GCHOOK*)lParam;

		if ( !_stricmp(gch->pDest->pszModule, m_szModuleName)) {
			switch (gch->pDest->iType) {
			case GC_USER_MESSAGE:
				if (gch->ptszText && lstrlen(gch->ptszText))
				{
					BOOL bSlowSend;
					CHAR szEMail[MAX_EMAIL_LEN] = {0};
					DWORD dwFlags = 0;
					size_t dwEMailSize, dwMessageSize;
					INT_PTR iSendRet;
					HANDLE hContact;

					//rtrim(gch->ptszText);
					//TCHAR* buf = NEWTSTR_ALLOCA(gch->ptszText);
					//UnEscapeChatTags( buf );

					dwMessageSize = lstrlenW(gch->ptszText);
					dwEMailSize = (WideCharToMultiByte(MRA_CODE_PAGE, 0, gch->pDest->ptszID, -1, szEMail, SIZEOF(szEMail), NULL, NULL)-1);
					hContact = MraHContactFromEmail(szEMail, dwEMailSize, FALSE, TRUE, NULL);
					bSlowSend = mraGetByte(NULL, "SlowSend", MRA_DEFAULT_SLOW_SEND);

					if (mraGetByte(NULL, "RTFSendEnable", MRA_DEFAULT_RTF_SEND_ENABLE) && (MraContactCapabilitiesGet(hContact)&FEATURE_FLAG_RTF_MESSAGE))
					{
						dwFlags |= MESSAGE_FLAG_RTF;
					}

					iSendRet = MraMessageW(bSlowSend, hContact, ACKTYPE_MESSAGE, dwFlags, szEMail, dwEMailSize, gch->ptszText, dwMessageSize, NULL, 0);
					if (bSlowSend == FALSE)
					{
						ProtoBroadcastAckAsynchEx(hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE)iSendRet, (LPARAM)NULL, 0);
					}

					MraChatSessionEventSendByHandle(hContact, GC_EVENT_MESSAGE, GCEF_ADDTOLOG, NULL, 0, NULL, gch->ptszText, 0, (DWORD)_time32(NULL));
				}
				break;
			case GC_USER_PRIVMESS:
				MraChatSendPrivateMessage(gch->ptszUID);
				break;
			case GC_USER_LOGMENU:
				//sttLogListHook( this, item, gch );
				break;
			case GC_USER_NICKLISTMENU:
				//sttNickListHook( this, item, gch );
				break;
			case GC_USER_CHANMGR:
				//int iqId = SerialNext();
				//IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultGetMuc );
				//m_ThreadInfo->send( XmlNodeIq( _T("get"), iqId, item->jid ) << XQUERY( xmlnsOwner ));
				break;
			}
		}
	}
	return(iRet);
}

void CMraProto::MraChatSendPrivateMessage(LPWSTR lpwszEMail)
{
	BOOL bAdded;
	HANDLE hContact;
	CHAR szEMail[MAX_EMAIL_LEN] = {0};
	size_t dwEMailSize;

	dwEMailSize = (WideCharToMultiByte(MRA_CODE_PAGE, 0, lpwszEMail, -1, szEMail, SIZEOF(szEMail), NULL, NULL)-1);
	hContact = MraHContactFromEmail(szEMail, dwEMailSize, TRUE, TRUE, &bAdded);

	if (bAdded) DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);
	CallService(MS_IGNORE_UNIGNORE, (WPARAM)hContact, IGNOREEVENT_ALL);
	CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, 0);
}




