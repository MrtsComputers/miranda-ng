{$include compilers.inc}
{$IFDEF COMPILER_16_UP}
  {$WEAKLINKRTTI ON}
  {.$RTTI EXPLICIT METHODS([]) PROPERTIES([]) FIELDS([])}
{$ENDIF}
{$IMAGEBASE $13100000}
library quicksearch;

{$R qs.res}

uses
//  FastMM4,
  Windows,
  Messages,
  m_api,
  sr_optdialog,
  sr_global,
  sr_window,
  sr_frame,
  mirutils,
  common;

var
  opthook:cardinal;
  onloadhook:cardinal;
  onstatus,
  ondelete,
//  onaccount,
  onadd:cardinal;
  servshow:cardinal;

const
  icohook:THANDLE = 0;

function MirandaPluginInfoEx(mirandaVersion:DWORD):PPLUGININFOEX; cdecl;
begin
  result:=@PluginInfo;
  PluginInfo.cbSize     :=SizeOf(TPLUGININFOEX);
  PluginInfo.shortName  :='Quick Search Mod';
  PluginInfo.version    :=$01040112;
  PluginInfo.description:=
    'This Plugin allow you to quick search for nickname,'+
    'firstname, lastname, email, uin in your contact list.'+
    'And now you may add any setting to display - for example'+
    'users version of miranda,group or city.';
  PluginInfo.author     :='Awkward, based on Bethoven sources';
  PluginInfo.authorEmail:='panda75@bk.ru; awk1975@ya.ru';
  PluginInfo.copyright  :='(c) 2004,2005 Bethoven; 2006-2012 Awkward';
  PluginInfo.homepage   :='http://code.google.com/p/delphi-miranda-plugins/';
  PluginInfo.flags      :=UNICODE_AWARE;
  PluginInfo.uuid       :=MIID_QUICKSEARCH;
end;

function OnTTBLoaded(wParam:WPARAM;lParam:LPARAM):int;cdecl;
begin
  addtotoolbar;
  result:=0;
end;

function IconChanged(wParam:WPARAM;lParam:LPARAM):int;cdecl;
var
  mi:TCListMenuItem;
  ttb:TTBButton;
begin
  result:=0;
  FillChar(mi,SizeOf(mi),0);
  mi.cbSize:=sizeof(mi);
  mi.flags :=CMIM_ICON;

  mi.hIcon:=CallService(MS_SKIN2_GETICON,0,tlparam(QS_QS));
  CallService(MS_CLIST_MODIFYMENUITEM,MainMenuItem,tlparam(@mi));

// toptoolbar
  if ServiceExists(MS_TTB_GETBUTTONOPTIONS)<>0 then
  begin
    CallService(MS_TTB_GETBUTTONOPTIONS,(hTTBButton shl 16)+TTBO_ALLDATA,TLPARAM(@ttb));
    ttb.hIconUp:=CallService(MS_SKIN2_GETICON,0,TLPARAM(QS_QS));
    ttb.hIconDn:=ttb.hIconUp;
    CallService(MS_TTB_SETBUTTONOPTIONS,(hTTBButton shl 16)+TTBO_ALLDATA,TLPARAM(@ttb));
  end;

end;

procedure RegisterIcons;
var
  sid:TSKINICONDESC;
begin
  FillChar(sid,SizeOf(TSKINICONDESC),0);
  sid.cbSize     :=SizeOf(TSKINICONDESC);
  sid.cx         :=16;
  sid.cy         :=16;
  sid.szSection.a:=qs_module;

  sid.hDefaultIcon   :=LoadImage(hInstance,MAKEINTRESOURCE(IDI_QS),IMAGE_ICON,16,16,0);
  sid.pszName        :=QS_QS;
  sid.szDescription.a:=qs_name;
  Skin_AddIcon(@sid);
  DestroyIcon(sid.hDefaultIcon);

  sid.hDefaultIcon   :=LoadImage(hInstance,MAKEINTRESOURCE(IDI_NEW),IMAGE_ICON,16,16,0);
  sid.pszName        :=QS_NEW;
  sid.szDescription.a:='New Column';
  Skin_AddIcon(@sid);
  DestroyIcon(sid.hDefaultIcon);

  sid.hDefaultIcon   :=LoadImage(hInstance,MAKEINTRESOURCE(IDI_ITEM),IMAGE_ICON,16,16,0);
  sid.pszName        :=QS_ITEM;
  sid.szDescription.a:='Save Column';
  Skin_AddIcon(@sid);
  DestroyIcon(sid.hDefaultIcon);

  sid.hDefaultIcon   :=LoadImage(hInstance,MAKEINTRESOURCE(IDI_UP),IMAGE_ICON,16,16,0);
  sid.pszName        :=QS_UP;
  sid.szDescription.a:='Column Up';
  Skin_AddIcon(@sid);
  DestroyIcon(sid.hDefaultIcon);

  sid.hDefaultIcon   :=LoadImage(hInstance,MAKEINTRESOURCE(IDI_DOWN),IMAGE_ICON,16,16,0);
  sid.pszName        :=QS_DOWN;
  sid.szDescription.a:='Column Down';
  Skin_AddIcon(@sid);
  DestroyIcon(sid.hDefaultIcon);

  sid.hDefaultIcon   :=LoadImage(hInstance,MAKEINTRESOURCE(IDI_DELETE),IMAGE_ICON,16,16,0);
  sid.pszName        :=QS_DELETE;
  sid.szDescription.a:='Delete Column';
  Skin_AddIcon(@sid);
  DestroyIcon(sid.hDefaultIcon);

  sid.hDefaultIcon   :=LoadImage(hInstance,MAKEINTRESOURCE(IDI_DEFAULT),IMAGE_ICON,16,16,0);
  sid.pszName        :=QS_DEFAULT;
  sid.szDescription.a:='Default';
  Skin_AddIcon(@sid);
  DestroyIcon(sid.hDefaultIcon);

  sid.hDefaultIcon   :=LoadImage(hInstance,MAKEINTRESOURCE(IDI_RELOAD),IMAGE_ICON,16,16,0);
  sid.pszName        :=QS_RELOAD;
  sid.szDescription.a:='Reload';
  Skin_AddIcon(@sid);
  DestroyIcon(sid.hDefaultIcon);

  sid.hDefaultIcon   :=LoadImage(hInstance,MAKEINTRESOURCE(IDI_MALE),IMAGE_ICON,16,16,0);
  sid.pszName        :=QS_MALE;
  sid.szDescription.a:='Male';
  Skin_AddIcon(@sid);
  DestroyIcon(sid.hDefaultIcon);

  sid.hDefaultIcon   :=LoadImage(hInstance,MAKEINTRESOURCE(IDI_FEMALE),IMAGE_ICON,16,16,0);
  sid.pszName        :=QS_FEMALE;
  sid.szDescription.a:='Female';
  Skin_AddIcon(@sid);
  DestroyIcon(sid.hDefaultIcon);

  icohook:=HookEvent(ME_SKIN2_ICONSCHANGED,@IconChanged);
end;

function OnOptInitialise(wParam:WPARAM;lParam:LPARAM):int;cdecl;
var
  odp:TOPTIONSDIALOGPAGE;
begin
  ZeroMemory(@odp,sizeof(odp));
  odp.cbSize     :=SizeOf(odp);  //for 0.6+ compatibility
  odp.Position   :=900003000;
  odp.hInstance  :=hInstance;
  odp.pszTemplate:=PAnsiChar(IDD_DIALOG1);
  odp.szTitle.a  :=qs_name;
  odp.szGroup.a  :='Contact List';
  odp.pfnDlgProc :=@sr_optdialog.DlgProcOptions;
  odp.flags      :=ODPF_BOLDGROUPS;
  Options_AddPage(wParam,@odp);
//  CallService(MS_OPT_ADDPAGE,wParam,tlparam(@odp));
  Result:=0;
end;

function OpenSearchWindow(wParam:WPARAM;lParam:LPARAM):int_ptr;cdecl;
begin
  result:=0;
  if not opened then
    OpenSrWindow(pointer(wParam),lParam)
  else
    BringToFront;
end;

function OnModulesLoaded(wParam:WPARAM;lParam:LPARAM):int;cdecl;
begin
  UnhookEvent(onloadhook);

  CallService(MS_DBEDIT_REGISTERSINGLEMODULE,twparam(qs_module),0);

  RegisterIcons;
  RegisterColors;

  servshow:=CreateServiceFunction(QS_SHOWSERVICE,@OpenSearchWindow);
  AddRemoveMenuItemToMainMenu;

  reghotkeys;

  onadd    :=HookEvent(ME_DB_CONTACT_ADDED        ,@OnContactAdded);
  ondelete :=HookEvent(ME_DB_CONTACT_DELETED      ,@OnContactDeleted);
  onstatus :=HookEvent(ME_CLIST_CONTACTICONCHANGED,@OnStatusChanged);
//  onaccount:=HookEvent(ME_PROTO_ACCLISTCHANGED    ,@OnAccountChanged);
  HookEvent(ME_TTB_MODULELOADED,@OnTTBLoaded);

  CreateFrame(0);
  Result:=0;
end;

function Load():Integer;cdecl;
begin
  Result:=0;
  Langpack_register;
  opthook   :=HookEvent(ME_OPT_INITIALISE      ,@OnOptInitialise);
  onloadhook:=HookEvent(ME_SYSTEM_MODULESLOADED,@OnModulesLoaded);
  loadopt_db(true);
end;

function Unload:Integer;cdecl;
begin
  result:=0;
  removetoolbar; //??
  DestroyFrame;

  DestroyServiceFunction(servshow);
  UnhookEvent(opthook);
  UnhookEvent(onadd);
  UnhookEvent(ondelete);
  UnhookEvent(onstatus);
//  UnhookEvent(onaccount);
  if icohook<>0 then
    UnhookEvent(icohook);

//  unreghotkeys;

  CloseSrWindow;

  clear_columns;
end;

exports
  Load, Unload,
  MirandaPluginInfoEx;

begin
end.
