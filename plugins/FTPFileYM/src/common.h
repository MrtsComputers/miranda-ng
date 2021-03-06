/*
FTP File YM plugin
Copyright (C) 2007-2010 Jan Holub

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

#pragma once

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER					// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501			// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT			// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS			// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410	// Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE				// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600		// Change this to the appropriate value to target other versions of IE.
#endif

#define WIN32_LEAN_AND_MEAN	
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <windowsx.h>
#include <shlobj.h>
#include <vector>
using namespace std;

#include <newpluginapi.h>
#include <m_button.h>
#include <m_clist.h>
#include <m_database.h>
#include <m_hotkeys.h>
#include <m_icolib.h>
#include <m_message.h>
#include <m_langpack.h>
#include <m_options.h>
#include <m_protosvc.h>
#include <m_skin.h>
#include <win2k.h>

#include <m_ftpfile.h>
#include <m_msg_buttonsbar.h>

#include "curl.h"

#include "mir_db.h"
#include "utils.h"
#include "dialog.h"
#include "options.h"
#include "serverlist.h"
#include "deletetimer.h"
#include "dbentry.h"
#include "manager.h"
#include "job_delete.h"
#include "job_packer.h"
#include "job_upload.h"
#include "version.h"
#include "job_generic.h"
#include "resource.h"

#ifndef FREE
#define FREE(X)	if (X) { mir_free(X); X = NULL; }
#endif

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#define new DEBUG_CLIENTBLOCK
#define DEBUG_CLIENTBLOCK   new( _CLIENT_BLOCK, __FILE__, __LINE__)
#else
#define DEBUG_CLIENTBLOCK
#endif

#define MS_FTPFILE_CONTACTMENU	"FTPFile/ContactMenu"
#define MS_FTPFILE_MAINMENU		"FTPFile/MainMenu"

#define MODULE				"FTPFile"
#define MODULE_FILES		"FTPFile_Files"

#define SOUND_UPCOMPLETE	"ftpfile_upcomplete"
#define SOUND_CANCEL		"ftpfile_cancel"

extern HINSTANCE hInst;