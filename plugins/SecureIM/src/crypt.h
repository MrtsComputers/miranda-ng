#ifndef __CRYPT_H__
#define __CRYPT_H__

#define KEYSIZE 256
#define NAMSIZE 512
#define DEFMSGS 4096

#define KEY_A_SIG	0x000000
#define KEY_B_SIG	0x010000

// struct to store all supported protocols
struct SupPro {
	LPSTR name;
	BOOL inspecting;
	int split_on, tsplit_on;
	int split_off, tsplit_off;
};
typedef SupPro *pSupPro;

// struct to store wainting messages
struct waitingMessage {
	LPSTR Message;
	WPARAM wParam;
	waitingMessage *nextMessage;
};
typedef waitingMessage* pWM;

struct partitionMessage {
	int id;
	LPSTR *message; // array of message parts
	partitionMessage *nextMessage;
};
typedef partitionMessage* pPM;

#define HEADER 0xABCD1234
#define FOOTER 0x9876FEDC
#define EMPTYH 0xF1E2D3C4

// memory struct for keys
struct UinKey
{
	HANDLE hContact;	// handle of contact
	u_int header;		// HEADER
	pSupPro proto;		// proto struct
	BYTE mode,tmode;	// mode: Native,PGP,GPG,RSA/AES,RSA [0..4]
	BYTE status,tstatus;	// status: Disabled,Enabled,AlwaysTry [0..2] for Native mode
	LPSTR msgSplitted;	// message to combine
	pPM msgPart;		// parts of message
	pWM msgQueue;		// last messages not sended or to resend;
	BOOL sendQueue;		// ���� ������� ������� - �� �������������� ���������
	BOOL offlineKey;
	char waitForExchange;	// 0 - �������� �������
				// 1 - �������
				// 2 - ������� � �����������
				// 3 - ������� ��� ���������� � ��������
	BOOL decoded;		// false on decode error
	short features;
	HANDLE cntx;		// crypto context
	BYTE keyLoaded;		// ( 1-PGP, 2-GPG ) | 1-RSA
	BYTE gpgMode,tgpgMode;	// 0-UTF8, 1-ANSI
	char *lastFileRecv;
	char *lastFileSend;
	char **fileSend;
	BOOL finFileRecv;
	BOOL finFileSend;
	LPSTR tmp;		// tmp text string
	u_int footer;		// FOOTER
};
typedef UinKey* pUinKey;

struct TFakeAckParams
{
	__forceinline TFakeAckParams(HANDLE p1, LONG p2, LPCSTR p3) :
		hContact(p1),
		id(p2),
		msg(p3)
		{}

	HANDLE hContact;
	LONG   id;
	LPCSTR msg;
};

extern char szUIN[NAMSIZE];
extern char szName[NAMSIZE];

extern LIST<SupPro> arProto;
extern LIST<UinKey> arClist;

// crypt_lists.cpp
void loadContactList();
void freeContactList();
pUinKey addContact(HANDLE hContact);
void    delContact(HANDLE hContact);
pSupPro getSupPro(HANDLE);
pUinKey findUinKey(HANDLE hContact);
pUinKey getUinKey(HANDLE hContact);
pUinKey getUinCtx(HANDLE);
void addMsg2Queue(pUinKey,WPARAM,LPSTR);

void getContactName(HANDLE hContact, LPSTR szName);
void getContactNameA(HANDLE hContact, LPSTR szName);
void getContactUin(HANDLE hContact, LPSTR szUIN);
void getContactUinA(HANDLE hContact, LPSTR szUIN);

// crypt_check.cpp
int getContactStatus(HANDLE);

bool isSecureProtocol(HANDLE hContact);
BYTE isContactSecured(HANDLE hContact);
bool isClientMiranda(pUinKey ptr, BOOL emptyMirverAsMiranda=FALSE);
bool isClientMiranda(HANDLE hContact, BOOL emptyMirverAsMiranda=FALSE);
bool isProtoSmallPackets(HANDLE);
bool isContactInvisible(HANDLE);
bool isNotOnList(HANDLE);
bool isContactNewPG(HANDLE);
bool isContactPGP(HANDLE);
bool isContactGPG(HANDLE);
bool isContactRSAAES(HANDLE);
bool isContactRSA(HANDLE);
bool isChatRoom(HANDLE);
bool isFileExist(LPCSTR);
bool isSecureIM(pUinKey ptr, BOOL emptyMirverAsSecureIM=FALSE);
bool isSecureIM(HANDLE hContact, BOOL emptyMirverAsSecureIM=FALSE);

// crypt_icons.cpp
HICON mode2icon(int,int);
HANDLE mode2clicon(int mode, int type);
void RefreshContactListIcons(void);
void ShowStatusIcon(HANDLE,UINT);
void ShowStatusIcon(HANDLE);
void ShowStatusIconNotify(HANDLE);

// crypt_popups.cpp
//static int CALLBACK PopupDlgProc(HWND,UINT,WPARAM,LPARAM);
void showPopup(LPCSTR,HANDLE,HICON,UINT);
void showPopupEC(HANDLE);
void showPopupDCmsg(HANDLE,LPCSTR);
void showPopupDC(HANDLE);
void showPopupKS(HANDLE);
void showPopupKRmsg(HANDLE,LPCSTR);
void showPopupKR(HANDLE);
void showPopupSM(HANDLE);
void showPopupRM(HANDLE);

// crypt_meta.cpp
BOOL isProtoMetaContacts(HANDLE);
BOOL isDefaultSubContact(HANDLE);
HANDLE getMetaContact(HANDLE);
HANDLE getMostOnline(HANDLE);
void DeinitMetaContact(HANDLE);

// crypt_dll.cpp
LPSTR InitKeyA(pUinKey,int);
int InitKeyB(pUinKey,LPCSTR);
void InitKeyX(pUinKey,BYTE*);
BOOL CalculateKeyX(pUinKey,HANDLE);
LPSTR encodeMsg(pUinKey,LPARAM);
LPSTR decodeMsg(pUinKey,LPARAM,LPSTR);
BOOL LoadKeyPGP(pUinKey);
BOOL LoadKeyGPG(pUinKey);

// crypt_misc.cpp
void waitForExchange(pUinKey ptr, int flag = 1);

#endif
