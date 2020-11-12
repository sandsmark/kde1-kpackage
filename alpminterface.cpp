#include "alpminterface.h"
#include <alpm.h>
#include <klocale.h>
#include <kmsgbox.h>
#include <qlayout.h>
#include <qlistbox.h>
#include "kpackage.h"
#include <fstream>

namespace {
// Keep in sync with the things below
enum InstallOptions {
    OnlyNeeded = 1 << 0,
    AsDeps = 1 << 1,
    IgnoreDeps = 1 << 2,
    IgnoreScripts = 1 << 3,
    DryRun = 1 << 4,
    OnlyDB = 1 << 5,
};
enum UninstallOptions {
    RemoveRevDeps = 1 << 0,
    RemoveDeps = 1 << 1,
    RemoveUnneeded = 1 << 2,
    UninstIgnoreDeps = 1 << 3,
    UninstDryRun = 1 << 4,
    UninstOnlyDb = 1 << 5,
};
}

static param pinstall[] =  {
  {"Only install needed",TRUE,FALSE},
  {"Mark as installed as a dependency",FALSE,FALSE},
  {"Ignore dependencies",FALSE,FALSE},
  {"Ignore scripts",FALSE,FALSE},
  {"Print only, don't do anything",FALSE,FALSE},
  {"Only modify database",FALSE,FALSE},
  {0,FALSE,FALSE}
};

static param puninstall[] =  {
  {"Remove packages depending on package",TRUE,FALSE},
  {"Remove dependencies of package",TRUE,FALSE},
  {"Remove unneeded packages",TRUE,FALSE},
  {"Ignore dependencies",FALSE,FALSE},
  {"Print only, don't do anything",FALSE,FALSE},
  {"Only modify database",FALSE,FALSE},
  {0,FALSE,FALSE}
};

static alpmInterface *s_instance;

static void eventCallback(alpm_event_t *event)
{
    switch(event->type) {
    /** Dependencies will be computed for a package. */
    case ALPM_EVENT_CHECKDEPS_START:
        kpkg->kp->setStatus(i18n("Checking dependencies..."));
        break;
    /** Dependencies were computed for a package. */
    case ALPM_EVENT_CHECKDEPS_DONE:
        kpkg->kp->setStatus(i18n("Completed checking dependencies."));
        break;
    /** File conflicts will be computed for a package. */
    case ALPM_EVENT_FILECONFLICTS_START:
        kpkg->kp->setStatus(i18n("Checking for file conflicts..."));
        break;
    /** File conflicts were computed for a package. */
    case ALPM_EVENT_FILECONFLICTS_DONE:
        kpkg->kp->setStatus(i18n("Completed checking file conflicts."));
        break;
    /** Dependencies will be resolved for target package. */
    case ALPM_EVENT_RESOLVEDEPS_START:
        kpkg->kp->setStatus(i18n("Resolving dependencies..."));
        break;
    /** Dependencies were resolved for target package. */
    case ALPM_EVENT_RESOLVEDEPS_DONE:
        kpkg->kp->setStatus(i18n("Completed resolving dependencies."));
        break;
    /** Inter-conflicts will be checked for target package. */
    case ALPM_EVENT_INTERCONFLICTS_START:
        kpkg->kp->setStatus(i18n("Checking for conflicts..."));
        break;
    /** Inter-conflicts were checked for target package. */
    case ALPM_EVENT_INTERCONFLICTS_DONE:
        kpkg->kp->setStatus(i18n("Completed checking for conflicts."));
        break;
    /** Processing the package transaction is starting. */
    case ALPM_EVENT_TRANSACTION_START:
        kpkg->kp->setStatus(i18n("Starting package transaction..."));
        break;
    /** Processing the package transaction is finished. */
    case ALPM_EVENT_TRANSACTION_DONE:
        kpkg->kp->setStatus(i18n("Package transaction completed."));
        break;
    /** Package will be installed/upgraded/downgraded/re-installed/removed; See
     * alpm_event_package_operation_t for arguments. */
    case ALPM_EVENT_PACKAGE_OPERATION_START: {
        switch(event->package_operation.operation) {
        /** Package (to be) installed. (No oldpkg) */
        case ALPM_PACKAGE_INSTALL:
            kpkg->kp->setStatus(i18n("Starting installation..."));
            break;
        /** Package (to be) upgraded */
        case ALPM_PACKAGE_UPGRADE:
            kpkg->kp->setStatus(i18n("Starting upgrade..."));
            break;
        /** Package (to be) re-installed. */
        case ALPM_PACKAGE_REINSTALL:
            kpkg->kp->setStatus(i18n("Starting reinstallation..."));
            break;
        /** Package (to be) downgraded. */
        case ALPM_PACKAGE_DOWNGRADE:
            kpkg->kp->setStatus(i18n("Starting downgrade..."));
            break;
        /** Package (to be) removed. (No newpkg) */
        case ALPM_PACKAGE_REMOVE:
            kpkg->kp->setStatus(i18n("Starting removal..."));
            break;
        default:
            puts("unhandled");
            break;
        }

        break;
    }
    /** Package was installed/upgraded/downgraded/re-installed/removed; See
     * alpm_event_package_operation_t for arguments. */
    case ALPM_EVENT_PACKAGE_OPERATION_DONE: {
        switch(event->package_operation.operation) {
        /** Package (to be) installed. (No oldpkg) */
        case ALPM_PACKAGE_INSTALL:
            kpkg->kp->setStatus(i18n("Completed installation."));
            break;
        /** Package (to be) upgraded */
        case ALPM_PACKAGE_UPGRADE:
            kpkg->kp->setStatus(i18n("Completed upgrade."));
            break;
        /** Package (to be) re-installed. */
        case ALPM_PACKAGE_REINSTALL:
            kpkg->kp->setStatus(i18n("Completed reinstallation."));
            break;
        /** Package (to be) downgraded. */
        case ALPM_PACKAGE_DOWNGRADE:
            kpkg->kp->setStatus(i18n("Completed downgrade."));
            break;
        /** Package (to be) removed. (No newpkg) */
        case ALPM_PACKAGE_REMOVE:
            kpkg->kp->setStatus(i18n("Completed removal."));
            break;
        default:
            puts("unhandled");
            break;
        }
        break;
    }
    /** Target package's integrity will be checked. */
    case ALPM_EVENT_INTEGRITY_START:
        kpkg->kp->setStatus(i18n("Starting integrity check..."));
        break;
    /** Target package's integrity was checked. */
    case ALPM_EVENT_INTEGRITY_DONE:
        kpkg->kp->setStatus(i18n("Completed integrity check."));
        break;
    /** Target package will be loaded. */
    case ALPM_EVENT_LOAD_START:
        kpkg->kp->setStatus(i18n("Loading package..."));
        break;
    /** Target package is finished loading. */
    case ALPM_EVENT_LOAD_DONE:
        kpkg->kp->setStatus(i18n("Completed loading package."));
        break;
    /** Scriptlet has printed information; See alpm_event_scriptlet_info_t for
     * arguments. */
    case ALPM_EVENT_SCRIPTLET_INFO:
        kpkg->kp->setStatus(event->scriptlet_info.line); // TODO: should print this nicely somewhere
        break;
    /** Files will be downloaded from a repository. */
    case ALPM_EVENT_RETRIEVE_START:
        kpkg->kp->setStatus(i18n("Retrieving files..."));
        break;
    /** Files were downloaded from a repository. */
    case ALPM_EVENT_RETRIEVE_DONE:
        kpkg->kp->setStatus(i18n("Completed file retrieval."));
        break;
    /** Not all files were successfully downloaded from a repository. */
    case ALPM_EVENT_RETRIEVE_FAILED:
        kpkg->kp->setStatus(i18n("Failed to retrieve files.")); // todo: messagebox?
        break;
    /** A file will be downloaded from a repository; See alpm_event_pkgdownload_t
     * for arguments */
    case ALPM_EVENT_PKGDOWNLOAD_START:
        kpkg->kp->setStatus(i18n("Downloading packages..."));
        break;
    /** A file was downloaded from a repository; See alpm_event_pkgdownload_t
     * for arguments */
    case ALPM_EVENT_PKGDOWNLOAD_DONE:
        kpkg->kp->setStatus(i18n("Completed downloading packages."));
        break;
    /** A file failed to be downloaded from a repository; See
     * alpm_event_pkgdownload_t for arguments */
    case ALPM_EVENT_PKGDOWNLOAD_FAILED:
        kpkg->kp->setStatus(i18n("Failed to download packages.")); // todo: messagebox?
        break;
    /** Disk space usage will be computed for a package. */
    case ALPM_EVENT_DISKSPACE_START:
        kpkg->kp->setStatus(i18n("Checking disk space..."));
        break;
    /** Disk space usage was computed for a package. */
    case ALPM_EVENT_DISKSPACE_DONE:
        kpkg->kp->setStatus(i18n("Disk space check done."));
        break;
    /** An optdepend for another package is being removed; See
     * alpm_event_optdep_removal_t for arguments. */
    case ALPM_EVENT_OPTDEP_REMOVAL:
        kpkg->kp->setStatus(i18n("Removing optional dependency for another package"));
        break;
    /** A configured repository database is missing; See
     * alpm_event_database_missing_t for arguments. */
    case ALPM_EVENT_DATABASE_MISSING:
        KpMsgE("Package database is missing!", "", FALSE);
        break;
    /** Checking keys used to create signatures are in keyring. */
    case ALPM_EVENT_KEYRING_START:
        kpkg->kp->setStatus(i18n("Checking keyring..."));
        break;
    /** Keyring checking is finished. */
    case ALPM_EVENT_KEYRING_DONE:
        kpkg->kp->setStatus(i18n("Completed checking keyring."));
        break;
    /** Downloading missing keys into keyring. */
    case ALPM_EVENT_KEY_DOWNLOAD_START:
        kpkg->kp->setStatus(i18n("Downloading missing keys to keyring..."));
        break;
    /** Key downloading is finished. */
    case ALPM_EVENT_KEY_DOWNLOAD_DONE:
        kpkg->kp->setStatus(i18n("Completed download of keys."));
        break;
    /** A .pacnew file was created; See alpm_event_pacnew_created_t for arguments. */
    case ALPM_EVENT_PACNEW_CREATED:
        kpkg->kp->setStatus(i18n("Created a pacnew file."));
        break;
    /** A .pacsave file was created; See alpm_event_pacsave_created_t for
     * arguments */
    case ALPM_EVENT_PACSAVE_CREATED:
        kpkg->kp->setStatus(i18n("Created a pacsav file."));
        break;
    /** Processing hooks will be started. */
    case ALPM_EVENT_HOOK_START:
        kpkg->kp->setStatus(i18n("Starting processing hooks..."));
        break;
    /** Processing hooks is finished. */
    case ALPM_EVENT_HOOK_DONE:
        kpkg->kp->setStatus(i18n("Processing hooks complete."));
        break;
    /** A hook is starting */
    case ALPM_EVENT_HOOK_RUN_START:
        kpkg->kp->setStatus(i18n("Starting hook..."));
        break;
    /** A hook has finished running */
    case ALPM_EVENT_HOOK_RUN_DONE:
        kpkg->kp->setStatus(i18n("Hook complete."));
        break;
    default:
        puts("unexpected and unhandled");
        break;
    }
}
static void questionCallback(alpm_question_t *question)
{
    QString message;
    switch(question->type) {
    case ALPM_QUESTION_INSTALL_IGNOREPKG:
        message.sprintf(i18n("Install ignored package %s?"),
                        alpm_pkg_get_name(question->install_ignorepkg.pkg)
                        );
        if (KMsgBox::yesNoCancel(kpkg->kp, "Question from ALPM", message) == 1) {
            question->install_ignorepkg.install = 1;
        } else {
            question->install_ignorepkg.install = 0;
        }
        break;
    case ALPM_QUESTION_REPLACE_PKG:
        message.sprintf(i18n("Replace %s with %s?"),
                        alpm_pkg_get_name(question->replace.oldpkg),
                        alpm_pkg_get_name(question->replace.newpkg)
                        );
        if (KMsgBox::yesNoCancel(kpkg->kp, "Question from ALPM", message) == 1) {
            question->replace.replace = 1;
        } else {
            question->replace.replace = 0;
        }
        break;
    case ALPM_QUESTION_CONFLICT_PKG:
        message.sprintf(i18n("%s conflicts with %s, remove %s?"),
                        question->conflict.conflict->package1,
                        question->conflict.conflict->package2,
                        question->conflict.conflict->package2
                        );
        if (KMsgBox::yesNoCancel(kpkg->kp, "Question from ALPM", message) == 1) {
            question->conflict.remove = 1;
        } else {
            question->conflict.remove = 1;
        }
        break;
    case ALPM_QUESTION_CORRUPTED_PKG:
        message.sprintf(i18n("Delete corrupt (%s) file %s?"),
                        alpm_strerror(question->corrupted.reason),
                        question->corrupted.filepath
                        );
        if (KMsgBox::yesNoCancel(kpkg->kp, "Question from ALPM", message) == 1) {
            question->corrupted.remove = 1;
        } else {
            question->corrupted.remove = 0;
        }
        break;
    case ALPM_QUESTION_REMOVE_PKGS:
        message = "The following packages have unavailable dependencies, skip?:\n";
        for (alpm_list_t *it = question->remove_pkgs.packages; it != NULL; it = it->next) {
            message += alpm_pkg_get_name(reinterpret_cast<alpm_pkg_t*>(it->data));
            if (it->next) {
                message += ", ";
            }
        }
        if (KMsgBox::yesNoCancel(kpkg->kp, "Question from ALPM", message) == 1) {
            question->remove_pkgs.skip = 1;
        } else {
            question->remove_pkgs.skip = 0;
        }

        break;
    case ALPM_QUESTION_SELECT_PROVIDER: {
        // if there is a required version, we need to format it
        const char *prefix = "";
        const char *version = question->select_provider.depend->version;
        switch(question->select_provider.depend->mod) {
        case ALPM_DEP_MOD_ANY: version = ""; break;
        case ALPM_DEP_MOD_EQ: prefix = "="; break;
        case ALPM_DEP_MOD_GE: prefix = ">="; break;
        case ALPM_DEP_MOD_LE: prefix = "<="; break;
        case ALPM_DEP_MOD_GT: prefix = ">"; break;
        case ALPM_DEP_MOD_LT: prefix = "<"; break;
        default: puts("unhandled shit"); break;
        }
        message.sprintf(i18n("Please select a provider for the dependency %s%s%s"), question->select_provider.depend->name, prefix, version);

        QDialog *dialog = new QDialog(kpkg->kp, "dependencyproviderselection", TRUE); // modal
        dialog->setCaption(i18n("Select provider"));

        QLabel *text = new QLabel(dialog);
        text->setText(message);

        // List the options
        QListBox *options = new QListBox(dialog);
        for (alpm_list_t *it = question->remove_pkgs.packages; it != NULL; it = it->next) {
            options->insertItem(alpm_pkg_get_name(reinterpret_cast<alpm_pkg_t*>(it->data)));
        }

        // Layout nicely
        QBoxLayout *layout = new QBoxLayout(dialog, QBoxLayout::TopToBottom);
        layout->addWidget(text);
        layout->addWidget(options);

        // Get the choice
        if (dialog->exec() == QDialog::Rejected || options->currentItem() == -1) {
            question->select_provider.use_index = 0;
            delete dialog;
            return;
        }
        question->select_provider.use_index = options->currentItem();
        delete dialog;
        break;
    }
    case ALPM_QUESTION_IMPORT_KEY: {
        alpm_pgpkey_t *key = question->import_key.key;
        char created[12];
        time_t time = (time_t) key->created;

        if(strftime(created, 12, "%Y-%m-%d", localtime(&time)) == 0) {
          strcpy(created, "(unknown)");
        }

        const char *format = NULL;
        if (key->revoked) {
            format = i18n("Import PGP key %u%c/%s, '%s', created: %s (revoked!)");
        } else {
            format = i18n("Import PGP key %u%c/%s, '%s', created: %s");
        }

        message.sprintf(format,
                    key->length, key->pubkey_algo, key->fingerprint, key->uid, created
                    );
        if (KMsgBox::yesNoCancel(kpkg->kp, "Question from ALPM", message) == 1) {
            question->import_key.import = 1;
        } else {
            question->import_key.import = 0;
        }
        break;
    }
    default:
        puts("Nope");
        break;
    }

//    int choice = KMsgBox::yesNoCancel(kpkg->kp, "Question from ALPM", message);
//    if (choice == 3) { // Cancel, should we abort the whole thing?
//       alpm_trans_interrupt()
//    }


}
static void progressCallback(alpm_progress_t type, const char *packageName, int percent, size_t total, size_t current)
{
    QString typeString;
    switch(type) {
    case ALPM_PROGRESS_ADD_START:
        typeString = "Installing ";
        break;
    case ALPM_PROGRESS_UPGRADE_START:
        typeString = "Upgrading ";
        break;
    case ALPM_PROGRESS_DOWNGRADE_START:
        typeString = "Downgrading ";
        break;
    case ALPM_PROGRESS_REINSTALL_START:
        typeString = "Reinstalling ";
        break;
    case ALPM_PROGRESS_REMOVE_START:
        typeString = "Removing ";
        break;
    case ALPM_PROGRESS_CONFLICTS_START:
        typeString = "Checking for file conflicts ";
        break;
    case ALPM_PROGRESS_DISKSPACE_START:
        typeString = "Checking for free disk space ";
        break;
    case ALPM_PROGRESS_INTEGRITY_START:
        typeString = "Checking file integrity ";
        break;
    case ALPM_PROGRESS_LOAD_START:
        typeString = "Loading packages ";
        break;
    case ALPM_PROGRESS_KEYRING_START:
        typeString = "Checking keyring ";
        break;
    default:
        break;
    }

    // I don't trust the asprintf stuff of QString, it is broken internally
    QString percentString;
    percentString.setNum(percent);
    QString totalString;
    totalString.setNum(total);
    QString currentString;
    currentString.setNum(current);
    kpkg->kp->setStatus(typeString + QString(packageName) + " " + percentString + "% (" + currentString + "/" + totalString);

    if (percent >= 0 && percent <= 100) {
        kpkg->kp->setPercent(percent);
    }
}
static void downloadProgressCallback(const char *filename, off_t xfered, off_t total)
{
    off_t percent = 100 * xfered / total;
    QString message;
    message.sprintf("Downloading %s...", filename);
    kpkg->kp->setStatus(message);
    kpkg->kp->setPercent(percent);
}

static void totalDownloadSizeCallback(off_t total)
{
    QString totalString;
    totalString.setNum(total);
    kpkg->kp->setStatus("Downloading " + totalString + " bytes...");
}

// TODO: fetchNetFile() doesn't let us force a download path
//static int downloadFileCallback(const char *url, const char *localpath, int force);

alpmInterface::alpmInterface()
{
    if (s_instance) {
        fprintf(stderr, "It should only have one instance");
    }
    s_instance = this;
    head = "Pacman";

    m_handle = NULL;

    locatedialog = NULL;
    pict = NULL;

    packagePattern = "*.pkg.*";
    queryMsg = i18n("Querying package list: ");
    typeID = "/alpm";

    icon = "package.xpm";
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
    std::set<alpmPackageInfo*>::iterator it = m_instantiated.begin();
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
    (void)mode; // it's 'i' if its installed, idgaf

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
        // We have to do this song and dance because:
        //  a) alpm doesn't prefix with /
        //  b) we need to return a char*
        //  c) we can't use malloc or new char[] because QListT doesn't like that.
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
    // isn't used by the ui
    (void)name;
    (void)src;
    return NULL;
}

int alpmInterface::doUninstall(int installFlags, QString packs)
{
    if (!m_handle) {
        return 0;
    }
    if (geteuid() != 0) {
        KpMsgE("KPackage needs to run as root to do useful stuff. Trust me, it's safe.", "", FALSE);
        return -1;
    }

    return 0;
}

int alpmInterface::doInstall(int installFlags, QString packs)
{
    if (!m_handle) {
        return -1;
    }
    if (geteuid() != 0) {
        KpMsgE("KPackage needs to run as root to do useful stuff. Trust me, it's safe.", "", FALSE);
        return -1;
    }

    return 0;
}

QString alpmInterface::FindFile(const char *name)
{
    if (!m_handle) {
        return "";
    }
    alpm_db_t *db = alpm_get_localdb(m_handle);
    if (!db) {
        fprintf(stderr, "Failed loading local database: %s\n", alpm_strerror(alpm_errno(m_handle)));
        return "";
    }
    QString ret;

    alpm_list_t *pkglist = alpm_db_get_pkgcache(db);
    bool stripSlash = strchr(name, '/') == NULL;

    for(alpm_list_t *it = pkglist; it; it = alpm_list_next(it)) {
        alpm_pkg_t *pkg = reinterpret_cast<alpm_pkg_t*>(it->data);

        alpm_filelist_t *files = alpm_pkg_get_files(pkg);

        for (size_t i=0; i<files->count; i++) {
            const char *filename = NULL;
            if (stripSlash) {
                filename = rindex(files->files[i].name, '/');
            }
            if (!filename) {
                filename = files->files[i].name;
            }
            if (strstr(filename, name)) {
                ret += QString(alpm_pkg_get_name(pkg)) + "\t" + QString(files->files[i].name) + "\n";
            }
        }
    }
    return ret;
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
    int ret = alpm_option_set_eventcb(m_handle, eventCallback);
    ret = alpm_option_set_dlcb(m_handle, downloadProgressCallback);
    ret = alpm_option_set_progresscb(m_handle, progressCallback);
    ret = alpm_option_set_totaldlcb(m_handle, totalDownloadSizeCallback);
    ret = alpm_option_set_questioncb(m_handle, questionCallback);

    if (ret != 0) {
        fprintf(stderr, "we got some number %d\n", ret);
    }
    puts("ALPM initialized");
    return true;
}

int alpmInterface::uninstall(int uninstallFlags, QListT<packageInfo> *p)
{
    if (geteuid() != 0) {
        KpMsgE("KPackage needs to run as root to do useful stuff. Trust me, it's safe.", "", FALSE);
        return -1;
    }
    packageInfo *i;
    for (i = p->first(); i!= 0; i = p->next())  {
        if (!uninstall(uninstallFlags, i)) {
            return 1;
        }
    }
    return 0;
}

int alpmInterface::uninstall(int uninstallFlags, packageInfo *p)
{
    if (geteuid() != 0) {
        KpMsgE("KPackage needs to run as root to do useful stuff. Trust me, it's safe.", "", FALSE);
        return -1;
    }
    // etc.
    if (uninstallFlags & UninstDryRun) {

    }
    //TODO
    return 0;
}

int alpmInterface::install(int installFlags, QListT<packageInfo> *p)
{
    if (geteuid() != 0) {
        KpMsgE("KPackage needs to run as root to do useful stuff. Trust me, it's safe.", "", FALSE);
        return -1;
    }

    puts("Installing list");
    packageInfo *i;
    for (i = p->first(); i!= 0; i = p->next())  {
        if (!install(installFlags, i)) {
            return 1;
        }
    }
    return 0;
}

int alpmInterface::install(int installFlags, packageInfo *p)
{
    if (geteuid() != 0) {
        KpMsgE("KPackage needs to run as root to do useful stuff. Trust me, it's safe.", "", FALSE);
        return -1;
    }

    puts("Installing");
    //TODO
    return 0;
}

void alpmInterface::setLocation()
{
    if (!m_handle) {
        return;
    }
}

void alpmInterface::setAvail(LcacheObj *)
{
    // todo: I think this means we should re-initialize with a new root
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
        info->update(pki, typeID, alpm_pkg_get_origin(pkg) == ALPM_PKG_FROM_LOCALDB);
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
    data->insert("filename", new QString(alpm_pkg_get_filename(pkg)));
    data->insert("url", new QString(alpm_pkg_get_url(pkg)));
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
