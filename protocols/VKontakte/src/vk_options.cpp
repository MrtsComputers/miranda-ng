/*
Copyright (C) 2013 Miranda NG Project (http://miranda-ng.org)

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

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////////
// Account manager dialog

INT_PTR CALLBACK VKAccountProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CVkProto* ppro = (CVkProto*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (uMsg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		ppro = (CVkProto*)lParam;
		SetWindowLongPtr( hwndDlg, GWLP_USERDATA, lParam );

		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)Skin_GetIconByHandle(ppro->m_hProtoIcon, true));
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)Skin_GetIconByHandle(ppro->m_hProtoIcon));

		SendDlgItemMessage(hwndDlg, IDC_PASSWORD, EM_LIMITTEXT, 30, 0);
		{
			ptrT tszLogin( ppro->getTStringA("Login"));
			if (tszLogin != NULL)
				SetDlgItemText(hwndDlg, IDC_LOGIN, tszLogin);

			ptrT tszPassw( ppro->GetUserStoredPassword());
			if (tszPassw != NULL)
				SetDlgItemText(hwndDlg, IDC_PASSWORD, tszPassw);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_URL:
			CallService(MS_UTILS_OPENURL, 1, (LPARAM)"http://www.vk.com");
			break;

		case IDC_LOGIN:
		case IDC_PASSWORD:
			if (HIWORD(wParam) == EN_CHANGE && (HWND)lParam == GetFocus()) {
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			}
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			TCHAR str[128];
			GetDlgItemText(hwndDlg, IDC_LOGIN, str, SIZEOF(str));
			ppro->setTString("Login", str);
			
			GetDlgItemText(hwndDlg, IDC_PASSWORD, str, SIZEOF(str));
			ptrA szRawPasswd( mir_utf8encodeT(str));
			if (szRawPasswd != NULL) {
				CallService(MS_DB_CRYPT_ENCODESTRING, strlen(szRawPasswd), szRawPasswd);
				ppro->setString("Password", szRawPasswd);
			}
		}
		break;

	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		break;

	case WM_DESTROY:
		Skin_ReleaseIcon((HICON)SendMessage(hwndDlg, WM_GETICON, ICON_BIG, 0));
		Skin_ReleaseIcon((HICON)SendMessage(hwndDlg, WM_GETICON, ICON_SMALL, 0));
		break;
	}

	return FALSE;
}

INT_PTR CVkProto::SvcCreateAccMgrUI(WPARAM wParam, LPARAM lParam)
{
	return (INT_PTR)CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_ACCMGRUI), (HWND)lParam, VKAccountProc, (LPARAM)this);
}

//////////////////////////////////////////////////////////////////////////////
// Options

int CVkProto::OnOptionsInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {sizeof(odp)};
	odp.hInstance   = hInst;
	odp.ptszTitle   = m_tszUserName;
	odp.dwInitParam = LPARAM(this);
	odp.flags       = ODPF_BOLDGROUPS | ODPF_TCHAR | ODPF_DONTTRANSLATE;

	odp.position    = 1;
	odp.ptszGroup   = LPGENT("Network");
	odp.ptszTab     = LPGENT("Account");
	odp.pszTemplate = 0; // MAKEINTRESOURCEA(IDD_OPTIONS);
	odp.pfnDlgProc  = 0; // OptionsProc;
	Options_AddPage(wParam, &odp);
	return 0;
}