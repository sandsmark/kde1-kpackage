//////////////////////////////////////////////////////////////
//      $Id: slackInterface.cpp 21143 1999-05-09 22:54:36Z mueller $
//
// Author: Toivo Pedaste
//
#include "config.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>
#include <setjmp.h>

#include <qdir.h>
#include <qfileinf.h>
#include <qregexp.h>

#include <kurl.h>

#include "packageInfo.h"
#include "slackInterface.h"
#include "updateLoc.h"
#include "kpackage.h"
#include "managementWidget.h"
#include "utils.h"
#include "procbuf.h"
#include "options.h"
#include "cache.h"
#include <klocale.h>


#define DIR "/var/log/packages/"
#define FILELIST "FILE LIST:\n"

extern KApplication *app;
extern Params *params;

static param pinstall[] =  {
  {"Test (do not install)",FALSE,FALSE},
  {0,FALSE,FALSE}
};

static param puninstall[] =  {
  {"Test (do not uninstall)",FALSE,FALSE},
  {0,FALSE,FALSE}
};

enum {INITIAL, INSTALLED, UNINSTALLED};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
SLACK::SLACK():pkgInterface()
{
  head = "SLACK";
  icon = "slack.xpm";

  pict = new QPixmap();
  *pict = globalKIL->loadIcon(icon);
  bad_pict = new QPixmap();
  *bad_pict = globalKIL->loadIcon("sbad.xpm");
  updated_pict = new QPixmap();
  *updated_pict = globalKIL->loadIcon("supdated.xpm");
  new_pict = new QPixmap();
  *new_pict = globalKIL->loadIcon("snew.xpm");

  packagePattern = "*.tgz *.tar.gz";
  queryMsg = strdup(i18n("Querying SLACK package list: "));
  typeID = "/slack";
  procMsg = strdup(i18n("Kpackage: Waiting on SLACK"));

  locatedialog = 0;

  locatedialog = new Locations(i18n("Location of Slackware package archives"));
  locatedialog->pLocations(1, 1, this,  i18n("I"),
  "Slackware", "*.TXT *.txt *.tgz *.tar.gz",
  i18n("Location of a 'PACKAGES.TXT' file for extended information"));
  locatedialog->pLocations(4, 1, this,  i18n("P"),
  "Slackware", "*.tgz *.tar.gz",
  i18n("Location of 'PACKAGES.TXT' file for Slackware distribution"),
  i18n("Location of base directory of Slackware distribution"));
  locatedialog->dLocations(2, 6, this,  i18n("D"),
  "Slackware", "*.tgz *.tar.gz",
  i18n("Location of directories containg Slackware packages"));

  connect(locatedialog,SIGNAL(returnVal(LcacheObj *)),
  	  this,SLOT(setAvail(LcacheObj *)));
  locatedialog->apply_slot();

  pinstall[0].name = strdup(i18n("Test (do not install)"));

  puninstall[0].name = strdup(i18n("Test (do not uninstall)"));

  initTranslate();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
SLACK::~SLACK()
{
  QDictIterator<char> it(*trl);
  while (it.current()) {
    free(it.current());
    ++it;
  }
  delete trl;
  free(procMsg);
}

//////////////////////////////////////////////////////////////////////////////
void SLACK::initTranslate()
{
  trl = new QDict<char>(53);

  trl->insert("a",strdup(i18n("Base System")));
  trl->insert("ap",strdup(i18n("Linux Applications")));
  trl->insert("d",strdup(i18n("Program Development")));
  trl->insert("e",strdup(i18n("GNU EMacs")));
  trl->insert("f",strdup(i18n("FAQs")));
  trl->insert("k",strdup(i18n("Kernel Source")));
  trl->insert("n",strdup(i18n("Networking")));
  trl->insert("t",strdup(i18n("TeX Distribution")));
  trl->insert("tcl",strdup(i18n("TCL Script Language")));
  trl->insert("x",strdup(i18n("X Window System")));
  trl->insert("xap",strdup(i18n("X Applications")));
  trl->insert("xd",strdup(i18n("X Development Tools")));
  trl->insert("xv",strdup(i18n("XView and OpenLook")));
  trl->insert("y",strdup(i18n("Games")));
}

param *SLACK::initinstallOptions()
{
  return &(pinstall[0]);
}

param *SLACK::inituninstallOptions()
{
  return &(puninstall[0]);
}

// check if slack file
bool SLACK::isType(char *buf, const char *fname)
{
  if ((unsigned char)buf[0] == 037 && (unsigned char)buf[1] == 0213 ) {
    return true;
  } else
    return false;
}

bool SLACK::parseName(QString name, QString *n, QString *v)
{
  int  s1;

  s1 = name.findRev('.');
  if (s1 > 0) {
    *n = name.left(s1);
    v = new QString("");
    return TRUE;
  }
  return FALSE;
}

void SLACK::listPackages(QList<packageInfo> *pki)
{
  QString s;
  cacheObj *cp;

  if (packageLoc) {
    for (cp = packageLoc->first(); cp != 0; cp = packageLoc->next()) {
      // first entry is special
      if (cp->cacheFile == "SLACK_0_0") {
	s = getPackList(cp);
	if (!s.isEmpty()) {
	  listPackList(pki, s, cp, INITIAL);
	}
      }
    }
  }

  listInstalledPackages(pki);

  if (params->DisplayP != Params::INSTALLED) {
    if (packageLoc) {
      for (cp = packageLoc->first(); cp != 0; cp = packageLoc->next()) {
	if (cp->cacheFile == "SLACK_0_0") {
	  // already done
	} else if (cp->base != "-") {
	  s = getPackList(cp);
	  if (!s.isEmpty()) {
	    listPackList(pki, s, cp, UNINSTALLED);
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

void SLACK::listInstalledPackages(QList<packageInfo> *pki)
{
  FILE *file;
  char linebuf[1024];
  QString vb;
  packageInfo *p;
  QString fn, dr = DIR;

  QDir d(DIR);
  if (d.exists()) {
    QString sline = i18n("Querying SLACK package list: ");
    kpkg->kp->setStatus(sline.data());

    const QFileInfoList *list = d.entryInfoList();
    int count = list->count();
    QFileInfoListIterator it( *list );      // create list iterator
    QFileInfo *fi;                          // pointer for traversing

    kpkg->kp->setPercent(0);
    int cnt = 0;
    while ( (fi=it.current()) ) {           // for each file...
      int n = (cnt*100)/count;
      if (!(n % 5))
	kpkg->kp->setPercent(n);

      if (!fi->isDir() && fi->isReadable()) {
	fn = dr + fi->fileName();
	file = fopen(fn.data(),"r");
	if (file) {
          vb = 0;
	  while (fgets(linebuf,sizeof(linebuf),file)) {
	    if (strcmp(linebuf,FILELIST)) {
	      vb += linebuf;
	    } else {
	      break;
	    }
	  }
	  fclose(file);
	  p = collectInfo(vb.data(), INSTALLED);
	  if (p) {
	    smerge(p);
	    if (!p->update(pki, typeID, TRUE))
	      delete p;
	  }
	}
      }
      cnt++;
      ++it;                          // goto next list element
    }
    kpkg->kp->setPercent(100);
  }
}

//////////////////////////////////////////////////////////////////////////////
void SLACK::listPackList(QList<packageInfo> *pki, QString s,  cacheObj *cp, int insState)
{
  int np;
  QString vb;
  char linebuf[1024];
  FILE *file;
  packageInfo *p;

  QString sline = i18n("Querying SLACK package list: ");
  sline += cp->location;

  kpkg->kp->setStatus(sline.data());
  kpkg->kp->setPercent(0);

  np = 0;
  file= fopen(s, "r");
  vb = "";

  if (file) {
    while (fgets(linebuf,sizeof(linebuf),file)) {
      int len = strlen(linebuf);
      if (len > 1) {
	if (linebuf[len - 2] == '\r') {
	  linebuf[len - 2] = '\n';
	  linebuf[len - 1] = 0;
	}
      }
      if (strcmp(linebuf,"\n")) {
	vb += linebuf;
      } else if (vb != "") {
	p = collectInfo(vb.data(), insState);
	if (p) {
	  if (!p->update(pki, typeID, insState == INITIAL, insState == INITIAL)) {
	    delete p;
	  }  else if (cp && insState != INITIAL) {
	    p->info->insert("base",new QString(cp->base));
	  }
	  if (p && insState == INITIAL) {
	    p->packageState = packageInfo::NOLIST;
	    p->info->remove("summary");
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
packageInfo *SLACK::getPackageInfo(char mode, const char *name, const char *version)
{
  char linebuf[1024];
  packageInfo *pki = 0;
  QString vb, search, fn;
  QString n,v;
  FILE *file;

  switch(mode) {
    ////////////////////////////////////////////////////////////////////////
    // query an installed package!
  case 'i':
    fn = DIR;
    fn += name;
    file = fopen(fn.data(),"r");
    if (file) {
      vb = 0;
      while (fgets(linebuf,sizeof(linebuf),file)) {
	if (strcmp(linebuf,FILELIST)) {
	  vb += linebuf;
	} else {
	  break;
	}
      }
      fclose(file);
      pki = collectInfo(vb.data(), INSTALLED);
      if (pki) {
	smerge(pki);
      }
     }
    break;

    ////////////////////////////////////////////////////////////////////
    // query an uninstalled package
  case 'u':
    QFile f(name);
    if (f.exists()) {
      QDict<QString> *a = new QDict<QString>;
      a->setAutoDelete(TRUE);

      a->insert("group", new QString(i18n("OTHER")));
      a->insert("filename", new QString(name));

      QFileInfo f(name);f.baseName();
      a->insert("name", new QString(f.baseName()));

      QString *st = new QString("");
      st->setNum(f.size());
      a->insert("file-size", st);

      pki = new packageInfo(a,this);
      if (pki) {
	smerge(pki);
	pki->updated = TRUE;
      }
    }
    break;
  }
  return pki;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
packageInfo *SLACK::collectInfo(const char *_inp, int insState)
{
  QString stmp, fn = "";
  QDict<QString> *a = new QDict<QString>;
  a->setAutoDelete(TRUE);

  char *str, *xstr;
  QString qstr;

  char *inp = qstrdup(_inp);
  str = strtok(inp,"\n");

  if (str) {
    do {
      if (str[0] == 0)
	break;

      xstr = strchr(str,':');
      if (xstr) {
	*xstr++ = 0;
	xstr++;
	while (*xstr == ' ') {
	  xstr++;
	}

	qstrcpy(str, QString(str).lower());
	if (*str == ' ')
	  str++;

	if (!strcmp("package name",str)) {
	  fn = xstr;
	  QString st = xstr;
	  if (st.right(4) == ".tgz")
	    a->insert("name", new QString(st.left(st.length() - 4)));
	  else
	    a->insert("name", new QString(st));
	} else if (!strcmp("package description",str)) {
	  int i = 0;
	  QString qstr = "";

	  while ((str = strtok(NULL,"\n"))) {
	    xstr = strchr(str,':');
	    if (xstr) {
	      *xstr++ = 0;
	      if (*(xstr) != 0)
		xstr++;
	      while (*xstr == ' ') {
		xstr++;
	      }
	      if (i == 0) {
		a->insert("summary", new QString(xstr));
	      } else {
		if (!strcmp(xstr,"") && (i != 1)) {
		  qstr += "\n";
		} else {
		  qstr += xstr;
		  qstr += " ";
		}
	      }
	    }
	    i++;
	  }
	  a->insert("description", new QString(qstr));
	} else if (!strcmp("package location",str)) {
	  QString sl = xstr;
	  if (insState != INSTALLED) {
	    int sls = sl.findRev("/");
	    if (sls >= 0) {
	      QRegExp num("[0-9][0-9]*");
	      int slf = sl.find(num,sls);
	      if (slf >= 0) {
		sls++;
		QString *gt = new QString(sl.mid(sls,slf-sls));
		if (trl->find(*gt)) {
		  *gt = trl->find(*gt);
		}
		a->insert("group",gt);
	      }
	    }
	    sl = sl.right(sl.length() - 2);
	    sl += "/";
	    sl += fn;
	  }
	  if (insState == UNINSTALLED) {
	    a->insert("filename", new QString(sl));
	  }
	} else if (!strcmp("section",str)) {
	  a->insert("group", new QString(xstr));
	} else if (!strcmp("compressed package size",str) ||
		   !strcmp("package size (compressed)",str)) {
	  QString *stmp = new QString(xstr);
	  stmp->truncate(stmp->length() - 2);
	  *stmp += "000";
	  a->insert("file-size", stmp);
	} else if (!strcmp("uncompressed package size",str) ||
		   !strcmp("package size (uncompressed)",str)) {
	  QString *stmp = new QString(xstr);
	  stmp->truncate(stmp->length() - 2);
	  *stmp += "000";
	  a->insert("size", stmp);
	} else {
	  a->insert(str, new QString(xstr));
	}
      }
    } while ((str = strtok(NULL,"\n")));
  }

  delete [] inp;

  if (!a->find("name")) {
    delete a;
    return 0;
  } else {
    packageInfo *i = new packageInfo(a,this);
    i->packageState = packageInfo::INSTALLED;
    i->fixup();
    return i;
  }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
QList<char> *SLACK::getFileList(packageInfo *p)
{
  char linebuf[1024];
  QString st, fn;
  FILE *file;
  const char *name;
  char mode;
  char *str;

  fn = p->getFilename();
  if(!fn.isEmpty())
    mode = 'u';
  else
    mode = 'i';

  QList<char> *filelist = new QList<char>;
  filelist->setAutoDelete(TRUE);

  switch(mode) {
      ////////////////////////////////////////////////////////////////////////
      // query an installed package!
    case 'i':
      name = p->getProperty("name")->data();

      fn = DIR;
      fn += name;
      file = fopen(fn.data(),"r");
      if (file) {
	while (fgets(linebuf,sizeof(linebuf),file)) {
	  if (!strcmp(linebuf,FILELIST)) {
	    break;
	  }
	}
	while (fgets(linebuf,sizeof(linebuf),file)) {
	  st = "/";
	  st += linebuf;
	  st.truncate(st.length() -1);
	  if (st.left(8) != "/install") {
	    filelist->append(strdup(st));
	  }
	}
	fclose(file);
      }
      break;

      ////////////////////////////////////////////////////////////////////
      // query an uninstalled package
    case 'u':
           name = fn.data();
	   reader.setup("kpackage_slack_l");
	   *reader.proc <<name;

	   if (!reader.start(0,TRUE))
	     return 0;

           char *buffer = qstrdup(reader.buf);
	   str = strtok(buffer,"\n");
	   do {
	     st = "/";
	     st += str;
	     filelist->append(strdup(st));
	   } while ((str = strtok(NULL,"\n")));
	   delete [] buffer;
      break;
    }

  return filelist;
}

//////////////////////////////////////////////////////////////////////////////
// Call the script to uninstall packages setting parameters
// to slack dependent on flags, returning whether everyting worked
//////////////////////////////////////////////////////////////////////////////
int SLACK::doUninstall(int uninstallFlags, QString packs)
{
  reader.setup("removepkg");

  if ((uninstallFlags>>0 & 1) ^ pinstall[0].invert)
    *reader.proc << "-warn";

  *reader.proc << packs.data();

  reader.start(procMsg);
  if (reader.buf.length() > 0) {
    KpMsg(0,"%s",reader.buf,FALSE);
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////////
// Call the script to install packages setting parameters
// to slack dependent on flags, returning whether everyting worked
//////////////////////////////////////////////////////////////////////////////
int SLACK::install(int installFlags, QList<packageInfo> *plist)
{
  packageInfo *pk;
  int i = 0;
  for (pk = plist->first(); pk != 0; pk = plist->next()) {
    QString fname = pk->fetchFilename();
    if (fname != "") {
      doInstall(installFlags, fname);
      i++;
    }
  }
  return 0;
}

int SLACK::doInstall(int installFlags, QString packs)
{

  reader.setup("installpkg");

  if ((installFlags>>0 & 1) ^ pinstall[0].invert)
    *reader.proc << "-warn";

  *reader.proc <<  packs.data();

  reader.start(procMsg);
  if (reader.buf.length() > 0) {
    KpMsg(0,"%s",reader.buf,FALSE);
  }

  return  0;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
QString SLACK::FindFile(const char *name)
{
  FILE *file;
  char linebuf[1024];
  QString buf, st;
  QString fn, dr = DIR;

  QDir d(DIR);
  if (d.exists()) {
    QString sline = i18n("Querying SLACK package list: ");
    kpkg->kp->setStatus(sline.data());

    const QFileInfoList *list = d.entryInfoList();
    int count = list->count();
    QFileInfoListIterator it( *list );      // create list iterator
    QFileInfo *fi;                          // pointer for traversing

    kpkg->kp->setPercent(0);
    int cnt = 0;
    while ( (fi=it.current()) ) {           // for each file...
      int n = (cnt*100)/count;
      if (!(n % 5))
	kpkg->kp->setPercent(n);

      if (!fi->isDir() && fi->isReadable()) {
	fn = dr + fi->fileName();
	file = fopen(fn.data(),"r");
	if (file) {
	  while (fgets(linebuf,sizeof(linebuf),file)) {
	    if (!strcmp(linebuf,FILELIST)) {
	      break;
	    }
	  }
	  while (fgets(linebuf,sizeof(linebuf),file)) {
	    if (strstr(linebuf,name)) {
	      st = "/";
	      st += linebuf;
	      st.truncate(st.length() -1);
	      if (st.left(8) != "/install") {
		buf += fi->fileName();
		buf += "\t";
		buf += st;
		buf += "\n";
	      }
	    }
	  }
	  fclose(file);
	}
      }
      cnt++;
      ++it;                          // goto next list element
    }
  }
  return buf;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void SLACK::setLocation()
{
    locatedialog->restore();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void SLACK::setAvail(LcacheObj *slist)
{
  if (packageLoc)
    delete packageLoc;
  packageLoc = slist;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void SLACK::smerge(packageInfo *p)
{
  p->smerge(typeID);
}
