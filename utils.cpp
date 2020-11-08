//////////////////////////////////////////////////////////////         
//      $Id: utils.cpp 20996 1999-05-07 14:13:11Z toivo $ 
//
// Author: Toivo Pedaste
//
#include "../config.h"
#include "utils.h"
#include <kmsgbox.h>
#include <kpackage.h>

void KpMsgE( const char *format, const char *str, bool stop)
{
 KpMsg( "Error", format, str,  stop);
}

void KpMsg(const char *lab, const char *format, const char *str, bool stop)
{
  QString qtmp; 
  int flag ;
  char buf[201];

  if (stop)
    flag = KMsgBox::STOP;
  else
    flag = KMsgBox::EXCLAMATION;

  if (format) {
    strncpy(buf,str,200);
    buf[200] = 0;
    qtmp.sprintf(format,buf);
    KMsgBox::message(kpkg, lab, qtmp.data(), flag);
  } else {
     KMsgBox::message(kpkg, lab, str, flag);
  }
}

char *getGroup(char **gs)
{
  char *gsc = strdup(*gs);
  char *p=gsc;
  while(*p && *p != '/')
    {
      p++;
      (*gs)++;
    }
  *p='\0';
  if(**gs)
    (*gs)++;
  return gsc;
}


QListViewItem *findGroup(char *name, QListViewItem *search)
{
  if(!search)
    return NULL;

  do {
    if(strcmp(name, search->text(0)) == 0) {
      return search;
    }
  } while( (search=search->nextSibling()) != NULL);

  return NULL;
}



