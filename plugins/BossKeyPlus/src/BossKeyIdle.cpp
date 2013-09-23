/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2005 Miranda ICQ/IM project,
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

#include "BossKey.h"

POINT mousepos;
UINT_PTR hTimer; 
UINT mouseidle, minutes;

VOID CALLBACK IdleTimer(HWND hwnd, UINT umsg, UINT idEvent, DWORD dwTime);

static bool IsScreenSaverRunning()
{
	bool rc = FALSE;
	SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &rc, FALSE);
	return rc;
}

static bool IsUserIdle()
{
	DWORD dwTick;
	if ( g_wMaskAdv & OPT_HIDEIFMIRIDLE ) {
		CallService(MS_SYSTEM_GETIDLE, 0, (DWORD)&dwTick);
		return GetTickCount() - dwTick > (minutes * 60 * 1000);
	}
	
	LASTINPUTINFO ii;
	ZeroMemory(&ii,sizeof(ii));
	ii.cbSize=sizeof(ii);
	if (GetLastInputInfo(&ii)) 
		return GetTickCount() - ii.dwTime > (minutes * 60 * 1000);

	return FALSE;
}


VOID CALLBACK IdleTimer(HWND hwnd, UINT umsg, UINT_PTR idEvent, DWORD dwTime)
{
	if ( hTimer == idEvent && !g_bWindowHidden && 
		((g_wMaskAdv & (OPT_HIDEIFWINIDLE | OPT_HIDEIFMIRIDLE) && IsUserIdle()) || 
		(g_wMaskAdv & OPT_HIDEIFSCRSVR && IsScreenSaverRunning()))) 

			BossKeyHideMiranda(0, 0);
}

void InitIdleTimer()
{
	minutes = db_get_b(NULL,MOD_NAME,"time",10);
	hTimer=SetTimer(NULL, 0, 2000, IdleTimer);
}

void UninitIdleTimer()
{
	KillTimer(NULL, hTimer);
}
