//////////////////////////////////////////////////////////////         
//      $Id: findf.h 20671 1999-05-02 14:28:37Z coolo $ 
//
// Author: Toivo Pedaste
//
#ifndef FINDF_H
#define FINDF_H

#include "config.h"

// Standard Headers
#include <stdio.h>

// Qt Headers
#include <qdir.h>
#include <qwidget.h>
#include <qframe.h>
#include <qlabel.h>
#include <qfiledlg.h> 
#include <qgrpbox.h> 
#include <qchkbox.h>
#include <qlayout.h> 
#include <qlistview.h> 

// KDE headers
#include <kapp.h>
#include <kmenubar.h>
#include <kmsgbox.h>
#include <ktopwidget.h> 
#include <drag.h>

class FindF : public QDialog
{
  Q_OBJECT

public:

  FindF ( QWidget *parent = 0, const char *name=0);
  ~FindF(); 
  void resizeEvent(QResizeEvent *);

private:
  void setupDropzone();

  void doFind(const char *str);
  // Do the actual search

  KDNDDropZone *dropzone;
  // The dropzone where URLs can be dropped

  QPushButton *ok, *cancel;
  QGroupBox *frame1;
  QLineEdit *value;
  QListView *tab;
  QVBoxLayout* vl;
  QVBoxLayout* vtop, vf;
    
  QHBoxLayout* hb;

signals:
    void findf_signal();
    void findf_done_signal();

public slots:
  void done_slot();
  void ok_slot();
  void search(QListViewItem *);
  void dropAction(KDNDDropZone *);
};
#endif
