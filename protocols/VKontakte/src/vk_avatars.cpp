/*
Copyright (c) 2013-14 Miranda NG project (http://miranda-ng.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"

void CVkProto::OnReceiveAvatar(NETLIBHTTPREQUEST *reply, AsyncHttpRequest* pReq)
{
	if (reply->resultCode != 200)
		return;

	PROTO_AVATAR_INFORMATIONT AI = { sizeof(AI) };
	GetAvatarFileName(pReq->pUserInfo, AI.filename, SIZEOF(AI.filename));
	AI.format = ProtoGetBufferFormat(reply->pData);

	FILE *out = _tfopen(AI.filename, _T("wb"));
	if (out == NULL) {
		ProtoBroadcastAck(pReq->pUserInfo, ACKTYPE_AVATAR, ACKRESULT_FAILED, &AI, 0);
		return;
	}

	fwrite(reply->pData, 1, reply->dataLength, out);
	fclose(out);
	ProtoBroadcastAck(pReq->pUserInfo, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, &AI, 0);
}

INT_PTR CVkProto::SvcGetAvatarCaps(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
	case AF_MAXSIZE:
		((POINT*)lParam)->x = 100;
		((POINT*)lParam)->y = 100;
		return 0;

	case AF_PROPORTION:
		return PIP_SQUARE;

	case AF_FORMATSUPPORTED:
	case AF_ENABLED:
	case AF_DONTNEEDDELAYS:
	case AF_FETCHIFPROTONOTVISIBLE:
	case AF_FETCHIFCONTACTOFFLINE:
		return 1;
	}

	return 0;
}

INT_PTR CVkProto::SvcGetAvatarInfo(WPARAM wParam, LPARAM lParam)
{
	PROTO_AVATAR_INFORMATIONT* AI = (PROTO_AVATAR_INFORMATIONT*)lParam;

	ptrA szUrl( getStringA(AI->hContact, "AvatarUrl"));
	if (szUrl == NULL)
		return GAIR_NOAVATAR;

	TCHAR tszFileName[MAX_PATH];
	GetAvatarFileName(AI->hContact, tszFileName, SIZEOF(tszFileName));
	_tcsncpy(AI->filename, tszFileName, SIZEOF(AI->filename));

	AI->format = ProtoGetAvatarFormat(AI->filename);

	if (::_taccess(AI->filename, 0) == 0)
		return GAIR_SUCCESS;

	if ( IsOnline()) {
		AsyncHttpRequest *pReq = new AsyncHttpRequest();
		pReq->flags = NLHRF_NODUMP | NLHRF_REDIRECT;
		pReq->m_szUrl = szUrl;
		pReq->pUserInfo = (char*)AI->hContact;
		pReq->m_pFunc = &CVkProto::OnReceiveAvatar;
		pReq->requestType = REQUEST_GET;
		Push(pReq);

		debugLogA("Requested to read an avatar from '%s'", szUrl);
		return GAIR_WAITFOR;
	}

	debugLogA("No avatar");
	return GAIR_NOAVATAR;
}

void CVkProto::GetAvatarFileName(HANDLE hContact, TCHAR* pszDest, size_t cbLen)
{
	int tPathLen = mir_sntprintf(pszDest, cbLen, _T("%s\\%S"), VARST(_T("%miranda_avatarcache%")), m_szModuleName);

	DWORD dwAttributes = GetFileAttributes(pszDest);
	if (dwAttributes == 0xffffffff || (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		CreateDirectoryTreeT(pszDest);

	pszDest[ tPathLen++ ] = '\\';

	const TCHAR* szFileType = _T(".jpg");
	ptrT szUrl( getTStringA(hContact, "AvatarUrl"));
	if (szUrl) {
		TCHAR *p = _tcsrchr(szUrl, '.');
		if (p != NULL)
			szFileType = p;
	}

	LONG id = getDword(hContact, "ID", -1);
	mir_sntprintf(pszDest + tPathLen, MAX_PATH - tPathLen, _T("%d%s"), id, szFileType);
}

void CVkProto::SetAvatarUrl(HANDLE hContact, LPCTSTR ptszUrl)
{
	ptrT oldUrl( getTStringA(hContact, "AvatarUrl"));
	if (!lstrcmp(ptszUrl, oldUrl))
		return;

	if (ptszUrl == NULL) {
		delSetting(hContact, "AvatarUrl");
		ProtoBroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, NULL, 0);
	}
	else {
		setTString(hContact, "AvatarUrl", ptszUrl);

		PROTO_AVATAR_INFORMATIONT AI = { sizeof(AI) };
		AI.hContact = hContact;
		GetAvatarFileName(AI.hContact, AI.filename, SIZEOF(AI.filename));
		AI.format = ProtoGetAvatarFormat(AI.filename);
		ProtoBroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, (HANDLE)&AI, 0);
	}
}
