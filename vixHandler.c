#include "vixHandler.h"
#include "funcTools.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Create a host session */
VIX_BOOL NewHostTicket(char* hostName, int hostPort, char* userName, char* passWord, HOSTTICKET * ptrHT){
	if(hostName == NULL || userName == NULL || passWord == NULL){
		return VIX_FALSE;
	}

	memset(ptrHT, 0, sizeof(HOSTTICKET));
	strncpy(ptrHT->HostName,hostName, MAX_URLLEN);
	ptrHT->HostPort = hostPort;
	strncpy(ptrHT->UserName, userName, MAX_NAMELEN);
	strncpy(ptrHT->PassWord, passWord, MAX_PASSLEN);
	ptrHT->hostHandle = VIX_INVALID_HANDLE;
	ptrHT->jobHandle = VIX_INVALID_HANDLE;
	return VIX_TRUE;
}

/* Build an active connection based on a session */
VIX_BOOL HostConnect(HOSTTICKET* nHostTicket){
	if(nHostTicket == NULL)
		return VIX_FALSE;
	//fprintf(stderr, "TEST |%s| |%d| |%s| |%s|\n", nHostTicket->HostName, nHostTicket->HostPort, nHostTicket->UserName, nHostTicket->PassWord);
	nHostTicket->jobHandle = VixHost_Connect(VIX_API_VERSION,
								CONNTYPE,
								nHostTicket->HostName,
								nHostTicket->HostPort,
								nHostTicket->UserName,
								nHostTicket->PassWord,
								0,
								VIX_INVALID_HANDLE,
								NULL,
								NULL);
	nHostTicket->err = VixJob_Wait(nHostTicket->jobHandle,
		VIX_PROPERTY_JOB_RESULT_HANDLE,
		&(nHostTicket->hostHandle),
		VIX_PROPERTY_NONE);
	
	Vix_ReleaseHandle(nHostTicket->jobHandle);
	if(VIX_FAILED(nHostTicket->err)){
		return VIX_FALSE;
	}
	return VIX_TRUE;
}

/* Free an active connection based on a session */
VIX_BOOL HostDisconnect(HOSTTICKET* nHostTicket){
	Vix_ReleaseHandle(nHostTicket->jobHandle);
	VixHost_Disconnect(nHostTicket->hostHandle);
	return VIX_TRUE;
}



VIX_BOOL HostFindItems(HOSTTICKET* nHostTicket, VixFindItemType searchType, VixEventProc * ptrProc){
	/* Looking for virtual machines */
	VixHandle jobHandle = VIX_INVALID_HANDLE;
	VixError err;
	char * errmsg = NULL;
	VIX_BOOL ret = VIX_TRUE;

	jobHandle = VixHost_FindItems(nHostTicket->hostHandle,
		searchType,
		VIX_INVALID_HANDLE,
		-1,
		ptrProc,
		NULL);
	err = VixJob_Wait(jobHandle,VIX_PROPERTY_NONE);
	if(VIX_FAILED(err)){
		errmsg = Vix_GetErrorText(err,NULL);
		fprintf(stderr, "HostFindItems Error:%s\n", errmsg);
		ret = VIX_FALSE;
		goto abort;
	}
abort:
	Vix_ReleaseHandle(jobHandle);
	return ret;
}
	

/************************************************************************/
/* VM Operation based on a host session                                                                     */
/************************************************************************/
/* Register a vmx to an active VM  */
VIX_BOOL RegisterVM(HOSTTICKET* nHostTicket, char* vmxFilePath){
	if(nHostTicket == NULL || vmxFilePath == NULL)
		return VIX_FALSE;	
	
	Vix_ReleaseHandle(nHostTicket->jobHandle);
	
	nHostTicket->jobHandle = VixHost_RegisterVM(nHostTicket->hostHandle,
		vmxFilePath,
		NULL,
		NULL);
	
	nHostTicket->err = VixJob_Wait(nHostTicket->jobHandle, 
		VIX_PROPERTY_NONE);
	if(VIX_OK != nHostTicket->err){
		return VIX_FALSE;
	}
    Vix_ReleaseHandle(nHostTicket->jobHandle);
	return VIX_TRUE;
}

/* Unregister an active VM */
VIX_BOOL UnregisterVM(HOSTTICKET* nHostTicket, char* vmxFilePath){
	if(nHostTicket == NULL || vmxFilePath == NULL)
		return VIX_FALSE;
	
	
	Vix_ReleaseHandle(nHostTicket->jobHandle);
	
	nHostTicket->jobHandle = VixHost_UnregisterVM(nHostTicket->hostHandle,
		vmxFilePath,
		NULL,
		NULL);
	
	nHostTicket->err = VixJob_Wait(nHostTicket->jobHandle, 
		VIX_PROPERTY_NONE);
	if(VIX_OK != nHostTicket->err){
		return VIX_FALSE;
	}
	Vix_ReleaseHandle(nHostTicket->jobHandle);
	return VIX_TRUE;
}

/* Locate the specified Virtual Machine */
VIX_BOOL OpenVM(HOSTTICKET* nHostTicket, char* vmxPath, VMTICKET * nVmTicket){
	VixError locErr = VIX_OK;
	char err[BUFMAXLEN] = "\0";
	int dfd = openlog();
	
	if(nHostTicket == NULL || vmxPath == NULL){
		closelog(dfd);
		return VIX_FALSE;
	}
	//bzero(nVmTicket, sizeof(VMTICKET));
    fprintf(stderr, "Try to opening %s\n", vmxPath);
	nHostTicket->jobHandle = VixHost_OpenVM(nHostTicket->hostHandle,
		vmxPath, 
		VIX_VMOPEN_NORMAL, 
		VIX_INVALID_HANDLE, 
		NULL, NULL);
	locErr = VixJob_Wait(nHostTicket->jobHandle, 
		VIX_PROPERTY_JOB_RESULT_HANDLE,
		&(nVmTicket->vmHandle),
		VIX_PROPERTY_NONE);
	if(VIX_FAILED(locErr)){
		strcpy(err, Vix_GetErrorText(locErr, NULL)); 
		dumplog(dfd, err);
	}
	Vix_ReleaseHandle(nHostTicket->jobHandle);
	/*
	nHostTicket->jobHandle = VixVM_Open(nHostTicket->hostHandle,
		vmxPath,
		NULL,
		NULL);
	locErr= VixJob_Wait(nHostTicket->jobHandle,
		VIX_PROPERTY_JOB_RESULT_HANDLE,
		&(nVmTicket->vmHandle),
		VIX_PROPERTY_NONE);
	if(VIX_FAILED(locErr)){
		strcpy(err, Vix_GetErrorText(locErr, NULL)); 
		dumplog(dfd, err);
		free(nVmTicket);
		nVmTicket = NULL;
	}
	Vix_ReleaseHandle(nHostTicket->jobHandle);
	*/
	closelog(dfd);
	return VIX_TRUE;
}

/* Free the Virtual Machine Control */
void CloseVM(VMTICKET* nVMTicket){
	if(nVMTicket != NULL){
		bzero(nVMTicket, sizeof(VMTICKET));
	}
}

/* Delete the Virtual Machine Control */
void DeleteVM(VMTICKET* nVMTicket){
	VixHandle jobHandle = VIX_INVALID_HANDLE;

	if(nVMTicket != NULL){
		jobHandle = VixVM_Delete(nVMTicket->vmHandle, VIX_VMDELETE_DISK_FILES, NULL, NULL);
		nVMTicket = NULL;
	}
}


/* Operate the Power Control of the Virtual Machine */
VIX_BOOL OperateVM(VMTICKET* nVMTicket, VMOPRATION vmOperation){
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixError locErr;
	
	if(nVMTicket == NULL)
		return VIX_FALSE;
	
	switch(vmOperation){
	case POWERON:
		locJobHandle = VixVM_PowerOn(nVMTicket->vmHandle,
			VMPOWEROPTIONS,
			VIX_INVALID_HANDLE,
			NULL,
			NULL);
		locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
		break;
	case POWEROFF:
		locJobHandle = VixVM_PowerOff(nVMTicket->vmHandle,
			VMPOWEROPTIONS,
			VIX_INVALID_HANDLE,
			/*NULL, for Windows*/
			NULL);
		locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
		break;
	case RESET:
		locJobHandle = VixVM_Reset(nVMTicket->vmHandle,
			VIX_VMPOWEROP_NORMAL,
			NULL,
			NULL);
		locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
		break;
	default:
		break;
	}
	
	Vix_ReleaseHandle(locJobHandle);
	if(VIX_FAILED(locErr)){
		return VIX_FALSE;
	}
	return VIX_TRUE;
}

/* Login into the Virtual Machine */
VIX_BOOL LoginVM(VMTICKET* nVMTicket, char* osUsername, char* osPassword){
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixError locErr;
	
	if(nVMTicket == NULL || osUsername == NULL || osPassword == NULL)
		return VIX_FALSE;
	
	locJobHandle = VixVM_WaitForToolsInGuest(nVMTicket->vmHandle,
		TOOLS_TIMEOUT,
		NULL,
		NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(locJobHandle);
	if(VIX_FAILED(locErr)){
		fprintf(stderr, "failed to wait for tools in virtual machine (%"FMT64"d %s)\n", locErr,
			Vix_GetErrorText(locErr, NULL));
		return VIX_FALSE;
	}
	
	locJobHandle = VixVM_LoginInGuest(nVMTicket->vmHandle, osUsername, osPassword, 0, NULL, NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(locJobHandle);
	if(VIX_FAILED(locErr)){
		return VIX_FALSE;
	}
	return VIX_TRUE;
}

/* Logout the Virtual Machine */
VIX_BOOL LogoutVM(VMTICKET* nVMTicket){
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixError locErr;
	
	if(nVMTicket == NULL)
		return VIX_FALSE;
	
	locJobHandle = VixVM_LogoutFromGuest(nVMTicket->vmHandle, NULL, NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(locJobHandle);
	if(VIX_FAILED(locErr)){
		return VIX_FALSE;
	}
	return VIX_TRUE;
}

/* Get power statue of the Virtual Machine */
VMPOWER GetVMPowerState(VMTICKET* nVMTicket, char * errbuf){
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixError locErr;
	VMPOWER ret = PWUNKNOWN;
	char vmpowerstate = -1;
	
	
	if(nVMTicket == NULL)
		return ret;
	
	locErr = Vix_GetProperties(nVMTicket->vmHandle, VIX_PROPERTY_VM_POWER_STATE, &vmpowerstate, VIX_PROPERTY_NONE);
	if(VIX_FAILED(locErr)){
		if(errbuf){
			sprintf(errbuf, "Unknown powerstate %d", vmpowerstate);
		}
		return ret;
	}
	
	if(VIX_POWERSTATE_POWERING_OFF & vmpowerstate){
		ret = POWERING_OFF;
	}else if(VIX_POWERSTATE_POWERED_OFF & vmpowerstate){
		ret = POWERD_OFF;
	}else if(VIX_POWERSTATE_POWERING_ON & vmpowerstate){
		ret = POWERING_ON;
	}else if(VIX_POWERSTATE_POWERED_ON & vmpowerstate){
		ret = POWERED_ON;
	}		
	return ret;
}

/* Make directory in the Virtual Machine */
VIX_BOOL CreateDirectoryInGuest(VMTICKET* nVMTicket, char* pathName){
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixError locErr;
	
	if(nVMTicket == NULL || pathName ==NULL)
		return VIX_FALSE;
	
	locJobHandle = VixVM_CreateDirectoryInGuest(nVMTicket->vmHandle, pathName, VIX_INVALID_HANDLE, NULL, NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(locJobHandle);
	if(VIX_FAILED(locErr)){
		return VIX_FALSE;
	}
	return VIX_TRUE;
}

/* Delete directory in the Virtual Machine */
VIX_BOOL DeleteDirectoryInGuest(VMTICKET* nVMTicket, char* pathName){
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixError locErr;
	
	if(nVMTicket == NULL || pathName ==NULL)
		return VIX_FALSE;
	
	locJobHandle = VixVM_DeleteDirectoryInGuest(nVMTicket->vmHandle, pathName, 0, NULL, NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(locJobHandle);
	if(VIX_FAILED(locErr)){
		return VIX_FALSE;
	}
	return VIX_TRUE;
}

/* List the directory in VM */
VIX_BOOL ListDirInGuest(VMTICKET* nVMTicket, char* dirpath){
	
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixError locErr;
	int num = -1;
	int i = -1;
	char * fname = NULL;

	locJobHandle = VixVM_ListDirectoryInGuest(nVMTicket->vmHandle, dirpath, 0, NULL, NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	if (VIX_FAILED(locErr)) {
		fprintf(stderr, "failed to list dir '%s' in vm '%s'(%"FMT64"d %s)\n", dirpath, nVMTicket->VMName, locErr, Vix_GetErrorText(locErr, NULL));
		return VIX_FALSE;
	}
	   
	num = VixJob_GetNumProperties(locJobHandle, VIX_PROPERTY_JOB_RESULT_ITEM_NAME);
	for (i = 0; i < num; i++) {
		locErr = VixJob_GetNthProperties(locJobHandle, i, VIX_PROPERTY_JOB_RESULT_ITEM_NAME,&fname, VIX_PROPERTY_NONE);
		fprintf(stderr, "file #%d '%s'\n", i, fname);
		Vix_FreeBuffer(fname);
	}
	Vix_ReleaseHandle(locJobHandle);
	return VIX_TRUE;
}

/* Delete file in the Virtual Machine */
VIX_BOOL DeleteFileInGuest(VMTICKET* nVMTicket, char* pathFileName){
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixError locErr;
	
	if(nVMTicket == NULL || pathFileName ==NULL)
		return VIX_FALSE;
	
	locJobHandle = VixVM_DeleteFileInGuest(nVMTicket->vmHandle, pathFileName, /*0, for windows*/ NULL, NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(locJobHandle);
	if(VIX_FAILED(locErr)){
		return VIX_FALSE;
	}
	return VIX_TRUE;
}

/* Copy file from Virtual Machine to native */
VIX_BOOL CopyFileFromGuestToHost(VMTICKET* nVMTicket, char* srcName, char* dstName){
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixError locErr;
	
	if(nVMTicket == NULL || srcName == NULL || dstName == NULL)
		return VIX_FALSE;
	
	fprintf(stderr, "--- copying from %s to %s\n", srcName, dstName);
	locJobHandle = VixVM_CopyFileFromGuestToHost(nVMTicket->vmHandle, srcName, dstName, 0, VIX_INVALID_HANDLE, NULL, NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(locJobHandle);
	if(VIX_FAILED(locErr)){
		return VIX_FALSE;
	}
	return VIX_TRUE;
}

/* Copy file from native to Virtual Machine */
VIX_BOOL CopyFileFromHostToGuest(VMTICKET* nVMTicket, char* srcName, char* dstName){
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixError locErr;
	
	if(nVMTicket == NULL || srcName == NULL || dstName == NULL)
		return VIX_FALSE;
	
	fprintf(stderr, "--- copying from %s to %s\n", srcName, dstName);
	locJobHandle = VixVM_CopyFileFromHostToGuest(nVMTicket->vmHandle, srcName, dstName, 0, VIX_INVALID_HANDLE, NULL, NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(locJobHandle);
	if(VIX_FAILED(locErr)){
		return VIX_FALSE;
	}
	return VIX_TRUE;
}

/* Clone a registered Virtual Machine with new name */
VIX_BOOL CloneVM(VMTICKET* nVMTicket, char* newVMPath){
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixHandle cloneVMHandle = VIX_INVALID_HANDLE;
	VixError locErr;

	if(nVMTicket == NULL || newVMPath == NULL){
		printf("Wrong parameters\n");
		return VIX_FALSE;
	}

	locJobHandle = VixVM_Clone(nVMTicket->vmHandle,
		VIX_INVALID_HANDLE,
		VIX_CLONETYPE_FULL,
		newVMPath,
		0,
		VIX_INVALID_HANDLE,
		NULL,
		NULL);
	locErr = VixJob_Wait(locJobHandle,
		VIX_PROPERTY_JOB_RESULT_HANDLE,
		&cloneVMHandle,
		VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(locJobHandle);
	Vix_ReleaseHandle(cloneVMHandle);
	if(VIX_FAILED(locErr)){
		return VIX_FALSE;
	}
	return VIX_TRUE;
}

/* Run the program with arguments in the Virtual Machine */
VIX_BOOL RunProgramInGuest(VMTICKET* nVMTicket, char* progName, char* args){
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixError locErr;
	
	if(nVMTicket == NULL || progName == NULL || args == NULL){
		return VIX_FALSE;
	}
	
	locJobHandle = VixVM_RunProgramInGuest(nVMTicket->vmHandle, progName, args, 0, VIX_INVALID_HANDLE, NULL, NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(locJobHandle);
	if(VIX_FAILED(locErr)){
		return VIX_FALSE;
	}
	return VIX_TRUE;
}

/* Run the script with the interpreter in the Virtual Machine */
VIX_BOOL RunScriptInGuest(VMTICKET* nVMTicket, char* interpreter, char* scriptText, int block){
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixError locErr;
	
	if(nVMTicket == NULL || interpreter == NULL || scriptText == NULL)
		return VIX_FALSE;
	
	if(strcmp(interpreter, "null") == 0){
		fprintf(stdout, "Try cmd.exe\n");
		interpreter = NULL;
	}
	if(block == 1)
		locJobHandle = VixVM_RunScriptInGuest(nVMTicket->vmHandle, interpreter, scriptText, 0, VIX_INVALID_HANDLE, NULL, NULL);
	else
		locJobHandle = VixVM_RunScriptInGuest(nVMTicket->vmHandle, interpreter, scriptText, VIX_RUNPROGRAM_RETURN_IMMEDIATELY, VIX_INVALID_HANDLE, NULL, NULL);
	if(block == 1)
		locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(locJobHandle);
	if(VIX_FAILED(locErr)){
		return VIX_FALSE;
	}
	return VIX_TRUE;
}


/* Create an original VM snapshot */
VIX_BOOL CreateVMSnapshot(VMTICKET* nVMTicket, char* ssname, char* ssdesc){
	VixError locErr;
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixHandle ssHandle = VIX_INVALID_HANDLE;
	VIX_BOOL ret = VIX_TRUE;

	if(nVMTicket == NULL || ssname == NULL || ssdesc == NULL){
		fprintf(stdout, "[Error] Arguments error\n");
		return VIX_FALSE;
	}

	//Power On VM
	/*
	locJobHandle = VixVM_PowerOn(nVMTicket->vmHandle, 0, VIX_INVALID_HANDLE, NULL, NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	Vix_ReleaseHandle(locJobHandle);
	*/
	//Create SnapShot
	locJobHandle = VixVM_CreateSnapshot(nVMTicket->vmHandle, ssname, ssdesc, VIX_SNAPSHOT_INCLUDE_MEMORY, VIX_INVALID_HANDLE, NULL, NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_JOB_RESULT_HANDLE, &ssHandle, VIX_PROPERTY_NONE);
	if(VIX_FAILED(locErr)){
		ret = VIX_FALSE;
		printf("Failed to create snapshot for %s!\n", nVMTicket->VMName);
	}
	Vix_ReleaseHandle(locJobHandle);
	Vix_ReleaseHandle(ssHandle);
	return ret;
}

/* Get number of VM snapshot */
int GetVMSnapshotNumber(VMTICKET* nVMTicket){
	int ret = -1;
	VixError locErr;

	if(nVMTicket == NULL){
		fprintf(stdout, "[Error] Arguments error!\n");
		return ret;
	}
	locErr = VixVM_GetNumRootSnapshots(nVMTicket->vmHandle, &ret);
	if(VIX_FAILED(locErr)){
		fprintf(stdout, "[Error] Failed to get the number of root-snapshot of the VM!\n");
		ret = locErr;
	}
	return ret;
}

/* Get child snapshot number of the root snapshot  */
int GetVMSnapChildNumber(VMTICKET* nVMTicket){
	int ret = -1, index;
	int numRootSnapshot = -1;
	VixError locErr;
	VixHandle snapshotHandle = VIX_INVALID_HANDLE;
	
	if(nVMTicket == NULL){
		fprintf(stdout, "[Error] Arguments error!\n");
		return ret;
	}
	// Get the root snapshot
	locErr = VixVM_GetNumRootSnapshots(nVMTicket->vmHandle, &numRootSnapshot);
	if(VIX_FAILED(locErr)){
		fprintf(stdout, "[Error] Failed to get the number of root-snapshot of the VM!\n");
		ret = -1;
		return ret;
	}
	if(numRootSnapshot < 1){
		fprintf(stderr, "No root shapshot found!\n");
		ret = -1;
		return ret;
	}
	locErr = VixVM_GetRootSnapshot(nVMTicket->vmHandle , 0, &snapshotHandle);
	if(VIX_FAILED(locErr)){
		fprintf(stdout, "[Error] Failed to get the first root snapshot!\n");
		ret = -1;
		return ret;
	}

	// Count its children number
	locErr = VixSnapshot_GetNumChildren(snapshotHandle, &ret);
	if(VIX_FAILED(locErr)){
		fprintf(stdout, "[Error] Failed to get the children number of the  root snapshot!\n");
		ret = -1;
		Vix_ReleaseHandle(snapshotHandle);
		return ret;
	}
	Vix_ReleaseHandle(snapshotHandle);
	return ret;
}

/* Get Snapshot by name */
VixHandle GetVMSnapshotByName(VMTICKET * nVMTicket, char* ssname){	
	VixError locErr;
	VixHandle ssHandle = VIX_INVALID_HANDLE;

	locErr = VixVM_GetNamedSnapshot(nVMTicket->vmHandle, ssname, &ssHandle);
	if(VIX_FAILED(locErr)){
		fprintf(stdout, "[Error] Failed to get the Snapshot!\n");
	}
	return ssHandle;
}

int HasVMSnapshot(VMTICKET * nVMTicket, char * ssname){
	VixError locErr;
	VixHandle ssHandle = VIX_INVALID_HANDLE;
	int has = 0;

	locErr = VixVM_GetNamedSnapshot(nVMTicket->vmHandle, ssname, &ssHandle);
	if(VIX_FAILED(locErr)){
		fprintf(stdout, "Can not find the Snapshot!\n");
	}else{
		has = 1;
	}
	Vix_ReleaseHandle(ssHandle);
	return has;
}

/* Revert to named Snapshot */
VIX_BOOL RevertSnapshotByName(VMTICKET * nVMTicket, char* ssname){
	
	VixError locErr;
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixHandle ssHandle = VIX_INVALID_HANDLE;
	VIX_BOOL ret = VIX_TRUE;

	if(nVMTicket == NULL || ssname == NULL){
		fprintf(stdout, "[Error] Arguments Error!\n");
		return VIX_FALSE;
	}
	//Locate the Snapshot by name
	ssHandle = GetVMSnapshotByName(nVMTicket, ssname);
	if(ssHandle == VIX_INVALID_HANDLE){
		fprintf(stdout, "[Error] Failed to get the namded Snapshot!\n");
		return VIX_FALSE;
	}
	fprintf(stdout, "Located and to revert\n");
	//Do Revert
	locJobHandle = VixVM_RevertToSnapshot(nVMTicket->vmHandle, ssHandle, 0, VIX_INVALID_HANDLE, NULL, NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	if(VIX_FAILED(locErr)){
		fprintf(stdout, "[Error] Failed to revert to named Snapshot!\n");
		ret = VIX_FALSE;
	}
	Vix_ReleaseHandle(ssHandle);
	Vix_ReleaseHandle(locJobHandle);
	return ret;
}

VIX_BOOL RemoveSnapshotByName(VMTICKET * nVMTicket, char* ssname){
	VixError locErr;
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixHandle ssHandle = VIX_INVALID_HANDLE;
	VIX_BOOL ret = VIX_TRUE;

	if(nVMTicket == NULL || ssname == NULL){
		fprintf(stdout, "[Error] Arguments Error!\n");
		return VIX_FALSE;
	}

	// Has it?
	if(HasVMSnapshot(nVMTicket, ssname) == 0){
		// Nothing to remove
		return VIX_TRUE;
	}

	//Locate the Snapshot by name
	ssHandle = GetVMSnapshotByName(nVMTicket, ssname);
	if(ssHandle == VIX_INVALID_HANDLE){
		fprintf(stdout, "[Error] Failed to get the named Snapshot!\n");
		return VIX_FALSE;
	}

	//Do remove with its children snapshots
	locJobHandle = VixVM_RemoveSnapshot(nVMTicket->vmHandle, ssHandle, 0, NULL, NULL);
	locErr = VixJob_Wait(locJobHandle, VIX_PROPERTY_NONE);
	if(VIX_FAILED(locErr)){
		fprintf(stderr, "[Error] Failed to remove the named Snapshot!\n");
		ret =  VIX_FALSE;
	}
	Vix_ReleaseHandle(ssHandle);
	Vix_ReleaseHandle(locJobHandle);
	return ret;
}

VIX_BOOL RemoveSnapshotById(VMTICKET * nVMTicket, long ssid){
	VixError locErr;
	VixHandle ssHandle = VIX_INVALID_HANDLE;
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VIX_BOOL ret = VIX_TRUE;

	ssHandle = (VixHandle)ssid;
	locJobHandle = VixVM_RemoveSnapshot(nVMTicket->vmHandle,
		ssHandle,
		0,
		NULL,
		NULL);
	locErr = VixJob_Wait(locJobHandle,VIX_PROPERTY_NONE);
	if(VIX_FAILED(locErr)){
		ret = VIX_FALSE;
	}
	Vix_ReleaseHandle(locJobHandle);
	return ret;	
}

/* Un-install the VMware Tools with complicated operation */
/*
VIX_BOOL UninstallVMwareTools(VMTICKET* nVMTicket, char* srcDeployFile, char* dstDeployFile, char* interpreter, char* scriptText){
	VixHandle locJobHandle = VIX_INVALID_HANDLE;
	VixError locErr;
	
	if(nVMTicket == NULL || srcDeployFile == NULL || dstDeployFile == NULL || interpreter == NULL || scriptText == NULL)
		return VIX_FALSE;
	
	if(CopyFileFromHostToGuest(nVMTicket, srcDeployFile, dstDeployFile)){
		if(RunScriptInGuest(nVMTicket, interpreter, scriptText, 0)){
			printf("Step I OK!\n");
		}else{
			printf("Fail 2\n");
		}
	}else{
		printf("Fail 1\n");
	}
	return VIX_TRUE;
}
*/

