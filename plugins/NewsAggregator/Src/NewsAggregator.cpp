/*
Copyright (C) 2012 Mataes

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.
*/

#include "Common.h"

HINSTANCE hInst = NULL;

int hLangpack;
HANDLE hPrebuildMenuHook = NULL;
HANDLE hProtoService[8];
HWND hAddFeedDlg;
HANDLE hChangeFeedDlgList = NULL;
XML_API xi = {0};
TCHAR tszRoot[MAX_PATH] = {0};
HANDLE hUpdateMutex;
#define NUM_SERVICES 7
HANDLE hService[NUM_SERVICES];

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
	// {56CC3F29-CCBF-4546-A8BA-9856248A412A}
	{0x56cc3f29, 0xccbf, 0x4546, {0xa8, 0xba, 0x98, 0x56, 0x24, 0x8a, 0x41, 0x2a}}
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfoEx;
}

extern "C" __declspec(dllexport) int Load(void)
{

	mir_getLP(&pluginInfoEx);
	mir_getXI(&xi);

	// Add options hook
	HookEvent(ME_OPT_INITIALISE, OptInit);
	HookEvent(ME_SYSTEM_MODULESLOADED, NewsAggrInit);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, NewsAggrPreShutdown);

	hUpdateMutex = CreateMutex(NULL, FALSE, NULL);
	hChangeFeedDlgList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST,0,0);

	// register weather protocol
	PROTOCOLDESCRIPTOR pd = { PROTOCOLDESCRIPTOR_V3_SIZE };
	pd.szName = MODULE;
	pd.type = PROTOTYPE_VIRTUAL;
	CallService(MS_PROTO_REGISTERMODULE,0,(LPARAM)&pd);

	hProtoService[0] = CreateProtoServiceFunction(MODULE, PS_GETNAME, NewsAggrGetName);
	hProtoService[1] = CreateProtoServiceFunction(MODULE, PS_GETCAPS, NewsAggrGetCaps);
	hProtoService[2] = CreateProtoServiceFunction(MODULE, PS_SETSTATUS, NewsAggrSetStatus);
	hProtoService[3] = CreateProtoServiceFunction(MODULE, PS_GETSTATUS, NewsAggrGetStatus);
	hProtoService[4] = CreateProtoServiceFunction(MODULE, PS_LOADICON, NewsAggrLoadIcon);
	hProtoService[5] = CreateProtoServiceFunction(MODULE, PSS_GETINFO, NewsAggrGetInfo);
	hProtoService[6] = CreateProtoServiceFunction(MODULE, PS_GETAVATARINFOT, NewsAggrGetAvatarInfo);
	hProtoService[7] = CreateProtoServiceFunction(MODULE, PSR_MESSAGE, NewsAggrRecvMessage);

	hService[0] = CreateServiceFunction(MS_NEWSAGGREGATOR_CHECKALLFEEDS, CheckAllFeeds);
	hService[1] = CreateServiceFunction(MS_NEWSAGGREGATOR_ADDFEED, AddFeed);
	hService[2] = CreateServiceFunction(MS_NEWSAGGREGATOR_IMPORTFEEDS, ImportFeeds);
	hService[3] = CreateServiceFunction(MS_NEWSAGGREGATOR_EXPORTFEEDS, ExportFeeds);
	hService[4] = CreateServiceFunction(MS_NEWSAGGREGATOR_CHECKFEED, CheckFeed);
	hService[5] = CreateServiceFunction(MS_NEWSAGGREGATOR_CHANGEFEED, ChangeFeed);
	hService[6] = CreateServiceFunction(MS_NEWSAGGREGATOR_ENABLED, EnableDisable);

	return 0;
}

extern "C" __declspec(dllexport) int Unload(void)
{
	for (int i = 0;i<NUM_SERVICES;i++)
		DestroyServiceFunction(hService[i]);

	DestroyUpdateList();
	CloseHandle(hUpdateMutex);

	return 0;
}