#ifndef VIX_HANDLER
#define VIX_HANDLER

#include "vixVarStruct.h"

/************************************************************************/
/* Host (ESXi) Session Operation Functions                                                                     */
/************************************************************************/
/* Create a host session */
VIX_BOOL NewHostTicket(char* hostName, int hostPort, char* userName, char* passWord, HOSTTICKET * ptrHT);

/* Build an active connection based on a session */
VIX_BOOL HostConnect(HOSTTICKET* nHostTicket);

/* Free an active connection based on a session */
VIX_BOOL HostDisconnect(HOSTTICKET* nHostTicket);

/* List FindItems on Host */
VIX_BOOL HostFindItems(HOSTTICKET* nHostTicket, VixFindItemType searchType, VixEventProc * ptrProc);

/* List Existing Guests on Host */


/* List Running Guests on Host */


/************************************************************************/
/* VM Operation based on a host session                                                                     */
/************************************************************************/
/* Register a vmx to an active VM  */
VIX_BOOL RegisterVM(HOSTTICKET* nHostTicket, char* vmxFilePath);

/* Unregister an active VM */
VIX_BOOL UnregisterVM(HOSTTICKET* nHostTicket, char* vmxFilePath);

/* Locate the specified Virtual Machine */
VIX_BOOL OpenVM(HOSTTICKET* nHostTicket, char* vmxPath,  VMTICKET * nVmTicket);

/* Free the Virtual Machine Control */
void CloseVM(VMTICKET* nVMTicket);

/* Delete the Virtual Machine Control */
void DeleteVM(VMTICKET* nVMTicket);

/* Operate the Power Control of the Virtual Machine */
VIX_BOOL OperateVM(VMTICKET* nVMTicket, VMOPRATION vmOperation);

/* Login into the Virtual Machine */
VIX_BOOL LoginVM(VMTICKET* nVMTicket, char* osUsername, char* osPassword);

/* Logout the Virtual Machine */
VIX_BOOL LogoutVM(VMTICKET* nVMTicket);

/* Get power statue of the Virtual Machine */
VMPOWER GetVMPowerState(VMTICKET* nVMTicket, char * errbuf);

/* Make directory in the Virtual Machine */
VIX_BOOL CreateDirectoryInGuest(VMTICKET* nVMTicket, char* pathName);

/* Delete directory in the Virtual Machine */
VIX_BOOL DeleteDirectoryInGuest(VMTICKET* nVMTicket, char* pathName);

/* Delete file in the Virtual Machine */
VIX_BOOL DeleteFileInGuest(VMTICKET* nVMTicket, char* pathFileName);

/* Copy file from Virtual Machine to native */
VIX_BOOL CopyFileFromGuestToHost(VMTICKET* nVMTicket, char* srcName, char* dstName);

/* Copy file from native to Virtual Machine */
VIX_BOOL CopyFileFromHostToGuest(VMTICKET* nVMTicket, char* srcName, char* dstName);

/* Clone a registered Virtual Machine with new name */
VIX_BOOL CloneVM(VMTICKET* nVMTicket, char* newVMName);

/* Run the program with arguments in the Virtual Machine */
VIX_BOOL RunProgramInGuest(VMTICKET* nVMTicket, char* progName, char* args);

/* Run the script with the interpreter in the Virtual Machine */
VIX_BOOL RunScriptInGuest(VMTICKET* nVMTicket, char* interpreter, char* scriptText, int block);

/* List the directory in VM */
VIX_BOOL ListDirInGuest(VMTICKET* nVMTicket, char* dirpath);

/* Create an original VM snapshot */
VIX_BOOL CreateVMSnapshot(VMTICKET* nVMTicket, char* ssname, char* ssdesc);

/* Get number of VM snapshot */
int GetVMSnapshotNumber(VMTICKET* nVMTicket);

/* Get child snapshot number of the root snapshot  */
int GetVMSnapChildNumber(VMTICKET* nVMTicket);

/* Get Snapshot by name */
VixHandle GetVMSnapshotByName(VMTICKET * nVMTicket, char* ssname);

/* Check existing snapshot by name */
int HasVMSnapshot(VMTICKET * nVMTicket, char * ssname);

/* Revert to named Snapshot */
VIX_BOOL RevertSnapshotByName(VMTICKET * nVMTicket, char* ssname);

/* Remove named Snapshot */
VIX_BOOL RemoveSnapshotByName(VMTICKET * nVMTicket, char* ssname);

VIX_BOOL RemoveSnapshotById(VMTICKET * nVMTicket, long ssid);


/* Un-install the VMware Tools with complicated operation */
//VIX_BOOL UninstallVMwareTools(VMTICKET* nVMTicket, char* srcDeployFile, char* dstDeployFile, char* interpreter, char* scriptText);

#endif
