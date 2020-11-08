//////////////////////////////////////////////////////////////         
//      $Id: search.h 20671 1999-05-02 14:28:37Z coolo $ 
//
// Author: Toivo Pedaste
//
#ifndef SEARCH_H
#define SEARCH_H

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

// KDE headers
#include <kapp.h>
#include <kmenubar.h>
#include <kmsgbox.h>
#include <ktopwidget.h> 
#include <kbuttonbox.h> 



class Search : public QDialog
{
  Q_OBJECT

public:

  Search ( QWidget *parent = 0, const char *name=0);
  ~Search(); 
  void resizeEvent(QResizeEvent *);

private:
  QPushButton *ok, *cancel;
  QCheckBox *substr;
  QCheckBox *wrap;
  QGroupBox *frame1;
  QLineEdit *value;

  QVBoxLayout* vl;
  QVBoxLayout* vtop, vf;
  QHBoxLayout* hc;
  KButtonBox* hb;

signals:
    void search_signal();
    void search_done_signal();

public slots:
    void done_slot();
    void ok_slot();
};
#endif
