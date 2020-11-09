#ifndef ALPMINTERFACE_H
#define ALPMINTERFACE_H

#include "pkgInterface.h"

typedef struct __alpm_handle_t alpm_handle_t;
typedef struct __alpm_db_t alpm_db_t;
typedef struct __alpm_pkg_t alpm_pkg_t;

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

public slots:
    void setLocation() override;
    void setAvail(LcacheObj *) override;

private:
    void parseDatabase(alpm_db_t *db, QList<packageInfo> *pki, const char *dbName);
    bool loadConfig();
    packageInfo *createInfo(alpm_pkg_t *pkg, const char *dbName);

    alpm_handle_t *m_handle;
    QList<char> m_repos;
};

#endif // ALPMINTERFACE_H
