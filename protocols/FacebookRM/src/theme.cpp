/*

Facebook plugin for Miranda Instant Messenger
_____________________________________________

Copyright � 2009-11 Michal Zelinka, 2011-12 Robert P�sel

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "common.h"

extern OBJLIST<FacebookProto> g_Instances;

static IconItem icons[] =
{
	{ LPGEN("Facebook Icon"),             "facebook",      IDI_FACEBOOK },
	{ LPGEN("Mind"),                       "mind",         IDI_MIND },
	
	{ LPGEN("Cancel friendship"),         "authRevoke",    IDI_AUTH_REVOKE },
	{ LPGEN("Cancel friendship request"), "authRevokeReq", IDI_AUTH_REVOKE },
	{ LPGEN("Request friendship"),        "authAsk",       IDI_AUTH_ASK },
	{ LPGEN("Approve friendship"),        "authGrant",     IDI_AUTH_GRANT },
	
	{ LPGEN("Visit Profile"),             "homepage",      0  },
};

// TODO: uninit
void InitIcons(void)
{
	Icon_Register(g_hInstance, "Protocols/Facebook", icons, SIZEOF(icons)-1, "Facebook");
	icons[ SIZEOF(icons)-1 ].hIcolib = LoadSkinnedIconHandle(SKINICON_EVENT_URL);
}

HANDLE GetIconHandle(const char* name)
{
	for(size_t i=0; i<SIZEOF(icons); i++)
		if(strcmp(icons[i].szName, name) == 0)
			return icons[i].hIcolib;

	return 0;
}

char *GetIconDescription(const char* name)
{
	for(size_t i=0; i<SIZEOF(icons); i++)
		if(strcmp(icons[i].szName, name) == 0)
			return icons[i].szDescr;

	return "";
}

// Contact List menu stuff
HANDLE hHookPreBuildMenu;
HANDLE g_hContactMenuItems[CMITEMS_COUNT];
HANDLE g_hContactMenuSvc[CMITEMS_COUNT];

// Helper functions
static FacebookProto * GetInstanceByHContact(HANDLE hContact)
{
	char *proto = GetContactProto(hContact);
	if( !proto )
		return 0;

	for(int i=0; i<g_Instances.getCount(); i++)
		if(!strcmp(proto,g_Instances[i].m_szModuleName))
			return &g_Instances[i];

	return 0;
}

template<int (__cdecl FacebookProto::*Fcn)(WPARAM,LPARAM)>
INT_PTR GlobalService(WPARAM wParam,LPARAM lParam)
{
	FacebookProto *proto = GetInstanceByHContact(reinterpret_cast<HANDLE>(wParam));
	return proto ? (proto->*Fcn)(wParam,lParam) : 0;
}

static int PrebuildContactMenu(WPARAM wParam,LPARAM lParam)
{
	for (size_t i=0; i<SIZEOF(g_hContactMenuItems); i++)
	{
		EnableMenuItem(g_hContactMenuItems[i], false);
	}

	FacebookProto *proto = GetInstanceByHContact(reinterpret_cast<HANDLE>(wParam));
	return proto ? proto->OnPrebuildContactMenu(wParam,lParam) : 0;
}

void InitContactMenus()
{
	hHookPreBuildMenu = HookEvent(ME_CLIST_PREBUILDCONTACTMENU,PrebuildContactMenu);

	CLISTMENUITEM mi = {sizeof(mi)};
	mi.flags = CMIF_ICONFROMICOLIB;

	mi.position=-2000006000;
	mi.icolibItem = GetIconHandle("homepage");
	mi.pszName = GetIconDescription("homepage");
	mi.pszService = "FacebookProto/VisitProfile";
	g_hContactMenuSvc[CMI_VISIT_PROFILE] = CreateServiceFunction(mi.pszService,GlobalService<&FacebookProto::VisitProfile>);
	g_hContactMenuItems[CMI_VISIT_PROFILE] = Menu_AddContactMenuItem(&mi);

	mi.position=-2000006001;
	mi.icolibItem = GetIconHandle("authRevoke");
	mi.pszName = GetIconDescription("authRevoke");
	mi.pszService = "FacebookProto/CancelFriendship";
	g_hContactMenuSvc[CMI_AUTH_REVOKE] = CreateServiceFunction(mi.pszService,GlobalService<&FacebookProto::CancelFriendship>);
	g_hContactMenuItems[CMI_AUTH_REVOKE] = Menu_AddContactMenuItem(&mi);

	mi.position=-2000006001;
	mi.icolibItem = GetIconHandle("authRevokeReq");
	mi.pszName = GetIconDescription("authRevokeReq");
	mi.pszService = "FacebookProto/CancelFriendshipRequest";
	g_hContactMenuSvc[CMI_AUTH_CANCEL] = CreateServiceFunction(mi.pszService,GlobalService<&FacebookProto::OnCancelFriendshipRequest>);
	g_hContactMenuItems[CMI_AUTH_CANCEL] = Menu_AddContactMenuItem(&mi);

	mi.position=-2000006002;
	mi.icolibItem = GetIconHandle("authAsk");
	mi.pszName = GetIconDescription("authAsk");
	mi.pszService = "FacebookProto/RequestFriendship";
	g_hContactMenuSvc[CMI_AUTH_ASK] = CreateServiceFunction(mi.pszService,GlobalService<&FacebookProto::RequestFriendship>);
	g_hContactMenuItems[CMI_AUTH_ASK] = Menu_AddContactMenuItem(&mi);

	mi.position=-2000006003;
	mi.icolibItem = GetIconHandle("authGrant");
	mi.pszName = GetIconDescription("authGrant");
	mi.pszService = "FacebookProto/ApproveFriendship";
	g_hContactMenuSvc[CMI_AUTH_GRANT] = CreateServiceFunction(mi.pszService,GlobalService<&FacebookProto::ApproveFriendship>);
	g_hContactMenuItems[CMI_AUTH_GRANT] = Menu_AddContactMenuItem(&mi);
}

void UninitContactMenus()
{
	for(size_t i=0; i<SIZEOF(g_hContactMenuItems); i++)
		CallService(MS_CLIST_REMOVECONTACTMENUITEM,(WPARAM)g_hContactMenuItems[i],0);

	for(size_t i=0; i<SIZEOF(g_hContactMenuSvc); i++)
		DestroyServiceFunction(g_hContactMenuSvc[i]);
	
	UnhookEvent(hHookPreBuildMenu);
}

void EnableMenuItem(HANDLE hMenuItem, bool enable)
{
	CLISTMENUITEM clmi = { sizeof(clmi) };
	clmi.flags = CMIM_FLAGS;
	if (!enable)
		clmi.flags |= CMIF_HIDDEN;

	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hMenuItem, (LPARAM)&clmi);
}

int FacebookProto::OnPrebuildContactMenu(WPARAM wParam,LPARAM lParam)
{	
	HANDLE hContact = reinterpret_cast<HANDLE>(wParam);

	EnableMenuItem(g_hContactMenuItems[CMI_VISIT_PROFILE], true);

	if (!isOffline() && !DBGetContactSettingByte(hContact, m_szModuleName, "ChatRoom", 0))
	{
		bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

		BYTE type = DBGetContactSettingByte(hContact, m_szModuleName, FACEBOOK_KEY_CONTACT_TYPE, 0);

		EnableMenuItem(g_hContactMenuItems[CMI_AUTH_ASK], ctrlPressed || type == FACEBOOK_CONTACT_NONE || !type);
		EnableMenuItem(g_hContactMenuItems[CMI_AUTH_GRANT], ctrlPressed || type == FACEBOOK_CONTACT_APPROVE);
		EnableMenuItem(g_hContactMenuItems[CMI_AUTH_REVOKE], ctrlPressed || type == FACEBOOK_CONTACT_FRIEND);
		EnableMenuItem(g_hContactMenuItems[CMI_AUTH_CANCEL], ctrlPressed || type == FACEBOOK_CONTACT_REQUEST);		
	}

	return 0;
}

int FacebookProto::OnBuildStatusMenu(WPARAM wParam,LPARAM lParam)
{
	char text[200];
	strcpy(text,m_szModuleName);
	char *tDest = text+strlen(text);

	CLISTMENUITEM mi = {sizeof(mi)};
	mi.pszService = text;

	HGENMENU hRoot = MO_GetProtoRootMenu(m_szModuleName);
	if (hRoot == NULL) {
		mi.popupPosition = 500085000;
		mi.hParentMenu = HGENMENU_ROOT;
		mi.flags = CMIF_ICONFROMICOLIB | CMIF_ROOTPOPUP | CMIF_TCHAR | CMIF_KEEPUNTRANSLATED | ( this->isOnline() ? 0 : CMIF_GRAYED );
		mi.icolibItem = GetIconHandle( "facebook" );
		mi.ptszName = m_tszUserName;
		hRoot = m_hMenuRoot = Menu_AddProtoMenuItem(&mi);
	} else {
		if ( m_hMenuRoot )
			CallService( MS_CLIST_REMOVEMAINMENUITEM, ( WPARAM )m_hMenuRoot, 0 );
		m_hMenuRoot = NULL;
	}

	mi.flags = CMIF_ICONFROMICOLIB | CMIF_CHILDPOPUP | ( this->isOnline() ? 0 : CMIF_GRAYED );
	mi.position = 201001;

	CreateProtoService(m_szModuleName,"/Mind",&FacebookProto::OnMind,this);
	strcpy(tDest,"/Mind");
	mi.hParentMenu = hRoot;
	mi.pszName = LPGEN("Mind...");
	mi.icolibItem = GetIconHandle("mind");
	m_hStatusMind = Menu_AddProtoMenuItem(&mi);

	CreateProtoService(m_szModuleName,"/VisitProfile",&FacebookProto::VisitProfile,this);
	strcpy(tDest,"/VisitProfile");
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_CHILDPOPUP;
	mi.pszName = LPGEN("Visit Profile");
	mi.icolibItem = GetIconHandle("homepage");
	// TODO RM: remember and properly free in destructor?
	/*m_hStatusMind = */Menu_AddProtoMenuItem(&mi);

	// Services...
	mi.pszName = LPGEN("Services...");
	strcpy( tDest, "/Services" );
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_CHILDPOPUP | ( this->isOnline() ? 0 : CMIF_GRAYED );
	mi.icolibItem = NULL;
	m_hMenuServicesRoot = Menu_AddProtoMenuItem(&mi);

	CreateProtoService(m_szModuleName,"/RefreshBuddyList",&FacebookProto::RefreshBuddyList,this);
	strcpy(tDest,"/RefreshBuddyList");
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_ROOTHANDLE;
	mi.pszName = LPGEN("Refresh Buddy List");
	mi.pszPopupName = LPGEN("Services");
	mi.icolibItem = NULL;
	mi.hParentMenu = m_hMenuServicesRoot;
	Menu_AddProtoMenuItem(&mi);

	CreateProtoService(m_szModuleName,"/CheckFriendRequests",&FacebookProto::CheckFriendRequests,this);
	strcpy(tDest,"/CheckFriendRequests");
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_ROOTHANDLE;
	mi.pszName = LPGEN("Check Friends Requests");
	mi.icolibItem = NULL;
	mi.hParentMenu = m_hMenuServicesRoot;
	Menu_AddProtoMenuItem(&mi);

	CreateProtoService(m_szModuleName,"/CheckNewsfeeds",&FacebookProto::CheckNewsfeeds,this);
	strcpy(tDest,"/CheckNewsfeeds");
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_ROOTHANDLE;
	mi.pszName = LPGEN("Check Newsfeeds");
	mi.pszPopupName = LPGEN("Services");
	mi.icolibItem = NULL;
	mi.hParentMenu = m_hMenuServicesRoot;
	Menu_AddProtoMenuItem(&mi);

	return 0;
}

void FacebookProto::ToggleStatusMenuItems( BOOL bEnable )
{
	CLISTMENUITEM clmi = {sizeof(clmi)};
	clmi.flags = CMIM_FLAGS | (( bEnable ) ? 0 : CMIF_GRAYED);

	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuRoot,   ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hStatusMind, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuServicesRoot, ( LPARAM )&clmi );
}
