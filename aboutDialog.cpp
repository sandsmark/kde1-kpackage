/////////////////////////////////////////////////////////////////////////////////
// $Id: aboutDialog.cpp 19960 1999-04-17 22:36:57Z coolo $	
// File  : aboutDialog.cpp
// Author: Damyan Pepper
// Author: Toivo Pedaste
//
// This is the implementation of about dialog.  See aboutDialog.h for more
// information
////////////////////////////////////////////////////////////////////////////////

#include "../config.h"
// Qt headers
#include <qlabel.h>
#include <qlayout.h>
#include <kapp.h>

// ksetup headers
#include "aboutDialog.h"
#include <klocale.h>
//#include "aboutDialog.moc"


// Constructor
aboutDialog::aboutDialog(QWidget *parent, const char *name)
  : QDialog(parent,name,TRUE)
{
  // Make the dialog a nice size
  resize(250,250);

  // Create the Ok button
  okButton = new QPushButton(i18n("OK"), this);

  // Create the labels
  QLabel *line1 = new QLabel(PACKAGE,this);
  QLabel *line2 = new QLabel("Version "VERSION,this);
  QLabel *line3 = new QLabel(i18n("By Toivo Pedaste"),this);
  QLabel *line3a = new QLabel(i18n("Originally: Damyan Pepper"),this);
  QLabel *line3b = new QLabel(i18n("Addional Developers:"),this);
  QLabel *line3c = new QLabel(i18n("Bill McFarland and Thinh Van"),this);
  QLabel *line3d = new QLabel(i18n("Marco Zuehlke"),this);

  // Setup the labels
  line1->setAlignment(AlignCenter);
  line1->setFont(QFont("Helvetica",18, QFont::Bold));
  line1->setAutoResize(TRUE);

  line2->setAlignment(AlignCenter);
  line3->setAlignment(AlignCenter);
  line3a->setAlignment(AlignCenter);
  line3b->setAlignment(AlignCenter);
  line3c->setAlignment(AlignCenter);
  line3d->setAlignment(AlignCenter);

  // Create the layout manager
  QBoxLayout *layout = new QBoxLayout(this,QBoxLayout::TopToBottom);

  layout->addStretch(1);
  layout->addWidget(line1, 1);
  layout->addWidget(line2, 1);
  layout->addWidget(line3, 1);
  layout->addWidget(line3a,1);
  layout->addWidget(line3b,1);
  layout->addWidget(line3c,1);
  layout->addWidget(line3d,1);
  layout->addStretch(2);
  layout->addWidget(okButton, 1);
  layout->activate();

  // Connect the ok button
  connect(okButton,SIGNAL(clicked()), SLOT(okClicked()));
}

// Destructor -- does nothing for now
aboutDialog::~aboutDialog()
{
}

// The ok button has been clicked
void aboutDialog::okClicked()
{
  // Exit the dialog
  accept();
}

