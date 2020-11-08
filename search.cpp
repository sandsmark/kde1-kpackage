//////////////////////////////////////////////////////////////         
//      $Id: search.cpp 19960 1999-04-17 22:36:57Z coolo $ 
//
// Author: Toivo Pedaste
//
#include "config.h"

#include "kpackage.h"
#include "managementWidget.h"
#include "search.h"
#include <klocale.h>

Search::Search(QWidget *parent, const char *name)
    : QDialog(parent, name,FALSE){

    this->setFocusPolicy(QWidget::StrongFocus);
    QVBoxLayout* vtop = new QVBoxLayout( this, 10, 10, "vtop");

    frame1 = new QGroupBox(i18n("Find Package"), this, "frame1");
    vtop->addWidget(frame1,1);
    QVBoxLayout* vf = new QVBoxLayout( frame1, 20, 10, "vf");

    value = new QLineEdit( frame1, "v");
    vf->addWidget(value,0);
    value->setFocus();
#if QT_VERSION < 200
    value->setFixedHeight(value->sizeHint().height());
#endif
    connect(value, SIGNAL(returnPressed()), this, SLOT(ok_slot()));

    QHBoxLayout* hc = new QHBoxLayout( );
    vf->addLayout(hc,0);

    substr = new QCheckBox(i18n("Sub string"), frame1, "substr");
    substr->setChecked(TRUE);
    hc->addWidget(substr,1,AlignLeft);
#if QT_VERSION < 200
    substr->setFixedSize(substr->sizeHint());
    hc->addStretch(1);
#endif

    wrap = new QCheckBox(i18n("Wrap search"), frame1, "wrap");
    wrap->setChecked(TRUE);
    hc->addWidget(wrap,1,AlignRight);
#if QT_VERSION < 200
    wrap->setFixedSize(wrap->sizeHint());
#endif

    hb = new KButtonBox( this, KButtonBox::HORIZONTAL, 6,6 );
    vtop->addWidget(hb,0);

    ok = hb->addButton(i18n("Find"));
    connect(ok, SIGNAL(clicked()), this, SLOT(ok_slot()));

#if QT_VERSION < 200
    hb->addStretch(1);
#endif

    cancel = hb->addButton(i18n("Done"));
    connect(cancel, SIGNAL(clicked()), this, SLOT(done_slot()));

    hb->layout();
#if QT_VERSION < 200
    vf->activate();
    vtop->activate();
    resize(300, minimumSize().height());
    setMaximumHeight(height());
#endif
    show();
}

Search::~Search()
{
}

void Search::ok_slot() 
{
  QListViewItem *pkg;

  QString to_find = this->value->text();
  
  pkg = kpkg->kp->management->search(to_find.data(),
		 substr->isChecked(),FALSE,FALSE);
  if (pkg == 0 && wrap->isChecked()) {
    pkg = kpkg->kp->management->search(to_find.data(),
		 substr->isChecked(),TRUE,FALSE);
  }
  if (pkg == 0)
    KpMsg(i18n("Note"),
	  i18n("%s was not found"),to_find.data(),TRUE);
}

void Search::done_slot()
{
  this->hide();
}
void Search::resizeEvent(QResizeEvent *){
}

