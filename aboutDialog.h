////////////////////////////////////////////////////////////////////////////////
// $Id: aboutDialog.h 16726 1999-02-11 04:18:43Z toivo $	
// File  : aboutDialog.h
// Author: Damyan Pepper
// Author: Toivo Pedaste
//
// This dialog displays information about ksetup.  It has one button, Ok,
// which, when clicked, causes the dialog to exit.
////////////////////////////////////////////////////////////////////////////////

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include "config.h"
// Qt headers
#include <qdialog.h>
#include <qpushbt.h>


class aboutDialog : public QDialog
{
  Q_OBJECT;
public:
  aboutDialog(QWidget *parent=0, const char *name=0);
  ~aboutDialog();

private slots:
  void okClicked();

private:
  QPushButton *okButton;
};

#endif
