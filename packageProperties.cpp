//////////////////////////////////////////////////////////////         
// 	$Id: packageProperties.cpp 19960 1999-04-17 22:36:57Z coolo $	
//
// Author: Toivo Pedaste
//

#include "../config.h"
#include <stdio.h>
#include "kpackage.h"
#include "packageProperties.h"
#include <klocale.h>
//#include "packageProperties.moc"

packagePropertiesWidget::packagePropertiesWidget(QWidget *parent,
						 const char *name)
  : QTableView(parent,name)
{
  setFrameStyle(QFrame::Sunken | QFrame::Panel);
  package=NULL;
  setTableFlags(Tbl_clipCellPainting);
  setTableFlags(Tbl_autoVScrollBar);
  QColorGroup cg = colorGroup();
  setBackgroundColor(cg.base());
  setNumCols(2);
  setNumRows(15);
  initTranslate();
}

packagePropertiesWidget::~packagePropertiesWidget()
{
  delete trl;
}

void packagePropertiesWidget::iList(char *txt, const char *itxt)
{
  trl->insert(txt,strdup(itxt));
  pList->append(txt);
}


void packagePropertiesWidget::initTranslate()
{
  trl = new QDict<char>(53);
  pList = new QStrList(FALSE);
  cList = new QStrList(FALSE);

  iList("name", i18n("name"));
  iList("summary", i18n("summary"));
  iList("version", i18n("version"));
  iList("old-version", i18n("old-version"));
  iList("status", i18n("status"));
  iList("group", i18n("group"));
  iList("size", i18n("size"));
  iList("file-size", i18n("file-size"));
  iList("description", i18n("description"));
  iList("architecture", i18n("architecture"));

  iList("unsatisfied dependencies", i18n("unsatisfied dependencies"));
  iList("pre-depends", i18n("pre-depends"));
  iList("dependencies", i18n("dependencies"));
  iList("depends", i18n("depends"));
  iList("conflicts", i18n("conflicts"));
  iList("provides", i18n("provides"));
  iList("recommends", i18n("recommends"));
  iList("replaces", i18n("replaces"));
  iList("suggests", i18n("suggests"));
  iList("priority", i18n("priority"));

  iList("essential", i18n("essential"));
  iList("installtime", i18n("installtime"));
  iList("config-version", i18n("config-version"));
  iList("distribution", i18n("distribution"));
  iList("vendor", i18n("vendor"));
  iList("maintainer", i18n("maintainer"));
  iList("packager", i18n("packager"));
  iList("source", i18n("source"));
  iList("build-time", i18n("build-time"));
  iList("build-host", i18n("build-host"));
  iList("base", i18n("base"));
  iList("filename", i18n("filename"));
  iList("serial", i18n("serial"));

  iList("also in", i18n("also in"));
  iList("run depends", i18n("run depends"));
  iList("build depends", i18n("build depends"));
  iList("available as", i18n("available as"));
}

void packagePropertiesWidget::changePackage(packageInfo *p)
{
  char *s;

  package = p;
  cList->clear();
  if (p) {
    // append properties in ordered list to current list
    for (s = pList->first(); s != 0; s = pList->next()) {
      if (p->getProperty(s) && !(p->getProperty(s)->isEmpty())) {
	cList->append(s);
      }
    }
    // append other properties to end
    QDictIterator<QString> it(*(p->getDict()));
    while (it.current()) {
      if (!trl->find(it.currentKey())) {
	cList->append(it.currentKey());
      }
      ++it;
    }
  }

  update();
  updateTableSize();
  setTopLeftCell(0,0);
}

int packagePropertiesWidget::cellHeight(int row)
{
  int h;

  if(package) {
    if (row < (int)cList->count()) {
      QPainter p0;
      QWidget *temp0 = new QWidget(this);
      QString its = trl->find(cList->at(row)) ?
	trl->find(cList->at(row)) : cList->at(row);

      temp0->hide();
      p0.begin(temp0);
      QRect r0 = p0.boundingRect(0,0,cellWidth(0), 1000, 
				 AlignLeft|AlignTop|WordBreak,
				 its);
      p0.end();
      delete temp0;

      QPainter p;
      QWidget *temp = new QWidget(this);
      temp->hide();
      p.begin(temp);
      QRect r = p.boundingRect(0,0,cellWidth(1), 1000, 
			       AlignLeft|AlignTop|WordBreak,
			       package->getProperty(cList->at(row))->data());
      p.end();
      delete temp;
      if (r.height() > r0.height()) {
	h = r.height();
      } else {
	h = r0.height();
      }
      return h + 2;
    }
  }
  return 0;
}

int packagePropertiesWidget::cellWidth(int col)
{
  if(col==0)
    return( width() / 4 );
  if(col==1)
    return(((width() / 4) * 3) - 23);
  return 0;
}

void packagePropertiesWidget::paintCell(QPainter *p, int row, int col)
{
  if (row < (int)cList->count()) {
    QRect r(0,0,cellWidth(col),cellHeight(row));

    if(package) {
      QString its = trl->find(cList->at(row)) ?
	trl->find(cList->at(row)) : cList->at(row);
      if(col==0)
	{
	  p->drawText(2,0,r.width()-4,r.height(),
		      AlignLeft|AlignTop|WordBreak,its);
	}
      else
	{
	  p->drawText(2,0,r.width(),r.height(),
		      AlignLeft|AlignTop|WordBreak,
		      package->getProperty(cList->at(row))->data());
	}
      updateScrollBars();
    } 
  }
  // p->drawLine(r.bottomLeft(),r.bottomRight());
}
