#include "../config.h"
#ifdef HAVE_RPM

extern "C"
{
#include <fcntl.h>
#include <rpm/rpmlib.h>
#include "rpmInstall.h"
#include "rpmVerify.h"
#include "rpmMessages.h"
}
 
#include "rpmutils.h"
#include <qlist.h>
#include <klocale.h>
 
#include "kpackage.h"

#ifdef  RPMTAG_EPOCH


 
static int verifyHeader(const char * prefix, Header h, int verifyFlags,
                        QList<char> *list);
static int verifyMatches(const char * prefix, rpmdb db, dbiIndexSet matches,
                         int verifyFlags, QList<char> *list);
static int verifyDependencies(rpmdb db, Header h,
			      QList<char> *list);

static int verifyHeader(const char * prefix, Header h, int verifyFlags,
                        QList<char>* list)
{
    const char ** fileList;
    int count, type;
    int verifyResult;
    int i, ec, rc;
    int_32 * fileFlagsList;
    int omitMask = 0;

    ec = 0;
    if (!(verifyFlags & VERIFY_MD5)) omitMask = RPMVERIFY_MD5;

    if (headerGetEntry(h, RPMTAG_FILEFLAGS, NULL, (void **) &fileFlagsList, NULL) &&
       headerGetEntry(h, RPMTAG_FILENAMES, &type, (void **) &fileList, &count)) {
        int oldpercentage=0;
	for (i = 0; i < count; i++) {
            int percentage = (int)(count
                                ? ((float) ((((float) i) / count) * 100))
                                : 100.0);
            if(percentage-oldpercentage > 5) {
               kpkg->kp->setPercent(percentage);
               oldpercentage = percentage;
            }
	    if ((rc = rpmVerifyFile(prefix, h, i, &verifyResult, omitMask)) != 0) {
		fprintf(stdout, i18n("missing    %s\n"), fileList[i]);
                list->append(strdup(fileList[i]));
	    } else {
		const char * size, * md5, * link, * mtime, * mode;
		const char * group, * user, * rdev;
		static const char * aok = ".";
		static const char * unknown = "?";

		if (!verifyResult) continue;
	    
		rc = 1;

#define	_verify(_RPMVERIFY_F, _C)	\
	((verifyResult & _RPMVERIFY_F) ? _C : aok)
#define	_verifylink(_RPMVERIFY_F, _C)	\
	((verifyResult & RPMVERIFY_READLINKFAIL) ? unknown : \
	 (verifyResult & _RPMVERIFY_F) ? _C : aok)
#define	_verifyfile(_RPMVERIFY_F, _C)	\
	((verifyResult & RPMVERIFY_READFAIL) ? unknown : \
	 (verifyResult & _RPMVERIFY_F) ? _C : aok)
	
		md5 = _verifyfile(RPMVERIFY_MD5, "5");
		size = _verify(RPMVERIFY_FILESIZE, "S");
		link = _verifylink(RPMVERIFY_LINKTO, "L");
		mtime = _verify(RPMVERIFY_MTIME, "T");
		rdev = _verify(RPMVERIFY_RDEV, "D");
		user = _verify(RPMVERIFY_USER, "U");
		group = _verify(RPMVERIFY_GROUP, "G");
		mode = _verify(RPMVERIFY_MODE, "M");

#undef _verify
#undef _verifylink
#undef _verifyfile

		fprintf(stdout, "%s%s%s%s%s%s%s%s %c %s\n",
		       size, mode, md5, rdev, link, user, group, mtime, 
		       fileFlagsList[i] & RPMFILE_CONFIG ? 'c' : ' ', 
		       fileList[i]);
                list->append(strdup(fileList[i]));
 	    }
	    if (rc)
		ec = rc;
	}
	
	free(fileList);
    }
    return ec;
}

static int verifyDependencies(rpmdb db, Header h, QList<char> *list) {
    rpmTransactionSet rpmdep;
    struct rpmDependencyConflict * conflicts;
    int numConflicts;
    const char * name, * version, * release;
    int type, count, i;
    QString ctmp;
 
    rpmdep = rpmtransCreateSet(db, NULL);
    rpmtransAddPackage(rpmdep, h, NULL, NULL, 0, NULL);

    rpmdepCheck(rpmdep, &conflicts, &numConflicts);
    rpmtransFree(rpmdep);

    if (numConflicts) {
	headerGetEntry(h, RPMTAG_NAME, &type, (void **) &name, &count);
	headerGetEntry(h, RPMTAG_VERSION, &type, (void **) &version, &count);
	headerGetEntry(h, RPMTAG_RELEASE, &type, (void **) &release, &count);
        rpmMessage(RPMMESS_WARNING,(char *)i18n("Unsatisfied dependencies for %s-%s-%s: "),
              name, version, release);
        for (i = 0; i < numConflicts; i++)
         {
           ctmp =  conflicts[i].needsName;
           if (conflicts[i].needsFlags)
             {
               ctmp += " ";
               if (conflicts[i].needsFlags & RPMSENSE_LESS)
                 ctmp += "<";
               if (conflicts[i].needsFlags & RPMSENSE_GREATER)
                 ctmp += ">";
               if (conflicts[i].needsFlags & RPMSENSE_EQUAL)
                 ctmp += "=";
               if (conflicts[i].needsFlags & RPMSENSE_SERIAL)
                 ctmp += "S";
  
               ctmp += " ";
               ctmp +=  conflicts[i].needsVersion;
             }
           list->append(strdup(ctmp.data()));
           rpmMessage(RPMMESS_WARNING,"%s",ctmp.data());
        }
	rpmdepFreeConflicts(conflicts, numConflicts);
	return 1;
    }
    return 0;
}

static int verifyPackage(const char * root, rpmdb db, Header h, int verifyFlags,
                     QList<char>* list)
{
    int ec, rc;
    FD_t fdo;
    ec = 0;
    if ((verifyFlags & VERIFY_DEPS) &&
	(rc = verifyDependencies(db, h, list)) != 0)
	    ec = rc;
    if (verifyFlags & VERIFY_FILES) {
         list->clear();
         if ((rc = verifyHeader(root, h, verifyFlags, list)) != 0)
	    ec = rc;
    }
    fdo = fdDup(STDOUT_FILENO);
    if ((verifyFlags & VERIFY_SCRIPT) &&
	(rc = rpmVerifyScript(root, h, fdo)) != 0)
	    ec = rc;
    fdClose(fdo);
    return ec;
}

static int verifyMatches(const char * prefix, rpmdb db, dbiIndexSet matches,
			  int verifyFlags, QList<char>* list) {
    int ec, rc;
    int i;
    Header h;

    ec = 0;
    for (i = 0; i < dbiIndexSetCount(matches); i++) {
	unsigned int recOffset = dbiIndexRecordOffset(matches, i);
	if (recOffset == 0)
	    continue;
	rpmMessage(RPMMESS_DEBUG, (char *)i18n("verifying record number %u\n"),
		recOffset);
	    
	h = rpmdbGetRecord(db, recOffset);
	if (h == NULL) {
		KpMsgE("%s", i18n("error: could not read database record\n"), TRUE);
		ec = 1;
	} else {
		if ((rc = verifyPackage(prefix, db, h, verifyFlags, list)) != 0)
		    ec = rc;
		headerFree(h);
	}
    }
    return ec;
}

int doVerify(const char * prefix, enum verifysources source, const char ** argv,
	      int verifyFlags, QList<char>* result) {
    Header h;
    int offset;
    int ec, rc;
    int isSource;
    rpmdb db;
    dbiIndexSet matches;

    ec = 0;
    if (source == VERIFY_RPM && !(verifyFlags & VERIFY_DEPS)) {
	db = NULL;
    } else {
	if (rpmdbOpen(prefix, &db, O_RDONLY, 0644)) {
	    return 1;	/* XXX was exit(EXIT_FAILURE) */
	}
    }

    if (source == VERIFY_EVERY) {
	for (offset = rpmdbFirstRecNum(db);
	     offset != 0;
	     offset = rpmdbNextRecNum(db, offset)) {
		h = rpmdbGetRecord(db, offset);
		if (h == NULL) {
		    KpMsgE("%s", i18n("could not read database record!\n"), TRUE);
		    return 1;	/* XXX was exit(EXIT_FAILURE) */
		}
		
		if ((rc = verifyPackage(prefix, db, h, verifyFlags, result)) != 0)
		    ec = rc;
		headerFree(h);
	}
    } else {
	while (*argv) {
	    const char *arg = *argv++;

	    rc = 0;
	    switch (source) {
	    case VERIFY_RPM:
	      { FD_t fd;

		fd = fdOpen(arg, O_RDONLY, 0);
		if (fd == NULL) {
		    KpMsgE(i18n("open of %s failed\n"), arg, TRUE);
		    break;
		}

		if (fdFileno(fd) >= 0) {
		    rc = rpmReadPackageHeader(fd, &h, &isSource, NULL, NULL);
		}

		fdClose(fd);

		switch (rc) {
		case 0:
			rc = verifyPackage(prefix, db, h, verifyFlags, result);
			headerFree(h);
			break;
		case 1:
			KpMsgE(i18n("%s is not an RPM\n"), arg, FALSE);
			break;
		}
	      }	break;

	    case VERIFY_GRP:
		if (rpmdbFindByGroup(db, arg, &matches)) {
		    KpMsgE(i18n("group %s does not contain any packages\n"), 
				arg, FALSE);
		} else {
		    rc = verifyMatches(prefix, db, matches, verifyFlags, result);
		    dbiFreeIndexRecord(matches);
		}
		break;

	    case VERIFY_PATH:
		if (rpmdbFindByFile(db, arg, &matches)) {
		    KpMsgE(i18n("file %s is not owned by any package\n"), 
				arg, FALSE);
		} else {
		    rc = verifyMatches(prefix, db, matches, verifyFlags, result);
		    dbiFreeIndexRecord(matches);
		}
		break;

	    case VERIFY_PACKAGE:
		rc = rpmdbFindByLabel(db, arg, &matches);
		if (rc == 1) 
		    KpMsgE(i18n("package %s is not installed\n"), arg, FALSE);
		else if (rc == 2) {
		    KpMsgE(i18n("error looking for package %s\n"), arg, FALSE);
		} else {
		    rc = verifyMatches(prefix, db, matches, verifyFlags, result);
		    dbiFreeIndexRecord(matches);
		}
		break;

	    case VERIFY_EVERY:
		break;
	    }
	    if (rc)
		ec = rc;
	}
    }
   
    if (source != VERIFY_RPM || verifyFlags & VERIFY_DEPS) {
	rpmdbClose(db);
    }
    return ec;
}

#else  /*RPMTAG_EPOCH*/

#include <errno.h>


static void verifyHeader(char * prefix, Header h, int verifyFlags, 
			 QList<char> *list);
static void verifyMatches(char * prefix, rpmdb db, dbiIndexSet matches,
			  int verifyFlags, QList<char> *list);
static void verifyDependencies(rpmdb db, Header h, QList<char> *list);

static void verifyHeader(char * prefix, Header h, int verifyFlags, 
			 QList<char>* list) 
{
  char ** fileList;
  int count, type;
  int verifyResult;
  int i;
  char * size, * md5, * link, * mtime, * mode;
  char * group, * user, * rdev;
  int_32 * fileFlagsList;
  int omitMask = 0;
  
  if (!(verifyFlags & VERIFY_MD5)) omitMask = RPMVERIFY_MD5;
  
  headerGetEntry(h, RPMTAG_FILEFLAGS, NULL, (void **) &fileFlagsList, NULL);

  if (headerGetEntry(h, RPMTAG_FILENAMES, &type, (void **) &fileList, 
		     &count)) 
    {
      int oldpercentage=0;
      for (i = 0; i < count; i++) 
	{
	  int percentage = (int)(count
				 ? ((float) ((((float) i) / count) * 100))
				 : 100.0);
	  if(percentage-oldpercentage > 5)
	    {
	      kpkg->kp->setPercent(percentage);
	      oldpercentage = percentage;
	    }

	  if (rpmVerifyFile(prefix, h, i, &verifyResult, omitMask))
	    {
	      rpmMessage(RPMMESS_WARNING,i18n("missing    %s\n"), fileList[i]);
	      list->append(strdup(fileList[i]));
	    }
	  else 
	    {
	      size = md5 = link = mtime = mode = ".";
	      user = group = rdev = ".";
	      
	      if (!verifyResult) continue;
	      
	      if (verifyResult & RPMVERIFY_MD5)
		md5 = "5";
	      if (verifyResult & RPMVERIFY_FILESIZE)
		size = "S";
	      if (verifyResult & RPMVERIFY_LINKTO)
		link = "L";
	      if (verifyResult & RPMVERIFY_MTIME)
		mtime = "T";
	      if (verifyResult & RPMVERIFY_RDEV)
		rdev = "D";
	      if (verifyResult & RPMVERIFY_USER)
		user = "U";
	      if (verifyResult & RPMVERIFY_GROUP)
		group = "G";
	      if (verifyResult & RPMVERIFY_MODE)
		mode = "M";
	      
	      rpmMessage(RPMMESS_WARNING,"%s%s%s%s%s%s%s%s %c %s\n",
		     size, mode, md5, rdev, link, user, group, mtime, 
		     fileFlagsList[i] & RPMFILE_CONFIG ? 'c' : ' ', 
		     fileList[i]);
	      list->append(strdup(fileList[i]));
	    }
	}
      
      free(fileList);
    }
}

static void verifyDependencies(rpmdb db, Header h, QList<char> *list) 
{
  list=list;			// stop warnings!

  rpmDependencies rpmdep;
  struct rpmDependencyConflict * conflicts;
  int numConflicts;
  char * name, * version, * release;
  int type, count, i;
  QString ctmp;
  
  rpmdep = rpmdepDependencies(db);
  rpmdepAddPackage(rpmdep, h, 0);
  
  rpmdepCheck(rpmdep, &conflicts, &numConflicts);
  rpmdepDone(rpmdep);
  
  if (numConflicts) 
    {
      headerGetEntry(h, RPMTAG_NAME, &type, (void **) &name, &count);
      headerGetEntry(h, RPMTAG_VERSION, &type, (void **) &version, &count);
      headerGetEntry(h, RPMTAG_RELEASE, &type, (void **) &release, &count);
      rpmMessage(RPMMESS_WARNING,i18n("Unsatisfied dependencies for %s-%s-%s: "),
	     name, version, release);
      for (i = 0; i < numConflicts; i++) 
	{
	  ctmp =  conflicts[i].needsName;
	  if (conflicts[i].needsFlags) 
	    {
	      ctmp += " ";
	      if (conflicts[i].needsFlags & RPMSENSE_LESS)
		ctmp += "<";
	      if (conflicts[i].needsFlags & RPMSENSE_GREATER)
		ctmp += ">";
	      if (conflicts[i].needsFlags & RPMSENSE_EQUAL)
		ctmp += "=";
	      if (conflicts[i].needsFlags & RPMSENSE_SERIAL)
		ctmp += "S";
		    
	      ctmp += " ";
	      ctmp +=  conflicts[i].needsVersion;
	    }
	  list->append(strdup(ctmp.data()));
	  rpmMessage(RPMMESS_WARNING,"%s",ctmp.data());
	}
      rpmdepFreeConflicts(conflicts, numConflicts);
    }
}

static void verifyPackage(char * root, rpmdb db, Header h, int verifyFlags,
			  QList<char>* list) 
{
  if (verifyFlags & VERIFY_DEPS)
    verifyDependencies(db, h, list);
  if (verifyFlags & VERIFY_FILES) {		  
    list->clear();
    verifyHeader(root, h, verifyFlags, list);
  }
  if (verifyFlags & VERIFY_SCRIPT) 
    {
      rpmVerifyScript(root, h, 1);
    }
}

static void verifyMatches(char * prefix, rpmdb db, dbiIndexSet matches,
			  int verifyFlags, QList<char>* list) 
{
  int i;
  Header h;
  
  for (i = 0; i < matches.count; i++) 
    {
      if (matches.recs[i].recOffset) 
	{
	  rpmMessage(RPMMESS_DEBUG,
		     i18n("verifying record number %d\n"),
		     matches.recs[i].recOffset);
	  
	  h = rpmdbGetRecord(db, matches.recs[i].recOffset);
	  if (!h) 
	    {
		      rpmMessage(RPMMESS_WARNING,i18n("error: could not read database record\n"));
	    } 
	  else 
	    {
	      verifyPackage(prefix, db, h, verifyFlags, list);
	      headerFree(h);
	    }
	}
    }
}

int doVerify(const char * prefix, enum verifysources source, const char ** argv,
	      int verifyFlags, QList<char>* result) 
{
    Header h;
    int offset;
    int fd;
    int rc;
    int isSource;
    rpmdb db;
    dbiIndexSet matches;
    const char * arg;
    result->setAutoDelete(TRUE);
	    

    if (source == VERIFY_RPM && !(verifyFlags & VERIFY_DEPS)) 
      {
	db = NULL; 
      } 
    else 
      {
	if (rpmdbOpen(prefix, &db, O_RDONLY, 0644)) 
	  {
	    return 0;
	  }
      }
    
    if (source == VERIFY_EVERY) 
      {
	offset = rpmdbFirstRecNum(db);
	while (offset) 
	  {
	    h = rpmdbGetRecord(db, offset);
	    if (!h) 
	      {
		rpmMessage(RPMMESS_WARNING,i18n("could not read database record!\n"));
		return 0;
	      }
	    verifyPackage(prefix, db, h, verifyFlags, result);
	    headerFree(h);
	    offset = rpmdbNextRecNum(db, offset);
	  }
      } 
    else 
      {
	while (*argv) 
	  {
	    arg = *argv++;
	    
	    switch (source) 
	      {
	      case VERIFY_RPM:
		fd = open(arg, O_RDONLY);
		if (fd < 0) 
		  {
		    rpmMessage(RPMMESS_WARNING,i18n("open of %s failed: %s\n"), arg, 
			       strerror(errno));
		  } 
		else 
		  {
		    rc = rpmReadPackageHeader(fd, &h, &isSource, NULL, NULL);
		    close(fd);
		    switch (rc) 
		      {
		      case 0:
			verifyPackage(prefix, db, h, verifyFlags,result);
			headerFree(h);
			break;
		      case 1:
			rpmMessage(RPMMESS_WARNING,i18n("%s is not an RPM\n"), arg);
		      }
		  }
		
		break;
		
	      case VERIFY_GRP:
		if (rpmdbFindByGroup(db, (char *)arg, &matches)) 
		  {
		    rpmMessage(RPMMESS_WARNING,i18n("group %s does not contain any pacakges\n"), 
			       arg);
		  } 
		else 
		  {
		    verifyMatches(prefix, db, matches, verifyFlags,result);
		    dbiFreeIndexRecord(matches);
		  }
		break;
		
	      case VERIFY_PATH:
		if (rpmdbFindByFile(db, (char *)arg, &matches)) 
		  {
		    rpmMessage(RPMMESS_WARNING,i18n("file %s is not owned by any package\n"), 
			       arg);
		  } 
		else 
		  {
		    verifyMatches(prefix, db, matches, verifyFlags,result);
		    dbiFreeIndexRecord(matches);
		  }
		break;
		
	      case VERIFY_PACKAGE:
		rc = findPackageByLabel(db, arg, &matches);
		if (rc == 1) 
		  rpmMessage(RPMMESS_WARNING,i18n("package %s is not installed\n"), arg);
		else if (rc == 2) 
		  {
		    rpmMessage(RPMMESS_WARNING,i18n("error looking for package %s\n"), arg);
		  } 
		else 
		  {
		    verifyMatches(prefix, db, matches, verifyFlags,result);
		    dbiFreeIndexRecord(matches);
		  }
		break;
		
	      case VERIFY_EVERY:
		; /* nop */
	      }
	  }
      }
    
    if (db) 
      {
	rpmdbClose(db);
      }
    return 0;
  }

#endif /*RPMTAG_EPOCH*/

#endif /*HAVE_RPM*/

