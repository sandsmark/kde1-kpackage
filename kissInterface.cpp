//////////////////////////////////////////////////////////////         
//      $Id: kissInterface.cpp 20708 1999-05-03 07:21:48Z coolo $ 
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

#include <kurl.h>

#include "packageInfo.h"
#include "kissInterface.h"
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

static param pinstall[] =  {
  {0,FALSE,FALSE}
};

static param puninstall[] =  {
  {0,FALSE,FALSE}
};


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
KISS::KISS():pkgInterface()
{
  head = "KISS";
  icon = "kiss.xpm";  

  pict = new QPixmap();
  *pict = globalKIL->loadIcon(icon); 
  bad_pict = new QPixmap();
  *bad_pict = globalKIL->loadIcon("dbad.xpm");
  updated_pict = new QPixmap();
  *updated_pict = globalKIL->loadIcon("dupdated.xpm");
  new_pict = new QPixmap();
  *new_pict = globalKIL->loadIcon("dnew.xpm");

  packagePattern = "*.kiss";
  queryMsg = i18n("Querying KISS package list: ");
  typeID = "/kiss";
  procMsg = strdup(i18n("Kpackage: Waiting on KISS"));

  locatedialog = 0;

}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
KISS::~KISS()
{
  free(procMsg);
}

//////////////////////////////////////////////////////////////////////////////
param *KISS::initinstallOptions()
{
  return &(pinstall[0]);
}

param *KISS::inituninstallOptions()
{
  return &(puninstall[0]);
}

// check if debian file
bool KISS::isType(char *buf, const char *fname)
{
    return false;
}

bool KISS::parseName(QString name, QString *n, QString *v)
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

void KISS::listInstalledPackages(QList<packageInfo> *pki)
{
  QString vb;
  packageInfo *p;

  QString sline = i18n("Querying KISS package list: ");
 
  reader.setup("kiss");
  *reader.proc << "-qq";
  if (!reader.start(0,FALSE))
    return;
  
  kpkg->kp->setStatus(sline.data());
  kpkg->kp->setPercent(0);

  vb = "" ;

  int sc, sp = 0;
  while ((sc  = reader.buf.find("\n\n",sp)) >= 0) {
    if (sc+1 == (signed int)reader.buf.length())
      break;
    p = collectInfo(reader.buf.mid(sp,sc-sp).data());
    if (p) {
      if (!p->update(pki, typeID, TRUE)) {
	delete p;
      }
    }
    sp = sc + 2;
  }

  kpkg->kp->setPercent(100);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// mode: i = query installed    u = query uninstalled
packageInfo *KISS::getPackageInfo(char mode, const char *name, const char *version)
{
  packageInfo *pki = 0; 
  QString vb,search;
  
  switch(mode)
    {
      ////////////////////////////////////////////////////////////////////////
      // query an installed package!
    case 'i':
      reader.setup("kiss");
      *reader.proc << "-f" << name;
      if (reader.start(0,FALSE)) {
	reader.buf += "package: ";
	reader.buf += name;
	reader.buf += "\n";
	pki = collectInfo(reader.buf.data());
      }	
      break;

      ////////////////////////////////////////////////////////////////////
      // query an uninstalled package      
    case 'u':
      break;
    }
  return pki;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
packageInfo *KISS::collectInfo(const char *_inp)
{
  QDict<QString> *a = new QDict<QString>;
  a->setAutoDelete(TRUE);

  char *str, *xstr;
  QString qstr;

  char *inp = qstrdup(_inp); 
  str = strtok(inp,"\n");
  do {
    qstrcpy(str, QString(str).lower());
    xstr = strchr(str,':');
    if (*str == ' ')
      str++;
    if (!strncmp("package",str,7))
      break;
  } while ((str = strtok(NULL,"\n")));

  // parse 'name: text' elements
    
  if (str) {
    do {
      if (str[0] == 0)
	break;

    xstr = strchr(str,':');
    if (xstr) {
      *xstr++ = 0;
      xstr++;
	  
      qstrcpy(str, QString(str).lower());
      if (*str == ' ')
	str++;

      if (!strcmp("package",str)) {
	a->insert("name", new QString(xstr));
      } else if (!strcmp("name",str)) {
	a->insert("summary", new QString(xstr));
      } else if (!strcmp("section",str)) {
	a->insert("group", new QString(xstr));
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
  i->packageState = packageInfo::INSTALLED;
  i->fixup();
  delete [] inp;
  return i;

}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
QList<char> *KISS::getFileList(packageInfo *p)
{
  QString vb, fn;
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

  switch(mode)
    {
      ////////////////////////////////////////////////////////////////////////
      // query an installed package!
    case 'i':
      name = p->getProperty("name")->data();

      reader.setup("kiss");
      *reader.proc << "-f" << name;
      if (reader.start(0,FALSE)) {
	char *buffer = qstrdup(reader.buf);
	str = strtok(buffer,"\n");
	if (str) {
	  do {
	    filelist->append(strdup(str));
	  } while ((str = strtok(NULL,"\n")));
	}
	delete [] buffer;
      }
      break;

      ////////////////////////////////////////////////////////////////////
      // query an uninstalled package      
    case 'u':
      break;
    }
   
  return filelist;
}

//////////////////////////////////////////////////////////////////////////////
// Call the script to uninstall packages setting parameters
// to kiss dependent on flags, returning whether everyting worked
//////////////////////////////////////////////////////////////////////////////
int KISS::doUninstall(int uninstallFlags, QString packs)
{
  reader.setup("kiss");
  *reader.proc << "-d"; 
  *reader.proc << packs.data();

  reader.start(procMsg);
  
  return 0;
}

//////////////////////////////////////////////////////////////////////////////
// Call the script to install packages setting parameters
// to kiss dependent on flags, returning whether everyting worked
//////////////////////////////////////////////////////////////////////////////
int KISS::doInstall(int installFlags, QString packs)
{

  
  return  0;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
QString KISS::FindFile(const char *name)
{
  QString buf;

  reader.setup("kiss");
  *reader.proc << "-p"<<name;

  if (!reader.start(procMsg))
    return "";

  char *buffer = qstrdup(reader.buf);
  char *str = strtok(buffer,"\n");
  if (str) {
    do {
      buf += str;
      buf += "\t";
      buf += name;
      buf += "\n";
    } while ((str = strtok(NULL,"\n")));
  }
  delete [] buffer;

  return buf;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void KISS::setLocation()
{
    locatedialog->restore();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void KISS::setAvail(LcacheObj *slist)
{
  if (packageLoc)
    delete packageLoc;
  packageLoc = slist;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
