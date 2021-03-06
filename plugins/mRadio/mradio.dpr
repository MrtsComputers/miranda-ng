{.$DEFINE CHANGE_NAME_BUFFERED}
{$include compilers.inc}
{$IFDEF COMPILER_16_UP}
  {$WEAKLINKRTTI ON}
  {.$RTTI EXPLICIT METHODS([]) PROPERTIES([]) FIELDS([])}
{$ENDIF}
{$IMAGEBASE $13300000}
library mradio;

uses
//  FastMM4,
  Windows,messages,commctrl
  ,common,io,wrapper,wrapdlgs,syswin
  ,Dynamic_Bass,dynbasswma
  ,m_api,dbsettings,mirutils,playlist,memini;

{$include mr_rc.inc}
{$r mradio.res}

{$include i_vars.inc}

const
  cPluginName = 'mRadio';
const
  PluginName:PAnsiChar = cPluginName;

function MakeMessage:pWideChar;
var
  p,artist,title:pWideChar;
  len:cardinal;
begin
  artist:=DBReadUnicode(0,PluginName,optArtist);
  title :=DBReadUnicode(0,PluginName,optTitle);
  len:=StrLenW(artist);
  if (artist<>nil) and (title<>nil) then
    inc(len,3);
  inc(len,StrLenW(title));

  if len>0 then
  begin
    mGetMem(result,(len+1)*SizeOf(WideChar));
    p:=result;
    if artist<>nil then
    begin
      p:=StrCopyEW(p,artist);
      if title<>nil then
        p:=StrCopyEW(p,' - ');
      mFreeMem(artist);
    end;
    if title<>nil then
    begin
      StrCopyW(p,title);
      mFreeMem(title);
    end;
  end
  else
    result:=nil;
end;

procedure SetStatus(hContact:THANDLE;status:integer);
begin
//  if Status=ID_STATUS_OFFLINE then
//    MyStopBass;

  if status=ID_STATUS_OFFLINE then
  begin
    if (AsOffline=BST_UNCHECKED) or (PluginStatus<>ID_STATUS_OFFLINE) then
      status:=ID_STATUS_INVISIBLE;
  end;

  if hContact=0 then
  begin
    hContact:=db_find_first(PluginName);
    while hContact<>0 do
    begin
      DBWriteWord(hContact,PluginName,optStatus,status);
      hContact:=db_find_next(hContact,PluginName);
    end;
  end
  else
    DBWriteWord(hContact,PluginName,optStatus,status);
end;

function GetDefaultRecPath:pWideChar;
var
  dat:TREPLACEVARSDATA;
  mstr,szData:pWideChar;
  buf:array [0..MAX_PATH-1] of WideChar;
begin
  FillChar(dat,SizeOf(dat),0);
  dat.cbSize :=SizeOf(TREPLACEVARSDATA);
  dat.dwFlags:=RVF_UNICODE;
  szData:='%miranda_userdata%'+'\'+cPluginName;
  mstr:=pWideChar(CallService(MS_UTILS_REPLACEVARS, WPARAM(szData), LPARAM(@dat)));
  PathToRelativeW(mstr,buf);
  StrDupW(result,buf);
  mir_free(mstr);
end;

{$include i_search.inc}
{$include i_bass.inc}
{$include i_cc.inc}
{$include i_variables.inc}
{$include i_service.inc}
{$include i_myservice.inc}
{$include i_hotkey.inc}
{$include i_frameapi.inc}
{$include i_tray.inc}
{$include i_visual.inc}
{$include i_optdlg.inc}

function MirandaPluginInfoEx(mirandaVersion:DWORD):PPLUGININFOEX; cdecl;
begin
  result:=@PluginInfo;
  PluginInfo.cbSize     :=SizeOf(TPLUGININFOEX);
  PluginInfo.shortName  :='mRadio Mod';
  PluginInfo.version    :=$00000203;
  PluginInfo.description:='This plugin plays and records Internet radio streams.'+
                          ' Also local media files can be played.';
  PluginInfo.author     :='Awkward';
  PluginInfo.authorEmail:='panda75@bk.ru; awk1975@ya.ru';
  PluginInfo.copyright  :='(c) 2007-2013 Awkward';
  PluginInfo.homepage   :='http://code.google.com/p/delphi-miranda-plugins/';
  PluginInfo.flags      :=UNICODE_AWARE;
  PluginInfo.uuid       :=MIID_MRADIO;
end;

function OnModulesLoaded(wParam:WPARAM;lParam:LPARAM):int;cdecl;
var
  nlu:TNETLIBUSER;
  szTemp:array [0..255] of AnsiChar;
  i:integer;
begin
  DBWriteDWord(0,PluginName,optVersion,PluginInfo.version);

  szTemp[0]:='E';
  szTemp[1]:='Q';
  szTemp[2]:='_';
  szTemp[4]:=#0;
  for i:=0 to 9 do
  begin
    szTemp[3]:=AnsiChar(ORD('0')+i);
    eq[i].param.fGain:=DBReadByte(0,PluginName,szTemp,15)-15;
  end;
  LoadPresets;

  RegisterIcons;
  CreateMenu;
  CreateMIMTrayMenu;

  FillChar(nlu,SizeOf(nlu),0);
  StrCopy(szTemp,Translate('%s server connection'));
  StrReplace(szTemp,'%s',PluginName);
  nlu.szDescriptiveName.a:=szTemp;
  nlu.cbSize             :=SizeOf(nlu);
  nlu.flags              :=NUF_HTTPCONNS or NUF_NOHTTPSOPTION or NUF_OUTGOING;
  nlu.szSettingsModule   :=PluginName;
  hNetLib:=CallService(MS_NETLIB_REGISTERUSER,0,tlparam(@nlu));

//  CallService(MS_RADIO_COMMAND,MRC_RECORD,2); record off - not so necessary

  recpath:=DBReadUnicode(0,PluginName,optRecPath);
  if (recpath=nil) or (recpath^=#0) then
  begin
    recpath:=GetDefaultRecPath;
  end;

  usedevice:=DBReadString(0,PluginName,optDevice);

  sPreBuf:=DBReadWord(0,PluginName,optPreBuf,75);
  BASS_SetConfig(BASS_CONFIG_NET_PREBUF,sPreBuf);

  sBuffer:=DBReadWord(0,PluginName,optBuffer,5000);
  BASS_SetConfig(BASS_CONFIG_NET_BUFFER,sBuffer);

  sTimeout:=DBReadWord(0,PluginName,optTimeout,5000);
  BASS_SetConfig(BASS_CONFIG_NET_TIMEOUT,sTimeout);

  doLoop    :=DBReadByte(0,PluginName,optLoop);
  doShuffle :=DBReadByte(0,PluginName,optShuffle);
  doContRec :=DBReadByte(0,PluginName,optContRec);
  PlayFirst :=DBReadByte(0,PluginName,optPlayFirst);
  isEQ_OFF  :=DBReadByte(0,PluginName,optEQ_OFF);
  AuConnect :=DBReadByte(0,PluginName,optConnect);
  AuMute    :=DBReadByte(0,PluginName,optAutoMute);
  AsOffline :=DBReadByte(0,PluginName,optOffline);
  gVolume   :=DBReadByte(0,PluginName,optVolume,50);
  NumTries  :=DBReadByte(0,PluginName,optNumTries,1);
  ForcedMono:=DBReadByte(0,PluginName,optForcedMono);
  if NumTries<1 then NumTries:=1;

  SetStatus(0,ID_STATUS_OFFLINE);

  StatusTmpl:=DBReadUnicode(0,PluginName,optStatusTmpl,'%radio_title%');

  CallService(MS_RADIO_COMMAND,MRC_STATUS,RD_STATUS_NOSTATION);

  IsMultiThread:=true;

  RegisterVariables;

  if AuConnect<>BST_UNCHECKED then
    ActiveContact:=LoadContact(PluginName,optLastStn)
  else
    ActiveContact:=0;

  HookEvent(ME_DB_CONTACT_SETTINGCHANGED,@OnSettingsChanged);
  HookEvent(ME_DB_CONTACT_DELETED       ,@OnContactDeleted);
  randomize;
  CreateFrame(0);

  RegisterHotKey;

  result:=0;
end;

function PreShutdown(wParam:WPARAM;lParam:LPARAM):int;cdecl;
{
var
  buf:array [0..MAX_PATH-1] of AnsiChar;
  fdata:WIN32_FIND_DATAA;
  p:pAnsiChar;
  fi:THANDLE;
}
begin
  RemoveTrayItems;

  CallService(MS_RADIO_COMMAND,MRC_STOP,1);
  UnregisterHotKey;

  DestroyProtoServices;
  DestroyWindow(hiddenwindow);
  DestroyFrame();
  MyFreeBASS;
  DBWriteByte(0,PluginName,optVolume,gVolume);

  DestroyHookableEvent(hhRadioStatus);


  CallService(MS_NETLIB_CLOSEHANDLE,hNetLib,0);
  mFreeMem(storage);
  mFreeMem(storagep);
  mFreeMem(recpath);
  mFreeMem(usedevice);
  mFreeMem(StatusTmpl);
  mFreeMem(basspath);
  FreePresets;
{
  //delete cover files
  buf[0]:=#0;
  GetTempPathA(MAX_PATH,buf);
  p:=StrEnd(buf);
  StrCopy(p,'mrAvt*.*');

  fi:=FindFirstFileA(buf,fdata);
  if fi<>THANDLE(INVALID_HANDLE_VALUE) then
  begin
    repeat
      StrCopy(p,fdata.cFileName);
      DeleteFileA(buf);
    until not FindNextFileA(fi,fdata);
    FindClose(fi);
  end;
}
  result:=0;
end;

function Load(): int; cdecl;
var
  desc:TPROTOCOLDESCRIPTOR;
  szTemp:array [0..MAX_PATH-1] of WideChar;
  pc:pWideChar;
  custom:pWideChar;
begin
  Langpack_register;

  GetModuleFileNameW(0,szTemp,MAX_PATH-1);
  pc:=StrEndW(szTemp);
  repeat
    dec(pc);
  until pc^='\';
  inc(pc);
  pc^:=#0;

  custom:=DBReadUnicode(0,PluginName,optBASSPath,nil);

  if MyLoadBASS(szTemp,custom) then
  begin
    StrCopyW(pc,'plugins\mradio.ini');
//    StrDup(storage,szTemp);
    FastWideToAnsi(szTemp,storage);
    mGetMem(storagep,MAX_PATH+32);
    CallService(MS_DB_GETPROFILEPATH,MAX_PATH-1,lparam(storagep));
    StrCat(storagep,'\mradio.ini');

    FillChar(desc,SizeOf(desc),0);
    desc.cbSize:=PROTOCOLDESCRIPTOR_V3_SIZE;//SizeOf(desc);
    desc.szName:=PluginName;
    desc._type :=PROTOTYPE_VIRTUAL;
    CallService(MS_PROTO_REGISTERMODULE,0,lparam(@desc));

    hhRadioStatus:=CreateHookableEvent(ME_RADIO_STATUS);

    CreateServiceFunction(MS_RADIO_PLAYSTOP,@Service_RadioPlayStop);
    CreateServiceFunction(MS_RADIO_RECORD  ,@Service_RadioRecord);
    CreateServiceFunction(MS_RADIO_SETTINGS,@Service_RadioSettings);
    CreateServiceFunction(MS_RADIO_SETVOL  ,@Service_RadioSetVolume);
    CreateServiceFunction(MS_RADIO_GETVOL  ,@Service_RadioGetVolume);
    CreateServiceFunction(MS_RADIO_MUTE    ,@Service_RadioMute);
    CreateServiceFunction(MS_RADIO_COMMAND ,@ControlCenter);
    CreateServiceFunction(MS_RADIO_EQONOFF ,@Service_EqOnOff);

    hiddenwindow:=CreateHiddenWindow;
    CreateServiceFunction(MS_RADIO_TRAYMENU,@CreateTrayMenu);

    CreateServiceFunction(MS_RADIO_EXPORT  ,@ExportAll);
    CreateServiceFunction(MS_RADIO_IMPORT  ,@ImportAll);


    CreateProtoServices;
    HookEvent(ME_SYSTEM_MODULESLOADED     ,@OnModulesLoaded);
    HookEvent(ME_SYSTEM_OKTOEXIT          ,@PreShutdown);
    HookEvent(ME_CLIST_DOUBLECLICKED      ,@Service_RadioPlayStop{@DblClickProc});
    HookEvent(ME_OPT_INITIALISE           ,@OnOptInitialise);
    HookEvent(ME_CLIST_PREBUILDCONTACTMENU,@OnContactMenu);

    PluginStatus:=ID_STATUS_OFFLINE;
  end;
  mFreeMem(custom);
  Result:=0;
end;

function Unload: int; cdecl;
begin
  Unload_BASSDLL;
  Result:=0;
end;

exports
  Load, Unload,
  MirandaPluginInfoEx;

begin
  DisableThreadLibraryCalls(hInstance);
end.
