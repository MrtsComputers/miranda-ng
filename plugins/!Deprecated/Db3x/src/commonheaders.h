/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (c) 2012-14 Miranda NG project (http://miranda-ng.org)
Copyright (�) 2000-03 Miranda ICQ/IM project,
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

#define _CRT_SECURE_NO_WARNINGS

#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <process.h>
#include <memory>

#include <newpluginapi.h>
#include <win2k.h>
#include <m_system_cpp.h>
#include <m_database.h>
#include <m_langpack.h>

#include "version.h"
#include "dbintf3x.h"
#include "resource.h"

#ifdef __GNUC__
#define mir_i64(x) (x##LL)
#else
#define mir_i64(x) (x##i64)
#endif

#ifndef MODULAR
void Encrypt(char*msg,BOOL up);
#endif

extern LIST<CDb3x> g_Dbs;
extern DBSignature dbSignature;