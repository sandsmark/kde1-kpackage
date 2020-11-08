//////////////////////////////////////////////////////////////         
//      $Id: updateLoc.h 20671 1999-05-02 14:28:37Z coolo $ 
//
// Author: Toivo Pedaste
//
#ifndef DEBLOCATE_H
#define DEBLOCATE_H

#include "../config.h"

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
#include <qtabdlg.h>

// KDE headers
#include <kapp.h>
#include <kmenubar.h>
//#include <kmsgbox.h>
#include <ktopwidget.h> 
#include <kbuttonbox.h> 
#include <kfiledialog.h> 

class pkgInterface;
class updateLoc;
class cacheObj;
class LcacheObj;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class dpanel : public QWidget
{
  Q_OBJECT

public:
  dpanel(updateLoc *upd, char *Pfilter, QWidget *parent,
	 bool buse = TRUE, const char *name  = 0 );
  ~dpanel();


  KFileDialog* getFileDialog(const char* captiontext, bool dir);
  QString getText();
  void setText(const char *s);
  bool getUse();
  void setUse(int n);

private:

  QCheckBox *puse;
  QHBoxLayout* pack;
  QLineEdit *pent;
  QPushButton *pbut;

  dpanel *base;
  char *filter;

 public slots:
  void fileOpen();
  void dirOpen();

};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class updateLoc : public QDialog
{
  Q_OBJECT

public:

  updateLoc (QWidget *p, int panelNumber, int numberLines, pkgInterface *inter,
	     const char *msg, char *iname, char *filter,
	     const char *lmsg, const char *bmsg = 0);
  ~updateLoc(); 

  void restore();
  // show window, setting the buttons

  void readSettings();
  void writeSettings();

  void applyS(LcacheObj *slist);

  bool haveBase;
  dpanel *base;

private:

  char *interName;
  QString packL, packU, availB;
  int panNumber;

  int wdth;
  
  QPushButton  *butloc;

  QHBoxLayout* hloc;

  pkgInterface *interface;

  QGroupBox *fbase;
  QVBoxLayout* vbase;

  static const int PNUM = 8;
  int numLines;
  QGroupBox *floc;
  QVBoxLayout* vloc;
  dpanel *dp[PNUM];

  QVBoxLayout* vl;
  QVBoxLayout* vtop, *vf;
  QGroupBox *frame1;
  KButtonBox* hb;

};
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Locations : public QDialog
{
  Q_OBJECT

public:
Locations( const char *msg);
void dLocations(int numberDirs,  int numberLines,
		     pkgInterface *inter, const char *label,
		     char *iname, char *filter, const char *dirMsg);
void pLocations(int numberDirs,  int numberLines,
		     pkgInterface *inter, const  char *label,
		     char *iname, char *filter,
		     const char *packMsg, const char *baseMsg = 0);

  // bmsg indicates the panel has a base entry
~Locations();
void restore();

QTabDialog *tab;
int numPanels;
static const int PANNUM = 12;
updateLoc *pn[PANNUM];

public slots:
    void apply_slot();

signals:
  void returnVal(LcacheObj *);

};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#endif
