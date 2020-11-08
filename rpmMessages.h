/* 	$Id: rpmMessages.h 20996 1999-05-07 14:13:11Z toivo $	 */
#ifndef H_MESSAGES
#define H_MESSAGES

#include "config.h"
/* all the rest of what was here moved to rpmlib.h */

#ifndef  RPMTAG_EPOCH
void rpmMessage(int level, char * format, ...);
#endif

 void rpmMess(char *buff);
 void rpmBuff(char *buff);
 void rpmErr();

#endif
