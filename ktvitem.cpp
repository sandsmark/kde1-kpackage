//////////////////////////////////////////////////////////////         
//      $Id: ktvitem.cpp 16726 1999-02-11 04:18:43Z toivo $ 
//
// Author: Toivo Pedaste
//
#include "../config.h"
#include <qpixmap.h>

// kpackage.headers
#include "packageInfo.h"
#include <ktvitem.h>

KTVItem::KTVItem( QListViewItem *parent, packageInfo* pinfo,
                  const QPixmap& thePixmap,
                  const char * label1, const char * label2 ,
                  const char * label3 , const char * label4 ,
                  const char * label5 , const char * label6 ,
                  const char * label7 , const char * label8 
 
) :  QListViewItem(parent, label1, label2, label3, label4, label5,
                label6, label7, label8)
{
  info = pinfo;
  setPixmap(0, thePixmap);
}
 
KTVItem::KTVItem( QListView *parent, packageInfo* pinfo,
                  const QPixmap& thePixmap,
                  const char * label1, const char * label2 ,
                  const char * label3 , const char * label4 ,
                  const char * label5 , const char * label6 ,
                  const char * label7 , const char * label8 
) :  QListViewItem(parent, label1, label2, label3, label4, label5,
                label6, label7, label8)
{
  info = pinfo;
  setPixmap(0, thePixmap);
}

