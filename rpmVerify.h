/* 	$Id: rpmVerify.h 20996 1999-05-07 14:13:11Z toivo $	 */

#ifndef H_VERIFY
#define H_VERIFY

#include "../config.h"
extern "C++"
{
#include <qlist.h>
}

extern "C"
{
#include <rpm/rpmlib.h>
}

#define VERIFY_FILES		(1 << 0)
#define VERIFY_DEPS		(1 << 1)
#define VERIFY_SCRIPT		(1 << 2)
#define VERIFY_MD5		(1 << 3)

enum verifysources { VERIFY_PATH, VERIFY_PACKAGE, VERIFY_EVERY, VERIFY_RPM, 
			VERIFY_GRP };

int doVerify(const char * prefix, enum verifysources source, const char ** argv,
	      int verifyFlags, QList<char>* result);

#endif
