//////////////////////////////////////////////////////////////         
// 	$Id: rpmutils.cpp 20304 1999-04-25 13:27:08Z toivo $	
//
#include "config.h"
#ifdef HAVE_RPM
#include <klocale.h>
 
#include "rpmutils.h"
#include <kpackage.h>

// #undef DB_FIRST       //defined in db.h
#include <kmsgbox.h>

///////////////////// RPM Utility Functions

/* 0 found matches */
/* 1 no matches */
/* 2 error */
int findPackageByLabel(rpmdb db, const char * arg, dbiIndexSet * matches) 
{
  char * localarg, * chptr;
  char * release;
  int rc;
  
  if (!strlen(arg)) return 1;
  
  /* did they give us just a name? */
  rc = findMatches(db, (char *)arg, NULL, NULL, matches);
  if (rc != 1) return rc;
  
  /* maybe a name and a release */
  localarg = (char*) alloca(strlen(arg) + 1);
  strcpy(localarg, arg);
  
  chptr = (localarg + strlen(localarg)) - 1;
  while (chptr > localarg && *chptr != '-') chptr--;
  if (chptr == localarg) return 1;
  
  *chptr = '\0';
  rc = findMatches(db, localarg, chptr + 1, NULL, matches);
  if (rc != 1) return rc;
  
  /* how about name-version-release? */
  
  release = chptr + 1;
  while (chptr > localarg && *chptr != '-') chptr--;
  if (chptr == localarg) return 1;
  
  *chptr = '\0';
  return findMatches(db, localarg, chptr + 1, release, matches);
}


/* 0 found matches */
/* 1 no matches */
/* 2 error */
int findMatches(rpmdb db, char * name, char * version, char * release,
		dbiIndexSet * matches) 
{
  int gotMatches;
  int rc;
  int i;
  char * pkgRelease, * pkgVersion;
  int count, type;
  int goodRelease, goodVersion;
  Header h;
  
  if ((rc = rpmdbFindPackage(db, name, matches))) 
    if (rc == -1) 
      return 2; 
    else 
      return 1;
  
  
  if (!version && !release) 
    return 0;
  
  gotMatches = 0;
  
  /* make sure the version and releases match */
  for (i = 0; i < matches->count; i++) 
    {
      if (matches->recs[i].recOffset) 
	{
	  h = rpmdbGetRecord(db, matches->recs[i].recOffset);
	  if (!h) 
	    {
	      printf(i18n("error: could not read database record\n"));
	      dbiFreeIndexRecord(*matches);
	      return 2;
	    }
	  
	  headerGetEntry(h, RPMTAG_VERSION, &type, (void **) &pkgVersion, 
			 &count);
	  headerGetEntry(h, RPMTAG_RELEASE, &type, (void **) &pkgRelease, 
			 &count);
	  
	  goodRelease = goodVersion = 1;
	  
	  if (release && strcmp(release, pkgRelease)) goodRelease = 0;
	  if (version && strcmp(version, pkgVersion)) goodVersion = 0;
	  
	  if (goodRelease && goodVersion) 
	    gotMatches = 1;
	  else 
	    matches->recs[i].recOffset = 0;
	}
    }
  
  if (!gotMatches) 
    {
      dbiFreeIndexRecord(*matches);
      return 1;
    }
  
  return 0;
}
#endif
