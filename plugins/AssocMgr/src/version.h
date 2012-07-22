/*

'File Association Manager'-Plugin for Miranda IM

Copyright (C) 2005-2007 H. Herkenrath

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program (AssocMgr-License.txt); if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#define __MAJOR_VERSION				0
#define __MINOR_VERSION				1
#define __RELEASE_NUM				1
#define __BUILD_NUM					0

#define __FILEVERSION_STRING		__MAJOR_VERSION,__MINOR_VERSION,__RELEASE_NUM,__BUILD_NUM
#define __FILEVERSION_DOTS			__MAJOR_VERSION.__MINOR_VERSION.__RELEASE_NUM.__BUILD_NUM

#define __STRINGIFY_IMPL(x)			#x
#define __STRINGIFY(x)				__STRINGIFY_IMPL(x)
#define __VERSION_STRING			__STRINGIFY(__FILEVERSION_DOTS)

#define __PLUGIN_NAME				"File Association Manager"
#define __INTERNAL_NAME				"AssocMgr"
#define __FILENAME					"AssocMgr.dll"
#define __DESCRIPTION 				"Handles file type associations and URLs like aim, ymsgr, xmpp, wpmsg, gg, tlen."
#define __AUTHOR					"H. Herkenrath"
#define __AUTHOREMAIL				"hrathh@users.sourceforge.net"
#define __AUTHORWEB					"http://nightly.miranda.im/"
#define __COPYRIGHT					"� 2005-2007 H. Herkenrath"
