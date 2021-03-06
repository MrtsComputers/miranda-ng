/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (c) 2012-14 Miranda NG project (http://miranda-ng.org),
Copyright (c) 2000-12 Miranda IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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
#include "commonheaders.h"

#define LANGPACK_BUF_SIZE 4000

static int CompareMuuids(const MUUID *p1, const MUUID *p2)
{
	return memcmp(p1, p2, sizeof(MUUID));
}

static LIST<MUUID> lMuuids(10, CompareMuuids);
static MUUID *pCurrentMuuid = NULL;
static HANDLE hevChanged = 0;

static BOOL bModuleInitialized = FALSE;

struct LangPackEntry
{
	DWORD englishHash;
	char *szLocal;
	wchar_t *wszLocal;
	MUUID *pMuuid;
	LangPackEntry* pNext;  // for langpack items with the same hash value
};

struct
{
	TCHAR filename[MAX_PATH];
	TCHAR filePath[MAX_PATH];
	char  language[64];
	char  lastModifiedUsing[64];
	char  authors[256];
	char  authorEmail[128];
	LangPackEntry *entry;
	int entryCount, entriesAlloced;
	LCID localeID;
	UINT defaultANSICp;
}
static langPack;

static int IsEmpty(const char *str)
{
	for (int i = 0; str[i]; i++)
		if (str[i] != ' ' && str[i] != '\r' && str[i] != '\n')
			return 0;

	return 1;
}

static int ConvertBackslashes(char *str, UINT fileCp)
{
	int shift = 0;
	char *pstr;
	for (pstr = str; *pstr; pstr = CharNextExA(fileCp, pstr, 0)) {
		if (*pstr == '\\') {
			shift++;
			switch (pstr[1]) {
				case 'n': *pstr = '\n'; break;
				case 't': *pstr = '\t'; break;
				case 'r': *pstr = '\r'; break;
				case 's': *pstr = ' ';  break;
				default:  *pstr = pstr[1]; break;
			}
			memmove(pstr + 1, pstr + 2, strlen(pstr + 2) + 1);
		}
	}
	return shift;
}

#ifdef _DEBUG
//#pragma optimize("gt", on)
#endif

// MurmurHash2
MIR_CORE_DLL(unsigned int) mir_hash(const void * key, unsigned int len)
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	// Initialize the hash to a 'random' value
	unsigned int h = len;

	// Mix 4 bytes at a time into the hash
	const unsigned char *data = (const unsigned char*)key;

	while (len >= 4) {
		unsigned int k = *(unsigned int*)data;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		len -= 4;
	}

	// Handle the last few bytes of the input array
	switch (len) {
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
		h *= m;
	}

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

static unsigned int __fastcall hashstrW(const char *key)
{
	if (key == NULL) return 0;
	const unsigned int len = (unsigned int)wcslen((const wchar_t*)key);
	char *buf = (char*)alloca(len + 1);
	for (unsigned i = 0; i <= len; ++i)
		buf[i] = key[i << 1];
	return mir_hash(buf, len);
}

static int SortLangPackHashesProc(LangPackEntry *arg1, LangPackEntry *arg2)
{
	if (arg1->englishHash < arg2->englishHash) return -1;
	if (arg1->englishHash > arg2->englishHash) return 1;

	return (arg1->pMuuid < arg2->pMuuid) ? -1 : 1;
}

static void swapBytes(void *p, size_t iSize)
{
	char *head = (char*)p; // here
	char *tail = head + iSize - 1;

	for (; tail > head; --tail, ++head) {
		char temp = *head;
		*head = *tail;
		*tail = temp;
	}
}

static bool EnterMuuid(const char *p, MUUID &result)
{
	if (*p++ != '{')
		return false;

	BYTE *d = (BYTE*)&result;

	for (int nBytes = 0; *p && nBytes < 24; p++) {
		if (*p == '-')
			continue;

		if (*p == '}')
			break;

		if (!isxdigit(*p))
			return false;

		if (!isxdigit(p[1]))
			return false;

		int c = 0;
		if (sscanf(p, "%2x", &c) != 1)
			return false;

		*d++ = (BYTE)c;
		nBytes++;
		p++;
	}

	if (*p != '}')
		return false;

	swapBytes(&result.a, sizeof(result.a));
	swapBytes(&result.b, sizeof(result.b));
	swapBytes(&result.c, sizeof(result.c));
	return true;
}

static void LoadLangPackFile(FILE *fp, char *line, UINT fileCp)
{
	while (!feof(fp)) {
		if (fgets(line, LANGPACK_BUF_SIZE, fp) == NULL)
			break;

		if (IsEmpty(line) || line[0] == ';' || line[0] == 0)
			continue;

		rtrim(line);

		if (line[0] == '#') {
			strlwr(line);

			if (!memcmp(line + 1, "include", 7)) {
				TCHAR tszFileName[MAX_PATH];
				TCHAR* fileName = mir_a2t(ltrim(line + 9));
				mir_sntprintf(tszFileName, SIZEOF(tszFileName), _T("%s%s"), langPack.filePath, fileName);
				mir_free(fileName);

				FILE *p = _tfopen(tszFileName, _T("r"));
				if (p) {
					line[0] = 0;
					fgets(line, SIZEOF(line), p);

					UINT fileCp = CP_ACP;
					if (strlen(line) >= 3 && line[0] == '\xef' && line[1] == '\xbb' && line[2] == '\xbf') {
						fileCp = CP_UTF8;
						fseek(p, 3, SEEK_SET);
					}
					else {
						fileCp = langPack.defaultANSICp;
						fseek(p, 0, SEEK_SET);
					}

					LoadLangPackFile(p, line, fileCp);
					fclose(p);
				}
			}
			else if (!memcmp(line + 1, "muuid", 5)) {
				MUUID t;
				if (!EnterMuuid(line + 7, t))
					continue;

				MUUID *pNew = (MUUID*)mir_alloc(sizeof(MUUID));
				memcpy(pNew, &t, sizeof(t));
				lMuuids.insert(pNew);
				pCurrentMuuid = pNew;
			}

			continue;
		}

		char cFirst = line[0];

		ConvertBackslashes(line, fileCp);

		size_t cbLen = strlen(line) - 1;
		if (cFirst == '[' && line[cbLen] == ']') {
			if (langPack.entryCount && langPack.entry[langPack.entryCount - 1].wszLocal == NULL)
				langPack.entryCount--;

			char *pszLine = line + 1;
			line[cbLen] = '\0';
			if (++langPack.entryCount > langPack.entriesAlloced) {
				langPack.entriesAlloced += 128;
				langPack.entry = (LangPackEntry*)mir_realloc(langPack.entry, sizeof(LangPackEntry)*langPack.entriesAlloced);
			}

			LangPackEntry *E = &langPack.entry[langPack.entryCount - 1];
			E->englishHash = mir_hashstr(pszLine);
			E->szLocal = NULL;
			E->wszLocal = NULL;
			E->pMuuid = pCurrentMuuid;
			E->pNext = NULL;
			continue;
		}

		if (!langPack.entryCount)
			continue;

		LangPackEntry *E = &langPack.entry[langPack.entryCount - 1];
		int iNeeded = MultiByteToWideChar(fileCp, 0, line, -1, 0, 0), iOldLen;
		if (E->wszLocal == NULL) {
			iOldLen = 0;
			E->wszLocal = (wchar_t *)mir_alloc((iNeeded + 1) * sizeof(wchar_t));
			MultiByteToWideChar(fileCp, 0, line, -1, E->wszLocal, iNeeded);
		}
		else {
			iOldLen = (int)wcslen(E->wszLocal);
			E->wszLocal = (wchar_t*)mir_realloc(E->wszLocal, (sizeof(wchar_t)* (iOldLen + iNeeded + 2)));
			E->wszLocal[iOldLen++] = '\n';
		}
		MultiByteToWideChar(fileCp, 0, line, -1, E->wszLocal + iOldLen, iNeeded);
	}
}

MIR_CORE_DLL(int) LoadLangPack(const TCHAR *szLangPack)
{
	int startOfLine = 0;
	USHORT langID;

	lstrcpy(langPack.filename, szLangPack);
	lstrcpy(langPack.filePath, szLangPack);
	TCHAR *p = _tcsrchr(langPack.filePath, '\\');
	if (p)
		p[1] = 0;

	FILE *fp = _tfopen(szLangPack, _T("rt"));
	if (fp == NULL)
		return 1;

	char line[LANGPACK_BUF_SIZE] = "";
	fgets(line, SIZEOF(line), fp);

	UINT fileCp = CP_ACP;
	size_t lineLen = strlen(line);
	if (lineLen >= 3 && line[0] == '\xef' && line[1] == '\xbb' && line[2] == '\xbf') {
		fileCp = CP_UTF8;
		memmove(line, line + 3, lineLen - 2);
	}

	lrtrim(line);
	if (lstrcmpA(line, "Miranda Language Pack Version 1")) {
		fclose(fp);
		return 2;
	}

	//headers
	while (!feof(fp)) {
		startOfLine = ftell(fp);
		if (fgets(line, SIZEOF(line), fp) == NULL)
			break;

		lrtrim(line);
		if (IsEmpty(line) || line[0] == ';' || line[0] == 0)
			continue;

		if (line[0] == '[' || line[0] == '#')
			break;

		char *pszColon = strchr(line, ':');
		if (pszColon == NULL) {
			fclose(fp);
			return 3;
		}

		*pszColon++ = 0;
		if (!lstrcmpA(line, "Language")) { mir_snprintf(langPack.language, sizeof(langPack.language), "%s", pszColon); lrtrim(langPack.language); }
		else if (!lstrcmpA(line, "Last-Modified-Using")) { mir_snprintf(langPack.lastModifiedUsing, sizeof(langPack.lastModifiedUsing), "%s", pszColon); lrtrim(langPack.lastModifiedUsing); }
		else if (!lstrcmpA(line, "Authors")) { mir_snprintf(langPack.authors, sizeof(langPack.authors), "%s", pszColon); lrtrim(langPack.authors); }
		else if (!lstrcmpA(line, "Author-email")) { mir_snprintf(langPack.authorEmail, sizeof(langPack.authorEmail), "%s", pszColon); lrtrim(langPack.authorEmail); }
		else if (!lstrcmpA(line, "Locale")) {
			char szBuf[20], *stopped;

			lrtrim(pszColon + 1);
			langID = (USHORT)strtol(pszColon, &stopped, 16);
			langPack.localeID = MAKELCID(langID, 0);
			GetLocaleInfoA(langPack.localeID, LOCALE_IDEFAULTANSICODEPAGE, szBuf, 10);
			szBuf[5] = 0;                       // codepages have max. 5 digits
			langPack.defaultANSICp = atoi(szBuf);
			if (fileCp == CP_ACP)
				fileCp = langPack.defaultANSICp;
		}
	}

	//body
	fseek(fp, startOfLine, SEEK_SET);
	langPack.entriesAlloced = 0;

	LoadLangPackFile(fp, line, fileCp);
	fclose(fp);
	pCurrentMuuid = NULL;

	qsort(langPack.entry, langPack.entryCount, sizeof(LangPackEntry), (int(*)(const void*, const void*))SortLangPackHashesProc);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

static int SortLangPackHashesProc2(LangPackEntry *arg1, LangPackEntry *arg2)
{
	if (arg1->englishHash < arg2->englishHash) return -1;
	if (arg1->englishHash > arg2->englishHash) return 1;
	return 0;
}

static char *LangPackTranslateString(MUUID *pUuid, const char *szEnglish, const int W)
{
	if (langPack.entryCount == 0 || szEnglish == NULL)
		return (char*)szEnglish;

	LangPackEntry key, *entry;
	key.englishHash = W ? hashstrW(szEnglish) : mir_hashstr(szEnglish);
	entry = (LangPackEntry*)bsearch(&key, langPack.entry, langPack.entryCount, sizeof(LangPackEntry), (int(*)(const void*, const void*))SortLangPackHashesProc2);
	if (entry == NULL)
		return (char*)szEnglish;

	// try to find the exact match, otherwise the first entry will be returned
	if (pUuid) {
		for (LangPackEntry *p = entry->pNext; p != NULL; p = p->pNext) {
			if (p->pMuuid == pUuid) {
				entry = p;
				break;
			}
		}
	}

	if (W)
		return (char*)entry->wszLocal;

	if (entry->szLocal == NULL && entry->wszLocal != NULL)
		entry->szLocal = mir_u2a_cp(entry->wszLocal, langPack.defaultANSICp);
	return entry->szLocal;
}

MIR_CORE_DLL(int) Langpack_GetDefaultCodePage()
{
	return langPack.defaultANSICp;
}

MIR_CORE_DLL(int) Langpack_GetDefaultLocale()
{
	return (langPack.localeID == 0) ? LOCALE_USER_DEFAULT : langPack.localeID;
}

MIR_CORE_DLL(TCHAR*) Langpack_PcharToTchar(const char *pszStr)
{
	if (pszStr == NULL)
		return NULL;

	int len = (int)strlen(pszStr);
	TCHAR *result = (TCHAR*)alloca((len + 1)*sizeof(TCHAR));
	MultiByteToWideChar(Langpack_GetDefaultCodePage(), 0, pszStr, -1, result, len);
	result[len] = 0;
	return mir_wstrdup(TranslateW(result));
}

/////////////////////////////////////////////////////////////////////////////////////////

MIR_CORE_DLL(char*) TranslateA_LP(const char *str, int hLangpack)
{
	return (char*)LangPackTranslateString(Langpack_LookupUuid(hLangpack), str, FALSE);
}

MIR_CORE_DLL(WCHAR*) TranslateW_LP(const WCHAR *str, int hLangpack)
{
	return (WCHAR*)LangPackTranslateString(Langpack_LookupUuid(hLangpack), (LPCSTR)str, TRUE);
}

MIR_CORE_DLL(void) TranslateMenu_LP(HMENU hMenu, int hLangpack)
{
	MUUID *uuid = Langpack_LookupUuid(hLangpack);

	MENUITEMINFO mii = { sizeof(mii) };
	for (int i = GetMenuItemCount(hMenu) - 1; i >= 0; i--) {
		TCHAR str[256];
		mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
		mii.dwTypeData = (TCHAR*)str;
		mii.cch = SIZEOF(str);
		GetMenuItemInfo(hMenu, i, TRUE, &mii);

		if (mii.cch && mii.dwTypeData) {
			TCHAR *result = (TCHAR*)LangPackTranslateString(uuid, (const char*)mii.dwTypeData, TRUE);
			if (result != mii.dwTypeData) {
				mii.dwTypeData = result;
				mii.fMask = MIIM_TYPE;
				SetMenuItemInfo(hMenu, i, TRUE, &mii);
			}
		}

		if (mii.hSubMenu != NULL)
			TranslateMenu_LP(mii.hSubMenu, hLangpack);
	}
}

static void TranslateWindow(MUUID *pUuid, HWND hwnd)
{
	TCHAR title[2048];
	GetWindowText(hwnd, title, SIZEOF(title));

	TCHAR *result = (TCHAR*)LangPackTranslateString(pUuid, (const char*)title, TRUE);
	if (result != title)
		SetWindowText(hwnd, result);
}

struct LANGPACKTRANSLATEDIALOG
{
	HWND hwndDlg;
	int  hLangpack;
};

static BOOL CALLBACK TranslateDialogEnumProc(HWND hwnd, LPARAM lParam)
{
	int hLangpack = (int)lParam;
	TCHAR szClass[32];
	int id = GetDlgCtrlID(hwnd);

	MUUID *uuid = Langpack_LookupUuid(hLangpack);

	GetClassName(hwnd, szClass, SIZEOF(szClass));
	if (!lstrcmpi(szClass, _T("static")) || !lstrcmpi(szClass, _T("hyperlink")) || !lstrcmpi(szClass, _T("button")) || !lstrcmpi(szClass, _T("MButtonClass")) || !lstrcmpi(szClass, _T("MHeaderbarCtrl")))
		TranslateWindow(uuid, hwnd);
	else if (!lstrcmpi(szClass, _T("edit"))) {
		if (GetWindowLongPtr(hwnd, GWL_STYLE) & ES_READONLY)
			TranslateWindow(uuid, hwnd);
	}
	return TRUE;
}

MIR_CORE_DLL(void) TranslateDialog_LP(HWND hDlg, int hLangpack)
{
	TranslateWindow(Langpack_LookupUuid(hLangpack), hDlg);
	EnumChildWindows(hDlg, TranslateDialogEnumProc, hLangpack);
}

/////////////////////////////////////////////////////////////////////////////////////////

MIR_CORE_DLL(MUUID*) Langpack_LookupUuid(WPARAM wParam)
{
	int idx = (wParam >> 16) & 0xFFFF;
	return (idx > 0 && idx <= lMuuids.getCount()) ? lMuuids[idx - 1] : NULL;
}

MIR_CORE_DLL(int) Langpack_MarkPluginLoaded(PLUGININFOEX *pInfo)
{
	int idx = lMuuids.getIndex(&pInfo->uuid);
	if (idx == -1)
		return 0;

	return (idx + 1) << 16;
}

MIR_CORE_DLL(void) Langpack_SortDuplicates(void)
{
	if (langPack.entryCount == 0)
		return;

	LangPackEntry *s = langPack.entry + 1, *d = s, *pLast = langPack.entry;
	DWORD dwSavedHash = langPack.entry->englishHash;
	bool bSortNeeded = false;

	for (int i = 1; i < langPack.entryCount; i++, s++) {
		if (s->englishHash != dwSavedHash) {
			pLast = d;
			if (s != d)
				*d++ = *s;
			else
				d++;
			dwSavedHash = s->englishHash;
		}
		else {
			bSortNeeded = true;
			LangPackEntry *p = (LangPackEntry*)mir_alloc(sizeof(LangPackEntry));
			*p = *s;
			pLast->pNext = p; pLast = p;
		}
	}

	if (bSortNeeded) {
		langPack.entryCount = (int)(d - langPack.entry);
		qsort(langPack.entry, langPack.entryCount, sizeof(LangPackEntry), (int(*)(const void*, const void*))SortLangPackHashesProc);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

MIR_CORE_DLL(int) LoadLangPackModule(void)
{
	bModuleInitialized = TRUE;

	ZeroMemory(&langPack, sizeof(langPack));

	hevChanged = CreateHookableEvent(ME_LANGPACK_CHANGED);

	TCHAR szSearch[MAX_PATH];
	PathToAbsoluteT(_T("langpack_*.txt"), szSearch);

	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(szSearch, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		PathToAbsoluteT(fd.cFileName, szSearch);
		FindClose(hFind);
		LoadLangPack(szSearch);
	}
	return 0;
}

void UnloadLangPackModule()
{
	if (!bModuleInitialized) return;

	int i;
	for (i = 0; i < lMuuids.getCount(); i++)
		mir_free(lMuuids[i]);
	lMuuids.destroy();

	LangPackEntry *p = langPack.entry;
	for (i = 0; i < langPack.entryCount; i++, p++) {
		if (p->pNext != NULL) {
			for (LangPackEntry *p1 = p->pNext; p1 != NULL;) {
				LangPackEntry *p2 = p1; p1 = p1->pNext;
				mir_free(p2->szLocal);
				mir_free(p2->wszLocal);
				mir_free(p2);
			}
		}

		mir_free(p->szLocal);
		mir_free(p->wszLocal);
	}

	if (langPack.entryCount) {
		mir_free(langPack.entry);
		langPack.entry = 0;
		langPack.entryCount = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

MIR_CORE_DLL(void) ReloadLangpack(TCHAR *pszStr)
{
	if (pszStr == NULL)
		pszStr = langPack.filename;

	UnloadLangPackModule();
	LoadLangPack(pszStr);
	Langpack_SortDuplicates();

	NotifyEventHooks(hevChanged, 0, 0);
}
