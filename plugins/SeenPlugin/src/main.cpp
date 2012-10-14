/*
"Last Seen mod" plugin for Miranda IM
Copyright ( C ) 2002-03  micron-x
Copyright ( C ) 2005-07  Y.B.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $URL: http://svn.berlios.de/svnroot/repos/mgoodies/trunk/lastseen-mod/main.c $
Revision       : $Rev: 1571 $
Last change on : $Date: 2007-12-30 04:55:51 +0300 (Вс, 30 дек 2007) $
Last change by : $Author: y_b $
*/
#include "seen.h"
#include "version.h"

HINSTANCE hInstance;
HANDLE ehdb = NULL, ehproto = NULL, ehmissed = NULL, ehuserinfo = NULL, ehmissed_proto = NULL, hOptInit = NULL, hMainInit = NULL;

int hLangpack;

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	{ 0x2d506d46,0xc94e,0x4ef8,{0x85, 0x37, 0xf1, 0x12, 0x33, 0xa8, 0x03, 0x81}}/* 2d506d46-c94e-4ef8-8537-f11233a80381 */
};

#define TRANSNUMBER 2
DBVTranslation idleTr[TRANSNUMBER]={
	{any_to_IdleNotidleUnknown, _T("Any to Idle/Not Idle/Unknown"),0},
	{any_to_Idle, _T("Any to /Idle or empty"),0}
};

BOOL includeIdle;
logthread_info **contactQueue = NULL;
int contactQueueSize = 0;

int MainInit(WPARAM wparam,LPARAM lparam)
{
	contactQueueSize = 16*sizeof(logthread_info *);
	contactQueue = (logthread_info **)malloc(contactQueueSize);
	memset(&contactQueue[0], 0, contactQueueSize);
	contactQueueSize = 16;
	includeIdle = (BOOL )db_get_b(NULL,S_MOD,"IdleSupport",1);
	hOptInit = HookEvent(ME_OPT_INITIALISE, OptionsInit);
	
	if ( db_get_b(NULL,S_MOD,"MenuItem",1))
		InitMenuitem();
	
	if ( db_get_b(NULL,S_MOD,"UserinfoTab",1))
		ehuserinfo = HookEvent(ME_USERINFO_INITIALISE,UserinfoInit);

	if ( db_get_b(NULL,S_MOD,"FileOutput",0))
		InitFileOutput();

	if ( db_get_b(NULL,S_MOD,"MissedOnes",0))
		ehmissed_proto=HookEvent(ME_PROTO_ACK,ModeChange_mo);

	ehdb = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, UpdateValues);
	ehproto = HookEvent(ME_PROTO_ACK,ModeChange);

	SkinAddNewSoundExT("LastSeenTrackedStatusChange", LPGENT("LastSeen"), LPGENT("User status change"));
	SkinAddNewSoundExT("LastSeenTrackedStatusOnline", LPGENT("LastSeen"), LPGENT("Changed to Online"));
	SkinAddNewSoundExT("LastSeenTrackedStatusOffline", LPGENT("LastSeen"), LPGENT("User Logged Off"));
	SkinAddNewSoundExT("LastSeenTrackedStatusFromOffline", LPGENT("LastSeen"), LPGENT("User Logged In"));
	
	// known modules list
	if (ServiceExists("DBEditorpp/RegisterSingleModule"))
		CallService("DBEditorpp/RegisterSingleModule", (WPARAM)S_MOD, 0);
	db_set_s(NULL,"Uninstall",Translate("Last seen"),S_MOD);


	if ( ServiceExists(MS_TIPPER_ADDTRANSLATION)) {
		int i;
		for (i=0; i < TRANSNUMBER; i++)
			CallService(MS_TIPPER_ADDTRANSLATION, 0, (LPARAM)&idleTr[i]);
	}

	return 0;
}

extern "C" __declspec(dllexport) PLUGININFOEX * MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfo;
}

extern "C" __declspec(dllexport) int Unload(void)
{
	UnhookEvent(ehdb);
	if (ehmissed) UnhookEvent(ehmissed);
	UnhookEvent(ehproto);
	if (ehmissed_proto) UnhookEvent(ehmissed_proto);
	UnhookEvent(hOptInit);
	UnhookEvent(hMainInit);
	if (ehuserinfo) UnhookEvent(ehuserinfo);
	UninitMenuitem();
//	free(contactQueue);
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID lpvReserved)
{
	hInstance=hinst;
	return 1;
}

extern "C" __declspec(dllexport) int Load(void)
{

	mir_getLP(&pluginInfo);
	// this isn't required for most events
	// but the ME_USERINFO_INITIALISE
	// I decided to hook all events after
	// everything is loaded because it seems
	// to be safer in my opinion
	hMainInit = HookEvent(ME_SYSTEM_MODULESLOADED,MainInit);
	return 0;
}