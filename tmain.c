#include <stdio.h>
#include <string.h>

#include "config.h"
#include "vixVarStruct.h"
#include "vixHandler.h"
#include "funcTools.h"



int main(int argc, char * argv[]){
	/*
	int i = 0;
	char hostIP[MAX_NAMELEN] = "\0";
	char hostPass[MAX_PASSLEN] = "\0";
	HOSTTICKET* conTicket = NULL;
	VMTICKET* vmTicket = NULL;
	INFOLIST infoList;
	INFONODE * ptr = NULL;
	VMPOWER vmp;
	int t = 0x0008;
	
	printf("Hello VMM Engine! %d\n", t);
	if(argc == 3){
		strcpy(hostIP, argv[1]);
		strcpy(hostPass, argv[2]);
	}
	printf("Connect %s with root/%s\n" , hostIP, hostPass);
	conTicket = NewHostTicket(hostIP,0,"root",hostPass);
	if(HostConnect(conTicket) == VIX_TRUE){
		printf("Connected!\n");
		vmTicket = OpenVM(conTicket,"[ha-datacenter/datastore2] POT.HS.WindowsServer2003EEASP_8/POT.HS.WindowsServer2003EEASP_0.vmx");
		printf("Opened\n");

		vmp = GetVMPowerState(vmTicket,NULL);
		printf("Powerstate: %d\n", vmp);
		
		CloseVM(vmTicket);
		printf("Closed\n");
		HostDisconnect(conTicket);
		printf("Disconnected!\n");
	}
	*/
	INFOLIST il;
	char name[10] = "\0";
	char value[10] = "\0";
	InitInfoList(&il);
	AppendInfoNode(&il,"Name1","John1");
	AppendInfoNode(&il,"Name2","John2");
	AppendInfoNode(&il,"Name3","John3");
	AppendInfoNode(&il,"Name4","John4");
	AppendInfoNode(&il,"Name5","John5");
	TraverseInfoList(&il);
	PickInfoNode(&il,name,value);
	printf("Picked %s = %s\n", name, value);
	TraverseInfoList(&il);
	
	PickInfoNode(&il,name,value);
	printf("Picked %s = %s\n", name, value);
	TraverseInfoList(&il);
	
	FreeInfoList(&il);
	
	
	printf("Done.\n");
	return 0;
}



