/*
Miranda IM Country Flags Plugin
Copyright (C) 2006-1007 H. Herkenrath

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program (Flags-License.txt); if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "flags.h"

/* Services */
static HANDLE hServiceDetectContactOrigin;

/************************* Services *******************************/

static INT_PTR ServiceDetectContactOriginCountry(WPARAM wParam,LPARAM lParam)
{
	int countryNumber = 0xFFFF;
	char *pszProto = GetContactProto((HANDLE)wParam);
	/* ip detect */
	if (bUseIpToCountry)
		countryNumber = ServiceIpToCountry(db_get_dw((HANDLE)wParam,pszProto,"RealIP",0),0);
	/* fallback */
	if (countryNumber == 0xFFFF)
		countryNumber = db_get_w((HANDLE)wParam,pszProto,"Country",0);
	if (countryNumber == 0 || countryNumber == 0xFFFF)
		countryNumber = db_get_w((HANDLE)wParam,pszProto,"CompanyCountry",0);
	return (countryNumber == 0) ? 0xFFFF : countryNumber;
}

/************************* Extra Image ****************************/

#define EXTRAIMAGE_REFRESHDELAY  100  /* time for which setting changes are buffered */

static HANDLE hExtraIcon;

static void CALLBACK SetExtraImage(HANDLE hContact)
{
	if (!bShowExtraIcon)
		return;
		
	int countryNumber = ServiceDetectContactOriginCountry((WPARAM)hContact, 0);
	if (countryNumber == 0xFFFF && !bUseUnknown)
		ExtraIcon_Clear(hExtraIcon, hContact);
	else {
		char szId[20];
		wsprintfA(szId, (countryNumber == 0xFFFF) ? "%s0x%X" : "%s%i", "flags_", countryNumber);
		ExtraIcon_SetIcon(hExtraIcon, hContact, szId);
	}
}

// always call in context of main thread
static void RemoveExtraImages(void)
{
	for (HANDLE hContact = db_find_first(); hContact; hContact = db_find_next(hContact))
		ExtraIcon_Clear(hExtraIcon, hContact);
}

// always call in context of main thread
static void EnsureExtraImages(void)
{
	for (HANDLE hContact = db_find_first(); hContact; hContact = db_find_next(hContact))
		SetExtraImage(hContact);
}

static void CALLBACK UpdateExtraImages(LPARAM lParam)
{
	if (bShowExtraIcon)
		EnsureExtraImages();
	else
		RemoveExtraImages();
}

static int ExtraImageApply(WPARAM wParam,LPARAM lParam)
{
	if (bShowExtraIcon)
		SetExtraImage((HANDLE)wParam); /* unbuffered */
	return 0;
}

/************************* Status Icon ****************************/

#define STATUSICON_REFRESHDELAY  100  /* time for which setting changes are buffered */

// always call in context of main thread
static void __fastcall SetStatusIcon(HANDLE hContact,int countryNumber)
{
	StatusIconData sid = { sizeof(sid) };
	sid.szModule = MODULENAME;

	if (countryNumber != 0xFFFF || bUseUnknown) {
		/* copy icon as status icon API will call DestroyIcon() on it */
		sid.hIcon = LoadFlagIcon(countryNumber);
		sid.szTooltip = (char*) CallService(MS_UTILS_GETCOUNTRYBYNUMBER,countryNumber,0);
	}	
	else sid.flags = MBF_HIDDEN;

	Srmm_ModifyIcon(hContact, &sid);

	if (sid.hIcon)
		Skin_ReleaseIcon(sid.hIcon);
}

// always call in context of main thread
static void __fastcall UnsetStatusIcon(HANDLE hContact)
{
	StatusIconData sid = { sizeof(sid) };
	sid.szModule = MODULENAME;
	sid.flags = MBF_HIDDEN;
	Srmm_ModifyIcon(hContact, &sid);
}

static int MsgWndEvent(WPARAM wParam,LPARAM lParam)
{
	MessageWindowEventData *msgwe=(MessageWindowEventData*)lParam;
	switch(msgwe->uType) {
	case MSG_WINDOW_EVT_OPENING:
	case MSG_WINDOW_EVT_CLOSE:
		if (bShowStatusIcon) {
			int countryNumber = ServiceDetectContactOriginCountry((WPARAM)msgwe->hContact, 0);
			if (msgwe->uType == MSG_WINDOW_EVT_OPENING && countryNumber != 0xFFFF)
				SetStatusIcon(msgwe->hContact,countryNumber);
			else
				UnsetStatusIcon(msgwe->hContact);
		}
		/* ensure it is hidden, RemoveStatusIcons() only enums currently opened ones  */
		else UnsetStatusIcon(msgwe->hContact);
	}
	return 0;
}

static void CALLBACK UpdateStatusIcons(LPARAM lParam)
{
	MessageWindowInputData msgwi = { sizeof(msgwi) };
	msgwi.uFlags = MSG_WINDOW_UFLAG_MSG_BOTH;
	msgwi.hContact = db_find_first();
	while (msgwi.hContact != NULL) {
		/* is a message window opened for this contact? */
		MessageWindowData msgw; /* output */
		msgw.cbSize = sizeof(msgw);
		if (!CallService(MS_MSG_GETWINDOWDATA,(WPARAM)&msgwi,(LPARAM)&msgw) && msgw.uState & MSG_WINDOW_STATE_EXISTS) {
			if (bShowStatusIcon) {
				int countryNumber = ServiceDetectContactOriginCountry((WPARAM)msgwi.hContact, 0);
				SetStatusIcon(msgwi.hContact, countryNumber);
			}
			else UnsetStatusIcon(msgwi.hContact);
		}
		msgwi.hContact = db_find_next(msgw.hContact);
	}
}

static int StatusIconsChanged(WPARAM wParam,LPARAM lParam)
{
	if (bShowStatusIcon)
		CallFunctionBuffered(UpdateStatusIcons, 0, FALSE, STATUSICON_REFRESHDELAY);
	return 0;
}

static int ExtraImgSettingChanged(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTWRITESETTING *dbcws=(DBCONTACTWRITESETTING*)lParam;
	if ((HANDLE)wParam == NULL) {
		if (!lstrcmpA(dbcws->szModule,MODULENAME)) {
			/* Extra Image */
			if (!lstrcmpA(dbcws->szSetting,"ShowExtraImgFlag") ||
			    !lstrcmpA(dbcws->szSetting,"ExtraImgFlagColumn") ||
			    !lstrcmpA(dbcws->szSetting,"UseUnknownFlag") ||
			    !lstrcmpA(dbcws->szSetting,"UseIpToCountry"))
				CallFunctionBuffered(UpdateExtraImages,0,FALSE,EXTRAIMAGE_REFRESHDELAY);
			/* Status Icon */
			if (!lstrcmpA(dbcws->szSetting,"ShowStatusIconFlag") ||
			   !lstrcmpA(dbcws->szSetting,"UseUnknownFlag") ||
			   !lstrcmpA(dbcws->szSetting,"UseIpToCountry"))
			   if (ServiceExists(MS_MSG_REMOVEICON))
				   CallFunctionBuffered(UpdateStatusIcons,0,FALSE,STATUSICON_REFRESHDELAY);
		}
	}
	/* user details update */
	else if (!lstrcmpA(dbcws->szSetting,"RealIP") ||
	        !lstrcmpA(dbcws->szSetting,"Country") ||
	        !lstrcmpA(dbcws->szSetting,"CompanyCountry")) {
		/* Extra Image */
	   SetExtraImage((HANDLE)wParam);
		/* Status Icon */
		if (ServiceExists(MS_MSG_REMOVEICON))
		   CallFunctionBuffered(UpdateStatusIcons,0,FALSE,STATUSICON_REFRESHDELAY);
	}
	return 0;
}

/************************* Misc ***********************************/

static int ExtraImgModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	/* Options */
	if (ServiceExists("DBEditorpp/RegisterSingleModule"))
		CallService("DBEditorpp/RegisterSingleModule",(WPARAM)MODULENAME,0);

	/* Extra Image */
	hExtraIcon = ExtraIcon_Register("flags_extra", LPGEN("Country flag"));
	HookEvent(ME_CLIST_EXTRA_IMAGE_APPLY,ExtraImageApply);

	/* Status Icon */
	StatusIconData sid = { sizeof(sid) };
	sid.szModule = MODULENAME; // dwID = 0
	sid.flags = MBF_HIDDEN;
	Srmm_AddIcon(&sid);

	HookEvent(ME_MSG_WINDOWEVENT, MsgWndEvent);
	return 0;
}

void InitExtraImg(void)
{
	CreateServiceFunction(MS_FLAGS_DETECTCONTACTORIGINCOUNTRY, ServiceDetectContactOriginCountry);

	HookEvent(ME_SYSTEM_MODULESLOADED, ExtraImgModulesLoaded);
	HookEvent(ME_SKIN2_ICONSCHANGED, StatusIconsChanged);
	HookEvent(ME_OPT_INITIALISE, OnOptionsInit);
	HookEvent(ME_DB_CONTACT_SETTINGCHANGED, ExtraImgSettingChanged);
}
