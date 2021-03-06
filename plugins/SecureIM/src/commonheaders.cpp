#include "commonheaders.h"

HINSTANCE g_hInst, g_hIconInst;

char TEMP[MAX_PATH];
int  TEMP_SIZE = 0;

HANDLE   g_hEvent[2], g_hCLIcon=0, g_hFolders=0;
HGENMENU g_hMenu[15];

int iService=0;
int iHook=0;

HICON g_hICO[ICO_CNT], g_hPOP[POP_CNT], g_hIEC[1+IEC_CNT*MODE_CNT] = {0};
HANDLE g_IEC[1+IEC_CNT*MODE_CNT];

int iBmpDepth;
BOOL bMetaContacts = false, bPopupExists = false;
BOOL bPGPloaded = false, bPGPkeyrings = false, bUseKeyrings = false, bPGPprivkey = false;
BOOL bGPGloaded = false, bGPGkeyrings = false, bSavePass = false;
BOOL bSFT, bSOM, bASI, bMCD, bSCM, bDGP, bAIP, bNOL, bAAK, bMCM;
BYTE bPGP, bGPG;
DWORD iCoreVersion = 0;
CRITICAL_SECTION localQueueMutex;

PLUGININFOEX pluginInfoEx = {
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {1B2A39E5-E2F6-494D-958D-1808FD110DD5}
	{0x1B2A39E5, 0xE2F6, 0x494D, {0x95, 0x8D, 0x18, 0x08, 0xFD, 0x11, 0x0D, 0xD5}}
};

LPSTR myDBGetStringDecode(HANDLE hContact,const char *szModule,const char *szSetting)
{
	char *val = db_get_sa(hContact,szModule,szSetting);
	if (!val) return NULL;
	size_t len = strlen(val)+64;
	char *buf = (LPSTR)mir_alloc(len);
	strncpy(buf,val,len); mir_free(val);
	return buf;
}

int myDBWriteStringEncode(HANDLE hContact,const char *szModule,const char *szSetting,const char *val)
{
	int len = (int)strlen(val)+64;
	char *buf = (LPSTR)alloca(len);
	strncpy(buf,val,len);
	int ret = db_set_s(hContact,szModule,szSetting,buf);
	return ret;
}

void GetFlags()
{
	bSFT = db_get_b(0, MODULENAME, "sft", 0);
	bSOM = db_get_b(0, MODULENAME, "som", 0);
	bASI = db_get_b(0, MODULENAME, "asi", 0);
	bMCD = db_get_b(0, MODULENAME, "mcd", 0);
	bSCM = db_get_b(0, MODULENAME, "scm", 0);
	bDGP = db_get_b(0, MODULENAME, "dgp", 0);
	bAIP = db_get_b(0, MODULENAME, "aip", 0);
	bNOL = db_get_b(0, MODULENAME, "nol", 0);
	bAAK = db_get_b(0, MODULENAME, "aak", 0);
	bMCM = db_get_b(0, MODULENAME, "mcm", 0);
}

void SetFlags()
{
	db_set_b(0, MODULENAME, "sft", bSFT);
	db_set_b(0, MODULENAME, "som", bSOM);
	db_set_b(0, MODULENAME, "asi", bASI);
	db_set_b(0, MODULENAME, "mcd", bMCD);
	db_set_b(0, MODULENAME, "scm", bSCM);
	db_set_b(0, MODULENAME, "dgp", bDGP);
	db_set_b(0, MODULENAME, "aip", bAIP);
	db_set_b(0, MODULENAME, "nol", bNOL);
	db_set_b(0, MODULENAME, "aak", bAAK);
	db_set_b(0, MODULENAME, "mcm", bMCM);
}

int msgbox(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	LPWSTR lpwText = mir_a2u(lpText);
	LPWSTR lpwCaption = mir_a2u(lpCaption);
	int r = MessageBoxW(hWnd,TranslateW(lpwText),TranslateW(lpwCaption),uType);
	mir_free(lpwCaption);
	mir_free(lpwText);
	return r;
}

void CopyToClipboard(HWND hwnd,LPSTR msg)
{
	HGLOBAL hglbCopy;
	LPSTR lpstrCopy;

	hglbCopy = GlobalAlloc(GMEM_MOVEABLE, lstrlenA(msg)+1); 
	lpstrCopy = (LPSTR)GlobalLock(hglbCopy); 
	lstrcpyA(lpstrCopy, msg); 
	GlobalUnlock(hglbCopy); 

	OpenClipboard(NULL);
	EmptyClipboard(); 
	SetClipboardData(CF_TEXT, hglbCopy); 
	CloseClipboard(); 
}

HANDLE hNetlibUser;

void InitNetlib()
{
	NETLIBUSER nl_user = { sizeof(nl_user) };
	nl_user.szSettingsModule = (LPSTR)MODULENAME;
	nl_user.szDescriptiveName = (LPSTR)MODULENAME;
	nl_user.flags = NUF_NOOPTIONS;

	hNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nl_user);
}

void DeinitNetlib()
{
	if (hNetlibUser)
		CallService(MS_NETLIB_CLOSEHANDLE, (WPARAM)hNetlibUser, 0);
}

int Sent_NetLog(const char *fmt,...)
{
	va_list va;
	char szText[1024];

	va_start(va,fmt);
	mir_vsnprintf(szText,sizeof(szText),fmt,va);
	va_end(va);
	return CallService(MS_NETLIB_LOG,(WPARAM)hNetlibUser,(LPARAM)szText);
}

// EOF
