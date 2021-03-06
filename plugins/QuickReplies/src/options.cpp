/* 
Copyright (C) 2010 Unsane

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

#include "common.h"

static LRESULT CALLBACK MessageEditSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)  {
	case WM_CHAR:
		if (wParam == 1 && GetKeyState(VK_CONTROL) & 0x8000) 
		{	// ctrl-a
			SendMessage(hwnd, EM_SETSEL, 0, -1);
			return 0;
		}

		if (wParam == 26 && GetKeyState(VK_CONTROL) & 0x8000) 
		{	// ctrl-z
			SendMessage(hwnd, EM_UNDO, 0, 0);
			return 0;
		}

		if (wParam == 127 && GetKeyState(VK_CONTROL) & 0x8000) 
		{	// ctrl-backspace
			DWORD start, end;
			WCHAR text[1024];

			SendMessage(hwnd, EM_GETSEL, (WPARAM) & end, (LPARAM) (PDWORD) NULL);
			SendMessage(hwnd, WM_KEYDOWN, VK_LEFT, 0);
			SendMessage(hwnd, EM_GETSEL, (WPARAM) & start, (LPARAM) (PDWORD) NULL);
			GetWindowText(hwnd, text, 1024);
			MoveMemory(text + start, text + end, sizeof(WCHAR) * (wcslen(text) + 1 - end));
			SetWindowText(hwnd, text);
			//SAFE_FREE((void**)&text);
			SendMessage(hwnd, EM_SETSEL, start, start);
			SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), EN_CHANGE), (LPARAM) hwnd);
			return 0;
		}
		break;
	}

	return mir_callNextSubclass(hwnd, MessageEditSubclassProc, msg, wParam, lParam);
}

INT_PTR CALLBACK DlgProcOptionsPage(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
	case WM_INITDIALOG:
		{
			char key[64];
			int count = 0;
			DBVARIANT dbv = {0};
			tString replies = _T("");

			TranslateDialogDefault(hwndDlg);
			variables_skin_helpbutton(hwndDlg, IDC_VARIABLES);

			SendDlgItemMessage(hwndDlg, IDC_REPLIES, EM_LIMITTEXT, TEXT_LIMIT, 0);
			mir_subclassWindow( GetDlgItem(hwndDlg, IDC_REPLIES), MessageEditSubclassProc);

			mir_snprintf(key, 64, "ImmediatelySend_%x", iNumber);
			CheckDlgButton(hwndDlg, IDC_IMMEDIATELY, (BYTE)db_get_w(NULL, MODULE_NAME, key, 1));

			mir_snprintf(key, 64, "RepliesCount_%x", iNumber);
			count = db_get_w(NULL, MODULE_NAME, key, 0);

			for (int i = 0; i < count; i++)
			{
				mir_snprintf(key, 64, "Reply_%x_%x", iNumber, i);
				if (!db_get_ts(NULL, MODULE_NAME, key, &dbv))
					if(dbv.ptszVal != NULL)
						replies.append(dbv.ptszVal);
				if (i < count - 1)
					replies.append(_T("\r\n"));
			}
			SetDlgItemText(hwndDlg, IDC_REPLIES, replies.c_str());

			db_free(&dbv);
		}
		return TRUE;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) {
			switch(LOWORD(wParam)) {
			case IDC_VARIABLES:
				variables_showhelp(hwndDlg, IDC_REPLIES, VHF_SIMPLEDLG, NULL, NULL);
				break;

			case IDC_IMMEDIATELY:
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			}
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR *p = ((LPNMHDR)lParam);

			switch (p->code) {
			case PSN_APPLY:
				{
					char key[64];
					int count = 0;
					TCHAR tszReplies[TEXT_LIMIT];

					mir_snprintf(key, 64, "RepliesCount_%x", iNumber);
					count = db_get_b(NULL, MODULE_NAME, key, 0);

					for (int i = 0; i < count; i++)
					{
						mir_snprintf(key, 64, "Reply_%x_%x", iNumber, i);
						db_unset(NULL, MODULE_NAME, key);
					}

					GetDlgItemText(hwndDlg, IDC_REPLIES, tszReplies, TEXT_LIMIT);
					{
						tString replies = tszReplies;

						if (replies.length() > 0)
							replies.append(_T("\r\n"));

						count = 0;
						tString::size_type pos = tString::npos;
						while ((pos = replies.find(_T("\r\n"))) != tString::npos)
						{
							mir_snprintf(key, 64, "Reply_%x_%x", iNumber, count++);
							db_set_ts(NULL, MODULE_NAME, key, replies.substr(0, pos).c_str());
							replies = replies.substr(pos + 2);
						}
					}

					mir_snprintf(key, 64, "RepliesCount_%x", iNumber);
					db_set_w(NULL, MODULE_NAME, key, count);

					mir_snprintf(key, 64, "ImmediatelySend_%x", iNumber);
					db_set_b(NULL, MODULE_NAME, key, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_IMMEDIATELY));

					mir_free(key);

					return TRUE;
				}
				break;
			}
		}
		break;
	}

	if (HIWORD(wParam) == EN_CHANGE && GetFocus() == (HWND)lParam)
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);

	return FALSE;
}

int OnOptInitialized(WPARAM wParam, LPARAM lParam)
{
	char tabName[32];
	mir_snprintf(tabName, SIZEOF(tabName), "Button %x", iNumber + 1);

	OPTIONSDIALOGPAGE odp = { sizeof(odp) };
	odp.pszGroup = LPGEN("Message Sessions");
	odp.pszTitle = LPGEN("Quick Replies");
	odp.pszTab = tabName;
	odp.position = iNumber;
	odp.hInstance = hInstance;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS_PAGE);
	odp.pfnDlgProc = DlgProcOptionsPage;
	Options_AddPage(wParam, &odp);
	return 0;
}