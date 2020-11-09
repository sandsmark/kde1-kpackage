#include "alpminterface.h"
#include <alpm.h>
#include <klocale.h>
#include "kpackage.h"
#include <fstream>


static param pinstall[] =  {
  {"Test (do not install)",FALSE,FALSE},
  {0,FALSE,FALSE}
};

static param puninstall[] =  {
  {"Test (do not uninstall)",FALSE,FALSE},
  {0,FALSE,FALSE}
};

alpmInterface::alpmInterface()
{
    head = "Pacman";

    m_handle = NULL;

    locatedialog = NULL;
    pict = NULL;

    packagePattern = "*.pkg.*";
    queryMsg = i18n("Querying package list: ");
    typeID = "/alpm";

    icon = "rpm.xpm";
    pict = new QPixmap();
    *pict = globalKIL->loadIcon(icon);
    bad_pict = new QPixmap();
    *bad_pict = globalKIL->loadIcon("dbad.xpm");
    updated_pict = new QPixmap();
    *updated_pict = globalKIL->loadIcon("rupdated.xpm");
    new_pict = new QPixmap();
    *new_pict = globalKIL->loadIcon("rnew.xpm");

    initialize();
}

alpmInterface::~alpmInterface()
{
    std::unordered_set<alpmPackageInfo*>::iterator it = m_instantiated.begin();
    while (it != m_instantiated.end()) {
        (*it)->interface = NULL;
        delete *it;
        it++;
    }

    if (m_handle) {
        alpm_release(m_handle);
    }
    m_handle = NULL;
}


bool alpmInterface::isType(char *buf, const char *fname)
{
    static const char magic[] = ".BUILDINFO";
    if (strncmp(buf, magic, strlen(magic)) != 0) {
        puts("signature failed");
        return false;
    }

    if (!m_handle) {
        return false;
    }
    alpm_pkg_t *pkg = NULL;
    int ret = alpm_pkg_load(m_handle, // alpm handle
                            fname, // filanem
                            0, // only read metadata
                            ALPM_SIG_PACKAGE_OPTIONAL,
                            &pkg);
    if (ret == -1) {
        fprintf(stderr, "Failed parsing %s: %s\n", fname, alpm_strerror(alpm_errno(m_handle)));
        return false;
    }
    ret = alpm_pkg_free(pkg);
    if (ret == -1) {
        fprintf(stderr, "Failed freeing %s: %s\n", fname, alpm_strerror(alpm_errno(m_handle)));
        return false;
    }
    return true;
}

param *alpmInterface::initinstallOptions()
{
    return &(pinstall[0]);
}

param *alpmInterface::inituninstallOptions()
{
    return &(puninstall[0]);
}

packageInfo *alpmInterface::getPackageInfo(char mode, const char *name, const char *version)
{
    if (!m_handle) {
        return NULL;
    }
    alpm_db_t *db = alpm_get_localdb(m_handle);
    if (!db) {
        fprintf(stderr, "Failed loading local database: %s\n", alpm_strerror(alpm_errno(m_handle)));
        return NULL; // todo; could probably check the others, but if you don't have local ur fukd
    }
    alpm_pkg_t *pkg = findPackage(name, version);
    if (!pkg) {
        puts("Failed to find package");
        return NULL;
    }
    packageInfo *info = createInfo(pkg);
    if (info) {
        return info;
    }
    alpm_pkg_free(pkg);

    return NULL;
}

QListT<char> *alpmInterface::getFileList(packageInfo *p)
{
    if (!m_handle) {
        return NULL;
    }
    alpmPackageInfo *pkg = static_cast<alpmPackageInfo*>(p);
    QList<char> *filelist = new QList<char>;
    filelist->setAutoDelete(TRUE);
    alpm_filelist_t *files = alpm_pkg_get_files(pkg->alpm_pkg);

    for (size_t i=0; i<files->count; i++) {
        if (!files->files[i].name) {
            continue;
        }
        char *name = (char*)operator new(strlen(files->files[i].name) + 2);
        name[0] = '\0';
        name = strcat(name, "/");
        name = strcat(name, files->files[i].name);
        filelist->append(name);
    }

    return filelist;
}

QListT<char> *alpmInterface::depends(const char *name, int src)
{
    if (!m_handle) {
        return NULL;
    }
    return NULL;
}

int alpmInterface::doUninstall(int installFlags, QString packs)
{
    if (!m_handle) {
        return 0;
    }

    return 0;
}

int alpmInterface::doInstall(int installFlags, QString packs)
{
    if (!m_handle) {
        return 0;
    }

    return 0;
}

QString alpmInterface::FindFile(const char *name)
{
    if (!m_handle) {
        return "";
    }
    return 0;
}

bool alpmInterface::parseName(QString name, QString *n, QString *v)
{
    fprintf(stderr, "ALPM has sane package naming, sorry\n");
    (void)name;
    (void)n;
    (void)v;

    return false;
}

void alpmInterface::listPackages(QListT<packageInfo> *pki)
{
    listInstalledPackages(pki);

    puts ("Listing the rest");
    alpm_list_t *dblist = alpm_get_syncdbs(m_handle);
    for(alpm_list_t *it = dblist; it; it = alpm_list_next(it)) {
        alpm_db_t *db = reinterpret_cast<alpm_db_t*>(it->data);
        parseDatabase(db, pki);
    }
}

void alpmInterface::listInstalledPackages(QListT<packageInfo> *pki)
{
    if (!m_handle) {
        return;
    }

    alpm_db_t *db = alpm_get_localdb(m_handle);
    if (!db) {
        fprintf(stderr, "Failed loading local database: %s\n", alpm_strerror(alpm_errno(m_handle)));
        return;
    }
    parseDatabase(db, pki);
}

void alpmInterface::smerge(packageInfo *p)
{
    if (!m_handle) {
        return;
    }
    p->smerge(typeID);
}

bool alpmInterface::initialize()
{
    if (!loadConfig()) {
        puts("Failed to load config");
        return false;
    }

    alpm_errno_t err;
    m_handle = alpm_initialize(ROOTDIR, ROOTDIR DBPATH, &err);
    if (!m_handle) {
        // TODO: use the fancy message boxes
        fprintf(stderr, "Error initializing: %s (root: %s, database: %s)\n", alpm_strerror(err), ROOTDIR, DBPATH);
        return false;
    }

    for (size_t i=0; i<m_repos.size(); i++) {
        // TODO: read siglevel from config
        alpm_register_syncdb(m_handle, m_repos[i].c_str(), ALPM_SIG_USE_DEFAULT);
    }
    puts("ALPM initialized");
    return true;
}

void alpmInterface::setLocation()
{
    if (!m_handle) {
        return;
    }
}

void alpmInterface::setAvail(LcacheObj *)
{
    if (!m_handle) {
        return;
    }
}

void alpmInterface::parseDatabase(alpm_db_t *db, QListT<packageInfo> *pki)
{
    alpm_list_t *pkglist = alpm_db_get_pkgcache(db);

    for(alpm_list_t *it = pkglist; it; it = alpm_list_next(it)) {
        alpm_pkg_t *pkg = reinterpret_cast<alpm_pkg_t*>(it->data);

        packageInfo *info = createInfo(pkg);
        info->update(pki, typeID, false);
    }
}

bool alpmInterface::loadConfig()
{
    std::ifstream file(ROOTDIR "etc/pacman.conf"); // TODO: should maybe not hardcode it lol
    if (!file.good()) {
        puts ("Failed to open pacman conf");
        return false;
    }

    std::string line;

    m_repos.clear();
    while (std::getline(file, line)) {
        if (line.size() < 3) {
            continue;
        }
        if (line[0] != '[') {
            continue;
        }
        std::string repo = line.substr(1, line.size() - 2);
        puts(repo.c_str());
        if (repo == "options") {
            continue;
        }
        m_repos.push_back(repo);
    }
    return true;
}

packageInfo *alpmInterface::createInfo(alpm_pkg_t *pkg)
{
    QDict<QString> *data = new QDict<QString>;
    data->setAutoDelete(TRUE);

    data->insert("name", new QString(alpm_pkg_get_name(pkg)));
    data->insert("version", new QString(alpm_pkg_get_version(pkg)));
    data->insert("packager", new QString(alpm_pkg_get_packager(pkg)));
    data->insert("description", new QString(alpm_pkg_get_desc(pkg)));
    QString sizeStr;
    sizeStr.setNum(alpm_pkg_get_isize(pkg));
    data->insert("size", new QString(sizeStr));

    // TODO: better grouping
    const char *dbName = "Local";
    alpm_db_t *db = alpm_pkg_get_db(pkg);
    if (db) {
        dbName = alpm_db_get_name(db);
    } else {
        puts("FAiled to get db");
    }
    data->insert("group", new QString(dbName));

    alpmPackageInfo *info = new alpmPackageInfo(data, this, pkg);
    m_instantiated.insert(info);
    if (alpm_pkg_get_origin(pkg) == ALPM_PKG_FROM_LOCALDB) {
        info->packageState = packageInfo::INSTALLED;
    } else {
        info->packageState = packageInfo::AVAILABLE;
    }
    info->smerge(typeID);
    info->fixup();
    return info;

}

alpm_pkg_t *alpmInterface::findPackage(const char *name, const char *version)
{
    if (!m_handle) {
        return NULL;
    }
    alpm_db_t *db = alpm_get_localdb(m_handle);
    if (!db) {
        fprintf(stderr, "Failed loading local database: %s\n", alpm_strerror(alpm_errno(m_handle)));
        return NULL; // todo; could probably check the others, but if you don't have local ur fukd
    }
    alpm_pkg_t *pkg = alpm_db_get_pkg(db, name);
    if (!version || strcmp(alpm_pkg_get_version(pkg), version) == 0) {
        return pkg;
    }
    alpm_pkg_free(pkg);

    alpm_list_t *dblist = alpm_get_syncdbs(m_handle);
    for(alpm_list_t *it = dblist; it; it = alpm_list_next(it)) {
        alpm_db_t *db = reinterpret_cast<alpm_db_t*>(it->data);

        alpm_pkg_t *pkg = alpm_db_get_pkg(db, name);
        if (!version || strcmp(alpm_pkg_get_version(pkg), version) == 0) {
            return pkg;
        }
        alpm_pkg_free(pkg);
    }

    return NULL;
}

alpmPackageInfo::~alpmPackageInfo()
{
    if (interface) {
        static_cast<alpmInterface*>(interface)->removeAlpmPackage(this);
    }
    alpm_pkg_free(alpm_pkg);
}
