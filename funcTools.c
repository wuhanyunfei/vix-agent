#include "funcTools.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>



// core file name of log
const char logfilepath[]="/tmp/vmmengine.log";



int md5file(const char * filepath, char * md5val){
	int result = -1;
	FILE *fpr = NULL;
	int i, j;
	unsigned char buffer[16384], signature[16], csig[16];
	struct MD5Context md5c;
	char hex[3] = "\0";

	fpr = fopen(filepath, "rb");
	if(fpr == NULL){
		fprintf(stderr, "Failed to open file %s\n", filepath);
		return result;
	}

	MD5Init(&md5c);
	while((i = (int) fread(buffer, 1, sizeof(buffer), fpr)) > 0){
		MD5Update(&md5c, buffer, (unsigned)i);
	}
	fclose(fpr);
	MD5Final(signature, &md5c);

	for(j =0;j<sizeof(signature);j++){
		sprintf(hex, "%02x", signature[j]);
		md5val[j * 2] = hex[0];
		md5val[j * 2 + 1] = hex[1];
	}
	md5val[sizeof(signature) * 2] = 0;
	return 1;
	
}

int openlog(){
	char real_logfilepath[MAXPATHLEN] = "\0";
	struct tm *tstruct;
	time_t tsec;
	int fd = -1;

	tsec = time(NULL);
	tstruct = localtime(&tsec);

	sprintf(real_logfilepath, "%s.%04d-%02d-%02d", logfilepath, 1900+tstruct->tm_year, 1 + tstruct->tm_mon, tstruct->tm_mday);
	fd = open(real_logfilepath, O_CREAT|O_APPEND|O_WRONLY);
	return fd;
}

int dumplog(int logfd, const char * logmsg){
	struct tm *tstruct;
	time_t tsec;
	int fd = -1;
	char buf[BUFMAXLEN] = "\0";

	tsec = time(NULL);
	tstruct = localtime(&tsec);

	sprintf(buf, "[PID %d - %02d:%02d:%02d]: %s\n", getpid(), tstruct->tm_hour, tstruct->tm_min, tstruct->tm_sec, logmsg);
	return write(logfd, buf,strlen(buf));
	
}

int dumplogint(int logfd, int intger){
	struct tm *tstruct;
	time_t tsec;
	int fd = -1;
	char buf[BUFMAXLEN] = "\0";

	tsec = time(NULL);
	tstruct = localtime(&tsec);

	sprintf(buf, "[PID %d - %02d:%02d:%02d]: %d\n", getpid(), tstruct->tm_hour, tstruct->tm_min, tstruct->tm_sec, intger);
	return write(logfd, buf,strlen(buf));
	
}


void closelog(int logfd){
	close(logfd);
}

void InitInfoList(INFOLIST * infoList){
	if(infoList == NULL)
		return;
	infoList->ListHead = NULL;
	infoList->ListTail = NULL;
}

void AppendInfoNode(INFOLIST * infoList, char * infoName, char * infoValue){
	INFONODE * infoNode = NULL;

	int dfd = openlog();
	dumplog(dfd, "In AppendInfoNode");
	
	if(infoList == NULL || infoName == NULL || infoValue == NULL) {/* Invalid Parameters */
		dumplog(dfd, "A");
		closelog(dfd);
		return;
	}
	infoNode = (INFONODE *)malloc(sizeof(INFONODE));
	//memset(infoNode, 0, sizeof(INFONODE));
	if(infoNode == NULL) {/* malloc failed */
		dumplog(dfd, "B");
		closelog(dfd);
		return;
	}
	/* Set Values */
	strncpy(infoNode->Name, infoName, MAX_NAMELEN);
	strncpy(infoNode->Value, infoValue, MAX_URLLEN);
	infoNode->Next = NULL;
	dumplog(dfd, "C");
	
	if(infoList->ListHead == NULL || infoList->ListTail == NULL){
		dumplog(dfd, "D");
		// Fist time, make init node
		infoList->ListHead = infoNode;
		infoList->ListTail = infoNode;
		dumplog(dfd, "E");
	}else{
		// Append Data
		dumplog(dfd, "F");
		infoList->ListTail->Next = infoNode;
		infoList->ListTail = infoNode;
		dumplog(dfd, "G");
	}
	dumplog(dfd, "H");

	closelog(dfd);
}

void FreeInfoList(INFOLIST * infoList){
	INFONODE * preptr = infoList->ListHead;
	INFONODE * delptr = NULL;
	while(preptr != infoList->ListTail){
		delptr = preptr;
		preptr = preptr->Next;
		free(delptr);
	}
	if(preptr != NULL){
		free(preptr);
	}
	infoList->ListHead = NULL;
	infoList->ListTail = NULL;
}

int PickInfoNode(INFOLIST * infoList,char * infoName,char * infoValue){
	int ret = -1;
	INFONODE * preptr = infoList->ListHead;

	int dfd = openlog();
	dumplog(dfd, "In PickInfoNode");

	if(preptr == NULL || infoName == NULL || infoValue == NULL){
		dumplog(dfd, "null pointer");
		ret = 0;
	}else{
		dumplog(dfd, "O");
		bzero(infoName, strlen(infoName));
		bzero(infoValue, strlen(infoValue));
		strcpy(infoName, preptr->Name);
		strcpy(infoValue, preptr->Value);
		dumplog(dfd, "T");
		if(preptr != infoList->ListTail){
			dumplog(dfd, "A");
			infoList->ListHead = preptr->Next;
		}else{
			dumplog(dfd, "B");
			infoList->ListHead = NULL;
			infoList->ListTail = NULL;
		}
		free(preptr);
		ret = 1;
	}
	closelog(dfd);
	return ret;
}

int TraverseInfoList(INFOLIST * infoList){
	int ret = -1;
	INFONODE * ptr = infoList->ListHead;
	if(ptr != NULL){
		printf("%s = %s\n", ptr->Name, ptr->Value);
		do{
			ptr = ptr->Next;
			printf("%s = %s\n", ptr->Name, ptr->Value);
		}while(ptr!=infoList->ListTail);
	}
}


