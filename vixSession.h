#ifndef VIX_SESSION
#define VIX_SESSION

#include "vixVarStruct.h"

//statues
#define NONE_VIX           	0
#define CONNECTED_ESXI		1
#define OPENED_VM			2
#define LOGINED_VM			4

/* VIX command definitions  */
#define CMDLEN				15
#define ARGMAXNUM 			10
#define ARGMAXLEN			MAX_URLLEN
//Supported API
#define CID_TEST			0		//command id
#define CID_CONNECT			1
#define CID_DISCONNECT		2
#define CID_LISTGUESTS		3
#define CID_OPEN			4
#define CID_POWERON			5
#define CID_POWEROFF		6
#define CID_LOGIN			7
#define CID_LOGOUT			8
#define CID_PUT				9
#define CID_HPUT			10
#define CID_GET				11
#define CID_BAT				12
#define CID_BASH			13
#define CID_PERL			14
#define CID_DELFILE			15
#define CID_CLONE			16
#define CID_DIR				17
#define CID_CD				18
#define CID_MKDIR			19
#define CID_RMDIR			20
#define CID_CREATESS		21		 // create a snapshot
#define CID_SSRNUM			22		// get number of snapshots
#define CID_GOTOSS			23		// goto named snapshot
#define CID_SSNUM			24		// get children number of snapshot
#define CID_RMSS			25		// remove snapshot(s)
#define CID_SSLIST			26		// get list of snapshots, with dependency relationship
#define CID_POWER_STATE		27		 //get power state of guest
#define CID_REGIST_VM		28
#define CID_UNREGIST_VM		29
#define CID_CLOSE			30
#define CID_DELETE_VM		31
#define CID_GETNAMESS		32
#define CID_DELSSID			33


//ERROR Info definitions
#define VE_000		0 //OK!!!
#define VE_001		1 //failed to init host ticket
#define VE_002		2 //failed to connect esxi
#define VE_003		3 //failed to disconnect esxi
#define VE_004		4 //not connected to esxi yet
#define VE_005		5 // failed to get registed vms
#define VE_006		6 // failed to open vm with vmxpath
#define VE_007		7 // failed to register vmxpath
#define VE_008		8 // failed to un-register vmxpath
#define VE_009		9 // failed to close vm with cache vmxpath
#define VE_010		10 // failed to operate power state
#define VE_011		11 // failed to login vm
#define VE_012		12 // failed to logout vm
#define VE_013		13 // failed to transfer file to vm
#define VE_014		14 // failed to run in vm
#define VE_015		15 // failed to operate snapshot

//split in message
const char SPLIT[] = ";";

const char varpath[]= "/var/run/vmmengine";

const char null[] = "null";

const char CMDNAME_LIST[][CMDLEN] = {
	"test",							
	"connect",
	"disconn",
	"listguests",
	"open",
	"poweron",
	"poweroff",
	"login",
	"logout",
	"put",
	"hput",
	"get",
	"bat",
	"bash",
	"perl",
	"delfile",
	"clone",
	"dir",
	"cd",
	"mkdir",
	"rmdir",
	"createss",
	"ssrnum",
	"gotoss",
	"ssnum",
	"rmss",
	"sslist",
	"powerstate",
	"registvm",
	"unregistvm",
	"close",
	"deletevm",
	"getnamess",
	"delssid",
	NULL
};

const int CMDARGNUM_LIST[] = {
	0,								// 0 TEST				0		//num of argument
	3,								// 1 CONNECT			3		// ip,  user, passwd
	0,								// 2 DISCONNECT		0
	0,								// 3 LISTGUESTS		0
	1,								// 4 OPEN				1		//vmxpath
	0,								// 5 POWERON		0
	0,								// 6 POWEROFF		0
	2,								// 7 LOGIN			2		//guest_os_user, guest_os_passwd
	0,								// 8 LOGOUT			0
	2,								// 9 PUT				2		//local file path, file path in guest
	2,								// 10 HPUT				2		//local file path, file path in guest
	2,								// 11 GET				2		// file path in guest, local file path
	2,								// 12 BAT				2		// bat file path in guest, 0/1(if block)
	2,								// 13 BASH				2		// shell file path in guest, 0/1(if block)
	2,								// 14 PERL				2		//perl file path in guest, 0/1(if block)
	1,								// 15 DELFILE			1		// file path in guest
	1,								// 16 CLONE			1		//vmxpath
	0,								// 17 DIR				0
	1,								// 18 CD				1		// dir path in guest
	1,								// 19 MKDIR			1		// dir path in guest
	1,								// 20 RMDIR			1		// dir path in guest
	2,								// 21 CREATESS			2		//snapshot name, snapshot descr
	0,								// 22 SSRNUM			0
	1,								// 23 GOTOSS			1		// snapshot name
	0,								// 24 SSNUM			0
	1,								// 25 RMSS				1		// snapshot name
	0,								// 26 SSLIST			0
	0,								// 27 POWER_STATE		0
	1,								// 28 REGIST_VM		1
	1,								// 29 UNREGIST_VM		1
	0,								// 30 CLOSE			0             // close cached vmxpath
	0,								// 31 DELETE_VM		0 		// after opened
	1,								// 32 GETNAMESS		1
	1,								// 33 DELSSID
	-1
};


typedef struct _SESSIONINFO{
	char host_ip[MAX_URLLEN];
	char host_user[MAX_NAMELEN];
	char host_passwd[MAX_PASSLEN];
	char vmx_path[MAXPATHLEN];
	int state;
}SESSIONINFO;

int cmd_name_to_index(const char * cmdname);
int parse_request(char * request);

int handle_esxi_session(char * request, char * response);



#endif
