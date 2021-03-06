{$IFNDEF M_CHANGEKEYBOARDLAYOUT}
{$DEFINE M_CHANGEKEYBOARDLAYOUT}

const
// change keyboard layout of text
// wParam - HWND or NULL for current window
// lParam must be 0
// returns 0 on success and returns non-zero (-1) on error.
  MS_CKL_CHANGELAYOUT 'ChangeKeyboardLayout/ChangeLayout';

//wParam ������ ���� ����.
//lParam - LPCTSTR ������, ��������� �������� ��������� ����������,
//���������� HKL ��������� ������, ��� NULL � ������ ������.
//����������: ��� ����������� ��������� ����������� ����� "��������� ������ - ������� ���������"
  MS_CKL_GETLAYOUTOFTEXT = 'ChangeKeyboardLayout/GetLayoutOfText';

type
  CKLLayouts = record
    hklFrom:HKL;   // layout of the current text
    hklTo :HKL;    // layout of the result text
    bTwoWay:bool;
  end;

const
//wParam - LPCTSTR ��������� ������
//lParam - ��������� �� ��������� CKLLayouts, ���������� ��������� ��� 
//��������� ������ � ����� "���������������� ��������������"
//���������� LPTSTR �� �������������� ������
  MS_CKL_CHANGETEXTLAYOUT = 'ChangeKeyboardLayout/ChangeTextLayout';

{$ENDIF}
