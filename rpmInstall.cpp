//////////////////////////////////////////////////////////////
//     $Id: rpmInstall.cpp 20996 1999-05-07 14:13:11Z toivo $
#include "../config.h"

#ifdef HAVE_RPM

extern "C"
{
#include <fcntl.h>
#include <stdio.h>  
#include <unistd.h>

#include <rpm/rpmlib.h>
#include "rpmInstall.h"
#include "rpmMessages.h"
}

#include <klocale.h>

#include "kpackage.h"
#include "rpmutils.h"

#ifdef  RPMTAG_EPOCH

extern "C"
{
#include <rpm/rpmmacro.h>
}

static void printHash(const unsigned long amount, const unsigned long total);
static void * showProgress(const Header h, const rpmCallbackType what, 
			   const unsigned long amount, 
			   const unsigned long total,
			   const void * pkgKey, void * data);

static int hashesPrinted = 0;

static void printHash(const unsigned long amount, const unsigned long total)
{
    int hashesNeeded;

    if (hashesPrinted != 50) {
	hashesNeeded = (int)( 50 * (total ? (((float) amount) / total) : 1));
    }
    kpkg->kp->setPercent(hashesNeeded*2);   
}

static void * showProgress(const Header h, const rpmCallbackType what, 
			   const unsigned long amount, 
			   const unsigned long total,
			   const void * pkgKey, void * data) {
    char * s;
    int flags = (int) data;
    void * rc = NULL;
    const char * filename = (const char *)pkgKey;
    static FD_t fd;

    switch (what) {
      case RPMCALLBACK_INST_OPEN_FILE:
	fd = fdOpen(filename, O_RDONLY, 0);
	return fd;

      case RPMCALLBACK_INST_CLOSE_FILE:
	fdClose(fd);
	break;

      case RPMCALLBACK_INST_START:
	hashesPrinted = 0;
	if (flags & INSTALL_LABEL) {
	    if (flags & INSTALL_HASH) {
		s = headerSprintf(h, "%{NAME}",
				  rpmTagTable, rpmHeaderFormats, NULL);
		KpMsgE("%-28s", s, FALSE);
		fflush(stdout);
	    } else {
		s = headerSprintf(h, "%{NAME}-%{VERSION}-%{RELEASE}", 
				  rpmTagTable, rpmHeaderFormats, NULL);
		KpMsgE("%s\n", s, FALSE);
	    }
	    free(s);
	}
	break;

      case RPMCALLBACK_INST_PROGRESS:
	printHash(amount, total);
	break;

      case RPMCALLBACK_TRANS_PROGRESS:
      case RPMCALLBACK_TRANS_START:
      case RPMCALLBACK_TRANS_STOP:
      case RPMCALLBACK_UNINST_PROGRESS:
      case RPMCALLBACK_UNINST_START:
      case RPMCALLBACK_UNINST_STOP:
	/* ignore */
	break;
    }

    return rc;
}	

int doInstal(const char * rootdir, const char ** argv, int transFlags, 
	      int interfaceFlags, int probFilter, 
	      rpmRelocation * relocations) {
    rpmdb db = NULL;
    FD_t fd;
    int i;
    int mode, rc, major;
    const char ** packages, ** tmpPackages;
    const char ** filename;
    int numPackages;
    int numTmpPackages = 0, numBinaryPackages = 0, numSourcePackages = 0;
    int numFailed = 0;
    Header h;
    int isSource;
    rpmTransactionSet rpmdep = NULL;
    struct rpmDependencyConflict * conflicts;
    int numConflicts;
    int stopInstall = 0;
    size_t nb;
    int notifyFlags = interfaceFlags | (rpmIsVerbose() ? INSTALL_LABEL : 0 );
    int dbIsOpen = 0;
    const char ** sourcePackages;
    rpmRelocation * defaultReloc;

    if (transFlags & RPMTRANS_FLAG_TEST) 
	mode = O_RDONLY;
    else
	mode = O_RDWR | O_CREAT;

    for (defaultReloc = relocations; defaultReloc && defaultReloc->oldPath;
	 defaultReloc++);
    if (defaultReloc && !defaultReloc->newPath) defaultReloc = NULL;

    rpmMessage(RPMMESS_DEBUG, (char *)i18n("counting packages to install\n"));
    for (filename = argv, numPackages = 0; *filename; filename++, numPackages++)
	;

    rpmMessage(RPMMESS_DEBUG, (char *)i18n("found %d packages\n"), numPackages);

    nb = (numPackages + 1) * sizeof(char *);
    packages = (const char **) alloca(nb);
    memset(packages, 0, nb);
    tmpPackages = (const char **)alloca(nb);
    memset(tmpPackages, 0, nb);
    nb = (numPackages + 1) * sizeof(Header);

    for (filename = argv, i = 0; *filename; filename++)
      {
 	    packages[i++] = *filename;
	    break;
      }

    sourcePackages = (const char **)alloca(sizeof(*sourcePackages) * i);

    rpmMessage(RPMMESS_DEBUG, (char *)i18n("retrieved %d packages\n"), numTmpPackages);

    /* Build up the transaction set. As a special case, v1 source packages
       are installed right here, only because they don't have headers and
       would create all sorts of confusion later. */

    for (filename = packages; *filename; filename++) {
	fd = fdOpen(*filename, O_RDONLY, 0);
	if (fdFileno(fd) < 0) {
	    rpmMessage(RPMMESS_ERROR, (char *)i18n("cannot open file %s\n"), *filename);
	    numFailed++;
	    packages[i] = NULL;
	    continue;
	}

	rc = rpmReadPackageHeader(fd, &h, &isSource, &major, NULL);

	switch (rc) {
	case 1:
	    fdClose(fd);
	    rpmMessage(RPMMESS_ERROR, 
			(char *)i18n("%s does not appear to be a RPM package\n"), 
			*filename);
	    break;
	default:
	    rpmMessage(RPMMESS_ERROR, (char *)i18n("%s cannot be installed\n"), *filename);
	    numFailed++;
	    packages[i] = NULL;
	    break;
	case 0:
	    if (isSource) {
		sourcePackages[numSourcePackages++] = *filename;
		fdClose(fd);
	    } else {
		if (!dbIsOpen) {
		    if (rpmdbOpen(rootdir, &db, mode, 0644)) {
		      //	const char *dn;
		      //	dn = rpmGetPath( (rootdir ? rootdir : ""), 
		      //			"%{_dbpath}", NULL);
		      if (getuid != 0)
			rpmMessage(RPMMESS_ERROR, 
				   (char *)i18n("KPACKAGE has to run as ROOT"));
			//			xfree(dn);
			return(EXIT_FAILURE);
		    }
		    rpmdep = rpmtransCreateSet(db, rootdir);
		    dbIsOpen = 1;
		}

		if (defaultReloc) {
		    char ** paths;
		    char * name;
		    int c;

		    if (headerGetEntry(h, RPMTAG_PREFIXES, NULL,
				       (void **) &paths, &c) && (c == 1)) {
			defaultReloc->oldPath = paths[0];
			free(paths);
		    } else {
			headerGetEntry(h, RPMTAG_NAME, NULL, (void **) &name,
				       NULL);
			rpmMessage(RPMMESS_ERROR, 
			       (char *)i18n("package %s is not relocateable\n"), name);

			return numPackages;
		    }
		}

		rc = rpmtransAddPackage(rpmdep, h, NULL, *filename,
			       (interfaceFlags & INSTALL_UPGRADE) != 0,
			       relocations);
		if (rc) {
		    if (rc == 1)
			rpmMessage(RPMMESS_ERROR, 
			    (char *)i18n("error reading from file %s\n"), *filename);
		    else if (rc == 2)
			rpmMessage(RPMMESS_ERROR, 
			    (char *)i18n("file %s requires a newer version of RPM\n"),
			    *filename);
		    return numPackages;
		}

		if (defaultReloc)
		    defaultReloc->oldPath = NULL;

		fdClose(fd);
		numBinaryPackages++;
	    }
	    break;
	}
    }

    rpmMessage(RPMMESS_DEBUG, (char *)i18n("found %d source and %d binary packages\n"), 
		numSourcePackages, numBinaryPackages);

    if (numBinaryPackages && !(interfaceFlags & INSTALL_NODEPS)) {
	if (rpmdepCheck(rpmdep, &conflicts, &numConflicts)) {
	    numFailed = numPackages;
	    stopInstall = 1;
	}

	if (!stopInstall && conflicts) {
	  //	    rpmMessage(RPMMESS_ERROR, (char *)i18n("failed dependencies:\n"));
	    printDepProblems(stderr, conflicts, numConflicts);
	    rpmdepFreeConflicts(conflicts, numConflicts);
	    numFailed = numPackages;
	    stopInstall = 1;
	}
    }

    if (numBinaryPackages && !(interfaceFlags & INSTALL_NOORDER)) {
	if (rpmdepOrder(rpmdep)) {
	    numFailed = numPackages;
	    stopInstall = 1;
	}
    }

    if (numBinaryPackages && !stopInstall) {
	rpmProblemSet probs = NULL;
;
	rpmMessage(RPMMESS_DEBUG, (char *)i18n("installing binary packages\n"));
	rc = rpmRunTransactions(rpmdep, showProgress, (void *) notifyFlags, 
				    NULL, &probs, transFlags, probFilter);

	if (rc < 0) {
	    numFailed += numBinaryPackages;
	} else if (rc) {
	    numFailed += rc;
	    for (i = 0; i < probs->numProblems; i++) {
		if (!probs->probs[i].ignoreProblem) {
		    char *msg = rpmProblemString(probs->probs[i]);
		    rpmMessage(RPMMESS_ERROR, "%s\n", msg);
		    free(msg);
		}
	    }
	}

	if (probs) rpmProblemSetFree(probs);
    }

    if (numBinaryPackages) rpmtransFree(rpmdep);


    if (numSourcePackages && !stopInstall) {
	for (i = 0; i < numSourcePackages; i++) {
	    fd = fdOpen(sourcePackages[i], O_RDONLY, 0);
	    if (fdFileno(fd) < 0) {
		rpmMessage(RPMMESS_ERROR, (char *)i18n("cannot open file %s\n"), 
			   sourcePackages[i]);
		continue;
	    }

	    if (!(transFlags & RPMTRANS_FLAG_TEST))
		numFailed += rpmInstallSourcePackage(rootdir, fd, NULL,
				showProgress, (void *) notifyFlags, NULL);

	    fdClose(fd);
	}
    }

    for (i = 0; i < numTmpPackages; i++) {
	unlink(tmpPackages[i]);
	xfree(tmpPackages[i]);
    }

    /* FIXME how do we close our various fd's? */

    if (dbIsOpen) rpmdbClose(db);

    return numFailed;
}

int doUninstal(const char * rootdir, const char ** argv, int transFlags,
		 int interfaceFlags) {
    rpmdb db;
    dbiIndexSet matches;
    int i, j;
    int mode;
    int rc;
    int count;
    const char ** arg;
    int numFailed = 0;
    rpmTransactionSet rpmdep;
    struct rpmDependencyConflict * conflicts;
    int numConflicts;
    int stopUninstall = 0;
    int numPackages = 0;
    rpmProblemSet probs;

    if (transFlags & RPMTRANS_FLAG_TEST) 
	mode = O_RDONLY;
    else
	mode = O_RDWR | O_EXCL;
	
    if (rpmdbOpen(rootdir, &db, mode, 0644)) {
	const char *dn;
	dn = rpmGetPath( (rootdir ? rootdir : ""), "%{_dbpath}", NULL);
	rpmMessage(RPMMESS_ERROR, (char *)i18n("KPACKAGE has to run as ROOT\n"), dn);
	xfree(dn);
	return(EXIT_FAILURE);
    }

    j = 0;
    rpmdep = rpmtransCreateSet(db, rootdir);
    for (arg = argv; *arg; arg++) {
	rc = rpmdbFindByLabel(db, *arg, &matches);
	switch (rc) {
	case 1:
	    rpmMessage(RPMMESS_ERROR, (char *)i18n("package %s is not installed\n"), *arg);
	    numFailed++;
	    break;
	case 2:
	    rpmMessage(RPMMESS_ERROR, (char *)i18n("searching for package %s\n"), *arg);
	    numFailed++;
	    break;
	default:
	    count = 0;
	    for (i = 0; i < dbiIndexSetCount(matches); i++)
		if (dbiIndexRecordOffset(matches, i)) count++;

	    if (count > 1 && !(interfaceFlags & UNINSTALL_ALLMATCHES)) {
		rpmMessage(RPMMESS_ERROR, (char *)i18n("\"%s\" specifies multiple packages\n"), 
			*arg);
		numFailed++;
	    }
	    else { 
		for (i = 0; i < dbiIndexSetCount(matches); i++) {
		    unsigned int recOffset = dbiIndexRecordOffset(matches, i);
		    if (recOffset) {
			rpmtransRemovePackage(rpmdep, recOffset);
			numPackages++;
		    }
		}
	    }

	    dbiFreeIndexRecord(matches);
	    break;
	}
    }

    if (!(interfaceFlags & UNINSTALL_NODEPS)) {
	if (rpmdepCheck(rpmdep, &conflicts, &numConflicts)) {
	    numFailed = numPackages;
	    stopUninstall = 1;
	}

	if (!stopUninstall && conflicts) {
	  //	    rpmMessage(RPMMESS_ERROR, (char *)i18n("removing these packages would break "
	  //			      "dependencies:\n"));
	    printDepProblems(stderr, conflicts, numConflicts);
	    rpmdepFreeConflicts(conflicts, numConflicts);
	    numFailed += numPackages;
	    stopUninstall = 1;
	}
    }

    if (!stopUninstall) {
	numFailed += rpmRunTransactions(rpmdep, NULL, NULL, NULL, &probs,
					transFlags, 0);
    }

    rpmtransFree(rpmdep);
    rpmdbClose(db);

    return numFailed;
}

int doSourceInstall(const char * rootdir, const char * arg, const char ** specFile,
		    char ** cookie) {
    FD_t fd;
    int rc;

    fd = fdOpen(arg, O_RDONLY, 0);
    if (fdFileno(fd) < 0) {
	rpmMessage(RPMMESS_ERROR, (char *)i18n("cannot open %s\n"), arg);
	return 1;
    }

    if (rpmIsVerbose())
      KpMsgE(i18n("Installing %s\n"), arg, FALSE);

    rc = rpmInstallSourcePackage(rootdir, fd, specFile, NULL, NULL, 
				 cookie);
    if (rc == 1) {
	rpmMessage(RPMMESS_ERROR, (char *)i18n("%s cannot be installed\n"), arg);
	if (specFile) FREE(*specFile);
	if (cookie) FREE(*cookie);
    }

    fdClose(fd);

    return rc;
}

void printDepFlags(FILE * f, QString s, char * version, int flags) {
 
  if (flags) {
    fprintf(f, " ");
    s += " ";
  }
 
  if (flags & RPMSENSE_LESS) {
    fprintf(f, "<");
    s += "<";
  }
  if (flags & RPMSENSE_GREATER) {
    fprintf(f, ">");
    s += ">";
  }
  if (flags & RPMSENSE_EQUAL) {
    fprintf(f, "=");
    s += "=";
  }
  if (flags & RPMSENSE_SERIAL) {
    fprintf(f, "S");
    s += "S";
  }
  if (flags) {
    fprintf(f, " %s", version);
    s += " ";
    s += version;
  }
}
 
void printDepProblems(FILE * f, struct rpmDependencyConflict * conflicts,
			     int numConflicts) {
    int i;
    QString s, st;
 
     st += i18n("Dependency Problem:\n");
     for (i = 0; i < numConflicts; i++) {
	fprintf(f, "\t%s", conflicts[i].needsName);
	st += " ";
	st += conflicts[i].needsName;
	if (conflicts[i].needsFlags) {
	    printDepFlags(stderr, st, conflicts[i].needsVersion, 
			  conflicts[i].needsFlags);
	}

	if (conflicts[i].sense == (rpmDependencyConflict::RPMDEP_SENSE_REQUIRES)) { 
	    fprintf(f, i18n(" is needed by %s-%s-%s\n"), conflicts[i].byName, 
		    conflicts[i].byVersion, conflicts[i].byRelease);
            st += s.sprintf( i18n(" is needed by %s-%s-%s\n"),
                             conflicts[i].byName,
                    conflicts[i].byVersion, conflicts[i].byRelease);
	} else {
	    fprintf(f, i18n(" conflicts with %s-%s-%s\n"), conflicts[i].byName, 
		    conflicts[i].byVersion, conflicts[i].byRelease);
            st += s.sprintf( i18n(" conflicts with %s-%s-%s\n"),
                            conflicts[i].byName,
                    conflicts[i].byVersion, conflicts[i].byRelease);
 	}
    }
    rpmMessage(RPMMESS_ERROR,"%s",st.data());	
}

void rpmErr()
{
  KpMsgE(0,rpmErrorString(),TRUE);
}

#else


#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif


static int hashesPrinted = 0;

static void printHash(const unsigned long amount, const unsigned long total);
static void printPercent(const unsigned long amount, const unsigned long total);
static void printDepProblems(FILE * f, struct rpmDependencyConflict * conflicts,
			     int numConflicts);

static void printHash(const unsigned long amount, const unsigned long total) {
    int hashesNeeded;

    if (hashesPrinted != 50) {
	hashesNeeded = (int)(50 * (total ? (((float) amount) / total) : 1));
	while (hashesNeeded > hashesPrinted) {
	    printf("#");
	    hashesPrinted++;
	}
	fflush(stdout);
	hashesPrinted = hashesNeeded;

	if (hashesPrinted == 50)
	    printf("\n");
    }
}

static void printPercent(const unsigned long amount, const unsigned long total) 
{
  int percentage =  (int)(total
			  ? ((float) ((((float) amount) / total) * 100))
			  : 100.0);
  kpkg->kp->setPercent(percentage);
}

static int installPackages(char * rootdir, char ** packages, 
			    int numPackages, int installFlags, 
			    int interfaceFlags, rpmdb db) {
    int i, fd;
    int numFailed = 0;
    char ** filename;
    const char * printFormat = NULL;
    char * chptr;
    int rc;
    rpmNotifyFunction fn;
    char * netsharedPath = NULL;

    if (interfaceFlags & INSTALL_PERCENT)
	fn = printPercent;
    else if (interfaceFlags & INSTALL_HASH)
	fn = printHash;
    else
	fn = NULL;

    netsharedPath = rpmGetVar(RPMVAR_NETSHAREDPATH);

    for (i = 0, filename = packages; i < numPackages; i++, filename++) {
	if (!*filename) continue;

	hashesPrinted = 0;

	fd = open(*filename, O_RDONLY);
	if (fd < 0) {
	 KpMsgE(i18n("cannot open file %s"),*filename,FALSE);
	    numFailed++;
	    *filename = NULL;
	    continue;
	} 

	if (interfaceFlags & INSTALL_PERCENT) 
	    printFormat = "%%f %s:%s:%s\n";
	else if (rpmIsVerbose() && (interfaceFlags & INSTALL_HASH)) {
	    chptr = strrchr((const char *)*filename, '/');
	    if (!chptr)
		chptr = *filename;
	    else
		chptr++;

	    printFormat = "%-28s";
	} else if (rpmIsVerbose())
	    rpmMessage(RPMMESS_DEBUG, i18n("Installing %s\n"), *filename);

	if (db) {
	    rc = rpmInstallPackage(rootdir, db, fd, 0, installFlags, fn, 
				   NULL
#ifndef RPMSENSE_TRIGGERPOSTUN 
				   ,netsharedPath
#endif				   
				   ); 
	} else {
	    if (installFlags &= RPMINSTALL_TEST) {
		rpmMessage(RPMMESS_DEBUG, "stopping source install as we're "
			"just testing");
		rc = 0;
	    } else {
		rc = rpmInstallSourcePackage(rootdir, fd, NULL, fn,
					     (char *)printFormat,0);
	    }
	} 

	if (rc == 1) {
	  KpMsgE(i18n("%s does not appear to be a RPM package"),*filename,FALSE);
	}
	    
	if (rc) {
	  KpMsgE(i18n("%s cannot be installed"),*filename,TRUE);
	    numFailed++;
	}

	close(fd);
    }

    return numFailed;
}

int doInstal(char * rootdir, char ** argv, int installFlags, 
	      int interfaceFlags) {
    rpmdb db;
    int fd, i;
    int mode, rc;
    char ** packages, ** tmpPackages;
    char ** filename;
    int numPackages;
    int numTmpPackages = 0, numBinaryPackages = 0, numSourcePackages = 0;
    int numFailed = 0;
    Header * binaryHeaders;
    int isSource;
    rpmDependencies rpmdep;
    struct rpmDependencyConflict * conflicts;
    int numConflicts;
    int stopInstall = 0;

    if (installFlags & RPMINSTALL_TEST) 
	mode = O_RDONLY;
    else
	mode = O_RDWR | O_CREAT;

    rpmMessage(RPMMESS_DEBUG, i18n("counting packages to install"));
    for (filename = argv, numPackages = 0; *filename; filename++, numPackages++)
	;

    rpmMessage(RPMMESS_DEBUG, i18n("found %d packages"), numPackages);
    packages = (char**)alloca((numPackages + 1) * sizeof(char *));
    packages[numPackages] = NULL;
    tmpPackages = (char**)alloca((numPackages + 1) * sizeof(char *));
    binaryHeaders = (Header*)alloca((numPackages + 1) * sizeof(Header));
	
    for (filename = argv, i = 0; *filename; filename++) 
      {
	packages[i++] = *filename;
      }
    
    rpmMessage(RPMMESS_DEBUG,
	       i18n("finding source and binary packages"));
    for (filename = packages; *filename; filename++) 
      {
	fd = open(*filename, O_RDONLY);
	if (fd < 0) 
	  {
	  KpMsgE(i18n("cannot open file %s"),*filename,FALSE);
	    numFailed++;
	    *filename = NULL;
	    continue;
	  }
	
	rc = rpmReadPackageHeader(fd, &binaryHeaders[numBinaryPackages], &isSource,
				  NULL, NULL);
	
	close(fd);
	
	if (rc == 1) 
	  {
	  KpMsgE(i18n("%s does not appear to be a RPM package"),*filename,FALSE);	  }
	
	if (rc) 
	  {
	  KpMsgE(i18n("%s cannot be installed"),*filename,TRUE);
	    numFailed++;
	    *filename = NULL;
	  } 
	else if (isSource) 
	  {
	    /* the header will be NULL if this is a v1 source package */
	    if (binaryHeaders[numBinaryPackages])
	      headerFree(binaryHeaders[numBinaryPackages]);
	    
	    numSourcePackages++;
	  } 
	else 
	  {
	    numBinaryPackages++;
	  }
      }
    
    rpmMessage(RPMMESS_DEBUG,
	       i18n("found %d source and %d binary packages"), 
	       numSourcePackages, numBinaryPackages);
    
    if (numBinaryPackages) 
      {
	rpmMessage(RPMMESS_DEBUG,
		   i18n("opening database mode: 0%o"), mode);
	if (rpmdbOpen(rootdir, &db, mode, 0644)) {
	  KpMsgE(i18n("cannot open /var/lib/rpm/packages.rpm\nKpackage needs to be running as root"),"",TRUE);
	  return 1;
	}
	if (!(interfaceFlags & INSTALL_NODEPS)) 
	  {
	    rpmdep = rpmdepDependencies(db);
	    for (i = 0; i < numBinaryPackages; i++)
	      if (installFlags & RPMINSTALL_UPGRADE)
		rpmdepUpgradePackage(rpmdep, binaryHeaders[i],0);
	      else
		rpmdepAddPackage(rpmdep, binaryHeaders[i],0);
	    
	    if (rpmdepCheck(rpmdep, &conflicts, &numConflicts)) 
	      {
		numFailed = numPackages;
		stopInstall = 1;
	      }
	    
	    rpmdepDone(rpmdep);
	    
	    if (!stopInstall && conflicts) 
	      {
		rpmMessage(RPMMESS_DEBUG,
			   i18n("failed dependencies:"));
		printDepProblems(stderr, conflicts, numConflicts);
		rpmdepFreeConflicts(conflicts, numConflicts);
		numFailed = numPackages;
		stopInstall = 1;
	      }
	  }
      }
    else
      db = NULL;
    
    if (!stopInstall) 
      {
	rpmMessage(RPMMESS_DEBUG,
		   i18n("installing binary packages"));
	numFailed += installPackages(rootdir, packages, numPackages, 
				     installFlags, interfaceFlags, db);
      }
    
    for (i = 0; i < numTmpPackages; i++)
      unlink(tmpPackages[i]);
    
    for (i = 0; i < numBinaryPackages; i++) 
      headerFree(binaryHeaders[i]);
    
    if (db) rpmdbClose(db);
    
    return numFailed;
}

int doUninstal(char * rootdir, char ** argv, int uninstallFlags,
		 int interfaceFlags) {
    rpmdb db;
    dbiIndexSet matches;
    int i, j;
    int mode;
    int rc;
    int count;
    int numPackages;
    int * packageOffsets;
    char ** arg;
    int numFailed = 0;
    rpmDependencies rpmdep;
    struct rpmDependencyConflict * conflicts;
    int numConflicts;
    int stopUninstall = 0;

    rpmMessage(RPMMESS_DEBUG,
	       i18n("counting packages to uninstall"));
    for (arg = argv, numPackages = 0; *arg; arg++, numPackages++)
	;
    rpmMessage(RPMMESS_DEBUG,
	       i18n("found %d packages to uninstall"), numPackages);

    packageOffsets = (int*)alloca(sizeof(int *) * numPackages);

    if (uninstallFlags & RPMUNINSTALL_TEST) 
	mode = O_RDONLY;
    else
	mode = O_RDWR | O_EXCL;
	
    if (rpmdbOpen(rootdir, &db, mode, 0644)) {
      KpMsgE(i18n("cannot open /var/lib/rpm/packages.rpm\nKpackage needs to be running as root"),"",TRUE);
      return 1;
    }

    j = 0;
    for (arg = argv, numPackages = 0; *arg; arg++, numPackages++) {
	rc = findPackageByLabel(db, *arg, &matches);
	if (rc == 1) {
	  KpMsgE(i18n("package %s is not installed"),*arg,FALSE);
	    numFailed++;
	} else if (rc == 2) {
	  KpMsgE(i18n("error searching for package %s"),*arg,FALSE);
	    numFailed++;
	} else {
	    count = 0;
	    for (i = 0; i < matches.count; i++)
		if (matches.recs[i].recOffset) count++;

	    if (count > 1) {
	      KpMsgE(i18n("\"%s\" specifies multiple packages"),*arg,FALSE);
		numFailed++;
	    }
	    else { 
		for (i = 0; i < matches.count; i++) {
		    if (matches.recs[i].recOffset) {
			packageOffsets[j++] = matches.recs[i].recOffset;
		    }
		}
	    }

	    dbiFreeIndexRecord(matches);
	}
    }
    numPackages = j;

    if (!(interfaceFlags & UNINSTALL_NODEPS)) {
	rpmdep = rpmdepDependencies(db);
	for (i = 0; i < numPackages; i++)
	    rpmdepRemovePackage(rpmdep, packageOffsets[i]);

	if (rpmdepCheck(rpmdep, &conflicts, &numConflicts)) {
	    numFailed = numPackages;
	    stopUninstall = 1;
	}

	rpmdepDone(rpmdep);

	if (!stopUninstall && conflicts) {
	  rpmMessage(RPMMESS_WARNING,i18n("removing these packages would break "
			    "dependencies:\n"));
	    printDepProblems(stderr, conflicts, numConflicts);
	    rpmdepFreeConflicts(conflicts, numConflicts);
	    numFailed += numPackages;
	    stopUninstall = 1;
	}
    }

    if (!stopUninstall) {
	for (i = 0; i < numPackages; i++) {
	    rpmMessage(RPMMESS_DEBUG,
		       i18n("uninstalling record number %d"),
			packageOffsets[i]);
	    rpmRemovePackage(rootdir, db, packageOffsets[i], uninstallFlags);
	}
    }

    rpmdbClose(db);

    return numFailed;
}

int doSourceInstall(char * rootdir, char * arg, char ** specFile) {
    int fd;
    int rc;

    fd = open(arg, O_RDONLY);
    if (fd < 0) {
      KpMsgE(	    i18n("cannot open %s"),arg,TRUE);
	return 1;
    }

    if (rpmIsVerbose())
	rpmMessage(RPMMESS_DEBUG, i18n("Installing %s\n"), arg);

    rc = rpmInstallSourcePackage(rootdir, fd, specFile, NULL, NULL, 0);
    if (rc == 1) {
      KpMsgE(	    i18n("%s cannot be installed"),arg,TRUE);
    }

    close(fd);

    return rc;
}

void printDepFlags(FILE * f, QString s, char * version, int flags) {

  if (flags) {
    fprintf(f, " ");
    s += " ";
  }

  if (flags & RPMSENSE_LESS) {
    fprintf(f, "<");
    s += "<";
  }
  if (flags & RPMSENSE_GREATER) {
    fprintf(f, ">");
    s += ">";
  }
  if (flags & RPMSENSE_EQUAL) {
    fprintf(f, "=");
    s += "=";
  }
  if (flags & RPMSENSE_SERIAL) {
    fprintf(f, "S");
    s += "S";
  }
  if (flags) {
    fprintf(f, " %s", version);
    s += " ";
    s += version;
  }
}

static void printDepProblems(FILE * f, struct rpmDependencyConflict * conflicts,
			     int numConflicts) {
    int i;
    QString s, st;

    st += i18n("Dependency Problem:\n");
    for (i = 0; i < numConflicts; i++) {
	fprintf(f, "\t%s", conflicts[i].needsName);
	st += s.sprintf( "\t%s", conflicts[i].needsName);
	if (conflicts[i].needsFlags) {
	    printDepFlags(stderr, st, conflicts[i].needsVersion, 
			  conflicts[i].needsFlags);
	}

	if (conflicts[i].sense == rpmDependencyConflict::RPMDEP_SENSE_REQUIRES) {
	    fprintf(f, i18n(" is needed by %s-%s-%s\n"),
		    conflicts[i].byName, 
		    conflicts[i].byVersion, conflicts[i].byRelease);
	    st += s.sprintf( i18n(" is needed by %s-%s-%s\n"),
			     conflicts[i].byName, 
		    conflicts[i].byVersion, conflicts[i].byRelease);
	} else {
	    fprintf(f, i18n(" conflicts with %s-%s-%s\n"),
		    conflicts[i].byName, 
		    conflicts[i].byVersion, conflicts[i].byRelease);
	    st += s.sprintf( i18n(" conflicts with %s-%s-%s\n"),
			     conflicts[i].byName, 
		    conflicts[i].byVersion, conflicts[i].byRelease);
	}
    }
    KpMsgE(i18n("Package not installed: %s"),st.data(),TRUE);
}

#endif

void rpmBuff(char *buff)
{
  kpkg->kp->setStatus(buff);
}

void rpmMess(char *buff)
{
  KpMsgE(0,buff,TRUE);
}

#endif
