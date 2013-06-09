#include "skype_proto.h"
#include "skype_chat.h"
#include <m_message.h>
#include <m_history.h>

enum CHAT_LIST_MENU 
{
	ICM_CANCEL,

	ICM_DETAILS,
	ICM_AUTH_REQUEST,
	ICM_CONF_INVITE,
	ICM_ROLE, ICM_ROLE_ADMIN, ICM_ROLE_SPEAKER, ICM_ROLE_WRITER, ICM_ROLE_SPECTATOR,
	ICM_KICK, ICM_BAN,
	ICM_COPY_SID, ICM_COPY_URI
};

static struct gc_item crListItems[] =
{
	{ LPGENT("&User details"),        ICM_DETAILS,            MENU_ITEM      }, 
	{ LPGENT("&Request auth"),        ICM_AUTH_REQUEST,       MENU_ITEM      },
	{ NULL,                           0,                      MENU_SEPARATOR },
	{ LPGENT("Invite to conferance"), ICM_CONF_INVITE,        MENU_ITEM      },
	{ NULL,                           0,                      MENU_SEPARATOR },
	{ LPGENT("Set &role"),            ICM_ROLE,               MENU_NEWPOPUP  },
	{ LPGENT("&Administrator"),       ICM_ROLE_ADMIN,         MENU_POPUPITEM },
	{ LPGENT("&Speaker"),             ICM_ROLE_SPEAKER,       MENU_POPUPITEM },
	{ LPGENT("&Writer"),              ICM_ROLE_WRITER,        MENU_POPUPITEM },
	{ LPGENT("&Listener"),            ICM_ROLE_SPECTATOR,     MENU_POPUPITEM },
	{ NULL,                           0,                      MENU_SEPARATOR },
	{ LPGENT("&Kick"),                ICM_KICK,               MENU_ITEM      },
	{ LPGENT("Outlaw (&ban)"),        ICM_BAN,                MENU_ITEM      },	
	{ NULL,                           0,                      MENU_SEPARATOR },
	{ LPGENT("Copy &skypename"),      ICM_COPY_SID,           MENU_ITEM      },
	{ LPGENT("Copy room &uri"),       ICM_COPY_URI,           MENU_ITEM      }
};

static void DisableChatMenuItem(CHAT_LIST_MENU disabledId)
{
	for (int i = 0; i < SIZEOF(crListItems); i++)
	{
		if (crListItems[i].dwID == disabledId)
			crListItems[i].bDisabled = TRUE;
	}
}

static void DisableChatMenuItems(CHAT_LIST_MENU disabledIds[])
{
	for (int i = 0; i < SIZEOF(disabledIds); i++)
		DisableChatMenuItem(disabledIds[i]);
}

static void ResetChatMenuItem()
{
	for (int i = 0; i < SIZEOF(crListItems); i++)
		crListItems[i].bDisabled = FALSE;
}

static const COLORREF crCols[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

void CSkypeProto::InitChat()
{
	GCREGISTER gcr = {0};
	gcr.cbSize = sizeof(gcr);
	gcr.dwFlags = GC_TCHAR | GC_TYPNOTIF | GC_CHANMGR;
	gcr.iMaxText = 0;
	gcr.nColors = 16;
	gcr.pColors = (COLORREF*)crCols;
	gcr.ptszModuleDispName = this->m_tszUserName;
	gcr.pszModule = this->m_szModuleName;
	::CallServiceSync(MS_GC_REGISTER, 0, (LPARAM)&gcr);

	this->HookEvent(ME_GC_EVENT, &CSkypeProto::OnGCEventHook);
	this->HookEvent(ME_GC_BUILDMENU, &CSkypeProto::OnGCMenuHook);
}

///

wchar_t *ChatRoom::Roles[] = 
{ 
	L"",			// ---
	L"Creator",		// CREATOR	= 1
	L"Master",		// ADMIN	= 2
	L"Speaker",		// SPEAKER	= 3
	L"Writer",		// WRITER	= 4
	L"Spectator",	// SPECTATOR= 5
	L"Applicant",	// APPLICANT= 6
	L"Retried",		// RETIRED	= 7
	L"Outlaw",		// OUTLAW	= 8
};

ChatRoom::ChatRoom(const wchar_t *cid) : members(1, CompareMembers) 
{
	this->cid = ::mir_wstrdup(cid);
	this->name = NULL;
	this->me = NULL;
}

ChatRoom::ChatRoom(const wchar_t *cid, const wchar_t *name, CSkypeProto *ppro) : members(1, CompareMembers) 
{
	this->cid = ::mir_wstrdup(cid);
	this->name = ::mir_wstrdup(name);
	this->ppro = ppro;
	//
	this->me = new ChatMember(ppro->login);
	this->me->SetNick(::TranslateT("me"));
	this->me->SetRank(0);
	this->me->SetStatus(ID_STATUS_OFFLINE);
}

ChatRoom::~ChatRoom()
{
	if (this->cid != NULL)
		::mir_free(this->cid);
	if (this->name != NULL)
		::mir_free(this->name);
	if (this->me != NULL)
		delete this->me;
	for (int i = 0; i < this->members.getCount(); i++)
		delete this->members[i];
	this->members.destroy();
}

void ChatRoom::Start(bool showWindow)
{
	SEString data;

	// start chat session
	GCSESSION gcw = {0};
	gcw.cbSize = sizeof(gcw);
	gcw.iType = GCW_CHATROOM;
	gcw.dwFlags = GC_TCHAR;
	gcw.pszModule = ppro->m_szModuleName;
	gcw.ptszName = this->name;
	gcw.ptszID = this->cid;
	gcw.dwItemData = (DWORD)this;
	::CallServiceSync(MS_GC_NEWSESSION, 0, (LPARAM)&gcw);

	GCDEST gcd = { ppro->m_szModuleName, { NULL }, GC_EVENT_ADDGROUP };
	gcd.ptszID = this->cid;

	// load chat roles
	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.dwFlags = GC_TCHAR;
	gce.pDest = &gcd;
	for (int i = 1; i < SIZEOF(ChatRoom::Roles); i++)
	{
		gce.ptszStatus = ::TranslateW(ChatRoom::Roles[i]);
		::CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);
	}

	// init [and show window]
	gcd.iType = GC_EVENT_CONTROL;
	::CallServiceSync(MS_GC_EVENT, showWindow ? SESSION_INITDONE : WINDOW_HIDDEN, (LPARAM)&gce);
	::CallServiceSync(MS_GC_EVENT, SESSION_ONLINE, (LPARAM)&gce);
}

void ChatRoom::Start(const ParticipantRefs &participants, bool showWindow)
{
	SEString data;

	this->Start(showWindow);

	for (uint i = 0; i < participants.size(); i++)
	{
		auto participant = participants[i];

		participant->GetPropIdentity(data);
		ptrW sid = ::mir_utf8decodeW(data);

		ChatMember member(sid);
		member.SetRank(participant->GetUintProp(Participant::P_RANK));
				
		Contact::Ref contact;
		this->ppro->GetContact(data, contact);

		Contact::AVAILABILITY status;
		contact->GetPropAvailability(status);
		member.SetStatus(CSkypeProto::SkypeToMirandaStatus(status));

		contact->GetPropFullname(data);
		if (data.length() != 0)
		{
			ptrW nick = ::mir_utf8decodeW(data);
			member.SetNick(nick);
		}

		member.SetPaticipant(participant);
		member.participant.fetch();

		this->AddMember(member, NULL, NULL);
	}
}

void ChatRoom::LeaveChat()
{
	GCDEST gcd = { ppro->m_szModuleName, { NULL }, GC_EVENT_CONTROL };
	gcd.ptszID = this->cid;

	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.dwFlags = GC_TCHAR;
	gce.pDest = &gcd;
	::CallServiceSync(MS_GC_EVENT, SESSION_OFFLINE, (LPARAM)&gce);
	::CallServiceSync(MS_GC_EVENT, SESSION_TERMINATE, (LPARAM)&gce);
}

void ChatRoom::SendEvent(const ChatMember &item, int eventType, DWORD timestamp, DWORD flags, DWORD itemData, const wchar_t *status, const wchar_t *message)
{
	GCDEST gcd = { ppro->m_szModuleName, { NULL }, eventType };
	gcd.ptszID = this->cid;

	bool isMe = this->IsMe(item);

	GCEVENT gce = {0};
	gce.cbSize = sizeof(gce);
	gce.dwFlags = GC_TCHAR | flags;
	gce.pDest = &gcd;
	gce.ptszUID = item.GetSid();
	gce.ptszNick = !isMe ? item.GetNick() : ::TranslateT("me");
	gce.bIsMe = isMe;
	gce.dwItemData = itemData;
	gce.ptszStatus = status;
	gce.ptszText = message;
	gce.time = timestamp;
	::CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);
}

void ChatRoom::SendEvent(const wchar_t *sid, int eventType, DWORD timestamp, DWORD flags, DWORD itemData, const wchar_t *status, const wchar_t *message)
{
	if ( !this->IsMe(sid))
	{
		ChatMember search(sid);
		ChatMember *member = this->members.find(&search);
		if (member != NULL)
			this->SendEvent(*member, eventType, timestamp, flags, itemData, status, message);
	}
	else
		this->SendEvent(*this->me, eventType, timestamp, flags, itemData, status, message);
}

bool ChatRoom::IsMe(const wchar_t *sid) const
{
	return ::lstrcmpi(this->ppro->login, sid) == 0;
}

bool ChatRoom::IsMe(const ChatMember &item) const
{
	return ::lstrcmpi(this->ppro->login, item.GetSid()) == 0;
}

ChatMember *ChatRoom::FindChatMember(const wchar_t *sid)
{
	if ( !IsMe(sid))
	{
		ChatMember search(sid);
		return this->members.find(&search);
	}
	else
		return this->me;
}

void ChatRoom::AddMember(const ChatMember &item, const ChatMember *author, DWORD timestamp)
{
	if ( !this->IsMe(item))
	{
		ChatMember *member = this->FindChatMember(item.GetSid());
		if (member != NULL)
		{
			this->UpdateMember(item, timestamp);
		}
		else
		{
			this->members.insert(new ChatMember(item));

			this->SendEvent(item, GC_EVENT_JOIN, timestamp, GCEF_ADDTOLOG, 0, ::TranslateW(ChatRoom::Roles[item.GetRank()]));
			this->SendEvent(item, GC_EVENT_SETCONTACTSTATUS, timestamp, 0, item.GetStatus());
		}
	}
	else
	{
		if (this->me->GetRank() != item.GetRank())
		{
			this->SendEvent(*this->me, GC_EVENT_REMOVESTATUS, timestamp, 0, 0, ::TranslateW(ChatRoom::Roles[this->me->GetRank()]));
			this->SendEvent(*this->me, GC_EVENT_ADDSTATUS, timestamp, !this->me->GetRank() ? 0 : GCEF_ADDTOLOG, 0, ::TranslateW(ChatRoom::Roles[item.GetRank()]), author == NULL ? NULL : author->GetNick());
			this->me->SetRank(item.GetRank());
		}
	}
}

void ChatRoom::UpdateMember(const wchar_t *sid, const wchar_t *nick, int rank, int status, DWORD timestamp)
{
	ChatMember search(sid);
	ChatMember *member = this->members.find(&search);
	if (member != NULL)
	{
		if (::lstrcmp(member->GetNick(), nick) != 0)
		{
			this->SendEvent(*member, GC_EVENT_NICK, timestamp, GCEF_ADDTOLOG, 0, nick);
			member->SetNick(nick);
		}
		if (member->GetRank() != rank)
		{
			this->SendEvent(*member, GC_EVENT_REMOVESTATUS, timestamp, 0, 0, ::TranslateW(ChatRoom::Roles[member->GetRank()]));
			this->SendEvent(*member, GC_EVENT_ADDSTATUS, timestamp, GCEF_ADDTOLOG, 0, ::TranslateW(ChatRoom::Roles[rank]));
			member->SetRank(rank);
		}
		if (member->GetStatus() != status)
		{
			this->SendEvent(*member, GC_EVENT_SETCONTACTSTATUS, timestamp, 0, status);
			member->SetStatus(status);
		}
	}
}

void ChatRoom::UpdateMember(const ChatMember &item, DWORD timestamp)
{
	ChatMember *member = this->FindChatMember(item.GetSid());
	if (member != NULL)
	{
		ptrW nick(item.GetNick());
		if (::lstrcmp(member->GetNick(), nick) != 0)
		{
			this->SendEvent(*member, GC_EVENT_NICK, timestamp, GCEF_ADDTOLOG, 0, nick);
			member->SetNick(nick);
		}
		if (member->GetRank() != item.GetRank())
		{
			this->SendEvent(*member, GC_EVENT_REMOVESTATUS, timestamp, 0, 0, ::TranslateW(ChatRoom::Roles[member->GetRank()]));
			this->SendEvent(*member, GC_EVENT_ADDSTATUS, timestamp, GCEF_ADDTOLOG, 0, ::TranslateW(ChatRoom::Roles[item.GetRank()]));
			member->SetRank(item.GetRank());
		}
		if (member->GetStatus() != item.GetStatus())
		{
			this->SendEvent(*member, GC_EVENT_SETCONTACTSTATUS, timestamp, 0, item.GetStatus());
			member->SetStatus(item.GetStatus());
		}
	}
}

void ChatRoom::KickMember(const ChatMember &item, const ChatMember *author, DWORD timestamp)
{
	if ( !this->IsMe(item))
	{
		ChatMember *member = this->FindChatMember(item.GetSid());
		if (member != NULL)
		{
			this->SendEvent(*member, GC_EVENT_KICK, timestamp, GCEF_ADDTOLOG, 0, author->GetNick());
			this->members.remove(member);
			delete member;
		}
	}
	else
	{
		this->SendEvent(*this->me, GC_EVENT_KICK, timestamp, GCEF_ADDTOLOG, 0, author->GetNick());
		this->me->SetRank(/*APPLICANT= */6);
	}
}

void ChatRoom::KickMember(const wchar_t *sid, const wchar_t *author, DWORD timestamp)
{
	ChatMember member(sid);
	this->KickMember(member, this->FindChatMember(author), timestamp);
}

void ChatRoom::RemoveMember(const ChatMember &item, DWORD timestamp)
{
	if ( !this->IsMe(item))
	{
		ChatMember *member = this->FindChatMember(item.GetSid());
		if (member != NULL)
		{
			this->SendEvent(*member, GC_EVENT_QUIT, timestamp);
			this->members.remove(member);
			delete member;
		}
	}
	else
		this->LeaveChat();
}

void ChatRoom::RemoveMember(const wchar_t *sid, DWORD timestamp)
{
	ChatMember member(sid);
	this->RemoveMember(member, timestamp);
}

void ChatRoom::OnEvent(const ConversationRef &conversation, const MessageRef &message)
{
	uint messageType;
	messageType = message->GetUintProp(Message::P_TYPE);

	switch (messageType)
	{
	case CMessage::POSTED_EMOTE:
	case CMessage::POSTED_TEXT:
		{
			SEString data;

			message->GetPropAuthor(data);
			ptrW sid = ::mir_utf8decodeW(data);

			message->GetPropBodyXml(data);
			ptrW text =::mir_utf8decodeW(CSkypeProto::RemoveHtml(data));

			uint timestamp;
			message->GetPropTimestamp(timestamp);
			
			this->SendEvent(
				sid, 
				messageType == CMessage::POSTED_TEXT ? GC_EVENT_MESSAGE : GC_EVENT_ACTION,
				timestamp,
				GCEF_ADDTOLOG,
				0,
				NULL,
				text);
		}
		break;

	case Message::ADDED_CONSUMERS:
	case Message::ADDED_APPLICANTS:
		{
			SEString data;

			/*Message::CONSUMPTION_STATUS status;
			message->GetPropConsumptionStatus(status);
			if (status == Message::UNCONSUMED_NORMAL)*/
			{
				uint timestamp;
				message->GetPropTimestamp(timestamp);

				message->GetPropAuthor(data);
				ChatMember *author = this->FindChatMember((wchar_t *)ptrW(::mir_utf8decodeW(data)));

				ParticipantRefs participants;
				conversation->GetParticipants(participants);
				for (size_t i = 0; i < participants.size(); i++)
				{					
					participants[i]->GetPropIdentity(data);
					ptrW sid(::mir_utf8decodeW(data));
					if (this->FindChatMember(sid) == NULL)
					{
						ChatMember member(sid);
						member.SetRank(participants[i]->GetUintProp(Participant::P_RANK));

						Contact::Ref contact;
						this->ppro->GetContact(data, contact);

						Contact::AVAILABILITY status;
						contact->GetPropAvailability(status);
						member.SetStatus(CSkypeProto::SkypeToMirandaStatus(status));

						contact->GetPropFullname(data);
						ptrW nick(::mir_utf8decodeW(data));
						if (data.length() != 0)
						{
							ptrW nick = ::mir_utf8decodeW(data);
							member.SetNick(nick);
						}
						
						member.participant = participants[i];
						member.participant.fetch();

						this->AddMember(member, author, timestamp);
					}
				}

				// do not remove
				//message->GetPropIdentities(data);
				//char *identities = ::mir_strdup(data);
				//if (identities)
				//{
				//	char *identity = ::strtok(identities, " ");
				//	if (identity != NULL)
				//	{
				//		do
				//		{
				//			Contact::Ref contact;
				//			this->ppro->GetContact(identity, contact);

				//			contact->GetIdentity(data);
				//			ptrW sid = ::mir_utf8decodeW(data);

				//			ChatMember *member = new ChatMember(sid);
				//			//todo: fix rank
				//			
				//			member->rank = 
				//				messageType == Message::ADDED_APPLICANTS ? 
				//				Participant::APPLICANT : 
				//				Participant::SPEAKER;
				//				//conversation->GetUintProp(Conversation::P_OPT_ENTRY_LEVEL_RANK);
				//				//participants[i]->GetUintProp(Participant::P_RANK);

				//			Contact::AVAILABILITY status;
				//			contact->GetPropAvailability(status);
				//			member->status = CSkypeProto::SkypeToMirandaStatus(status);

				//			contact->GetPropFullname(data);
				//			member->nick = ::mir_utf8decodeW(data);

				//			this->AddMember(member, timestamp);

				//			identity = ::strtok(NULL, " ");
				//		}
				//		while (identity != NULL);
				//	}
				//	::mir_free(identities);
				//}
			}
		}
		break;

	case Message::RETIRED_OTHERS:
		{
			SEString data;

			/*Message::CONSUMPTION_STATUS status;
			message->GetPropConsumptionStatus(status);
			if (status == Message::UNCONSUMED_NORMAL)*/
			{
				uint timestamp;
				message->GetPropTimestamp(timestamp);

				message->GetPropAuthor(data);
				ptrW author = ::mir_utf8decodeW(data);

				message->GetPropIdentities(data);
				char *identities = ::mir_strdup(data);
				if (identities)
				{
					char *identity = ::strtok(identities, " ");
					if (identity != NULL)
					{
						do
						{
							ptrW sid(::mir_utf8decodeW(identity));
							this->KickMember(sid, author, timestamp);

							identity = ::strtok(NULL, " ");
						}
						while (identity != NULL);
					}
					::mir_free(identities);
				}
			}
		}
		break;

	case Message::RETIRED:
		{
			SEString data;

			Message::CONSUMPTION_STATUS status;
			message->GetPropConsumptionStatus(status);
			if (status == Message::UNCONSUMED_NORMAL)
			{
				uint timestamp;
				message->GetPropTimestamp(timestamp);

				message->GetPropAuthor(data);
				ptrW sid = ::mir_utf8decodeW(data);

				this->RemoveMember(sid, timestamp);
			}
		}
		break;

	case Message::SET_RANK:
		{
			SEString data;
			message->GetPropBodyXml(data);
			ptrA text = ::mir_strdup(data);
			int i = 0;

			/*Message::CONSUMPTION_STATUS status;
			message->GetPropConsumptionStatus(status);
			if (status == Message::UNCONSUMED_NORMAL)*/
			{
				message->GetPropAuthor(data);	
				ptrW sid = ::mir_utf8decodeW(data);

				ChatMember search(sid);
				ChatMember *member = this->FindChatMember(sid);
				if (member != NULL)
				{
					uint timestamp;
					message->GetPropTimestamp(timestamp);

					message->GetPropBodyXml(data);	
					ptrW rank = ::mir_utf8decodeW(data);

					member->SetRank(0);
				}
			}
		}
		break;

	case CMessage::STARTED_LIVESESSION:
		{
			SEString data;

			Message::CONSUMPTION_STATUS status;
			message->GetPropConsumptionStatus(status);
			if (status != Message::UNCONSUMED_NORMAL)
				break;

			message->GetPropAuthor(data);	
			ptrW sid = ::mir_utf8decodeW(data);

			uint timestamp;
			message->GetPropTimestamp(timestamp);
			
			this->SendEvent(
				sid, 
				GC_EVENT_INFORMATION, 
				timestamp,
				GCEF_ADDTOLOG, 
				0, 
				NULL, 
				::TranslateT("Incoming group call received"));
		}
		break;

	case CMessage::ENDED_LIVESESSION:
		{
			SEString data;

			Message::CONSUMPTION_STATUS status;
			message->GetPropConsumptionStatus(status);
			if (status != Message::UNCONSUMED_NORMAL)
				break;

			message->GetPropAuthor(data);	
			ptrW sid = ::mir_utf8decodeW(data);

			uint timestamp;
			message->GetPropTimestamp(timestamp);
			
			this->SendEvent(
				sid, 
				GC_EVENT_INFORMATION, 
				timestamp,
				GCEF_ADDTOLOG, 
				0, 
				NULL, 
				::TranslateT("Incoming group call finished"));
		}
		break;
	}
}

///

void CSkypeProto::ChatValidateContact(HANDLE hItem, HWND hwndList, const StringList &contacts)
{
	if (this->IsProtoContact(hItem) && !this->IsChatRoom(hItem))
	{
		ptrW sid( ::db_get_wsa(hItem, this->m_szModuleName, SKYPE_SETTINGS_LOGIN));
		if (sid == NULL || contacts.contains(sid))
			::SendMessage(hwndList, CLM_DELETEITEM, (WPARAM)hItem, 0);
	}
	else
		::SendMessage(hwndList, CLM_DELETEITEM, (WPARAM)hItem, 0);
}

void CSkypeProto::ChatPrepare(HANDLE hItem, HWND hwndList, const StringList &contacts)
{
	if (hItem == NULL)
		hItem = (HANDLE)::SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_ROOT, 0);

	while (hItem) 
	{
		HANDLE hItemN = (HANDLE)::SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXT, (LPARAM)hItem);

		if (IsHContactGroup(hItem))
		{
			HANDLE hItemT = (HANDLE)::SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_CHILD, (LPARAM)hItem);
			if (hItemT) this->ChatPrepare(hItemT, hwndList, contacts);
		}
		else if (IsHContactContact(hItem))
			this->ChatValidateContact(hItem, hwndList, contacts);

		hItem = hItemN;
	}
}

void CSkypeProto::GetInvitedContacts(HANDLE hItem, HWND hwndList, StringList &chatTargets)
{
	if (hItem == NULL)
		hItem = (HANDLE)::SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_ROOT, 0);

	while (hItem)
	{
		if (IsHContactGroup(hItem))
		{
			HANDLE hItemT = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_CHILD, (LPARAM)hItem);
			if (hItemT)
				this->GetInvitedContacts(hItemT, hwndList, chatTargets);
		}
		else
		{
			int chk = SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)hItem, 0);
			if (chk)
			{
				if (IsHContactInfo(hItem))
				{
					TCHAR buf[128] = _T("");
					SendMessage(hwndList, CLM_GETITEMTEXT, (WPARAM)hItem, (LPARAM)buf);

					if (buf[0]) chatTargets.insert(buf);
				}
				else
				{
					mir_ptr<wchar_t> login( ::db_get_wsa(hItem, this->m_szModuleName, SKYPE_SETTINGS_LOGIN));
					chatTargets.insert(login);
				}
			}
		}
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXT, (LPARAM)hItem);
	}
}

INT_PTR CALLBACK CSkypeProto::InviteToChatProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	InviteChatParam *param = (InviteChatParam *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		param = (InviteChatParam *)lParam;
		{
			HWND hwndClist = GetDlgItem(hwndDlg, IDC_CCLIST);
			SetWindowLongPtr(hwndClist, GWL_STYLE, GetWindowLongPtr(hwndClist, GWL_STYLE) & ~CLS_HIDEOFFLINE);
		}
		break;

	case WM_CLOSE:
		::EndDialog(hwndDlg, 0);
		break;

	case WM_NOTIFY:
		{
			NMCLISTCONTROL *nmc = (NMCLISTCONTROL *)lParam;
			if (nmc->hdr.idFrom == IDC_CCLIST)
			{
				switch (nmc->hdr.code)
				{
				case CLN_NEWCONTACT:
					if (param && (nmc->flags & (CLNF_ISGROUP | CLNF_ISINFO)) == 0)
					{
						param->ppro->ChatValidateContact(nmc->hItem, nmc->hdr.hwndFrom, param->invitedContacts);
					}
					break;

				case CLN_LISTREBUILT:
					if (param)
					{
						param->ppro->ChatPrepare(NULL, nmc->hdr.hwndFrom, param->invitedContacts);
					}
					break;
				}
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ADDSCR:
			if (param->ppro->IsOnline())
			{
				wchar_t sid[SKYPE_SID_LIMIT];
				::GetDlgItemText(hwndDlg, IDC_EDITSCR, sid, SIZEOF(sid));

				CLCINFOITEM cii = {0};
				cii.cbSize = sizeof(cii);
				cii.flags = CLCIIF_CHECKBOX | CLCIIF_BELOWCONTACTS;
				cii.pszText = ::wcslwr(sid);

				HANDLE hItem = (HANDLE)::SendDlgItemMessage(
					hwndDlg,
					IDC_CCLIST,
					CLM_ADDINFOITEM,
					0,
					(LPARAM)&cii);
				::SendDlgItemMessage(hwndDlg, IDC_CCLIST, CLM_SETCHECKMARK, (LPARAM)hItem, 1);
			}
			break;

		case IDOK:
			{
				HWND hwndList = ::GetDlgItem(hwndDlg, IDC_CCLIST);

				param->invitedContacts.clear();
				param->ppro->GetInvitedContacts(NULL, hwndList, param->invitedContacts);

				if ( !param->invitedContacts.empty())
				{
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
					::EndDialog(hwndDlg, IDOK);
				}
				else
					param->ppro->ShowNotification(::TranslateT("You did not select any contact"));
			}
			break;

		case IDCANCEL:
			::EndDialog(hwndDlg, IDCANCEL);
			break;
		}
		break;
	}
	return FALSE;
}

bool CSkypeProto::IsChatRoom(HANDLE hContact)
{
	return ::db_get_b(hContact, this->m_szModuleName, "ChatRoom", 0) == 1;
}

HANDLE CSkypeProto::GetChatRoomByCid(const wchar_t *cid)
{
	HANDLE hContact = NULL;

	::EnterCriticalSection(&this->contact_search_lock);

	for (hContact = ::db_find_first(this->m_szModuleName); hContact; hContact = ::db_find_next(hContact, this->m_szModuleName))
	{
		if (this->IsChatRoom(hContact))
		{
			ptrW chatSid( ::db_get_wsa(hContact, this->m_szModuleName, SKYPE_SETTINGS_LOGIN));
			if (::lstrcmpi(chatSid, cid) == 0)
				break;
		}
	}

	::LeaveCriticalSection(&this->contact_search_lock);

	return hContact;
}

HANDLE CSkypeProto::AddChatRoom(CConversation::Ref conversation)
{
	SEString data;

	conversation->GetPropIdentity(data);
	ptrW cid = ::mir_utf8decodeW(data);

	HANDLE hContact = this->GetChatRoomByCid(cid);
	if ( !hContact)
	{
		hContact = (HANDLE)::CallService(MS_DB_CONTACT_ADD, 0, 0);
		::CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)this->m_szModuleName);

		conversation->GetPropDisplayname(data);
		ptrW name = ::mir_utf8decodeW(data);

		conversation->GetJoinBlob(data);
		ptrW joinBlob = ::mir_utf8decodeW(data);

		::db_set_b(hContact, this->m_szModuleName, "ChatRoom", 1);
		::db_set_ws(hContact, this->m_szModuleName, SKYPE_SETTINGS_LOGIN, cid);
		::db_set_ws(hContact, this->m_szModuleName, "ChatRoomID", cid);
		::db_set_ws(hContact, this->m_szModuleName, "JoinBlob", joinBlob);
		::db_set_ws(hContact, this->m_szModuleName, "Nick", name);
		::db_set_w(hContact, this->m_szModuleName, "Status", ID_STATUS_OFFLINE);
		::db_set_w(hContact, this->m_szModuleName, "ApparentMode", ID_STATUS_OFFLINE);
		
		ptrW defaultGroup = ::db_get_wsa(NULL, "Chat", "AddToGroup");
		if (defaultGroup != NULL)
		{
			::db_set_ws(hContact, "CList", "Group", defaultGroup);
		}
	}

	return hContact;
}

void CSkypeProto::StartChat(StringList &invitedContacts)
{
	InviteChatParam *param = new InviteChatParam(NULL, invitedContacts, this);

	SEStringList needToAdd;
	for (size_t i = 0; i < param->invitedContacts.size(); i++)
		needToAdd.append(std::string(::mir_utf8encodeW(param->invitedContacts[i])).c_str());
	
	if (::DialogBoxParam(
		g_hInstance, 
		MAKEINTRESOURCE(IDD_CHATROOM_INVITE), 
		NULL, 
		CSkypeProto::InviteToChatProc, 
		(LPARAM)param) == IDOK)
	{
		for (size_t i = 0; i < param->invitedContacts.size(); i++)
		{
			std::string sid(::mir_utf8encodeW(param->invitedContacts[i]));
			if ( !needToAdd.contains(sid.c_str()))
				needToAdd.append(sid.c_str());
		}

		CConversation::Ref conversation;
		this->CreateConferenceWithConsumers(conversation, needToAdd);
	}

	delete param;
}

void CSkypeProto::StartChat()
{
	StringList empty;
	return this->StartChat(empty);
}

void CSkypeProto::LeaveChat(const wchar_t *cid)
{
	GC_INFO gci = {0};
	gci.Flags = BYID | DATA;
	gci.pszModule = this->m_szModuleName;
	gci.pszID = ::mir_wstrdup(cid);

	if ( !::CallServiceSync(MS_GC_GETINFO, 0, (LPARAM)&gci))
	{
		ChatRoom *room = (ChatRoom *)gci.dwItemData;
		room->LeaveChat();
	}
}

int __cdecl CSkypeProto::OnGCEventHook(WPARAM, LPARAM lParam)
{
	GCHOOK *gch = (GCHOOK *)lParam;
	if (!gch) return 1;

	if (::strcmp(gch->pDest->pszModule, this->m_szModuleName))
		return 0;

	GC_INFO gci = {0};
	gci.Flags = BYID | DATA;
	gci.pszModule = gch->pDest->pszModule;
	gci.pszID = gch->pDest->ptszID;

	::CallServiceSync(MS_GC_GETINFO, 0, (LPARAM)&gci);

	ChatRoom *room = (ChatRoom *)gci.dwItemData;

	if (room == NULL)
		return 0;

	switch (gch->pDest->iType)
	{
	/*case GC_SESSION_TERMINATE:
		{
			ptrA cid = ::mir_utf8encodeW(gch->pDest->ptszID);
			if (this->GetConversationByIdentity((char *)cid, conversation, false))
			{
				Participant::Refs participants;
				conversation->GetParticipants(participants, CConversation::MYSELF);
				participants[0]->Retire();
			}
		}
		break;*/

	case GC_USER_MESSAGE:
		if (gch->ptszText && gch->ptszText[0])
		{
			CMessage::Ref message;
			ptrA text(::mir_utf8encodeW(gch->ptszText));
			room->conversation->PostText((char *)text, message);
		}
		break;

	/*case GC_USER_CHANMGR:
		if (this->GetConversationByIdentity(::mir_utf8encodeW(cid), conversation, false))
		{
			StringList invitedContacts(this->GetChatUsers(cid));
			this->InviteConactsToChat(conversation, invitedContacts); 
		}
		break;*/

	case GC_USER_PRIVMESS:
		{
			::CallService(MS_MSG_SENDMESSAGE, (WPARAM)this->GetContactBySid(gch->ptszUID), 0);
		}
		break;

	case GC_USER_LOGMENU:
	case GC_USER_NICKLISTMENU:
		switch (gch->dwData)
		{
		case CHAT_LIST_MENU::ICM_ROLE_ADMIN:
		case CHAT_LIST_MENU::ICM_ROLE_SPEAKER:
		case CHAT_LIST_MENU::ICM_ROLE_WRITER:
		case CHAT_LIST_MENU::ICM_ROLE_SPECTATOR:
			{
				ChatMember *member = room->FindChatMember(gch->ptszUID);
				if (member != NULL)
				{
					Participant::RANK rank;
					switch (gch->dwData)
					{
						case CHAT_LIST_MENU::ICM_ROLE_ADMIN:
							rank = Participant::ADMIN;
							break;

						case CHAT_LIST_MENU::ICM_ROLE_SPEAKER:
							rank = Participant::SPEAKER;
							break;

						case CHAT_LIST_MENU::ICM_ROLE_WRITER:
							rank = Participant::WRITER;
							break;

						case CHAT_LIST_MENU::ICM_ROLE_SPECTATOR:
							rank = Participant::SPECTATOR;
							break;
					}
					if (member->participant && member->participant->SetRankTo(rank))
					{
						member->SetRank(rank);
						room->UpdateMember(member->GetSid(), member->GetNick(), rank, member->GetStatus());
					}
				}
			}
			break;

		case CHAT_LIST_MENU::ICM_KICK:
			{
				ChatMember *member = room->FindChatMember(gch->ptszUID);
				if (member != NULL)
				{
					if (member->participant && member->participant->Retire())
						room->KickMember(gch->ptszUID, room->me->GetSid());
				}
			}
			break;

		case CHAT_LIST_MENU::ICM_BAN:
			{
				ChatMember *member = room->FindChatMember(gch->ptszUID);
				if (member != NULL && member->participant)
				{
					member->participant->SetRankTo(Participant::OUTLAW);
					if (member->participant->Retire())
						room->KickMember(gch->ptszUID, room->me->GetSid());
				}
			}
			break;

		case CHAT_LIST_MENU::ICM_AUTH_REQUEST:
			{
				CContact::Ref contact;
				SEString sid((char *)ptrA(::mir_utf8encodeW(gch->ptszUID)));
				if (this->GetContact(sid, contact))
				{
					this->AuthRequest(
						this->AddContact(contact),
						LPGENT("Hi! I\'d like to add you to my contact list"));
				}
			}
			break;

		case CHAT_LIST_MENU::ICM_DETAILS:
			::CallService(MS_USERINFO_SHOWDIALOG, (WPARAM)this->GetContactBySid(gch->ptszUID), 0);
			break;

		case CHAT_LIST_MENU::ICM_COPY_SID:
			{
				HANDLE hContact = this->GetContactBySid(gch->ptszUID);
				if (!hContact)
				{
					ptrW sid = ::db_get_wsa(hContact, this->m_szModuleName, SKYPE_SETTINGS_LOGIN);
					if (sid != NULL)
						CSkypeProto::CopyToClipboard(sid);
				}
			}
			break;

		case CHAT_LIST_MENU::ICM_COPY_URI:
			{
				SEString data;
				room->conversation->GetJoinBlob(data);
				ptrW blob = ::mir_utf8decodeW(data);
				
				wchar_t uri[MAX_PATH];
				::mir_sntprintf(uri, SIZEOF(uri), L"skype:?chat&blob=%s", blob);

				CSkypeProto::CopyToClipboard(uri);
			}
			break;
		}
		break;

	//case GC_USER_TYPNOTIFY:
		//break;
	}
	return 0;
}

int __cdecl CSkypeProto::OnGCMenuHook(WPARAM, LPARAM lParam)
{
	GCMENUITEMS *gcmi = (GCMENUITEMS*) lParam;

	GC_INFO gci = {0};
	gci.Flags = BYID | DATA;
	gci.pszModule = gcmi->pszModule;
	gci.pszID = gcmi->pszID;

	::CallServiceSync(MS_GC_GETINFO, 0, (LPARAM)&gci);

	ChatRoom *room = (ChatRoom *)gci.dwItemData;

	if (room == NULL || ::stricmp(gcmi->pszModule, this->m_szModuleName))
		return 0;

	ResetChatMenuItem();

	/*if (gcmi->Type == MENU_ON_LOG)
	{
		gcmi->nItems = SIZEOF(crListItems);
		gcmi->Item = crListItems;
	}
	else if (gcmi->Type == MENU_ON_NICKLIST)
	{
		gcmi->nItems = SIZEOF(crListItems);
		gcmi->Item = crListItems;
	}*/

	if (room->me->GetRank() >= Participant::APPLICANT)
	{
		DisableChatMenuItem(ICM_CONF_INVITE);
	}

	if (gcmi->pszUID == NULL || this->GetContactBySid(gcmi->pszUID) != NULL)
	{
		DisableChatMenuItem(ICM_AUTH_REQUEST);
		DisableChatMenuItem(ICM_COPY_SID);
	}

	if (room->me->GetRank() > Participant::ADMIN && room->me->GetRank() == 0)
	{
		//CHAT_LIST_MENU adminItems[] = { ICM_ROLE, ICM_ROLE_ADMIN, ICM_ROLE_SPEAKER, ICM_ROLE_WRITER, ICM_ROLE_SPECTATOR };
		//DisableChatMenuItems(adminItems);

		DisableChatMenuItem(ICM_ROLE);
		DisableChatMenuItem(ICM_KICK);
		DisableChatMenuItem(ICM_BAN);
	}

	gcmi->nItems = SIZEOF(crListItems);
	gcmi->Item = crListItems;

	return 0;
}

wchar_t *CSkypeProto::GetChatUsers(const wchar_t *cid)
{
	GC_INFO gci = {0};
	gci.Flags = USERS;
	gci.pszModule = this->m_szModuleName;
	gci.pszID = ::mir_wstrdup(cid);
	::CallService(MS_GC_GETINFO, 0, (LPARAM)(GC_INFO *) &gci);

	::mir_free(gci.pszID);

	// todo: fix me
	return ::mir_a2u(gci.pszUsers);
}

void CSkypeProto::UpdateChatUserStatus(CContact::Ref contact)
{
	CContact::AVAILABILITY availability;
	contact->GetPropAvailability(availability);

	SEString identity;
	contact->GetIdentity(identity);
	ptrW sid(::mir_utf8decodeW(identity));

	GC_INFO gci = {0};
	gci.Flags = BYINDEX | DATA;
	gci.pszModule = this->m_szModuleName;

	int count = ::CallServiceSync(MS_GC_GETSESSIONCOUNT, 0, (LPARAM)this->m_szModuleName);
	for (int i = 0; i < count ; i++)
	{
		gci.iItem = i;
		::CallServiceSync(MS_GC_GETINFO, 0, (LPARAM)&gci);

		ChatRoom *room = (ChatRoom *)gci.dwItemData;
		if (room != NULL)
		{
			room->SendEvent(
				sid,
				GC_EVENT_SETCONTACTSTATUS,
				0,
				0,
				CSkypeProto::SkypeToMirandaStatus(availability));
		}
	}
}

INT_PTR __cdecl CSkypeProto::OnJoinChat(WPARAM wParam, LPARAM)
{
	HANDLE hContact = (HANDLE)wParam;
	if (hContact)
	{
		ptrW joinBlob(::db_get_wsa(hContact, this->m_szModuleName, "JoinBlob"));
		ptrW cid(::db_get_wsa(hContact, this->m_szModuleName, SKYPE_SETTINGS_LOGIN));

		SEString data;
		ConversationRef conversation;

		this->GetConversationByBlob(::mir_utf8encodeW(joinBlob), conversation);
		//this->GetConversationByIdentity(::mir_utf8encodeW(cid), conversation);
		if (conversation)
		{
			//conversation->Join();

			conversation->GetPropDisplayname(data);
			ptrW name(::mir_utf8decodeW(data));

			/*conversation->GetPropIdentity(data);
			ptrW cid(::mir_utf8decodeW(data));*/
			
			ChatRoom *room = new ChatRoom(cid, name, this);
			room->conversation = conversation;
			//room->conversation.fetch();

			Participant::Refs participants;
			conversation->GetParticipants(participants, Conversation::ALL);		

			room->Start(participants, true);
		}
	}
	
	return 0;
}

INT_PTR __cdecl CSkypeProto::OnLeaveChat(WPARAM wParam, LPARAM)
{
	HANDLE hContact = (HANDLE)wParam;
	if (hContact)
	{
		ptrW cid(::db_get_wsa(hContact, this->m_szModuleName, SKYPE_SETTINGS_LOGIN));
		
		GC_INFO gci = {0};
		gci.Flags = BYID | DATA;
		gci.pszModule = this->m_szModuleName;
		gci.pszID = cid;

		if ( !::CallServiceSync(MS_GC_GETINFO, 0, (LPARAM)&gci))
		{
			ChatRoom *room = (ChatRoom *)gci.dwItemData;
			room->conversation->RetireFrom();
			room->LeaveChat();
		}
	}

	return 0;
}

///

void __cdecl CSkypeProto::LoadChatList(void*)
{
	this->Log(L"Updating group chats list");
	CConversation::Refs conversations;
	this->GetConversationList(conversations);

	SEString data;
	for (uint i = 0; i < conversations.size(); i++)
	{
		auto conversation = conversations[i];

		uint convoType = conversation->GetUintProp(Conversation::P_TYPE);
		if (convoType == CConversation::CONFERENCE)
		{
			bool isBookmarked;
			conversation->GetPropIsBookmarked(isBookmarked);

			CConversation::MY_STATUS status;
			conversation->GetPropMyStatus(status);
			if (status == Conversation::APPLICANT || status == Conversation::CONSUMER || isBookmarked)
			{
				conversation->GetPropIdentity(data);
				ptrW cid = ::mir_utf8decodeW(data);

				conversation->GetPropDisplayname(data);
				ptrW name = ::mir_utf8decodeW(data);

				ChatRoom *room = new ChatRoom(cid, name, this);
				room->conversation = conversation;
				//room->conversation.fetch();
				this->AddChatRoom(conversation);

				Participant::Refs participants;
				conversation->GetParticipants(participants, Conversation::ALL);
				
				room->Start(participants);
			}
		}
	}
}

///

void CSkypeProto::OnChatEvent(const ConversationRef &conversation, const MessageRef &message)
{
	uint messageType;
	messageType = message->GetUintProp(Message::P_TYPE);

	SEString data;
	conversation->GetPropIdentity(data);
	ptrW cid = ::mir_utf8decodeW(data);

	GC_INFO gci = {0};
	gci.Flags = BYID | DATA;
	gci.pszModule = this->m_szModuleName;
	gci.pszID = cid;

	if ( !::CallServiceSync(MS_GC_GETINFO, 0, (LPARAM)&gci))
	{
		ChatRoom *room = (ChatRoom *)gci.dwItemData;
		room->OnEvent(conversation, message);
	}
	else if(messageType != Message::RETIRED || messageType != Message::RETIRED_OTHERS)
	{
		SEString data;

		conversation->GetPropIdentity(data);
		ptrW cid = ::mir_utf8decodeW(data);

		conversation->GetPropDisplayname(data);
		ptrW name = ::mir_utf8decodeW(data);

		ChatRoom *room = new ChatRoom(cid, name, this);
		room->conversation = conversation;
		//room->conversation.fetch();
		this->AddChatRoom(conversation);

		Participant::Refs participants;
		conversation->GetParticipants(participants, Conversation::ALL);
				
		room->Start(participants, true);
	}
}

void CSkypeProto::OnConversationListChange(
		const ConversationRef& conversation,
		const Conversation::LIST_TYPE& type,
		const bool& added)
{
	uint convoType = conversation->GetUintProp(Conversation::P_TYPE);
	if (convoType == Conversation::CONFERENCE && type == Conversation::INBOX_CONVERSATIONS  && added)
	{
		SEString data;

		conversation->GetPropIdentity(data);
		ptrW cid = ::mir_utf8decodeW(data);

		if ( !this->GetChatRoomByCid(cid))
		{
			conversation->GetPropIdentity(data);
			ptrW cid = ::mir_utf8decodeW(data);

			conversation->GetPropDisplayname(data);
			ptrW name = ::mir_utf8decodeW(data);

			ChatRoom *room = new ChatRoom(cid, name, this);
			room->conversation = conversation;
			//room->conversation.fetch();
			this->AddChatRoom(conversation);

			Participant::Refs participants;
			conversation->GetParticipants(participants, Conversation::ALL);
				
			room->Start(participants, true);
		}
	}
}