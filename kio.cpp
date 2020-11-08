#include <kapp.h>

#if QT_VERSION >= 200

#include "kio.h"

extern KApplication *app;

Kio::Kio()
{
  iojob = 0;
}

bool Kio::download(QString from, QString to)
{
  if (iojob == 0) {
    iojob = new KIOJob;
  }

  connect( iojob, SIGNAL( sigFinished( int ) ),
	   this, SLOT( slotIOJobFinished( int ) ) );
  connect( iojob, SIGNAL( sigError(int, const char *) ),
	   this, SLOT( slotIOJobError(int, const char *) ) );
  
  iojob->copy( from, to );
  printf("Enter...............................\n");
  app->enter_loop();
  iojob = 0;

  return worked;
}

void Kio::slotIOJobFinished( int )
{
  printf("Leave FIN...............................\n");
  worked = TRUE;
  app->exit_loop();
}

void Kio::slotIOJobError( int )
{
  printf("Leave ERR...............................\n");
  worked = FALSE;
  app->exit_loop();
}

Kiod::Kiod()
{
  job = new KIOJob;

  QObject::connect( job, SIGNAL( sigListEntry( int, const UDSEntry& ) ),
		    this, SLOT( slotListEntry( int, const UDSEntry& ) ) );
  QObject::connect( job, SIGNAL( sigFinished( int ) ),
		    this, SLOT( slotFinished( int ) ) );
  QObject::connect( job, SIGNAL( sigError( int, int, const char* ) ),
	   this, SLOT( slotError( int, int, const char* ) ) );
}

bool Kiod::listDir(QString url, QString fname)
{
  file = new QFile(fname);
  if (file->open(IO_WriteOnly)) {
    fileT = new QTextStream(file);
    job->listDir( url );

    app->enter_loop();

    file->close();
    if (worked)
      return TRUE;
    else
      return FALSE;
  } else
    return FALSE;
}

void Kiod::slotListEntry( int /*_id*/, const UDSEntry& entry )
{
  long size = 0;
  QString text;

  UDSEntry::const_iterator it = entry.begin();
  for ( ; it != entry.end(); it++ ) {
    if ( it->m_uds == UDS_SIZE )
      size = it->m_long;
    else if ( it->m_uds == UDS_NAME )
      text = it->m_str.c_str();
  }
  
  *fileT << text << "\n" << size << "\n";
}

void Kiod::slotError( int /*_id*/, int _errid, const char* _errortext )
{
  //  kioErrorDialog( _errid, _errortext );
  worked = FALSE;
  app->exit_loop();
}

void Kiod::slotFinished( int /*_id*/ )
{
  worked = TRUE;
  app->exit_loop();
}
#endif
