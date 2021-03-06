#pragma once

/* Define if the C++ compiler supports BOOL */
#undef HAVE_BOOL

#undef VERSION

#undef PACKAGE

/* defines if having libz */
#undef HAVE_LIBZ

/* defines if having libgif (always 1) */
#undef HAVE_LIBGIF

/* defines if having libjpeg (always 1) */
#undef HAVE_LIBJPEG

#undef HAVE_LIBPNG

/* defines which to take for ksize_t */
#undef ksize_t

/* define if you have setenv */
#undef HAVE_FUNC_SETENV

/* Define to 1 if NLS is requested.  */
#undef ENABLE_NLS

/* if you have IRIX */
#undef HAVE_IRIX

/* if you have Linux */
#undef HAVE_LINUX

/* define if you have PAM (Pluggable Authentication Modules); Redhat-Users! */
#undef HAVE_PAM

/* define this if you  compile --with-pam for SOLARIS (but not for Linux) */
#undef PAM_MESSAGE_NONCONST

/* if defined, changes the default name of the PAM service used by KDE */
#undef KDE_PAM_SERVICE

/* define if you have the PAM lib. Now, we have two different users, this will c
hange */
#undef HAVE_PAM_LIB

/* Default homepage prefix */
#undef KU_HOMEPREFIX

/* Default use private group */
#undef KU_USERPRIVATEGROUP

/* Defineis if you have the RPM library.  */
#undef HAVE_RPM

#define HAVE_MEDIATOOL 1

/*
 * jpeg.h needs HAVE_BOOLEAN, when the system uses boolean in system
 * headers and I'm too lazy to write a configure test as long as only
 * unixware is related
 */
#ifdef _UNIXWARE
#define HAVE_BOOLEAN
#endif

