/*
UserinfoEx plugin for Miranda IM

Copyright:
� 2006-2010 DeathAxe, Yasnovidyashii, Merlin, K. Romanov, Kreol

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#pragma once

namespace DB {

namespace MetaContact{
	INT_PTR SubCount(HANDLE hMetaContact);
	INT_PTR SubDefNum(HANDLE hMetaContact);
	HANDLE  Sub(HANDLE hMetaContact, int idx);

	bool    IsSub(HANDLE hContact);
	HANDLE  GetMeta(HANDLE hContact);
} /* namespace MetaContact */

/**
 * This namespace contains all functions used to access
 * or modify contacts in the database.
 **/
namespace Contact {
	LPTSTR  DisplayName(HANDLE hContact);
	LPSTR   Proto(HANDLE hContact);

	INT_PTR GetCount();

	HANDLE  Add();
	BYTE    Delete(HANDLE hContact);

	DWORD   WhenAdded(DWORD dwUIN, LPCSTR szBaseProto);

} /* namespace Contact */

namespace Module {
	void	Delete(HANDLE hContact, LPCSTR pszModule);
	bool  IsEmpty(HANDLE hContact, LPCSTR pszModule);
	bool  IsMeta(LPCSTR pszModule);
	bool  IsMetaAndScan(LPCSTR pszModule);

} /* namespace Module */

/**
 * This namespace defines all functions used to read and write settings from the database.
 **/
namespace Setting {

	BYTE	Get(HANDLE hContact, LPCSTR pszModule, LPCSTR pszSetting, DBVARIANT *dbv, const BYTE nType);
	static FORCEINLINE BYTE GetAsIs(HANDLE hContact, LPCSTR pszModule, LPCSTR pszSetting, DBVARIANT *dbv)
	{ return Get(hContact, pszModule, pszSetting, dbv, 0); }
	static FORCEINLINE BYTE GetAString(HANDLE hContact, LPCSTR pszModule, LPCSTR pszSetting, DBVARIANT *dbv)
	{ return Get(hContact, pszModule, pszSetting, dbv, DBVT_ASCIIZ); }

	LPSTR	GetAString(HANDLE hContact, LPCSTR pszModule, LPCSTR pszSetting);
	static FORCEINLINE BYTE	GetWString(HANDLE hContact, LPCSTR pszModule, LPCSTR pszSetting, DBVARIANT *dbv)
	{ return Get(hContact, pszModule, pszSetting, dbv, DBVT_WCHAR); }

	LPWSTR GetWString(HANDLE hContact, LPCSTR pszModule, LPCSTR pszSetting);
	static FORCEINLINE BYTE	GetUString(HANDLE hContact, LPCSTR pszModule, LPCSTR pszSetting, DBVARIANT *dbv)
	{ return Get(hContact, pszModule, pszSetting, dbv, DBVT_UTF8); }

	BYTE GetEx(HANDLE hContact, LPCSTR pszModule, LPCSTR szProto, LPCSTR pszSetting, DBVARIANT *dbv, const BYTE nType);
	static FORCEINLINE BYTE GetAsIsEx(HANDLE hContact, LPCSTR pszModule, LPCSTR szProto, LPCSTR pszSetting, DBVARIANT *dbv)
	{ return GetEx(hContact, pszModule, szProto, pszSetting, dbv, 0); }
	static FORCEINLINE LPSTR GetAStringEx(HANDLE hContact, LPCSTR pszModule, LPCSTR szProto, LPCSTR pszSetting)
	{ DBVARIANT dbv; return (!GetEx(hContact, pszModule, szProto, pszSetting, &dbv, DBVT_ASCIIZ) && dbv.type == DBVT_ASCIIZ) ? dbv.pszVal : NULL; }
	static FORCEINLINE LPWSTR GetWStringEx(HANDLE hContact, LPCSTR pszModule, LPCSTR szProto, LPCSTR pszSetting)
	{ DBVARIANT dbv; return (!GetEx(hContact, pszModule, szProto, pszSetting, &dbv, DBVT_WCHAR) && dbv.type == DBVT_WCHAR) ? dbv.pwszVal : NULL; }
	static FORCEINLINE LPSTR GetUStringEx(HANDLE hContact, LPCSTR pszModule, LPCSTR szProto, LPCSTR pszSetting)
	{ DBVARIANT dbv; return (!GetEx(hContact, pszModule, szProto, pszSetting, &dbv, DBVT_UTF8) && dbv.type == DBVT_UTF8) ? dbv.pszVal : NULL; }

	WORD GetCtrl(HANDLE hContact, LPCSTR pszModule, LPCSTR szSubModule, LPCSTR szProto, LPCSTR pszSetting, DBVARIANT *dbv, const BYTE nType);
	static FORCEINLINE WORD	GetAsIsCtrl(HANDLE hContact, LPCSTR pszModule, LPCSTR szSubModule, LPCSTR szProto, LPCSTR pszSetting, DBVARIANT *dbv)
	{ return GetCtrl(hContact, pszModule, szSubModule, szProto, pszSetting, dbv, 0); }
	static FORCEINLINE WORD	GetAStringCtrl(HANDLE hContact, LPCSTR pszModule, LPCSTR szSubModule, LPCSTR szProto, LPCSTR pszSetting, DBVARIANT *dbv)
	{ return GetCtrl(hContact, pszModule, szSubModule, szProto, pszSetting, dbv, DBVT_ASCIIZ); }
	static FORCEINLINE WORD	GetWStringCtrl(HANDLE hContact, LPCSTR pszModule, LPCSTR szSubModule, LPCSTR szProto, LPCSTR pszSetting, DBVARIANT *dbv)
	{ return GetCtrl(hContact, pszModule, szSubModule, szProto, pszSetting, dbv, DBVT_WCHAR); }
	static FORCEINLINE WORD	GetUStringCtrl(HANDLE hContact, LPCSTR pszModule, LPCSTR szSubModule, LPCSTR szProto, LPCSTR pszSetting, DBVARIANT *dbv)
	{ return GetCtrl(hContact, pszModule, szSubModule, szProto, pszSetting, dbv, DBVT_UTF8); }

	BYTE GetStatic(HANDLE hContact, LPCSTR pszModule, LPCSTR pszSetting, LPSTR szValue, int cchValue);

	#define GetTString     GetWString
	#define GetTStringEx   GetWStringEx
	#define GetTStringCtrl GetWStringCtrl
	#define WriteTString   WriteWString

	/**
	 * misc operations
	 **/
	BYTE	Exists(HANDLE hContact, LPCSTR pszModule, LPCSTR pszSetting);
	void	DeleteArray(HANDLE hContact, LPCSTR pszModule, LPCSTR pszFormat, int iStart);

} /* namespace Setting */

namespace Variant {
	BYTE	ConvertString(DBVARIANT* dbv, const BYTE destType);
	BYTE	dbv2String(DBVARIANT* dbv, const BYTE destType);
} /* namespace Variant */

namespace Event   {
	HANDLE FindLast(HANDLE hContact);
	bool   GetInfo(HANDLE hEvent, DBEVENTINFO *dbei);
	bool   GetInfoWithData(HANDLE hEvent, DBEVENTINFO *dbei);
	DWORD  GetTime(HANDLE hEvent);
	bool   Exists(HANDLE hContact, HANDLE& hDbExistingEvent, DBEVENTINFO *dbei);
} /* namespace Events */

/**
 * enumerating
 **/

class CEnumList : public ::LIST<CHAR>
{
	static int EnumProc        (LPCSTR pszName, DWORD ofsModuleName, LPARAM lParam);
	static int EnumSettingsProc(LPCSTR pszName, LPARAM lParam);

	static int CompareProc     (LPCSTR p1, LPCSTR p2);
	
public:
	CEnumList();
	~CEnumList();

	LPSTR	Insert(LPCSTR str);

	INT_PTR	EnumModules();
	INT_PTR	EnumSettings(HANDLE hContact, LPCSTR pszModule);
};

} /* namespace DB */