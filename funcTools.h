#ifndef _FUNC_TOOL
#define _FUNC_TOOL

#include "config.h"
#include "md5.h"
#include "vixVarStruct.h"



/* About md5sum */
int md5file(const char * filepath, char * md5val);

/* About Log handle */
int openlog();
int dumplog(int logfd, const char * logmsg);
int dumplogint(int logfd, int intger);
void closelog(int logfd);


/* Extended Data List Operations */
void InitInfoList(INFOLIST * infoList);

void FreeInfoList(INFOLIST * infoList);

void AppendInfoNode(INFOLIST * infoList, char * infoName, char * infoValue);

int PickInfoNode(INFOLIST * infoList, char * infoName, char * infoValue);

int TraverseInfoList(INFOLIST * infoList);




#endif


