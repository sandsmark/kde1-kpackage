//////////////////////////////////////////////////////////////         
//      $Id: utils.h 16726 1999-02-11 04:18:43Z toivo $ 
//
// Author: Toivo Pedaste
//
#include "../config.h"
#include <string.h>
#include <qlistview.h>

char *getGroup(char **gs);
QListViewItem  *findGroup(char *name, QListViewItem *search);


