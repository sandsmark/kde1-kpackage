//////////////////////////////////////////////////////////////         
//      $Id: debInterface.cpp 20708 1999-05-03 07:21:48Z coolo $ 
//
// Author: Toivo Pedaste
//
#include "config.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>		// for O_RDONLY
#include <time.h>		// for localtime
#include <setjmp.h>

#include <qdir.h> 
#include <qfileinf.h> 

#include <kurl.h>

#include "packageInfo.h"
#include "debInterface.h"
#include "updateLoc.h"
#include "kpackage.h"
#include "managementWidget.h"
#include "utils.h"
#include "procbuf.h"
#include "options.h"
#include "cache.h"
#include <klocale.h>


extern KApplication *app;
extern Params *params;

#define AVAIL          "/var/lib/dpkg/available"
#define STATUS          "/var/lib/dpkg/status"

#define INFODIR         "/var/lib/dpkg/info/"

static param pinstall[] =  {
  {"Allow Downgrade",TRUE,TRUE},
  {"Check Conflicts",TRUE,TRUE},
  {"Check Dependencies",TRUE,TRUE},
  {"Test (do not install)",FALSE,FALSE},
  {0,FALSE,FALSE}
};

static param puninstall[] =  {
  {"Purge Config Files",TRUE,FALSE},
  {"Check Dependencies",TRUE,TRUE},
  {"Test (do not uninstall)",FALSE,FALSE},
  {0,FALSE,FALSE}
};


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
DEB::DEB():pkgInterface()
{
  head = "DEB";
  icon = "deb.xpm";  

  pict = new QPixmap();
  *pict = globalKIL->loadIcon(icon); 
  bad_pict = new QPixmap();
  *bad_pict = globalKIL->loadIcon("dbad.xpm");
  updated_pict = new QPixmap();
  *updated_pict = globalKIL->loadIcon("dupdated.xpm");
  new_pict = new QPixmap();
  *new_pict = globalKIL->loadIcon("dnew.xpm");

  packagePattern = "*.deb";
  queryMsg = i18n("Querying DEB package list: ");
  typeID = "/deb";
  procMsg = strdup(i18n("Kpackage: Waiting on DPKG"));

  locatedialog = new Locations(i18n("Location of Debian package archives"));
  locatedialog->dLocations(2, 6, this, i18n("D"),
  "Deb", "*.deb",
  i18n("Location of directories containg Debian packages"));
  locatedialog->pLocations(4, 6, this, i18n("P"),
  "Deb", "*.deb",
  i18n("Location of 'Packages' files for sections of Debian distributions"),
  i18n("Location of base directory of Debian distribution"));
  connect(locatedialog,SIGNAL(returnVal(LcacheObj *)),
	  this,SLOT(setAvail(LcacheObj *)));
  locatedialog->apply_slot();

  pinstall[0].name = strdup(i18n("Allow Downgrade"));
  pinstall[1].name = strdup(i18n("Check Conflicts"));
  pinstall[2].name = strdup(i18n("Check Dependencies"));
  pinstall[3].name = strdup(i18n("Test (do not install)"));

  puninstall[0].name = strdup(i18n("Purge Config Files"));
  puninstall[1].name = strdup(i18n("Check Dependencies"));
  puninstall[2].name = strdup(i18n("Test (do not uninstall)"));

}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
DEB::~DEB()
{
  free(procMsg);
}

//////////////////////////////////////////////////////////////////////////////
param *DEB::initinstallOptions()
{
  return &(pinstall[0]);
}

param *DEB::inituninstallOptions()
{
  return &(puninstall[0]);
}

// check if debian file
bool DEB::isType(char *buf, const char *fname)
{
  if  (!strcmp(buf,"!<arch>\n")) {
    return true;
  } else if (!strncmp(buf,"0.9",3)) {
    return true;
  } else
    return false;
}

void DEB::listPackages(QList<packageInfo> *pki)
{ 
  QString s;
  cacheObj *cp;

  listInstalledPackages(pki);
  if (params->DisplayP != Params::INSTALLED) {
    if (packageLoc) {
      for (cp = packageLoc->first(); cp != 0; cp = packageLoc->next()) {
	if (cp->base != "-") {
	  s = getPackList(cp);
	  if (!s.isEmpty()) {
	    listPackList(pki,s.data(),cp);
	  }
	} else {
	  s = getDir(cp);
	  if (!s.isEmpty()) {
	    listDir(pki,s,cp->location);
	  }
	}
      }
    }
  }
}

bool DEB::parseName(QString name, QString *n, QString *v)
{
  int d1, d2, s1;

  s1 = name.findRev('.');
  if (s1 > 0) {
      d2 = name.findRev('-',s1-1);
      if (d2 > 0) {
	d1 = name.findRev('_',d2-1);
	if (d1 < 0)
	  d1 = d2;
	*n = name.left(d1);
	*v = name.mid(d1+1,s1-d1-1);
	return TRUE;
      }
  }
  return FALSE;
}

void DEB::listInstalledPackages(QList<packageInfo> *pki)
{
  listPackList(pki,STATUS,0);
}

void DEB::listPackList(QList<packageInfo> *pki, const char *fname, cacheObj *cp)
{
  int np;
  bool local = FALSE;
  QString vb;
  char linebuf[1024];
  FILE *file;
  packageInfo *p;

  QString sline = i18n("Querying DEB package list: ");
  sline += fname;

  if (cp) {
    KURL *u = new KURL(cp->base.data());
    if ( strcmp( u->protocol(), "file" ) == 0 ) {
      local = TRUE;
    }
    delete u;
  }

  kpkg->kp->setStatus(sline.data());
  kpkg->kp->setPercent(0);

  np = 0;
  file= fopen(fname,"r");
  vb = "";

  if (file) {
    while (fgets(linebuf,sizeof(linebuf),file)) {
      if (strcmp(linebuf,"\n")) {
	vb += linebuf;
      } else {
	p = collectInfo(vb.data());
	if (p) {
	  if (!p->update(pki, typeID, cp == 0)) {
	    delete p;
	  } else if (cp) {
	    p->info->insert("base",new QString(cp->base));
	  }
	}
	vb.truncate(0);
      }
    }
    fclose(file);
  }
  kpkg->kp->setPercent(100);
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// mode: i = query installed    u = query uninstalled
packageInfo *DEB::getPackageInfo(char mode, const char *name, const char *version)
{
  packageInfo *pki = 0; 
  QString vb,search;
  char linebuf[1024];
  FILE *file;
  bool found = false;
  
  switch(mode)
    {
      ////////////////////////////////////////////////////////////////////////
      // query an installed package!
    case 'i':
      file= fopen(STATUS,"r");
      vb = "";
      search = "Package: ";
      search += name;
      search += "\n";

      if (file) {
	while (fgets(linebuf,sizeof(linebuf),file)) {
	  if (!found) {
	    if (!strcmp(search.data(),linebuf)) {
	      found = true;
	    } else {
	      continue;
	    }
	  }
	  if (strcmp(linebuf,"\n")) {
	    vb += linebuf;
	  } else {
	    pki = DEB::collectInfo(vb.data());
	    break;	
	  }
	}
      }
      fclose(file);
      break;

      ////////////////////////////////////////////////////////////////////
      // query an uninstalled package      
    case 'u':
      reader.setup("dpkg");
      *reader.proc << "--info"<<name;
      if (reader.start()) {
	pki = DEB::collectInfo(reader.buf.data());
	if (pki)
	  pki->updated = TRUE;
      }
      break;
    }
  return pki;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
packageInfo *DEB::collectInfo(const char *_inp)
{
  QDict<QString> *a = new QDict<QString>;
  a->setAutoDelete(TRUE);

  char *inp = qstrdup(_inp);
  char *str, *xstr;
  QString qstr;
  bool bad_install = FALSE;
  bool available = FALSE;

    //   remove prefix output from dpkg
    str = strtok(inp,"\n");
    do {
      xstr = strchr(str,':');
      if (*str == ' ')
	str++;
      if (!strncmp("Package",str,7))
	break;
    } while ((str = strtok(NULL,"\n")));

    // parse 'name: text' elements
    
    if (str) {
      do {
cont:	if (str[0] == 0)
          break;

	xstr = strchr(str,':');
	if (xstr) {
	  *xstr++ = 0;
	  xstr++;
	  
	  qstrcpy(str, QString(str).lower());
	  if (*str == ' ')
	    str++;

	  if (!strcmp("conffiles",str)) {
	  } else if (!strcmp("description",str)) {
	    qstr = xstr;
	    qstr += "\n";
	    while ((str = strtok(NULL,"\n"))) {
	      if (str[0] == ' ') {
		qstr += str;
	      } else {
		a->insert("description", new QString(qstr.data()));
		goto cont;
	      }
	    }
	    a->insert("description", new QString(qstr.data()));

	  } else if (!strcmp("package",str)) {
	    a->insert("name", new QString(xstr));
	  } else if (!strcmp("md5sum",str)) {
	    available = TRUE;
	    bad_install = FALSE;
	  } else if (!strcmp("section",str)) {
	    a->insert("group", new QString(xstr));
	  } else if (!strcmp("status",str)) {
	    if (!strncmp(xstr+strlen(xstr)-13,"not-installed",13)) {
	      delete a;
	      delete [] inp;
	      return 0;
	    }
	    if (strcmp("install ok installed",xstr) && 
		strcmp("deinstall ok installed",xstr) &&
		strcmp("purge ok installed",xstr)) {
	      bad_install = TRUE;
	    }
	    a->insert(str, new QString(xstr));
	  } else if (!strcmp("size",str)) {
	    a->insert("file-size", new QString(xstr));
	  } else if (!strcmp("installed-size",str)) {
	    QString *stmp = new QString(xstr);
	    *stmp += "000";
	    a->insert("size", stmp);
	  } else {
	    a->insert(str, new QString(xstr));
	  }
	}
      } while ((str = strtok(NULL,"\n")));
    }

    packageInfo *i = new packageInfo(a,this);
    if (bad_install) {
      i->packageState = packageInfo::BAD_INSTALL;
    } else if (available) {
      i->packageState = packageInfo::AVAILABLE;
    } else {
      i->packageState = packageInfo::INSTALLED;
    }
    i->fixup();
    delete [] inp;
    return i;

}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
QList<char> *DEB::getFileList(packageInfo *p)
{
  const char *filelistfile;
  QString vb, fn;
  const char *name;
  char mode;
  char linebuf[1024];
  FILE *file;
  char *s, *str, *strp;
  int prlen;

  fn = p->getFilename();
  if(!fn.isEmpty())
    mode = 'u';
  else
    mode = 'i';

  QList<char> *filelist = new QList<char>;
  filelist->setAutoDelete(TRUE);

  switch(mode)
    {
      ////////////////////////////////////////////////////////////////////////
      // query an installed package!
    case 'i':
      name = p->getProperty("name")->data();

      vb = INFODIR;
      vb += name;
      vb += ".list";
      filelistfile= vb.data();
      file= fopen(filelistfile,"r");

      if (file) {
	while (fgets(linebuf,sizeof(linebuf),file)) {
	  s = strdup(linebuf);
	  s[strlen(s) - 1] = 0;    // remove new line
	  filelist->append(s);
	}
	fclose(file);
      }
      break;

      ////////////////////////////////////////////////////////////////////
      // query an uninstalled package      
    case 'u':
      name = fn.data();
      reader.setup("dpkg");
      *reader.proc << "--contents"<<name;

      if (!reader.start(procMsg))
	return 0;

      char *buffer = qstrdup(reader.buf);
      str = strtok(buffer,"\n");
      strp = strrchr(str,' ');
      if (strp) {
	prlen = strp - str;
      } else {
	prlen = 0;
      }
      do {
	filelist->append(strdup(str+prlen));
      } while ((str = strtok(NULL,"\n")));
      break;
      delete [] buffer;
    }

  return filelist;
}

//////////////////////////////////////////////////////////////////////////////
// Use the ~/.dpkg_ok file to tell if dpkg has worked or failed
// file created by the install/uninstall scripts
//////////////////////////////////////////////////////////////////////////////
int DEB::dpkgChange(int del)
{
    QDateTime st;
    st.setTime_t(0);

    char *tmpd = getenv("HOME");
    if (tmpd) {
      QString tfile;
      tfile.sprintf("%s/.dpkg_ok",tmpd);
      QFileInfo *fileInfo = new QFileInfo(tfile.data());
      if (fileInfo->exists()) {
	return st.secsTo(fileInfo->lastModified());
	if (del)
	  unlink(tfile.data());
      } else {
	return 0;
      }
    } else
      return 0;
}

//////////////////////////////////////////////////////////////////////////////
// Call the script to uninstall packages setting parameters
// to dpkg dependent on flags, returning whether everyting worked
//////////////////////////////////////////////////////////////////////////////
int DEB::doUninstall(int uninstallFlags, QString packs)
{
  int test = 0, ctime, nctime;

  ctime = dpkgChange(0);

  reader.setup("kvt");
  *reader.proc << "-e" << "kpackage_dpkg_rm";

  *reader.proc <<  i18n("'Delete this window to continue'");

  if ((uninstallFlags>>0 & 1) ^ puninstall[0].invert)
    *reader.proc << "--purge";
  else
    *reader.proc << "--remove";
  if ((uninstallFlags>>1 & 1) ^ puninstall[1].invert)
    *reader.proc << "--force-depends";
  if ((uninstallFlags>>2 & 1) ^ puninstall[2].invert) {
    *reader.proc << "--no-act";
    test = 1;
  }

  *reader.proc << packs.data();

  reader.start(procMsg);
  nctime = dpkgChange(1);
  
  return (nctime == ctime) || test;
}

//////////////////////////////////////////////////////////////////////////////
// Call the script to install packages setting parameters
// to dpkg dependent on flags, returning whether everyting worked
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int DEB::doInstall(int installFlags, QString packs)
{
  int test = 0, ctime, nctime;

  ctime = dpkgChange(0);

  reader.setup("kvt");
  *reader.proc << "-e" << "kpackage_dpkg_ins";
  *reader.proc <<  i18n("'Delete this window to continue'");

  if ((installFlags>>0 & 1) ^ pinstall[0].invert)
    *reader.proc << "--refuse-downgrade";
  if ((installFlags>>1 & 1) ^ pinstall[1].invert)
    *reader.proc << "--force-conflicts";
  if ((installFlags>>2 & 1) ^ pinstall[2].invert)
    *reader.proc << "--force-depends";
  if ((installFlags>>3 & 1) ^ pinstall[3].invert) {
    *reader.proc << "--no-act";
    test = 1;
  }

  *reader.proc <<  packs.data();

  reader.start(procMsg);
  nctime = dpkgChange(1);
  
  return (nctime == ctime) || test;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
QString DEB::FindFile(const char *name)
{
  reader.setup("dpkg");
  *reader.proc << "-S"<<name;

  if (!reader.start(procMsg))
    return "";

  int p = 0,lines = 0;
  while ((p = reader.buf.find(':',p)) >= 0) {
    lines++;
    reader.buf.replace(p,1,"\t");
    p = reader.buf.find('\n',p);
  }

  if (lines == 1) {
    if (reader.buf.find("not found") >= 0)
      reader.buf.truncate(0);
  }


  return reader.buf;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DEB::setLocation()
{
    locatedialog->restore();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DEB::setAvail(LcacheObj *slist)
{
  if (packageLoc)
    delete packageLoc;
  packageLoc = slist;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
