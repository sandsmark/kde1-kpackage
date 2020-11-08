//////////////////////////////////////////////////////////////         
//      $Id: rpmutils.h 16726 1999-02-11 04:18:43Z toivo $ 
//
#include "config.h"
#include <string.h>

extern "C"
{
  #include <rpm/rpmlib.h>
}

int findPackageByLabel(rpmdb db, const char *arg, dbiIndexSet *matches);
int findMatches(rpmdb db, char *name, char *version, char *release,
		dbiIndexSet *matches);

