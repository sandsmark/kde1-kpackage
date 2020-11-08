////////////////////////////////////////////////////////////////////////////
// $Id: pkguninstallDialog.h 16726 1999-02-11 04:18:43Z toivo $	
// File  : uninstallDialog.h
// Author: Damyan Pepper
// Author: Toivo Pedaste
//
////////////////////////////////////////////////////////////////////////////

#ifndef PKGUNINSTALLDIALOG_H
#define PKGUNINSTALLDIALOG_H
#include "config.h"

// Standard Headers

// Qt Headers
#include <qdialog.h>
#include <qpushbt.h>
#include <qchkbox.h>
#include <qlabel.h>
#include <qlayout.h>

// KDE headers

// ksetup headers
#include "packageInfo.h"
#include "pkgInterface.h"

class pkguninstallDialog : public QDialog
{
  Q_OBJECT;
public:
  pkguninstallDialog(param *pars, QWidget *parent=0, const char *name=0);
  ~pkguninstallDialog();

  void setup(packageInfo *p);
  // Display uninstall window

private slots:
  virtual void uninstallClicked();
  virtual void cancelClicked();

protected:
  packageInfo *package;

  QPushButton *uninstallButton, *cancelButton;
  QLabel *label;
  QBoxLayout *layout;

  QCheckBox **Boxs;
  // options buttons

  int bnumber;
  // number of option buttons

};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
class pkguninstallDialogMult :  public pkguninstallDialog
{
Q_OBJECT;
public:
  pkguninstallDialogMult(param *pars, QWidget *parent=0, const char *name=0);
  ~pkguninstallDialogMult();

  void setup(QList<packageInfo> *pl, QString type);
  // Display uninstall window

  void uninstallClicked();
  void cancelClicked();

  QList <packageInfo> *packList;
  // packages to uninstall

  bool notModified;
  // haven't tried to uninstall anything
};
#endif
