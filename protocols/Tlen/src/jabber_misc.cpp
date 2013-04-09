/*

Jabber Protocol Plugin for Miranda IM
Tlen Protocol Plugin for Miranda NG
Copyright (C) 2002-2004  Santithorn Bunchua
Copyright (C) 2004-2007  Piotr Piastucki

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "jabber.h"
#include "jabber_list.h"

void JabberDBAddEvent(TlenProtocol *proto, HANDLE hContact, int eventType, DWORD flags, PBYTE pBlob, DWORD cbBlob)
{
	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.szModule = proto->m_szModuleName;
	dbei.timestamp = (DWORD) time(NULL);
	dbei.flags = flags;
	dbei.eventType = eventType;
	dbei.cbBlob = cbBlob;
	dbei.pBlob = pBlob;
	db_event_add(hContact, &dbei);
}

void JabberDBAddAuthRequest(TlenProtocol *proto, char *jid, char *nick)
{
	char *s;
	PBYTE pCurBlob;
	PBYTE pBlob;
	DWORD cbBlob;
	HANDLE hContact;

	if ((hContact=JabberHContactFromJID(proto, jid)) == NULL) {
		hContact = (HANDLE) CallService(MS_DB_CONTACT_ADD, 0, 0);
		CallService(MS_PROTO_ADDTOCONTACT, (WPARAM) hContact, (LPARAM) proto->m_szModuleName);
		// strip resource if present
		s = JabberLoginFromJID(jid);
		_strlwr(s);
		db_set_s(hContact, proto->m_szModuleName, "jid", s);
		mir_free(s);
	}
	else {
		db_unset(hContact, proto->m_szModuleName, "Hidden");
	}
	db_set_s(hContact, proto->m_szModuleName, "Nick", nick);
	JabberLog(proto, "auth request: %s, %s", jid, nick);
	//blob is: uin(DWORD), hContact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)
	//blob is: 0(DWORD), hContact(HANDLE), nick(ASCIIZ), ""(ASCIIZ), ""(ASCIIZ), email(ASCIIZ), ""(ASCIIZ)
	cbBlob = sizeof(DWORD) + sizeof(HANDLE) + (int)strlen(nick) + (int)strlen(jid) + 5;
	pBlob = pCurBlob = (PBYTE) mir_alloc(cbBlob);
	*((PDWORD)pCurBlob) = 0; pCurBlob += sizeof(DWORD);
	*((PHANDLE)pCurBlob) = hContact; pCurBlob += sizeof(DWORD);
	strcpy((char *) pCurBlob, nick); pCurBlob += strlen(nick)+1;
	*pCurBlob = '\0'; pCurBlob++;		//firstName
	*pCurBlob = '\0'; pCurBlob++;		//lastName
	strcpy((char *) pCurBlob, jid); pCurBlob += strlen(jid)+1;
	*pCurBlob = '\0';					//reason
	JabberDBAddEvent(proto, NULL, EVENTTYPE_AUTHREQUEST, 0, pBlob, cbBlob);
}

char *JabberJIDFromHContact(TlenProtocol *proto, HANDLE hContact)
{
	char *p = NULL;
	DBVARIANT dbv;
	if (!db_get(hContact, proto->m_szModuleName, "jid", &dbv)) {
		p = mir_strdup(dbv.pszVal);
		db_free(&dbv);
	}
	return p;
}

HANDLE JabberHContactFromJID(TlenProtocol *proto, const char *jid)
{
	DBVARIANT dbv;
	char *szProto;
	char *p;
	if (jid == NULL)
		return NULL;

	for (HANDLE hContact = db_find_first(proto->m_szModuleName); hContact; hContact = db_find_next(hContact, proto->m_szModuleName)) {
		if ( db_get(hContact, proto->m_szModuleName, "jid", &dbv))
			continue;

		if ((p=dbv.pszVal) != NULL) {
			if (!stricmp(p, jid)) {	// exact match (node@domain/resource)
				db_free(&dbv);
				return hContact;
			}
		}
		db_free(&dbv);
	}

	return NULL;
}

HANDLE JabberDBCreateContact(TlenProtocol *proto, char *jid, char *nick, BOOL temporary)
{
	HANDLE hContact;
	if (jid == NULL || jid[0] == '\0')
		return NULL;

	if ((hContact=JabberHContactFromJID(proto, jid)) == NULL) {
		hContact = (HANDLE) CallService(MS_DB_CONTACT_ADD, 0, 0);
		CallService(MS_PROTO_ADDTOCONTACT, (WPARAM) hContact, (LPARAM) proto->m_szModuleName);
		db_set_s(hContact, proto->m_szModuleName, "jid", jid);
		if (nick != NULL && nick[0] != '\0')
			db_set_s(hContact, proto->m_szModuleName, "Nick", nick);
		if (temporary)
			db_set_b(hContact, "CList", "NotOnList", 1);
	}
	return hContact;
}

static void JabberContactListCreateClistGroup(char *groupName)
{
	char str[33], newName[128];
	int i;
	DBVARIANT dbv;
	char *name;

	for (i=0;;i++) {
		itoa(i, str, 10);
		if (db_get(NULL, "CListGroups", str, &dbv))
			break;
		name = dbv.pszVal;
		if (name[0] != '\0' && !strcmp(name+1, groupName)) {
			// Already exist, no need to create
			db_free(&dbv);
			return;
		}
		db_free(&dbv);
	}

	// Create new group with id = i (str is the text representation of i)
	newName[0] = 1 | GROUPF_EXPANDED;
	strncpy(newName+1, groupName, sizeof(newName)-1);
	newName[sizeof(newName)-1] = '\0';
	db_set_s(NULL, "CListGroups", str, newName);
	CallService(MS_CLUI_GROUPADDED, i+1, 0);
}

void JabberContactListCreateGroup(char *groupName)
{
	char name[128];
	char *p;

	if (groupName == NULL || groupName[0] == '\0' || groupName[0] == '\\') return;

	strncpy(name, groupName, sizeof(name));
	name[sizeof(name)-1] = '\0';
	for (p=name; *p != '\0'; p++) {
		if (*p == '\\') {
			*p = '\0';
			JabberContactListCreateClistGroup(name);
			*p = '\\';
		}
	}
	JabberContactListCreateClistGroup(name);
}


struct FORK_ARG {
	HANDLE hEvent;
	void (__cdecl *threadcode)(void*);
	void *arg;
};

static void __cdecl forkthread_r(struct FORK_ARG *fa)
{
	void (*callercode)(void*) = fa->threadcode;
	void *arg = fa->arg;
	Thread_Push(0);
	SetEvent(fa->hEvent);
	callercode(arg);
	Thread_Pop();
	return;
}

unsigned long JabberForkThread(
	void (__cdecl *threadcode)(void*),
	unsigned long stacksize,
	void *arg
)
{
	unsigned long rc;
	struct FORK_ARG fa;

	fa.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	fa.threadcode = threadcode;
	fa.arg = arg;
	rc = _beginthread((void (__cdecl *)(void*))forkthread_r, stacksize, &fa);
	if ((unsigned long) -1L != rc) {
		WaitForSingleObject(fa.hEvent, INFINITE);
	}
	CloseHandle(fa.hEvent);
	return rc;
}
