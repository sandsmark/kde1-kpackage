///////////////////////////////////////////////////////////////////////////////
// $Id: packageInfo.cpp 21143 1999-05-09 22:54:36Z mueller $
// File  : packageInfo.cpp
// Author: Damyan Pepper
// Author: Toivo Pedaste
//
///////////////////////////////////////////////////////////////////////////////

#include "../config.h"
// Standard headers
#include <stdlib.h>

#include <qregexp.h>
#include <qdir.h>

// KDE headers
#include <kapp.h>

// kpackage headers
#include "kpackage.h"
#include "ktvitem.h"
#include "packageInfo.h"
#include "pkgInterface.h"
#include "managementWidget.h"
#include "utils.h"
#include "options.h"
#include <klocale.h>

extern KIconLoader  *globalKIL;
extern Params *params;

// Global pixmap for
QPixmap *pict = NULL;

//////////////////////////////////////////////////////////////////////////////
// Constructor -- get the pixmap
packageInfo::packageInfo(QDict<QString> *_info, pkgInterface *type)
{
  interface = type;

  info = _info;
  item = NULL;
  packageState = UNSET;
  updated = FALSE;
  url = QString::null;
}

// Another constructor, for a packge with a url
packageInfo::packageInfo(QDict<QString> *_info, QString _url)
{
  info = _info;
  url = _url;
  item = NULL;
}

packageInfo::~packageInfo()
{
  if (info)
    delete info;
}

// Return a property
QString *packageInfo::getProperty(char *property)
{
  return info->find(property);
}

// Return the property dictionary
QDict<QString> *packageInfo::getDict()
{
  return info;
}

// Initialize fields if missing
void packageInfo::fixup()
{
  if (!info->find("group")) {
    info->insert("group", new QString(i18n("OTHER")));
  }

  if (!info->find("version")) {
    info->insert("version", new QString(""));
  }

  if (!info->find("name")) {
    QString *q = new QString("");;
    q->setNum((int)this);
    info->insert("name", q);
  }
}

// Set the file name
void packageInfo::setFilename(const char *f)
{
  url = f;
}

// Get the url
QString packageInfo::getUrl()
{
  if (url.isEmpty()) {
    if (getProperty("base")) {
      QString fs = getProperty("base")->data();
      fs += "/";
      if (getProperty("filename")) {
	fs += *getProperty("filename");
	url = fs;
      }
    }
  }
  return url;
}

QString packageInfo::fetchFilename()
{
  QString f = getFilename();

  if (!f.isEmpty()) {
    return f;
  } else {
    QString aurl = getUrl();
    if (!aurl.isEmpty()) {
      return kpkg->kp->fetchNetFile(aurl);
    } else {
      return "";
    }
  }
}

QString packageInfo::getFilename()
{
  QString cn = "";
  QString aurl = getUrl();
  if (!aurl.isEmpty()) {
    return KPACKAGE::getFileName(aurl,cn);
  } else {
    return "";
  }
}

int packageInfo::getDigElement(QString s, int *pos)
  // Extract the next element from the string
  // All digits
{
  int ni = *pos,  nf, val;
  const char *ss= s.data();
  ss += ni;

  if ((ni < 0) || s.length() == 0 || s.length() == (unsigned)ni)
    return -1;

  QRegExp ndig("[^0-9]");

  if (isdigit(*ss)) {
    nf = s.find(ndig,ni);
    if (nf >= 0) {
      val = s.mid(ni,nf-ni).toInt();
    } else {
      val = s.right(s.length()-ni).toInt();
      nf = s.length();
    }
  } else {
    val  = 0;
    nf = ni;
  }
  //  printf("n=%s %d %d\n",s.mid(nf,999).data(),nf,val);
  *pos = nf;
  return val;
}

QString packageInfo::getNdigElement(QString s, int *pos)
  // Extract the next element from the string
  // All  all non-digits
{
  int ni = *pos,  nf;
  const char *ss= s.data();
  ss += ni;
  QString str;

  if ((ni < 0) || s.length() == 0 || s.length() == (unsigned)ni)
    return "";

  QRegExp idig("[0-9]");

  if (!isdigit(*ss)) {
    nf = s.find(idig,ni);
    if (nf <  0)
      nf = s.length();
    str = s.mid(ni,nf-ni).data();
    for (unsigned int i = 0; i < str.length() ; i++) {
      if (!isalpha(str.data()[i])) {
	char t = str.data()[i];
	t += 128;
	str[i] = t;
      }
    }
  } else {
    str  = QString::null;
    nf = ni;
  }
  //  printf("s=%s %d %s\n",s.mid(nf,999).data(),nf,str.data());
  *pos = nf;
  return str;
}


int packageInfo::pnewer(QString s, QString sp)
{
  int ns = 0, nsp = 0, vs, vsp;

  while (TRUE) {
    vs = getDigElement(s,&ns);
    vsp = getDigElement(sp,&nsp);
    if (vs < 0 && vsp < 0)
      return 0;
    if (vs < 0 && vsp < 0)
      return 1;
    if (vs < 0 && vsp < 0)
      return -1;
    if (vsp > vs)
      return 1;
    else if (vs > vsp)
      return -1;

    QString svs = getNdigElement(s,&ns);
    QString svsp = getNdigElement(sp,&nsp);
    if (svs == "" && svsp == "")
      return 0;
    if (svs == "" && svsp != "")
      return 1;
    if (svs != "" && svsp == "")
      return -1;
    int n = strcmp(svsp.data(),svs.data());
    if (n != 0)
      return n;
  }
}

int packageInfo::newer(packageInfo *p)
{
  int vs, vsp, n;
  QString ss,is,fs,ssp,isp,fsp;

  QString s = *getProperty("version");
  QString sp = p->getProperty("version")->data();
  QString *rel = p->getProperty("release"); //RPM separates off release
  QString *ser = p->getProperty("serial"); //RPM separates off serial number

  if (rel) {
    sp += "-";
    sp += *rel;
  }

  if (ser) {
    QString stmp = sp;
    sp = *ser;
    sp += ":";
    sp += stmp;
  }

  vs = s.find(':');
  if (vs > 0) {
    ss = s.mid(0,vs);
    s = s.right(s.length()-(vs+1));
  } else {
    ss = "";
  }
  vs = s.findRev('-');
  if (vs > 0) {
    is = s.mid(0,vs);
    fs = s.right(s.length()-(vs+1));
  } else {
    is = s;
    fs = "";
  }

  vsp = sp.find(':');
  if (vsp > 0) {
    ssp = sp.mid(0,vsp);
    sp = sp.right(sp.length()-(vsp+1));
  } else {
    ssp = "";
  }
  vsp = sp.findRev('-');
  if (vsp > 0) {
    isp = sp.mid(0,vsp);
    fsp = sp.right(sp.length()-(vsp+1));
  } else {
    isp = sp;
    fsp = "";
  }

  //printf("s:%s=%s,i:%s=%s,f:%s=%s\n",
  //  ss.data(),ssp.data(),is.data(),isp.data(),fs.data(),fsp.data());
  n =  pnewer(ss,ssp);
  if (n)
    return n;
  else {
    n = pnewer(is,isp);
    if (n)
      return n;
    else
      return pnewer(fs,fsp);
  }
}


//////////////////////////////////////////////////////////////////////
// Place the package in a QListView

QListViewItem *packageInfo::place(QListView *tree, bool insertI)
{
  QListViewItem *search = tree->firstChild(), *parent=NULL, *child=NULL;
  QString qtmp, tmp;
  bool doit = FALSE;

  switch (params->DisplayP) {
  case Params::INSTALLED:
    if (packageState == INSTALLED || packageState == BAD_INSTALL)
      doit = TRUE;
    break;
  case Params::UPDATED:
    if (packageState == UPDATED)
      doit = TRUE;
    break;
  case Params::NEW:
    if  ((packageState == UPDATED) || (packageState == NEW))
      doit = TRUE;
    break;
  case Params::ALL:
      doit = TRUE;
    break;
  };
  if (packageState == NOLIST)
    doit = FALSE;

  if (doit) {
    qtmp = interface->head.data();
    qtmp += "/";
    qtmp += getProperty("group")->data();
    char *gstructc = strdup(qtmp.data());
    char *gstruct = gstructc;
    int cnt = 0;

    while(*gstruct) {			// get path to parent of this item
      char *gtrav = getGroup(&gstruct);
      QListViewItem *group;

      if( search && (group=findGroup(gtrav, search)) ) {
	parent = group;
	parent->setOpen(TRUE);
	search = group->firstChild();
      } else {
	if (parent) {
	  group = new KTVItem(parent, this, *interface->folder, gtrav);
	} else {
	  group = new KTVItem(tree, this, *interface->folder, gtrav);
	}
	parent = group;
	parent->setOpen(TRUE);
	search = NULL;
      }
      delete(gtrav);
      cnt++;
    }

    tmp = *info->find("name");

    if(item)
      delete item;

    QString sz = "";
    if (info->find("size")) {
      sz = info->find("size")->stripWhiteSpace().data();
      if (sz.length() > 3)
	sz.truncate(sz.length() - 3);
      else
	sz = "0";
      sz += "K";
    } else if (info->find("file-size")) {
      sz = info->find("file-size")->stripWhiteSpace().data();
      if (sz.length() > 3)
	sz.truncate(sz.length() - 3);
      else
	sz = "0";
      sz += "k";
    }
    sz = sz.rightJustify(6,' ');

    const char *ver = "";
    if (info->find("version")) {
      ver = info->find("version")->data();
    }

    const char *over = "";
    if (info->find("old-version")) {
      over = info->find("old-version")->data();
    }

    QPixmap pic;
    if (packageState == BAD_INSTALL) {
      pic = *interface->bad_pict;
    } else if (packageState == UPDATED) {
      pic = *interface->updated_pict;
    } else if (packageState == NEW) {
      pic = *interface->new_pict;
    } else if (packageState == INSTALLED) {
      pic = *interface->pict;
    } else {
      pic = *interface->pict;
    }

    if (child) {
      item =  new KTVItem(child, this, pic, tmp, sz, ver, over);
    } else {
      item = new KTVItem(parent, this, pic, tmp, sz, ver, over);
    }

    if (insertI) {
       parent->setOpen(TRUE);
    } else {
       parent->setOpen(FALSE);
    }

    free(gstructc);
    return item;
  } else {
    return 0;
  }
}

//////////////////////////////////////////////////////////////////////

// Get the QListViewItem
QListViewItem *packageInfo::getItem()
{
  return item;
}

//////////////////////////////////////////////////////////////////////////////
bool packageInfo::smerge( char *exp) {

  QDict<packageInfo> *dirInfoPackages = kpkg->kp->management->dirInfoPackages;
  QString pname = *getProperty("name") + exp;

  packageInfo *pi = dirInfoPackages->find(pname.data());
  if (pi) {
    QDictIterator<QString> it( *(pi->info) );

    while ( it.current() ) {
      if (!(it.currentKey() == "size" && info->find("size")) ||
	  !(it.currentKey() == "file-size"  && info->find("file-size"))) {
	info->insert(it.currentKey(), new QString(it.current()->data()));
      }
      ++it;
    }
    return TRUE;
  }
  return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
bool packageInfo::update(QList<packageInfo> *pki, char *exp,
			 bool installed, bool infoPackage)
{
  QDict<packageInfo> *dirInstPackages = kpkg->kp->management->dirInstPackages;
  QDict<packageInfo> *dirUninstPackages = kpkg->kp->management->dirUninstPackages;
  QDict<packageInfo> *dirInfoPackages = kpkg->kp->management->dirInfoPackages;

  QString pname = *getProperty("name") + exp;
  //  printf("U1=%s\n",pname.data());

  bool shouldUpdate = TRUE;
  packageInfo *pi = dirInstPackages->find(pname.data());

  //   if (pi) {
  //    printf("         C=%s %s %d\n",getProperty("version")->data(),
  //	   pi->getProperty("version")->data(),newer(pi));
  //  }

  if (pi && (newer(pi) >= 0)
      && (pi->packageState != BAD_INSTALL)
      && (pi->packageState != NOLIST))  {
    shouldUpdate = FALSE;
  } else {
    packageInfo *pu = dirUninstPackages->find(pname.data());
    if (pu && (newer(pu) >= 0)
	&& (pu->packageState != BAD_INSTALL)
	&& (pu->packageState != NOLIST))  {
      shouldUpdate = FALSE;
    }
  }

  if (getProperty("version")->isEmpty()) {
    shouldUpdate = TRUE;
  }

  if (shouldUpdate) {
    if (packageState != BAD_INSTALL) {
      if (installed)
	packageState = INSTALLED;
      else if (pi) {
	QString *version = pi->getProperty("version");
	if (version->isEmpty()) {
	  if (pi->packageState == NOLIST)
	    packageState = NEW;
	  else
	    packageState = UPDATED;
	} else {
	  packageState = UPDATED;
	  QString *oversion = pi->getProperty("old-version");
	  if (oversion) {
	    info->insert("old-version",new QString(oversion->data()));
	  } else if (version)
	    info->insert("old-version",new QString(version->data()));
	  QString *group = getProperty("group");
	  if (group && *group == "NEW") {
	    QString *ogroup = pi->getProperty("group");
	    if (ogroup)
	      info->replace("group",new QString(ogroup->data()));
	  }
	}
      } else
	packageState = NEW;
    }

    pki->insert(0,this);
    if (installed) {
      if (infoPackage)
	dirInfoPackages->insert(pname.data(),this);
      else
	dirInstPackages->insert(pname.data(),this);
    } else
      dirUninstPackages->insert(pname.data(),this);
    return TRUE;
  } else
    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

