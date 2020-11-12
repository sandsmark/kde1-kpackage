#ifndef ALPMINTERFACE_H
#define ALPMINTERFACE_H

#include "pkgInterface.h"
#include <vector>
#include <string>
#include <set>

typedef struct __alpm_handle_t alpm_handle_t;
typedef struct __alpm_db_t alpm_db_t;
typedef struct __alpm_pkg_t alpm_pkg_t;

struct alpmPackageInfo : public packageInfo
{
    alpmPackageInfo(QDict<QString> *_info, pkgInterface *type, alpm_pkg_t *me) :
        packageInfo(_info, type), alpm_pkg(me) {}
    alpmPackageInfo(QDict<QString> *_info, QString _filename, alpm_pkg_t *me) :
        packageInfo(_info, _filename),
        alpm_pkg(me) { }

    ~alpmPackageInfo();

    alpm_pkg_t *alpm_pkg;
};

class alpmInterface : public pkgInterface
{
public:
    alpmInterface();
    ~alpmInterface();

    // pkgInterface interface
public:
    bool isType(char *buf, const char *fname) override;
    param *initinstallOptions() override;
    param *inituninstallOptions() override;
    packageInfo *getPackageInfo(char mode, const char *name, const char *version) override;
    QListT<char> *getFileList(packageInfo *p) override;
    QListT<char> *depends(const char *name, int src) override;
    int doUninstall(int installFlags, QString packs) override;
    int doInstall(int installFlags, QString packs) override;
    QString FindFile(const char *name) override;
    bool parseName(QString name, QString *n, QString *v) override;
    void listPackages(QList<packageInfo> *pki) override;
    void listInstalledPackages(QListT<packageInfo> *pki) override;
    void smerge(packageInfo *p) override;

    bool initialize();

    void removeAlpmPackage(alpmPackageInfo *pkg) {
        m_instantiated.erase(pkg);
    }

    virtual int uninstall(int uninstallFlags, QList<packageInfo> *p);
    virtual int uninstall(int uninstallFlags, packageInfo *p);
    // uninstall package or packages

    virtual int install(int installFlags, QList<packageInfo> *p);
    virtual int install(int installFlags, packageInfo *p);

public slots:
    void setLocation() override;
    void setAvail(LcacheObj *) override;

private:
    void parseDatabase(alpm_db_t *db, QList<packageInfo> *pki);
    bool loadConfig();
    packageInfo *createInfo(alpm_pkg_t *pkg);
    alpm_pkg_t *findPackage(const char *name, const char *version);

    alpm_handle_t *m_handle;
    std::vector<std::string> m_repos;

    // I don't understand QListT, so just do it manually
    std::set<alpmPackageInfo*> m_instantiated;
};

#endif // ALPMINTERFACE_H
