/////////////////////////////////////////////////////////////////////////////
//      $Id: packageProperties.h 19960 1999-04-17 22:36:57Z coolo $ 
// File  : packageProperties.h
// Author: Damyan Pepper
// Author: Toivo Pedaste
//
// This widget is used to provide a list of all the properties that are
// found in the package's property dictionary
//
/////////////////////////////////////////////////////////////////////////////

#ifndef PACKAGEPROPERTIES_H
#define PACKAGEPROPERTIES_H
#include "config.h"

// Standard Headers

// Qt Headers
#include <qtablevw.h>
#include <qpainter.h>
class QStrList;

// KDE Headers

// kpackage Headers
#include "packageInfo.h"

class packageInfo;

class packagePropertiesWidget : public QTableView
{
  Q_OBJECT;
  ///////////// METHODS ------------------------------------------------------
public:

  packagePropertiesWidget(QWidget *parent=0, const char *name=0);
  // constructor

  ~packagePropertiesWidget();
  // destructor

  void changePackage(packageInfo *p);

protected:

  int cellHeight( int row);
  int cellWidth(int col);

  void paintCell(QPainter *p, int row, int col);

  ///////////// SLOTS --------------------------------------------------------

  ///////////// SIGNALS ------------------------------------------------------

  ///////////// DATA ---------------------------------------------------------
private:
  packageInfo *package;
 
  void initTranslate();
  void iList(const char *txt, const char *itxt);

  QDict<char> *trl;
  // allow for translation of labels

  QStrList *pList;
  // list specifying order of property distplay

  QStrList *cList;
  // list giving order of currently displayed properties


};


#endif
