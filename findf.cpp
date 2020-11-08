//////////////////////////////////////////////////////////////         
//      $Id: findf.cpp 21211 1999-05-11 09:16:28Z bieker $ 
//
// Author: Toivo Pedaste
//
#include "../config.h"

#include "kpackage.h"
#include "pkgInterface.h"
#include "managementWidget.h"
#include "pkgInterface.h"
#include "findf.h"
#include <klocale.h>

extern pkgInterface *kpinterface[];

FindF::FindF(QWidget *parent, const char *name)
    : QDialog(parent, name,FALSE){

    this->setFocusPolicy(QWidget::StrongFocus);

    QVBoxLayout* vtop = new QVBoxLayout( this, 20, 10, "vtop");

    frame1 = new QGroupBox(i18n("Find File"), this, "frame1");
    vtop->addWidget(frame1,4);
    QVBoxLayout* vf = new QVBoxLayout( frame1, 12, 10, "vf");

    value = new QLineEdit( frame1, "value");
    vf->addWidget(value,1);
    value->setFocus();
#if QT_VERSION < 200
    value->setFixedHeight(value->sizeHint().height());
#endif
    connect(value, SIGNAL(returnPressed()), this, SLOT(ok_slot()));

    tab = new QListView(frame1,"tab");
    connect(tab, SIGNAL(selectionChanged ( QListViewItem * )),
	    this, SLOT(search( QListViewItem * )));
    tab->addColumn(i18n("Type"),40);
    tab->addColumn(i18n("Package"),120);
    tab->addColumn(i18n("File Name"),300);
    tab->setAllColumnsShowFocus(TRUE);
    tab->setSorting(1);
    vf->addWidget(tab,4);

    QHBoxLayout* hb = new QHBoxLayout( );
    vtop->addLayout(hb,0);

    ok = new QPushButton(i18n("Find"), this, "find");
    hb->addWidget(ok,1,AlignLeft);
#if QT_VERSION < 200
    ok->setFixedSize(ok->sizeHint());
#endif
    connect(ok, SIGNAL(clicked()), this, SLOT(ok_slot()));
    
    hb->addStretch();

    cancel = new QPushButton(i18n("Done"), this, "cancel");
    hb->addWidget(cancel,1,AlignRight);
#if QT_VERSION < 200
    cancel->setFixedSize(cancel->sizeHint());
#endif
    connect(cancel, SIGNAL(clicked()), this, SLOT(done_slot()));
 
#if QT_VERSION < 200
    vtop->activate();
    setMinimumSize(minimumSize());
    setMaximumHeight(height());
#endif
    show();

    setupDropzone();
}

FindF::~FindF()
{
}

// Set up the drop zone
void FindF::setupDropzone()
{
  dropzone = new KDNDDropZone(this, DndURL);
  connect(dropzone,SIGNAL(dropAction(KDNDDropZone*)),
	  SLOT(dropAction(KDNDDropZone*)));
}

void FindF::ok_slot() 
{
  doFind(this->value->text());
}

void FindF::doFind(const char *str) 
{
  QString s, tmp;
  QString t;
  int i, cnt = 0;;

  tab->clear();

  for (i = 0; i < kpinterfaceN; i++) {
    s = kpinterface[i]->FindFile(str);

    if (s.length() > 0) {
      cnt++;
      int p = 0, pp = 0;
      while ((p = s.find('\n',p)) >= 0) {
	t = kpinterface[i]->head.data();
	tmp = s.mid(pp,p - pp);
	if (tmp.find("diversion by") >= 0) {
	  new QListViewItem(tab, "", tmp);
	}

	int t1 = tmp.find('\t');
	QString s1 = tmp.left(t1);
	QString s2 = tmp.right(tmp.length()-t1);

	new QListViewItem(tab, t, s1, s2);
	p++;
	pp = p;
      }
    }
  }

  if (!cnt) {
    new QListViewItem(tab, "", i18n("--Nothing found--"));
  }
}

void FindF::done_slot()
{
  this->hide();
}

void FindF::resizeEvent(QResizeEvent *){
}

void FindF::search(QListViewItem *item)
{
  int p;
  QString s = item->text(1);
    
  p = s.find(',');
  if (p > 0) {
    s.truncate(p);
  }
  kpkg->kp->management->search(s.data(),item->text(0));
}

void FindF::dropAction(KDNDDropZone *dz) // something has been dropped
{
  char *s;
  QStrList &list = dz->getURLList();
  QString url;

  s = list.first();
  KURL *u = new KURL( s );

  if (!u->isMalformed() && (strcmp( u->protocol(), "file" ) == 0 )) {
    url = u->path();
    if (url.right(1) == "/")
      url.truncate(url.length()-1);
    value->setText(url);
    doFind(url.data());
  } else {
    KpMsgE(i18n("Incorrect URL type"),"",FALSE);
  }
}
