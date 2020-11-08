//////////////////////////////////////////////////////////////         
// 	$Id: rpmMessages.c 20996 1999-05-07 14:13:11Z toivo $	
#include "config.h"

#ifdef HAVE_RPM
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <rpm/rpmlib.h>
#include "rpmMessages.h"

static int minLevel = RPMMESS_NORMAL;

void rpmIncreaseVerbosity(void) 
{
  minLevel--;
}

void rpmSetVerbosity(int level) 
{
  minLevel = level;
}

int rpmGetVerbosity(void)
{
  return minLevel;
}

int rpmIsDebug(void)
{
  return (minLevel <= RPMMESS_DEBUG);
}

int rpmIsVerbose(void)
{
  return (minLevel <= RPMMESS_VERBOSE);
}

void rpmMessage(int level, char * format, ...) 
{
  char buffer[800];
  char f[400];

  va_list args;

  strncpy(f,format,400);
  // remove newlines from format
  //  p = f;
  //  while(*p)
  //    {
  //      if(*p == '\n' && *(p+1))
  //	*p = ' ';
  //      p++;
  //   }


  va_start(args, format);

  if (level >= minLevel) 
    {
      switch (level) 
	{
	case RPMMESS_VERBOSE:
	case RPMMESS_NORMAL:
	  vsprintf(buffer, f, args);
	  break;
	  
	case RPMMESS_DEBUG:
	  vsprintf(buffer, f, args);
	  break;
	  
	case RPMMESS_WARNING:
	  vsprintf(buffer, f, args);
	  //	  printf("%s\n",buffer);
	  break;
	  
	case RPMMESS_ERROR:
	  vsprintf(buffer, f, args);
	  rpmMess(buffer);
 	  //	  printf("%s\n",buffer);
	  break;
	  
	case RPMMESS_FATALERROR:
	  vsprintf(buffer, f, args);
	  //	  exit(1);
	  break;
	}
      rpmBuff(buffer);
      //      kpkg->kp->setStatus(buffer);
    }
}
#endif
