#ifndef VIX_VAR_STRUCT
#define VIX_VAR_STRUCT

#include "vix.h"
#include <stdio.h>

/************************************************************************/
/* Macro Variable Definition Region                                                                     */
/************************************************************************/
/* Connection Type for VIX to ESXi */
#define CONNTYPE VIX_SERVICEPROVIDER_VMWARE_VI_SERVER

/* Operation Type for Power Control */
#define VMPOWEROPTIONS VIX_VMPOWEROP_NORMAL

/* The max length to url string */
#define MAX_URLLEN 300

/* The max length to the name string */
#define MAX_NAMELEN 100

/* The max length to password string */
#define MAX_PASSLEN 100

/* Time-out Setting for Operation Session through VMware Tools */
#define  TOOLS_TIMEOUT  900

/* Enum Type for True/False control */
typedef enum{VIX_FALSE = 0, VIX_TRUE} VIX_BOOL;

/* Power Control Type */
typedef enum{POWEROFF = 0, POWERON, RESET}VMOPRATION;

/* VM OS Type */
typedef enum{LINUX = 0, WINDOWS}OSTYPE;

/* VM Power Status */
typedef enum{PWUNKNOWN = 0, POWERING_OFF, POWERD_OFF, POWERING_ON, POWERED_ON}VMPOWER;

/************************************************************************/
/* Function Definition Region                                                                     */
/************************************************************************/

/* Structure for Operation Session to a host like ESXi */
typedef struct _HOSTTICKET{
	char HostName[MAX_URLLEN];
	int HostPort;
	char UserName[MAX_NAMELEN];
	char PassWord[MAX_PASSLEN];
	
	VixHandle hostHandle;
	VixHandle jobHandle;
	VixError err;
}HOSTTICKET;

/* Structure for Operation Session to a Virtual Machine */
typedef struct _VMTICKET{
	char VMName[MAX_NAMELEN];
	VixHandle vmHandle;
}VMTICKET;

/* Extensible data structure */
typedef struct _INFONODE{
	char Name[MAX_NAMELEN + 1];
	char Value[MAX_URLLEN + 1];
	struct _INFONODE * Next;
}INFONODE;

typedef struct _INFOLIST{
	INFONODE * ListHead;
	INFONODE * ListTail;
}INFOLIST;

#endif


