///////////////////////////////////////////////////////////////////////////////
// $Id: main.cpp 19331 1999-04-07 15:11:21Z toivo $	
// File  : main.cpp
// Author: Damyan Pepper
// Author: Toivo Pedaste
//
// This is the entry point to the program
///////////////////////////////////////////////////////////////////////////////
#include "config.h"
#include "kpackage.h"

#include <kapp.h>
#include <options.h>

#include "debInterface.h"
#include "kissInterface.h"
#include "slackInterface.h"
#include "fbsdInterface.h"
#include "alpminterface.h"
#ifdef HAVE_RPM
#include "rpmInterface.h"
#endif

// Keep a global pointer to ksetup
KPKG *kpkg;
char *argv0;
KApplication *app;
KIconLoader  *globalKIL;
Params *params;

#ifdef HAVE_RPM
  const int kpinterfaceN = 6;
#else
  const int kpinterfaceN = 5;
#endif
pkgInterface *kpinterface[kpinterfaceN];
 
int main(int argc, char **argv)
{
  kpkg = 0;
  params = new Params();

  // Create the application
  app = new KApplication(argc, argv, "kpackage");
  globalKIL  = app->getIconLoader();
   
  kpinterface[0] = new DEB();
  kpinterface[1] = new KISS();
  kpinterface[2] = new fbsdInterface();
  kpinterface[3] = new SLACK(); // Also catched BSD packages...
  kpinterface[4] = new alpmInterface;
#ifdef HAVE_RPM
  kpinterface[5] = new RPM();
#endif

  if ( app->isRestored() ) {
    if (KTopLevelWidget::canBeRestored(1)) {
     kpkg =  new KPKG(app->getConfig());  
      kpkg->restore(1);
    }
  } else {
    // Create the main widget and show it
    kpkg = new KPKG(app->getConfig());
    kpkg->show();
  }

  if(argc == 2)			// an argument has been given
    { 
      kpkg->kp->openNetFile(argv[1]); // and install package from argument
    }
  else {			// otherwise
    if (!kpkg->prop_restart)
      kpkg->kp->setMode(KPACKAGE::Management, 0, 1); // enter management mode
  }

  int r = app->exec();		// execute the application

  // delete objects on exit
  delete kpkg;
  for (int i = 0; i < kpinterfaceN; i++) {
    delete kpinterface[i];
  }
  delete globalKIL;

  return r;			// return the result
}

