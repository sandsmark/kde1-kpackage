/* 	$Id: rpmInstall.h 20996 1999-05-07 14:13:11Z toivo $	 */
#ifndef _H_INSTALL
#define _H_INSTALL

#include <stdio.h>
#include "../config.h"

class QString;

extern "C"
{
#include <fcntl.h>
#include <rpm/rpmlib.h>
#include "rpmVerify.h"
#include "rpmMessages.h"
}
 
#define INSTALL_PERCENT         (1 << 0)
#define INSTALL_HASH            (1 << 1)
#define INSTALL_NODEPS          (1 << 2)
#define INSTALL_NOORDER         (1 << 3)
#define INSTALL_LABEL           (1 << 4)  /* set if we're being verbose */
#define INSTALL_UPGRADE         (1 << 5)
 
#define UNINSTALL_NODEPS        (1 << 0)
#define UNINSTALL_ALLMATCHES    (1 << 1)

#ifdef  RPMTAG_EPOCH
int doInstal(const char * rootdir, const char ** argv, int installFlags,
              int interfaceFlags, int probFilter, rpmRelocation * relocations);
int doUninstal(const char * rootdir, const char ** argv, int uninstallFlags,
                 int interfaceFlags);
int doSourceInstall(const char * prefix, const char * arg, const char ** specFile,
                    char ** cookie);

void printDepFlags(FILE * f,  QString s, const char * version, int flags);
void printDepProblems(FILE * f, struct rpmDependencyConflict * conflicts,
                             int numConflicts);
#else
int doInstal(char * rootdir, char ** argv, int installFlags,
             int interfaceFlags);
int doSourceInstall(char * prefix, char * arg, char ** specFile);
int doUninstal(char * rootdir, char ** argv, int uninstallFlags,
                int interfaceFlags);
#endif
 
extern "C" {
 void rpmMess(char *buff);
 void rpmBuff(char *buff);
 void rpmErr();
}

#define FREE(x) { if (x) free((void *)x); x = NULL; }       
#endif

