//////////////////////////////////////////////////////////////         
//      $Id: fbsdInterface.cpp 20307 1999-04-25 15:38:26Z toivo $
//
// Author: Alex Hayward
// xelah@ferret.lmh.ox.ac.uk
//

#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <klocale.h> 

#include "fbsdInterface.h"
#include "kpackage.h"
#include "updateLoc.h"
#include "cache.h"
#include "options.h"

#define PKG_INFO_BIN "/usr/sbin/pkg_info"
#define PKG_ADD_BIN "/usr/sbin/pkg_add"
#define PKG_DELETE_BIN "/usr/sbin/pkg_delete"
//#define PKG_ADD_BIN "/bin/echo"
//#define PKG_DELETE_BIN "/bin/echo"

extern Params *params;

static param pinstall[] =  {
  {"Ignore Scripts",FALSE,FALSE},
  {"Check Dependencies",TRUE,TRUE},
  {"Test (do not install)",FALSE,FALSE},
  {0,FALSE,FALSE}
};

static param puninstall[] =  {
  {"Ignore Scripts",FALSE,FALSE},
  {"Check Dependencies",TRUE,TRUE},
  {"Test (do not uninstall)",FALSE,FALSE},
  {0,FALSE,FALSE}
};

fbsdInterface::fbsdInterface() {
  head = "BSD";

  locatedialog = new Locations(i18n("Location of BSD packages and ports"));
//  locatedialog->pLocations(1, 0, this, i18n("Ports"), "Pkg", "*.tgz",
//                           i18n("Location of extra ports index files"),
//                           i18n("Location of ports tree (eg /usr/ports or /usr/opt)"));
  locatedialog->dLocations(1, 1, this, i18n("Ports"), "Pkg", "*.tgz",
                           i18n("Location of ports tree (eg /usr/ports or /usr/opt)"));
  locatedialog->dLocations(1, 6, this, i18n("Packages"), "Pkg", "*.tgz",
                           i18n("Location of directories containing BSD packages or package trees"));
  connect(locatedialog, SIGNAL(returnVal(LcacheObj *)), this, SLOT(setAvail(LcacheObj *)));
  locatedialog->apply_slot();
  
  pinstall[0].name = strdup(i18n("Ignore Scripts"));
  pinstall[1].name = strdup(i18n("Check Dependencies"));
  pinstall[2].name = strdup(i18n("Test (do not install)"));

  puninstall[0].name = strdup(i18n("Ignore Scripts"));
  puninstall[1].name = strdup(i18n("Check Dependencies"));
  puninstall[2].name = strdup(i18n("Test (do not uninstall)"));

  icon = "rpm.xpm";
  pict = new QPixmap();
  *pict = globalKIL->loadIcon(icon); 
  bad_pict = new QPixmap();
  *bad_pict = globalKIL->loadIcon("dbad.xpm");
  updated_pict = new QPixmap();
  *updated_pict = globalKIL->loadIcon("rupdated.xpm");
  new_pict = new QPixmap();
  *new_pict = globalKIL->loadIcon("rnew.xpm");

  packagePattern = "*.tgz";
  queryMsg = i18n("Querying package list: ");
  typeID = "/tgz";
}

fbsdInterface::~fbsdInterface() {
}

bool fbsdInterface::isType(char *buf, const char *fname) {
  // These files are .tgz files. Pass it to pkg_info and see whether it
  // succeeds.
  reader.setup(PKG_INFO_BIN);
  *reader.proc << fname;
  if (reader.start(0, false)) return true;
  return false;
}


param *fbsdInterface::initinstallOptions() {
  return &(pinstall[0]);
}

param *fbsdInterface::inituninstallOptions() {
  return &(puninstall[0]);
}


packageInfo *fbsdInterface::getPackageInfo(char mode, const char *pname, const char *version) {
  bool installed = false;
  char *name = (char *) pname;
  kpkg->kp->setStatus(i18n("Getting package info"));

  if (mode == 'i' && version && version[0] != 0) {
    int nlen = strlen(pname), vlen = strlen(version);
    name = (char *) malloc(nlen + vlen + 2);
    memcpy(name, pname, nlen);
    name[nlen] = '-';
    memcpy(name + nlen+1, version, vlen);
    name[nlen+vlen+1] = 0;
  }
    
  QDict<QString> *a = new QDict<QString>;
  a->setAutoDelete(TRUE);      

  // Get the package name first (for mode = 'u').
  if (mode == 'u') {
    reader.setup(PKG_INFO_BIN);
    *reader.proc << "-qf" << name;

    char *last_dir = strrchr(name, '/');
    a->insert("filename", new QString(last_dir? (last_dir+1) : name));
    a->insert("base", last_dir? (new QString(name, last_dir - name + 2)) : new QString(""));
   
    // Look for a line of the form '@name <pkgname>'
    if (reader.start(0, FALSE)) {
      char *buf = strdup(reader.buf.data());
      char *n = strtok(buf, "\n");

      while (n != 0) {
	if (!memcmp(n, "@name", sizeof("@name")-1)) {
	  n += sizeof("@name ")-1;
	  while (isspace(*n)) n++;
	  addNV(a, n);
	  break;
	}
	n = strtok(0, "\n");
      }
      free(buf);
    }
  } else addNV(a, name);

  // Open a pipe to a pkg_info process in order to read the one line comment
  // and description for the package. This works for both installed packages
  // and for files.
  reader.setup(PKG_INFO_BIN);
  *reader.proc << "-q" << name;

  if (reader.start(0, false)) {
    char *buf = strdup(reader.buf.data());
    char *line = strtok(buf, "\n");

    // Everything up to the first newline is the one line comment.
    a->insert("summary", new QString(line));
    line = strtok(0, "\n");

    // Skip any blank lines and then read to EOF to get the description.
    while (line && line[0] == 0) line = strtok(0, "\n");

    QString *desc = new QString;
    while (line) {
      // Remove any new lines from the string. However, double ones should be preserved
      // as part of some semblence of an attempt to preserve the formatting...
      int line_len = strlen(line);
      if (line_len == 0) desc->append("\n");
      else {
	desc->append(QString(line));
	desc->append(" ");
      }

      line = strtok(0, "\n");

//      desc->append(QString(line));
//      desc->append(' ');
//      line = strtok(0, "\n");
    }

    bsdPortsIndexItem *inditem = bsdPortsIndexItem::find(name);

    if (inditem) {
      installed = inditem->installed;
      a->insert("maintainer", new QString(inditem->maint));

      assert(inditem->cats);
      const char *space = strchr(inditem->cats, ' ');
      if (space) {
	a->insert("group", new QString(inditem->cats, space - inditem->cats + 1));
	a->insert("also in", new QString(space+1));
      } else a->insert("group", new QString(inditem->cats));

      assert(inditem->bdeps);
      assert(inditem->rdeps);
      a->insert("run depends", new QString(inditem->rdeps[0]? inditem->rdeps : (const char *) i18n("none")));
      a->insert("build depends", new QString(inditem->bdeps[0]? inditem->bdeps : (const char *) i18n("none")));
      a->insert("available as", new QString(inditem->bin ? (inditem->port? i18n("binary package and source port") : i18n("binary package")) : i18n("source port")));
    }

    a->insert("description", desc);
    free(buf);
  } else {
    kpkg->kp->setStatus("");
    return 0;
  }

  packageInfo *ret = new packageInfo(a, this);
  ret->packageState = installed? packageInfo::INSTALLED : packageInfo::AVAILABLE;
  if (!installed) ret->smerge(typeID);
  ret->fixup();
  kpkg->kp->setStatus("");
  return ret;
}

QList<char> *fbsdInterface::getFileList(packageInfo *p) {
  assert(p != 0);

  // Run pkg_info on the package name to get the file list.
  // The file list is returned on stdout, one per line.
  kpkg->kp->setStatus(i18n("Getting file list"));

  QList<char> *ret = new QList<char>;
  ret->setAutoDelete(true);

  // Find the full name 'name-version', or just 'name' if version is empty.
  // Check first that it is actually installed.
  char *name;
  QString *qfname = p->getProperty("filename"), *qbname = p->getProperty("base");
  if (qfname != 0 && p->packageState != packageInfo::INSTALLED) {
    if (qbname) (*qfname) = *qbname + "/" + *qfname;
     name = strdup(qfname->data());
  } else {
    const char *pname = p->getProperty("name")->data(), *pvers = p->getProperty("version")->data();

    if (pname == 0) {
      ret->append(strdup(i18n("Can't find package name!")));
      kpkg->kp->setStatus("");
      return ret;
    }

    if (pvers == 0) pvers = "";

    int nlen = strlen(pname), vlen = strlen(pvers);
    name = (char *) malloc(nlen + vlen + 2);
    memcpy(name, pname, nlen);
    if (vlen != 0) {
      name[nlen] = '-';
      memcpy(name+nlen+1, pvers, vlen);
      name[nlen+1+vlen] = 0;
    } else name[nlen] = 0;
  }

  // Open a pipe to a pkg_info process in order to read the file list.
  // This works for both installed packages and for files.
  reader.setup(PKG_INFO_BIN);
  *reader.proc << "-L" << "-q" << name;
  free(name);
  if (reader.start()) {
    const char *op = reader.buf.data();
    if (op == 0) {
      ret->append(strdup(i18n("pkg_info returned no output")));
      kpkg->kp->setStatus("");
      return ret;
    }
    char *buf = strdup(op);
    char *line = strtok(buf, "\n");
    while (line != 0) {
      ret->append(strdup(line)); // FIXME: Hmm, allocating with malloc,
                                 // freeing with delete. Oh dear.
      line = strtok(0, "\n");
    }
    free(buf);
  } else {
    ret->append(strdup(i18n("Can't start pkg_info")));
  }
  
  kpkg->kp->setStatus("");
  return ret;
}


//  QList<char> *verify(packageInfo *p, QList<char> *files);

int fbsdInterface::doUninstall(int uninstallFlags, QString packs) {
  kpkg->kp->setStatus(i18n("Uninstalling"));
  reader.setup(PKG_DELETE_BIN);

  // Add uninstall flags to the command line.
  if (uninstallFlags & 1) *reader.proc << "-I";
  if (!(uninstallFlags & 2)) *reader.proc << "-f";
  if (uninstallFlags & 4) *reader.proc << "-n";

  // Add each package name as a separate argument.
  char *buf = strdup(packs.data());
  char *pname = strtok(buf, " ");
  while (pname != 0) {
    *reader.proc << pname;
    pname = strtok(0, " ");
  }
  free(buf);

  bool success = reader.start();

  kpkg->kp->setStatus("");
  return !success;
}


int fbsdInterface::doInstall(int installFlags, QString packs) {
  kpkg->kp->setStatus(i18n("Installing"));
  reader.setup(PKG_ADD_BIN);

  // Add install flags to the command line.
  if (installFlags & 1) *reader.proc << "-I";
  if (!(installFlags & 2)) *reader.proc << "-f";
  if (installFlags & 4) *reader.proc << "-n";

  // Add each package name as a separate argument.
  char *buf = strdup(packs.data());
  char *pname = strtok(buf, " ");
  while (pname != 0) {
    *reader.proc << pname;
    pname = strtok(0, " ");
  }
  free(buf);

  bool success = reader.start();

  kpkg->kp->setStatus("");
  return !success;
}

int fbsdInterface::uninstall(int uninstallFlags, packageInfo *p)
{
  QString packs;
  packs = p->getProperty("name")->data();
  QString vers = *p->getProperty("version");
  if (vers.length() > 0) packs += "-" + vers;

  return doUninstall(uninstallFlags, packs);
}

int fbsdInterface::uninstall(int uninstallFlags, QList<packageInfo> *p)
{
  QString packs = "";
  packageInfo *i;

  for (i = p->first(); i!= 0; i = p->next())  {
    packs += *i->getProperty("name");
    QString vers = *i->getProperty("version");
    if (vers.length() != 0) packs += "-" + vers;
    packs += " ";
  }
  return doUninstall( uninstallFlags, packs);
}

QString fbsdInterface::FindFile(const char *name) {
  return QString("");
}

bool fbsdInterface::parseName(QString name, QString *n, QString *v) {
  int m1;

  m1 = name.findRev('-');
  if (m1 <= 0) return false;
  *n = name.left(m1);
  *v = name.right(name.length() - m1 - 1);
  return true;
}

void fbsdInterface::addNV(QDict<QString> *d, const char *name) {
  QString *n = new QString, *v = new QString;

  if (!parseName(QString(name), n, v)) {
    *n = name;
    *v = "";
  }

  d->insert("name", n);
  d->insert("version", v);
}

  //public slots
void fbsdInterface::setLocation() {
  locatedialog->restore();
}

void fbsdInterface::setAvail(LcacheObj *slist) {
  if (packageLoc) delete packageLoc;
  packageLoc = slist;

  cacheObj *cp = packageLoc->first();
  bool ports_found = false; // Set to true when the ports tree config is found.

  if (cp && !cp->location.isEmpty()) {
    for (; cp != 0; cp = packageLoc->next()) {
      ports_found = cp->cacheFile == "BSD_0_0";

      QString oldloc = cp->location;
      cp->location += "/INDEX";
      QString s = getPackList(cp);
      if (!s.isEmpty()) bsdPortsIndexItem::processFile(s.data(), cp->cacheFile != "BSD_0_0", oldloc);
      cp->location = oldloc;
    }
  }

  if (!ports_found) {
    // Try the standard ports tree locations.
    bsdPortsIndexItem::processFile("/usr/ports/INDEX", false, "/usr/ports"); // FreeBSD/OpenBSD
    bsdPortsIndexItem::processFile("/usr/opt/INDEX", false, "/usr/opt");  // NetBSD
  }
}


void fbsdInterface::listPackages(QList<packageInfo> *pki) {
  int listscan;

  listInstalledPackages(pki);
  if (params->DisplayP == Params::INSTALLED) return;
  for (listscan = 0; listscan < 256; listscan++) {
    bsdPortsIndexItem *scan = bsdPortsIndexItem::lists[listscan];

    while (scan != 0) {
      if (!scan->installed && scan->bin) {
	QDict<QString> *a = new QDict<QString>;
	a->setAutoDelete(TRUE);

	addNV(a, scan->name);
	a->insert("summary", new QString(scan->name));
	a->insert("maintainer", new QString(scan->maint));

	const char *space = strchr(scan->cats, ' ');
	if (space) {
	  a->insert("group", new QString(scan->cats, space - scan->cats + 1));
	  a->insert("also in", new QString(space+1));
	} else a->insert("group", new QString(scan->cats));

	a->insert("run depends", new QString(scan->rdeps[0]? scan->rdeps : (const char *) i18n("none")));
	a->insert("build depends", new QString(scan->bdeps[0]? scan->bdeps : (const char *) i18n("none")));
	a->insert("available as", new QString(scan->bin ? (scan->port? i18n("binary package and source port") : i18n("binary package")) : i18n("source port")));

	assert(scan->bin_filename);
	a->insert("filename", new QString(scan->bin_filename));
	a->insert("base", new QString(scan->bin_filename_base));

	packageInfo *info = new packageInfo(a, this);
	info->packageState = packageInfo::AVAILABLE;
	info->smerge(typeID);
	info->fixup();
	info->update(pki, typeID, false);
//	pki->append(info);
      }

      scan = scan->next;
    }
  }
}

void fbsdInterface::listInstalledPackages(QList<packageInfo> *pki) {
#define INFO_SEPARATOR "f;g#z-@IqbX%"
  
  // Open a pipe to a pkg_info process in order to read the comment, name
  // and description for the packages.

  kpkg->kp->setStatus(i18n("Querying BSD packages database for installed packages"));

  reader.setup(PKG_INFO_BIN);
  *reader.proc << "-acdl" << INFO_SEPARATOR;
  if (!reader.start(0, FALSE)) {
    kpkg->kp->setStatus("");
    return;
  } else {
    if (reader.buf.isEmpty()) {
      kpkg->kp->setStatus("");
      return;
    }
    // We should now get:
    //  INFO_SEPARATORInformation for pkgname:
    //
    //  INFO_SEPARATORComment:
    //   <one line description>
    //
    //  INFO_SEPARATORDescription:
    //   <description>
    //
    //
    //  INFO_SEPARATOR
    //  INFO_SEPARATORInformation for [etc]

    char *buf = strdup(reader.buf.data());
    char *line = strtok(buf, "\n");

    while (line != 0) {
      QDict<QString> *a = new QDict<QString>;
      a->setAutoDelete(TRUE);

      if (memcmp(line, INFO_SEPARATOR, sizeof(INFO_SEPARATOR)-1)) {
	KpMsgE(i18n("Unexpected output from pkg_info: %s"), line, TRUE);
        kpkg->kp->setStatus("");
	return;
      }

      // Find the last word on this line (which should be the package name) minus a trailing :.
      char *lastword = strrchr(line, ' ');
      int lastword_len = lastword? strlen(lastword) : 0;
      if (lastword_len < 2 || lastword[lastword_len-1] != ':') {
	KpMsgE(i18n("Unexpected output from pkg_info (looking for package name): %s"), line, TRUE);
        kpkg->kp->setStatus("");
	return;
      }

      lastword[lastword_len-1] = 0;
      addNV(a, lastword+1);

      // Now find the package comment. Skip lines up to the next separator.
      bool done = true;
      do {
	// If not the first time round and we've found the separator then exit the loop when we reach the end.
	if (!done && !memcmp(line, INFO_SEPARATOR, sizeof(INFO_SEPARATOR)-1)) done = 1;
	else done = false;

	line = strtok(0, "\n");
	if (line == 0) {
	  KpMsgE(i18n("Unexpected EOF from pkg_info (looking for comment line)"), "", TRUE);
          kpkg->kp->setStatus("");
	  return;
	}
      } while (!done);

      a->insert("summary", new QString(line));

      // Now look for the package description. Skip to the next separator.
      // The current line doesn't contain one this time.
      do {
	line = strtok(0, "\n");
	if (line == 0) {
	  KpMsgE(i18n("Unexpected EOF from pkg_info (looking for comment line)"), "", TRUE);
          kpkg->kp->setStatus("");
	  return;
	}
      } while (memcmp(line, INFO_SEPARATOR, sizeof(INFO_SEPARATOR)-1));

      // Read to INFO_SEPARATOR to get the description.
      QString *desc = new QString;

      line = strtok(0, "\n");
      while (line != 0 && memcmp(line, INFO_SEPARATOR, sizeof(INFO_SEPARATOR)-1)) {
	// Remove any new lines from the string. However, double ones should be preserved
	// as part of some semblence of an attempt to preserve the formatting...
	int line_len = strlen(line);
	if (line_len == 0) desc->append("\n");
	else {
	  desc->append(QString(line));
	  desc->append(" ");
	}

	line = strtok(0, "\n");
      }

      bsdPortsIndexItem *inditem = bsdPortsIndexItem::find(lastword+1);

      if (inditem) {
	inditem->installed = true;

	a->insert("maintainer", new QString(inditem->maint));

	assert(inditem->cats);
	const char *space = strchr(inditem->cats, ' ');
	if (space) {
	  a->insert("group", new QString(inditem->cats, space - inditem->cats + 1));
	  a->insert("also in", new QString(space+1));
	} else a->insert("group", new QString(inditem->cats));

	assert(inditem->bdeps);
	assert(inditem->rdeps);
	a->insert("run depends", new QString(inditem->rdeps[0]? inditem->rdeps : (const char *) i18n("none")));
        a->insert("build depends", new QString(inditem->bdeps[0]? inditem->bdeps : (const char *) i18n("none")));
	a->insert("available as", new QString(inditem->bin ? (inditem->port? i18n("binary package and source port") : i18n("binary package")) : i18n("source port")));
      }

      a->insert("description", desc);
      
      packageInfo *info = new packageInfo(a, this);
      info->packageState = packageInfo::INSTALLED;
      info->fixup();
      //pki->append(info);
      info->update(pki, typeID, true);

      if (line != 0) line = strtok(0, "\n");
      }
    free(buf);
    }
    
#undef INFO_SEPARATOR
}


bsdPortsIndexItem::bsdPortsIndexItem(char *desc, bool binaries, QString dname) : bin(binaries), port(!binaries), installed(false) {
  int itemno;

  rdeps = ""; // Could otherwise end up unintialised.
  for (itemno = 0; *desc != 0; itemno++) {
    switch (itemno) {
    case 0:
      name = desc;
      break;
    case 1:
      path = desc;
      break;
    case 2:
      prefix = desc;
      break;
    case 3:
      comment = desc;
      break;
    case 4:
      desc_path = desc;
      break;
    case 5:
      maint = desc;
      break;
    case 6:
      cats = desc;
      break;
    case 7:
      bdeps = desc;
      break;
    case 8:
      rdeps = desc;
      break;
    default:
      assert(false);
    }

    while (*desc != 0 && *desc != '|') desc++;

    if (*desc == 0) break;

    *desc = 0;
    desc++;
  }

  if (itemno != 8) {
    fprintf(stderr, i18n("Warning: invalid INDEX file entry for %s; ignoring.\n"), name);
  } else {
    name_hash = calc_hash4(name);

    // Order the entries by hash.
    bsdPortsIndexItem **scan = &lists[hash1(name_hash)];
    while (*scan && (*scan)->name_hash < name_hash) scan = &((*scan)->next);

    if (*scan && (*scan)->name_hash == name_hash && !strcmp((*scan)->name, name)) {
      // Collision. May be port and package.
      (*scan)->bin = (*scan)->bin || bin;
      (*scan)->port = (*scan)->port || port;
      if (binaries) {
	(*scan)->bin_filename = QString(name) + ".tgz";
	(*scan)->bin_filename_base = dname + "/";
       }
      name = 0; // Acts as a 'not used' tag.
      return;
    }

    if (binaries) {
      bin_filename = QString(name) + ".tgz";
      bin_filename_base = dname + "/";
     }

    next = *scan;
    *scan = this;
  }
}

bsdPortsIndexItem *bsdPortsIndexItem::lists[256];

unsigned int bsdPortsIndexItem::calc_hash4(const char *pname) {
  unsigned int ret = 0;
  const char *name = pname;
  while (*name != 0) {
    ret += (*name) << (((name - pname) & 3) *8);
    name++;
  }
  return ret;
}

unsigned char bsdPortsIndexItem::calc_hash1(const char *name) {
  return hash1(calc_hash4(name));
}

unsigned char bsdPortsIndexItem::hash1(unsigned int h) {
  return ((h & 0x03000000) >> 24) | ((h & 0x0c0000) >> 16) | ((h & 0x3000) >> 8) | (h & 0xc0);
}

void bsdPortsIndexItem::processFile(const char *fname, bool binaries, const char *dname) {
  // Read the file in to a buffer and null terminate it.
  
  struct stat s;

  if (stat(fname, &s) == -1) {
    // Error message?
    return;
  }

  char *index = (char *) malloc(s.st_size);
  int fd;

  fd = open(fname, O_RDONLY);
  if (fd == -1) {
    // Error message?
    return;
  }

  int size = read(fd, index, s.st_size);
  index[size] = 0;
  close(fd);


  // Go through each line and create a new bsdPortsIndexItem.
  char *line = strtok(index, "\n");
  while (line != 0) {
    bsdPortsIndexItem *i = new bsdPortsIndexItem(line, binaries, QString(dname) + "/All/");
     if (i->name == 0) delete i;
    line = strtok(0, "\n");
  }
}

bsdPortsIndexItem *bsdPortsIndexItem::find(const char *name) {
  assert(name);

  if (name[0] == 0) return 0;

  unsigned int hash = calc_hash4(name);
  bsdPortsIndexItem *scan = lists[hash1(hash)];

  while (scan != 0) {
    if (scan->name_hash == hash || true) {
      if (!strcmp(name, scan->name)) return scan;
    }
    scan = scan->next;
  }

  return 0;
}

