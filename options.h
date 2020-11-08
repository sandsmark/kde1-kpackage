//////////////////////////////////////////////////////////////         
//      $Id: options.h 20671 1999-05-02 14:28:37Z coolo $ 
//
// Author: Toivo Pedaste
//
#ifndef OPTIONS_H
#define OPTIONS_H

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
#include <qlined.h>
#include <qbttngrp.h>
#include <qradiobt.h>
#include <qtabdlg.h>

// KDE headers
#include <kapp.h>
#include <kmenubar.h>
#include <ktopwidget.h> 
#include <kbuttonbox.h> 
#include <kfiledialog.h> 

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Options : public QDialog
{
  Q_OBJECT

public:

  Options ( QWidget *parent = 0, const char *name=0);
  ~Options(); 

  void restore();
  // show window, setting the buttons

private:

  void readSettings();
  void writeSettings();

  bool verifyFL;
  bool PkgRead;
  int displayP, dp, DCache, dc, PCache, pc;

  QTabDialog *tab;


  QVBoxLayout* vl;

  QVBoxLayout* vb;
  QButtonGroup *bg;
  QRadioButton *disp[4];

  QVBoxLayout*  vc;
  QButtonGroup *bc;
  QRadioButton *dcache[3];

  QVBoxLayout*  vp;
  QButtonGroup *bp;
  QRadioButton *pcache[3];

  QVBoxLayout* vr;
  QGroupBox *framer;
  QCheckBox *pkgRead;

  QVBoxLayout* vf;
  QGroupBox *framem;
  QCheckBox *valid;

public slots:
    void apply_slot();
    void PDisplay(int);
    void PDCache(int);
    void PPCache(int);
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class Params
{
public:
  bool VerifyFL;
  // config: verify the file list

  bool PkgRead;
  // read information about uninstalled packages from each RPM file itself

  enum {INSTALLED, UPDATED, NEW, ALL};
  int DisplayP;
  // which  packages to display in tree

  enum {ALWAYS, SESSION, NEVER};

  int DCache;
  // how much to cache uninstall package directories

  int PCache;
  // how much to cache uninstall packages

  Params();
  ~Params();
};
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#endif 
