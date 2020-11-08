//////////////////////////////////////////////////////////////         
//      $Id: procbuf.h 16726 1999-02-11 04:18:43Z toivo $ 
//
// Author: Toivo Pedaste
//
#include "config.h"
#include <kprocess.h>
#include <qdialog.h> 
#include <qobject.h> 

#ifndef PROCBUF
#define PROCBUF

class Modal : public QDialog {
  Q_OBJECT
public:
  Modal(const char *msg, QWidget *parent, const char *name );
  void terminate();
};

class procbuf: public QObject
{
Q_OBJECT;

public:
   procbuf();
   ~procbuf();
   void setup(const char *);
   QString buf;
   KProcess *proc; 
   Modal *m;
   const char *command;
   int start(const char *msg = 0, bool errorDlg = TRUE);

public slots:
  void slotReadInfo(KProcess *, char *, int);
  void slotExited(KProcess *);
};
#endif
