#include "vixSession.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


SESSIONINFO g_sessoninfo;
unsigned char ARGUMENTS[ARGMAXNUM][ARGMAXLEN] = {"\0"};
HOSTTICKET g_conTicket;
VMTICKET g_vmTicket;
char g_vmxPath[MAX_NAMELEN] = "\0";
char g_vmxPath2[MAX_NAMELEN] = "\0";
char g_dcPath[MAX_NAMELEN] = "\0";
char g_vmospath[MAXPATHLEN] = "\0";
char g_tmpfilepath[MAXPATHLEN] = "\0";
int g_tmpfd = NULL;


int cmd_name_to_index(const char * cmdname){
	int index = -1, i;
	for(i = 0;CMDNAME_LIST[i] != NULL;i++){
		if(strncmp(cmdname, CMDNAME_LIST[i], strlen(cmdname)) == 0){
			index = i;
			break;
		}
	}
	return index;
}
	

int parse_request(char * request){
	char * token;
	int get_cmd = 0;
	int argnum = 0;
	int cmd_id = -1;
	int logfd;

	int cmd_argnum;

	logfd = openlog();
	token = strtok(request, SPLIT);
	while(token != NULL){
		if(get_cmd == 0){
			cmd_id = cmd_name_to_index(token);
			if(cmd_id == -1)
				break;
			get_cmd = 1;
		}else{
			cmd_argnum = CMDARGNUM_LIST[cmd_id];
			strcpy(&ARGUMENTS[argnum], token);
			argnum++;
		}
		token = strtok(NULL, SPLIT);
	}
	bzero(&ARGUMENTS[argnum], ARGMAXLEN);
	closelog(logfd);
	return cmd_id;
}


void VixListGuestsProc(VixHandle jobHandle,VixEventType eventType,VixHandle moreEventInfo,void *clientData){
	VixError err = VIX_OK;
	char *url = NULL;
	char tmpbuf[BUFMAXLEN] = "\0";
	int slen = 0;

	int logfd = openlog();
	// Check callback event; ignore progress reports.
	if (VIX_EVENTTYPE_FIND_ITEM != eventType) {
		dumplog(logfd, "No mine!");
		fprintf(stderr, "No mine!\n");
		closelog(logfd);
		return;
	}

	// Found a virtual machine.
	err = Vix_GetProperties(moreEventInfo,
                           VIX_PROPERTY_FOUND_ITEM_LOCATION,
                           &url,
                           VIX_PROPERTY_NONE);
	if (VIX_OK != err) {
		// Handle the error...
		fprintf(stderr, "Failed to discovery!\n");
		goto abort;
	}
	if(url != NULL){
		sprintf(tmpbuf, "%s;", url);		
		write(g_tmpfd, tmpbuf,strlen(tmpbuf));
	}

abort:
	closelog(logfd);
	Vix_FreeBuffer(url);

}

int VixListSnapshotsProc(VMTICKET * vmTicket, int fd){
	/* Traverse Snapshots Tree of vm and save into global file as list */
	int ret = -1, i;
	int numRootSnapshots = -1;
	VixError err = VIX_OK;
	VixHandle vmHandle = VIX_INVALID_HANDLE;
	VixHandle snapshotHandle = VIX_INVALID_HANDLE;
	VixHandle childSnapshotHandle = VIX_INVALID_HANDLE;
	char * snapshotName = NULL;
	char * snapshotDesc = NULL;
	char buffer[BUFMAXLEN] = "\0";
	char ssName[MAX_NAMELEN] = "\0";
	char ssDesc[MAX_URLLEN] = "\0";	
	INFOLIST il;

	int dfd = openlog();

	if(vmTicket == NULL || fd == NULL){
		closelog(dfd);
		return ret;
	}
	vmHandle = vmTicket->vmHandle;
	
	InitInfoList(&il);

	dumplog(dfd, "Start Tree");
	err = VixVM_GetNumRootSnapshots(vmHandle,&numRootSnapshots);
	if(VIX_OK != err){
		goto abort;
	}
	dumplogint(dfd, numRootSnapshots);
	for(i=0;i<numRootSnapshots;i++){
		err = VixVM_GetRootSnapshot(vmHandle,i,&snapshotHandle);
		if(VIX_OK != err){
			goto abort;
		}
		err = Vix_GetProperties(snapshotHandle,VIX_PROPERTY_SNAPSHOT_DISPLAYNAME, &snapshotName, VIX_PROPERTY_SNAPSHOT_DESCRIPTION, &snapshotDesc, VIX_PROPERTY_NONE);
		if(VIX_OK != err){
			goto abort;
		}
		if(snapshotDesc == NULL){
			snapshotDesc = (char *)malloc(sizeof(null) + 1);
			bzero(snapshotDesc, sizeof(null) + 1);
			strcpy(snapshotDesc, null);
		}
		AppendInfoNode(&il,snapshotName,snapshotDesc);
		bzero(buffer, sizeof(buffer));
		sprintf(buffer, "%s\t%s\tNone;", snapshotName, snapshotDesc);
		dumplog(dfd, buffer);
		Vix_FreeBuffer(snapshotName);
		Vix_FreeBuffer(snapshotDesc);
		ret++;
		write(fd, buffer, strlen(buffer));
	}
	while(PickInfoNode(&il, ssName, ssDesc) == 1){
		dumplog(dfd, "ss in queue");
		dumplog(dfd, ssName);
		err = VixVM_GetNamedSnapshot(vmHandle,ssName,&snapshotHandle);
		if(VIX_OK != err){
			goto abort;
		}
		err = VixSnapshot_GetNumChildren(snapshotHandle,&numRootSnapshots);
		if(VIX_OK != err){
			goto abort;
		}
		dumplog(dfd, "numRootSnapshots");
		dumplogint(dfd, numRootSnapshots);
		if(numRootSnapshots < 1)
			continue;
		for(i=0;i<numRootSnapshots;i++){
			err = VixSnapshot_GetChild(snapshotHandle,i,&childSnapshotHandle);
			if(VIX_OK != err){
				goto abort;
			}
			err = Vix_GetProperties(childSnapshotHandle,VIX_PROPERTY_SNAPSHOT_DISPLAYNAME, &snapshotName, VIX_PROPERTY_SNAPSHOT_DESCRIPTION, &snapshotDesc, VIX_PROPERTY_NONE);
			if(VIX_OK != err){
				goto abort;
			}			
			if(snapshotDesc == NULL){
				snapshotDesc = (char *)malloc(sizeof(null) + 1);
				bzero(snapshotDesc, sizeof(null) + 1);
				strcpy(snapshotDesc, null);
			}
			AppendInfoNode(&il,snapshotName,snapshotDesc);
			bzero(buffer, sizeof(buffer));
			sprintf(buffer, "%s\t%s\t%s;", snapshotName, snapshotDesc, ssName);
			dumplog(dfd, buffer);
			Vix_FreeBuffer(snapshotName);
			Vix_FreeBuffer(snapshotDesc);
			ret++;
			write(fd, buffer, strlen(buffer));
		}
	}
	
	
abort:
	closelog(dfd);
	FreeInfoList(&il);
	if(snapshotHandle)
		Vix_ReleaseHandle(snapshotHandle);
	if(childSnapshotHandle){
		Vix_ReleaseHandle(childSnapshotHandle);
	}

	return ret;
}

int handle_esxi_session(char * request, char * response){
	int ret = VE_000;
	int vixcmd_id = -1;
	int logfd;
	char logbuf[BUFMAXLEN] = "\0";
	char parambuf[BUFMAXLEN] = "\0";
	char resp[MAXPACKLEN] = "\0";
	int rfd;
	int len;
	int trynum;
	char tmpfilepath[] = "/tmp/bfile";
	char md5val1[50] = "\0";
	char md5val2[50] = "\0";
	VMPOWER powerstate;
	int ssnum = -1;
	char * vmName = NULL;
	char * vmGuestOs = NULL;
	char * ssName = NULL;
	char * ssDesc = NULL;
	VixError err = VIX_OK;
	VixHandle snapshotHandle = VIX_PROPERTY_NONE;


	logfd = openlog();
	dumplog(logfd, "In handle_esxi_session");

	vixcmd_id = parse_request(request);

	dumplogint(logfd, vixcmd_id);

	bzero(logbuf, sizeof(logbuf));
	bzero(resp, sizeof(resp));
	sprintf(g_tmpfilepath, "%s/%d", varpath, getpid());
	bzero(logbuf, sizeof(logbuf));
	switch(vixcmd_id){
		case CID_TEST:
			break;
		case CID_CONNECT:
			bzero(&g_sessoninfo, sizeof(g_sessoninfo));
			sprintf(logbuf, "To connect esxi %s with %s/%s ...", ARGUMENTS[0], ARGUMENTS[1], ARGUMENTS[2]);
			dumplog(logfd, logbuf);
			if(g_sessoninfo.state& CONNECTED_ESXI){
				dumplog(logfd, "Already connected!");
			}else{
				strcpy(g_sessoninfo.host_ip, ARGUMENTS[0]);
				strcpy(g_sessoninfo.host_user, ARGUMENTS[1]);
				strcpy(g_sessoninfo.host_passwd, ARGUMENTS[2]);
				sprintf(parambuf, "http://%s/sdk",g_sessoninfo.host_ip);
				if(VIX_TRUE == NewHostTicket(parambuf,0,g_sessoninfo.host_user,g_sessoninfo.host_passwd, &g_conTicket)){
					if(HostConnect(&g_conTicket) == VIX_TRUE){
						g_sessoninfo.state = g_sessoninfo.state | CONNECTED_ESXI;
						dumplog(logfd,"Connected");
					}else{
						dumplog(logfd,"Failed to connect!");
						ret = VE_002;
					}
				}else{
					dumplog(logfd, "Failed to init Host Ticket!");
					ret = VE_001;
				}
			}
			break;
		case CID_DISCONNECT:
			dumplog(logfd, "To Disconnected");
			if(g_sessoninfo.state & CONNECTED_ESXI){
				CloseVM(&g_vmTicket);
				if(HostDisconnect(&g_conTicket) == VIX_TRUE){
					g_sessoninfo.state = g_sessoninfo.state - CONNECTED_ESXI;
					dumplog(logfd,"Disconnected!");
				}else{
					dumplog(logfd, "Failed to disconnect!");
					ret = VE_003;
				}
			}
			break;
		case CID_LISTGUESTS:
			dumplog(logfd, "To list guests");			
			//path
			mkdir(varpath, 0777);
			if(g_sessoninfo.state & CONNECTED_ESXI){
				g_tmpfd = open(g_tmpfilepath, O_CREAT|O_TRUNC|O_WRONLY);
				if(HostFindItems(&g_conTicket, VIX_FIND_REGISTERED_VMS, VixListGuestsProc) == VIX_TRUE){
					dumplog(logfd, "Got registed vms");
				}else{
					dumplog(logfd, "Failed to get registered vms");
					ret = VE_005;
				}
				if(g_tmpfd){
					close(g_tmpfd);
				}
			}else{
				dumplog(logfd, "Not ready!");
				ret = VE_004;
			}
			sprintf(resp, "F:%s", g_tmpfilepath);
			break;
		case CID_OPEN:
			sprintf(logbuf, "To open %s", ARGUMENTS[0]);
			dumplog(logfd, logbuf);
			if(g_sessoninfo.state & CONNECTED_ESXI){
				CloseVM(&g_vmTicket);
				if(VIX_TRUE == OpenVM(&g_conTicket, ARGUMENTS[0], &g_vmTicket)){
					strcpy(g_sessoninfo.vmx_path, ARGUMENTS[0]);
					g_sessoninfo.state = g_sessoninfo.state | OPENED_VM;
					dumplog(logfd, "Opened");
					// Get properties as necessary
					err = Vix_GetProperties(g_vmTicket.vmHandle,
					VIX_PROPERTY_VM_NAME,
					&vmName,
					VIX_PROPERTY_VM_GUESTOS,
					&vmGuestOs,
					VIX_PROPERTY_NONE);
					sprintf(resp, "%s;%s", vmName, vmGuestOs);
					Vix_FreeBuffer(vmName);
					Vix_FreeBuffer(vmGuestOs);					
				}else{
					dumplog(logfd, "Failed to open vm!");
					ret = VE_006;
				}
			}else{
				dumplog(logfd, "Not ready!");
				ret = VE_004;
			}
			break;
		case CID_REGIST_VM:
			sprintf(logbuf, "To register %s", ARGUMENTS[0]);
			dumplog(logfd, logbuf);
			if(g_sessoninfo.state & CONNECTED_ESXI){
				if(RegisterVM(&g_conTicket, ARGUMENTS[0]) == VIX_TRUE){
					dumplog(logfd, "Registered");
				}else{
					dumplog(logfd, "Failed to register!");
					ret = VE_007;
				}
			}else{
				dumplog(logfd, "Not connected to ESXi!");
				ret = VE_004;
			}			
			break;
		case CID_UNREGIST_VM:
			sprintf(logbuf, "To un-register %s", ARGUMENTS[0]);
			dumplog(logfd, logbuf);
			if(g_sessoninfo.state & CONNECTED_ESXI){
				if(UnregisterVM(&g_conTicket, ARGUMENTS[0]) == VIX_TRUE){
					dumplog(logfd, "UN-Registered");
				}else{
					dumplog(logfd, "Failed to un-register!");
					ret = VE_008;
				}
			}else{
				dumplog(logfd, "Not connected to ESXi!");
				ret = VE_004;
			}			
			break;
		case CID_CLOSE:
			sprintf(logbuf, "To close %s", g_sessoninfo.vmx_path);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				CloseVM(&g_conTicket);
				g_sessoninfo.state = g_sessoninfo.state - OPENED_VM;
				dumplog(logfd, "Closed");
			}else{
				dumplog(logfd, "Not connected to ESXi!");
				ret = VE_004;
			}
			break;
		case CID_DELETE_VM:
			sprintf(logbuf, "To delete %s", g_sessoninfo.vmx_path);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				DeleteVM(&g_vmTicket);
				g_sessoninfo.state = g_sessoninfo.state - OPENED_VM;
				dumplog(logfd, "Deleted");
			}else{
				dumplog(logfd, "Not connected to ESXi!");
				ret = VE_004;
			}			
			break;
		case CID_POWERON:
			sprintf(logbuf, "To power-on %s", g_sessoninfo.vmx_path);
			dumplog(logfd, logbuf);			
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				bzero(logbuf, sizeof(logbuf));
				if(GetVMPowerState(&g_vmTicket,logbuf) != POWERD_OFF){
					dumplog(logfd, logbuf);
				}else{
					if(OperateVM(&g_vmTicket,POWERON) == VIX_TRUE){
						dumplog(logfd, "Done");
					}else{
						dumplog(logfd, "Failed to power on");
						ret = VE_010;
					}
				}
			}else{
				dumplog(logfd, "Not connected to ESXi!");
				ret = VE_004;
			}			
			break;			
		case CID_POWEROFF:
			sprintf(logbuf, "To power-off %s", g_sessoninfo.vmx_path);
			dumplog(logfd, logbuf);		
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				bzero(logbuf, sizeof(logbuf));
				if(GetVMPowerState(&g_vmTicket,logbuf) != POWERED_ON){
					dumplog(logfd, logbuf);
					ret = VE_010;
				}else{
					if(OperateVM(&g_vmTicket,POWEROFF) == VIX_TRUE){
						dumplog(logfd, "Done");
					}else{
						dumplog(logfd, "Failed to power off");
						ret = VE_010;
					}
				}
			}else{
				dumplog(logfd, "Not connected to ESXi!");
				ret = VE_004;
			}
			break;
		case CID_POWER_STATE:
			sprintf(logbuf, "To get powerstate of %s", g_sessoninfo.vmx_path);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				bzero(logbuf, sizeof(logbuf));
				powerstate = GetVMPowerState(&g_vmTicket,logbuf);
				if(powerstate == PWUNKNOWN){
					dumplog(logfd, "Unkown power state");
					sprintf(resp, "%d-%s", powerstate, logbuf);
					ret = VE_010;
				}else{
					sprintf(resp, "%d", powerstate);
				}
			}else{
				dumplog(logfd, "Not connected to ESXi!");
				ret = VE_004;
			}
			break;
		case CID_LOGIN:
			sprintf(logbuf, "To login vm with %s/%s", ARGUMENTS[0], ARGUMENTS[1]);
			dumplog(logfd, logbuf);			
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				if(LoginVM(&g_vmTicket,ARGUMENTS[0],ARGUMENTS[1]) == VIX_TRUE){
					dumplog(logfd,"Logined");
					g_sessoninfo.state = g_sessoninfo.state | LOGINED_VM;
				}else{
					dumplog(logfd,"Failed to login");
					ret = VE_011;
				}
			}else{
				dumplog(logfd, "Not connected to ESXi!");
				ret = VE_004;
			}
			break;
		case CID_LOGOUT:
			sprintf(logbuf, "To logout vm");
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)&&(g_sessoninfo.state & LOGINED_VM)){
				if(LogoutVM(&g_vmTicket) == VIX_TRUE){
					g_sessoninfo.state= g_sessoninfo.state - LOGINED_VM;
					dumplog(logfd, "Logout");
				}else{
					dumplog(logfd, "Failed to logout");
					ret = VE_012;
				}
			}
			break;
		case CID_PUT:
			sprintf(logbuf, "To put in %s -> %s", ARGUMENTS[0], ARGUMENTS[1]);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)&&(g_sessoninfo.state & LOGINED_VM)){
				if(CopyFileFromHostToGuest(&g_vmTicket,ARGUMENTS[0],ARGUMENTS[1]) == VIX_TRUE){
					dumplog(logfd, "OK");
				}else{
					dumplog(logfd, "Failed");
					ret = VE_013;
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}			
			break;
		case CID_GET:
			sprintf(logbuf, "To get from %s -> %s", ARGUMENTS[0], ARGUMENTS[1]);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)&&(g_sessoninfo.state & LOGINED_VM)){
				if(CopyFileFromGuestToHost(&g_vmTicket,ARGUMENTS[0],ARGUMENTS[1]) == VIX_TRUE){
					dumplog(logfd, "OK");
				}else{
					dumplog(logfd, "Failed");
					ret = VE_013;
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}			
			break;
		case CID_HPUT:
			sprintf(logbuf, "To hput in %s -> %s", ARGUMENTS[0], ARGUMENTS[1]);
			dumplog(logfd, logbuf);
			ret = VE_013;
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)&&(g_sessoninfo.state & LOGINED_VM)){
				for(trynum = 0; trynum < 3; trynum++){
					if(CopyFileFromHostToGuest(&g_vmTicket,ARGUMENTS[0],ARGUMENTS[1]) == VIX_TRUE){
						if(CopyFileFromGuestToHost(&g_vmTicket,ARGUMENTS[1],tmpfilepath) == VIX_TRUE){
							if((md5file(ARGUMENTS[0], md5val1) == 1)&&(md5file(tmpfilepath, md5val2) == 1)){
								if(strcmp(md5val1, md5val2) == 0){
									dumplog(logfd, "OK");
									ret = VE_000;
									break;
								}else{
									dumplog(logfd, "Failed to md5 match");
								}
							}else{
								dumplog(logfd, "Failed to md5sum");
							}
						}else{
							dumplog(logfd, "Failed to get out file");
						}
					}else{
						dumplog(logfd,"Failed to put in file");
					}
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}
			break;
		case CID_BAT:
			sprintf(logbuf, "To bat %s with %s", ARGUMENTS[0], ARGUMENTS[1]);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)&&(g_sessoninfo.state & LOGINED_VM)){
				if(RunScriptInGuest(&g_vmTicket,"null",ARGUMENTS[0],atoi(ARGUMENTS[1])) == VIX_TRUE){
					dumplog(logfd, "OK");
				}else{
					dumplog(logfd, "Failed");
					ret = VE_014;
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}
			break;
		case CID_BASH:
			sprintf(logbuf, "To bash %s with %s", ARGUMENTS[0], ARGUMENTS[1]);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)&&(g_sessoninfo.state & LOGINED_VM)){
				if(RunScriptInGuest(&g_vmTicket,"/bin/sh",ARGUMENTS[0],atoi(ARGUMENTS[1])) == VIX_TRUE){
					dumplog(logfd, "OK");
				}else{
					dumplog(logfd, "Failed");
					ret = VE_014;
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}
			break;
		case CID_CD:
			bzero(g_vmospath, sizeof(g_vmospath));
			strcpy(g_vmospath, ARGUMENTS[0]);
			break;
		case CID_DIR:
			sprintf(logbuf, "To dir %s in debug mode", g_vmospath);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)&&(g_sessoninfo.state & LOGINED_VM)){
				if(ListDirInGuest(&g_vmTicket, g_vmospath)== VIX_TRUE){
					dumplog(logfd, "OK");
				}else{
					dumplog(logfd, "Failed");
					ret = VE_014;
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}
			break;
		case CID_CREATESS:
			sprintf(logbuf, "To create snapshot %s(%s) for %s", ARGUMENTS[0], ARGUMENTS[1], g_sessoninfo.vmx_path);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				if(CreateVMSnapshot(&g_vmTicket,ARGUMENTS[0],ARGUMENTS[1]) == VIX_TRUE){
					dumplog(logfd, "OK");
				}else{
					dumplog(logfd, "Failed");
					ret = VE_015;
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}
			break;
		case CID_SSRNUM:
			sprintf(logbuf, "To get root snapshot num of %s", g_sessoninfo.vmx_path);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				ssnum = GetVMSnapshotNumber(&g_vmTicket);
				if(ssnum > -1){
					sprintf(resp, "%d", ssnum);
				}else{
					dumplog(logfd, "Failed");
					ret = VE_015;
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}
			break;
		case CID_SSNUM:
			sprintf(logbuf, "To get child snapshot num of %s", g_sessoninfo.vmx_path);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				ssnum = GetVMSnapChildNumber(&g_vmTicket);
				if(ssnum > -1){
					sprintf(resp, "%d", ssnum);
					dumplog(logfd,"OK");
					dumplogint(logfd,ssnum);
				}else{
					dumplog(logfd, "Failed");
					ret = VE_015;
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}
			break;
		case CID_GOTOSS:
			sprintf(logbuf, "To revert to snapshot %s of %s", ARGUMENTS[0], g_sessoninfo.vmx_path);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				if(RevertSnapshotByName(&g_vmTicket,ARGUMENTS[0]) == VIX_TRUE){
					dumplog(logfd, "OK");
				}else{
					dumplog(logfd, "Failed");
					ret = VE_015;
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}			
			break;
		case CID_RMSS:
			sprintf(logbuf, "To delete snapshot %s of %s", ARGUMENTS[0], g_sessoninfo.vmx_path);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				if(RemoveSnapshotByName(&g_vmTicket,ARGUMENTS[0]) == VIX_TRUE){
					dumplog(logfd, "OK");
				}else{
					dumplog(logfd, "Failed");
					ret = VE_015;
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}				
			break;
		case CID_SSLIST:
			sprintf(logbuf, "To tree snapshots of %s", g_sessoninfo.vmx_path);
			dumplog(logfd, logbuf);
			mkdir(varpath, 0777);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				g_tmpfd = open(g_tmpfilepath, O_CREAT|O_TRUNC|O_WRONLY);
				if(VixListSnapshotsProc(&g_vmTicket,g_tmpfd) > -1){
					dumplog(logfd, "OK");
				}else{
					dumplog(logfd, "Failed");
					ret = VE_015;
				}
				if(g_tmpfd){
					close(g_tmpfd);
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}
			sprintf(resp, "F:%s", g_tmpfilepath);			
			break;
		case CID_GETNAMESS:
			sprintf(logbuf, "To get named snapshot %s of %s", ARGUMENTS[0], g_sessoninfo.vmx_path);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				snapshotHandle = GetVMSnapshotByName(&g_vmTicket,ARGUMENTS[0]);
				if(snapshotHandle != VIX_PROPERTY_NONE){
					// get property infos
					dumplog(logfd, "Got");
					// Get properties as necessary
					err = Vix_GetProperties(snapshotHandle,
					VIX_PROPERTY_SNAPSHOT_DISPLAYNAME,
					&ssName,
					VIX_PROPERTY_SNAPSHOT_DESCRIPTION,
					&ssDesc,
					VIX_PROPERTY_NONE);
					sprintf(resp, "%d;%s;%s", snapshotHandle, ssName, ssDesc);
					Vix_FreeBuffer(ssName);
					Vix_FreeBuffer(ssDesc);					
				}else{
					dumplog(logfd, "Failed");
					ret = VE_015;
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}			
			break;
		case CID_DELSSID:
			sprintf(logbuf, "To delete snapshod id %s", ARGUMENTS[0]);
			dumplog(logfd, logbuf);
			if((g_sessoninfo.state & CONNECTED_ESXI)&&(g_sessoninfo.state & OPENED_VM)){
				if(RemoveSnapshotById(&g_vmTicket,atol(ARGUMENTS[0])) == VIX_TRUE){
					dumplog(logfd, "Deleted");
					
				}else{
					dumplog(logfd, "Failed");
					ret = VE_015;
				}
			}else{
				dumplog(logfd, "Not ready");
				ret = VE_004;
			}				
			break;
		default:
			sprintf(logbuf, "Unkown request:%s", request);
			dumplog(logfd, logbuf);
			break;
	}
	
	sprintf(response, "[%d]%s", ret, resp);
	closelog(logfd);
	return ret;
}




