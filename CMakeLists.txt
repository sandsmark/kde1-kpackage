cmake_minimum_required (VERSION 3.0)

project (kpackage VERSION 0.91)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

find_package(Qt1 REQUIRED)
find_package(KDE1 REQUIRED)
find_package(X11 REQUIRED)

include(CMakePackageConfigHelpers)
include(Qt1Macros)
include(KDE1Macros)
include(KDE1InstallDirs)

option(ENABLE_SANITIZERS "Enable runtime sanitizing (for development)")
if (ENABLE_SANITIZERS)
    message("Enabling asan and ubsan")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
endif()

if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-write-strings")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -std=c++98 -Wno-write-strings")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
endif()

include_directories (
    ${KDE1_INCLUDE_DIR}
)

qt1_wrap_cpp(MOC_FILES
    aboutDialog.h
    debInterface.h
    fbsdInterface.h
    findf.h
    installationWidget.h
    kio.h
    kissInterface.h
    kpackage.h
    managementWidget.h
    options.h
    packageDisplay.h
    packageProperties.h
    pkginstallOptions.h
    pkgInterface.h
    pkguninstallDialog.h
    procbuf.h
    rpmInterface.h
    search.h
    slackInterface.h
    updateLoc.h
)


add_executable(kpackage
    kpackage.cpp managementWidget.cpp packageDisplay.cpp packageProperties.cpp
    pkguninstallDialog.cpp findf.cpp search.cpp installationWidget.cpp
    options.cpp pkginstallOptions.cpp aboutDialog.cpp packageInfo.cpp cache.cpp
    main.cpp utils.cpp kio.cpp updateLoc.cpp procbuf.cpp ktvitem.cpp

    debInterface.cpp pkgInterface.cpp  rpmInterface.cpp kissInterface.cpp
    slackInterface.cpp fbsdInterface.cpp rpmInstall.cpp rpmVerify.cpp
    rpmutils.cpp rpmMessages.c

    ${MOC_FILES}
)

target_link_libraries(kpackage
    ${KDE1_KDECORE}
    ${KDE1_KDEUI}
    ${KDE1_KFM}
    ${KDE1_KFILE}
    Qt::Qt1
    X11::X11
)

add_definitions(-DVERSION="0.91" -DPACKAGE="KPackage")

install(TARGETS kpackage RUNTIME DESTINATION ${KDE1_BINDIR})

install(FILES kpackage.kdelnk DESTINATION ${KDE1_APPSDIR}/Utilities/)

# Generic icons
install(FILES
    pics/cross.xpm
    pics/dbad.xpm
    pics/deb.xpm
    pics/dnew.xpm
    pics/dupdated.xpm
    pics/ptick.xpm
    pics/question.xpm
    pics/rnew.xpm
    pics/rpm.xpm
    pics/rupdated.xpm
    pics/kiss.xpm
    pics/knew.xpm
    pics/kupdated.xpm
    pics/slack.xpm
    pics/snew.xpm
    pics/supdated.xpm
    DESTINATION ${KDE1_DATADIR}/kpackage/pics/
    )

# Toolbar icons
install(FILES
    toolbar/findf.xpm
    toolbar/ftin.xpm
    toolbar/ftout.xpm
    DESTINATION ${KDE1_DATADIR}/kpackage/toolbar/
    )

# Icons
install(FILES
    mini-icon/kpackage.xpm
    mini-icon/debfile.xpm
    mini-icon/rpmfile.xpm

    DESTINATION ${KDE1_MINIDIR}
    )
install(FILES
    mini-icon/kpackage.xpm
    mini-icon/debfile.xpm
    mini-icon/rpmfile.xpm
    DESTINATION ${KDE1_ICONDIR}
    )

install(FILES
    mimetypes/x-debian-package.kdelnk
    mimetypes/x-rpm.kdelnk
    DESTINATION ${KDE1_MIMEDIR}/application
    )
