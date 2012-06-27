//#include "AggressiveOptimize.h"

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <stdio.h>

#include "../../include/newpluginapi.h"
#include "../../include/m_system.h"
#include "../../include/m_database.h"
#include "../../include/m_protomod.h"
#include "../../include/m_protosvc.h"
#include "../../include/m_langpack.h"
#include "../../include/m_clist.h"
#include "../../include/m_options.h"
#include "../../include/m_clui.h"
#include "../../include/m_clc.h"
#include "../../include/m_utils.h"
#include "../../include/m_skin.h"
#include "../../include/m_popup.h"
#include "../../include/m_message.h"
#include "../../include/m_button.h"
#include "../../include/m_netlib.h"
#include "../../include/m_file.h"
#include "../../include/win2k.h"

#define ARRAY_SIZE(n)	(sizeof(n)/sizeof(n[0]))

#define MAXBUFSIZE 4096
#define SERVICE_TITLE "file As Message"
#define SERVICE_NAME "FileAsMessage"

#define SERVICE_PREFIX "<%fAM-0023%>"

#define PLUGIN_URL "http://miranda-im.org/download/details.php?action=viewfile&id=1811"
#define NOPLUGIN_MESSAGE "\nIf you see this \"garbage\", probably you "\
                         "have no \"fileAsMessage\" plugin installed, see "\
						 PLUGIN_URL " for more information and download."
extern char *szServiceTitle;
extern char *szServicePrefix;

#define WM_FE_MESSAGE		WM_USER+100
#define WM_FE_STATUSCHANGE	WM_USER+101
#define WM_FE_SKINCHANGE	WM_USER+102

extern HINSTANCE hInst;
extern HANDLE hFileList;
extern HANDLE hEventNewFile;

extern HICON hIcons[5];

#ifdef __cplusplus
extern "C" {
#endif 
int __declspec(dllexport) Load( PLUGINLINK *link );
int __declspec(dllexport) Unload( void );
    __declspec(dllexport) PLUGININFO *MirandaPluginInfo( DWORD dwVersion );
#ifdef __cplusplus
}
#endif 
