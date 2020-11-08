//////////////////////////////////////////////////////////////         
//      $Id: ktvitem.h 16726 1999-02-11 04:18:43Z toivo $ 
//
// Author: Toivo Pedaste
//
#ifndef KTVITEM_H
#define KTVITEM_H

#include "config.h"
// Standard Headers

// Qt Headers
#include <qframe.h>
#include <qpushbt.h>
#include <qlist.h>
#include <qstring.h>
#include <qlayout.h>

// KDE headers

// ksetup headers
#include <qlistview.h>
#include "packageInfo.h"


class KTVItem : public QListViewItem
{
public:
KTVItem( QListViewItem *parent, packageInfo* pinfo,
                  const QPixmap& thePixmap,
                  const char * label1 = 0, const char * label2  = 0,
                  const char * label3  = 0, const char * label4  = 0,
                  const char * label5  = 0, const char * label6  = 0,
                  const char * label7  = 0, const char * label8 = 0);
 
 
KTVItem( QListView *parent, packageInfo* pinfo,
                  const QPixmap& thePixmap,
                  const char * label1 = 0, const char * label2  = 0,
                  const char * label3  = 0, const char * label4  = 0,
                  const char * label5  = 0, const char * label6  = 0,
                  const char * label7  = 0, const char * label8 = 0);
 

packageInfo *info;
};

#endif
