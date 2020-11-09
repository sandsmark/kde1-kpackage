//////////////////////////////////////////////////////////////         
//      $Id: procbuf.cpp 19960 1999-04-17 22:36:57Z coolo $ 
//
// Author: Toivo Pedaste
//
#include "config.h"
#include "procbuf.h"
#include <kprocess.h>
#include <kmsgbox.h>
#include "kpackage.h"
#include <klocale.h>

Modal::Modal(const char *msg, QWidget *parent, const char *name )
  : QDialog( parent, name, TRUE )
{
  QLabel *line1 = new QLabel(msg,this);
  line1->setAlignment(AlignCenter);
  line1->setAutoResize(true);

 }

void Modal::terminate()
{
  done(0);
}

procbuf::procbuf()
{
  m = NULL;
  proc = NULL;
}

procbuf::~procbuf()
{
}

void procbuf::setup(const char *cmd)
{
  buf.truncate(0);
  proc = new KProcess();
  connect(proc, SIGNAL( receivedStdout(KProcess *, char *, int)), 
			this, SLOT(slotReadInfo(KProcess *, char *, int)));
  connect(proc, SIGNAL( receivedStderr(KProcess *, char *, int)), 
			this, SLOT(slotReadInfo(KProcess *, char *, int)));
  connect(proc, SIGNAL( processExited(KProcess *)), 
			this, SLOT(slotExited(KProcess *)));
  proc->clearArguments();
  proc->setExecutable(cmd);
  command = cmd;
}

void procbuf::slotReadInfo(KProcess *, char *buffer, int buflen)
{
   char last;

   last = buffer[buflen - 1];
   buffer[buflen - 1] = 0;
   buf += buffer;
   buf += last;
}

void procbuf::slotExited(KProcess *)
{
  delete proc;
  proc = NULL;
  if (m)
    m->terminate();
}

int procbuf::start (const char *msg, bool errorDlg )
{

  if (!proc->start(msg ? KProcess::NotifyOnExit : KProcess::Block,  KProcess::AllOutput)) {
    KpMsgE(i18n("Kprocess Failure"),"",TRUE);
    return 0;
  };
  if (!proc) {
      puts("Quit too early");
      return 0;
  }

  proc->closeStdin();
  if (msg) {
    m = new Modal(msg,kpkg,i18n("Wait"));
    m->exec();
    delete m;
    m = NULL;
  }

  if (!proc->normalExit() || proc->exitStatus()) {
    if (errorDlg) {
      if (buf.length()) {
	KpMsg("Error","Kprocess error:%s", buf.data(), TRUE);
      } else {
	KpMsg("Error","Kprocess error: Cann't start %s", command, TRUE);
      }
    }
    return 0;
  }

  return 1;
}











