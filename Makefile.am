
appsdir = $(kde_appsdir)/Utilities
apps_DATA = kpackage.kdelnk

# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

# claim, which subdirectories you want to install
SUBDIRS = pics toolbar icon mini-icon mimetypes

bin_PROGRAMS =  kpackage 

# Which sources should be compiled for kpackage.
kpackage_SOURCES = kpackage.cpp managementWidget.cpp packageDisplay.cpp \
	  packageProperties.cpp  \
	  pkguninstallDialog.cpp findf.cpp search.cpp \
	  installationWidget.cpp  options.cpp \
	  pkginstallOptions.cpp aboutDialog.cpp \
	  packageInfo.cpp  cache.cpp main.cpp utils.cpp kio.cpp \
	  debInterface.cpp updateLoc.cpp procbuf.cpp ktvitem.cpp \
	  pkgInterface.cpp  rpmInterface.cpp kissInterface.cpp \
	  slackInterface.cpp fbsdInterface.cpp \
	  rpmInstall.cpp rpmVerify.cpp rpmutils.cpp rpmMessages.c

EXTRA_kpackage_SOURCES = rpmInterface.cpp rpmInstall.cpp  rpmMessages.cpp \
	rpmVerify.cpp rpmutils.cpp

kpackage_METASOURCES = USE_AUTOMOC

# the library search path
kpackage_LDFLAGS = -L/usr/lib $(all_libraries) $(KDE_RPATH)

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
kpackage_LDADD   = $(LIB_KFILE) $(LIB_KFM) $(LIBRPM) $(LIBDB) $(LIBINTL) $(LIBZ)  
 
# make messages.po. Move this one to ../po/ and "make merge" in po
messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(kpackage_SOURCES) && mv messages.po ../po/kpackage.pot

bin_SCRIPTS = kpackage_dpkg_rm kpackage_dpkg_ins kpackage_slack_l  

