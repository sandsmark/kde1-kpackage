#ifndef KP_KIO_H
#define KP_KIO_H


#include <vector>

#include "qobject.h"
#include "qfile.h"
#include "qtextstream.h"
#include "config.h"

#if QT_VERSION >= 200
#include <kio_job.h>
#endif

class Kio: public QObject
{
  Q_OBJECT

public:
  Kio();

#if QT_VERSION < 200
};
#else
  bool download(QString from, QString to);

private:
  KIOJob * iojob;
  bool worked;

private slots:
  void slotIOJobFinished( int );
  void slotIOJobError( int );
};
#endif

class Kiod: public QObject
{
  Q_OBJECT

public:
  Kiod();

#if QT_VERSION < 200
};
#else
 bool listDir(QString url, QString fname);

private:
  KIOJob* job;
  QFile *file;
  QTextStream *fileT;
  bool worked;

private slots:
void slotListEntry( int /*_id*/, const UDSEntry&  );
void slotError( int /*_id*/, int _errid, const char*  );
void slotFinished( int /*_id*/ );
};
#endif
#endif
