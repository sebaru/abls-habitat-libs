##################################################################################################################################
# CMakeLists.txt    Profil de compilation de la librairie abls-libs
# Projet Abls-Habitat version 4.7      Gestion d'habitat
# Auteur: LEFEVRE Sebastien                                                                              10.06.2026
##################################################################################################################################

cmake_minimum_required(VERSION 3.16)

execute_process(
  COMMAND git describe --tags
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE ABLS_LIBS_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

if(NOT ABLS_LIBS_VERSION)
  set(ABLS_LIBS_VERSION "unknown")
endif()

if(ABLS_LIBS_VERSION MATCHES "^v?([0-9]+\\.[0-9]+)")
  set(ABLS_LIBS_VERSION_NUMBER "${CMAKE_MATCH_1}")
else()
  set(ABLS_LIBS_VERSION_NUMBER "0.0")
endif()

project(abls-libs
  VERSION ${ABLS_LIBS_VERSION_NUMBER}
  DESCRIPTION "Abls Habitat shared logging library"
  HOMEPAGE_URL "https://docs.abls-habitat.fr"
  LANGUAGES C
)

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB    REQUIRED glib-2.0)
pkg_check_modules(GTHREAD REQUIRED gthread-2.0)
pkg_check_modules(JSON_GLIB REQUIRED json-glib-1.0)
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)
pkg_check_modules(UUID REQUIRED uuid)

##################################################################################################################################
# Librairie partagée
##################################################################################################################################
add_library(abls-libs SHARED
  src/info.c
  src/json.c
  src/mqtt.c
  src/config.c
  src/heure.c
  src/run.c
  src/string.c
  src/uuid.c
)

set_target_properties(abls-libs PROPERTIES
  VERSION   ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  PUBLIC_HEADER "include/info.h;include/json.h;include/mqtt.h;include/config.h;include/heure.h;include/run.h;include/string.h;include/uuid.h;include/abls-libs.h"
)

target_include_directories(abls-libs
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/abls-libs>
  PRIVATE
    ${GLIB_INCLUDE_DIRS}
    ${GTHREAD_INCLUDE_DIRS}
    ${JSON_GLIB_INCLUDE_DIRS}
    ${MOSQUITTO_INCLUDE_DIRS}
    ${UUID_INCLUDE_DIRS}
)

target_compile_options(abls-libs PRIVATE
  -Wall -Wno-unused-result -Wno-comment -pipe -g -ggdb
)

target_compile_definitions(abls-libs PRIVATE
  ABLS_LIBS_VERSION="${ABLS_LIBS_VERSION}"
)

target_link_libraries(abls-libs PRIVATE
  ${GLIB_LIBRARIES}
  ${GTHREAD_LIBRARIES}
  ${JSON_GLIB_LIBRARIES}
  ${MOSQUITTO_LIBRARIES}
  ${UUID_LIBRARIES}
)

##################################################################################################################################
# Installation
##################################################################################################################################
install(TARGETS abls-libs
  LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}             COMPONENT runtime
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/abls-libs COMPONENT devel
)

install(FILES sudoers/abls_sudo
  DESTINATION /etc/sudoers.d
  PERMISSIONS OWNER_READ GROUP_READ
  COMPONENT runtime
)

##################################################################################################################################
# pkg-config
##################################################################################################################################
configure_file(abls-libs.pc.in abls-libs.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/abls-libs.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  COMPONENT devel
)

##################################################################################################################################
# CPack — packaging RPM/DEB
##################################################################################################################################
set(CPACK_PACKAGE_NAME        "abls-libs")
set(CPACK_PACKAGE_VERSION     "${PROJECT_VERSION}")
set(CPACK_PACKAGE_RELEASE     "1")
set(CPACK_PACKAGE_VENDOR      "Abls-Habitat")
set(CPACK_PACKAGE_CONTACT     "sebastien.lefevre@abls-habitat.fr")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_LICENSE     "GPLv3")

set(CPACK_GENERATOR           "RPM;DEB")
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL      runtime devel)

# Suite separation is handled by the repository layout, not by a Debian version suffix.
set(CPACK_DEBIAN_PACKAGE_VERSION "${PROJECT_VERSION}")

# Do not claim ownership of system sudoers directories in RPM.
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/etc;/etc/sudoers.d")

# Package runtime
set(CPACK_RPM_RUNTIME_PACKAGE_NAME    "abls-libs")
set(CPACK_RPM_RUNTIME_PACKAGE_SUMMARY "Abls-Habitat shared library (runtime)")
set(CPACK_RPM_RUNTIME_PACKAGE_REQUIRES "glib2, json-glib")
set(CPACK_RPM_RUNTIME_POST_SCRIPTLET  "/sbin/ldconfig")
set(CPACK_RPM_RUNTIME_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_RUNTIME_DEBUGINFO_PACKAGE ON)
set(CPACK_RPM_RUNTIME_DEBUGINFO_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_RUNTIME_BUILD_SOURCE_DIRS_PREFIX "/usr/src/abls/libs")

# Package devel
set(CPACK_RPM_DEVEL_PACKAGE_NAME    "abls-libs-devel")
set(CPACK_RPM_DEVEL_PACKAGE_SUMMARY "Abls-Habitat shared library (development files)")
set(CPACK_RPM_DEVEL_PACKAGE_REQUIRES "abls-libs = ${PROJECT_VERSION}, glib2-devel, json-glib-devel")
set(CPACK_RPM_DEVEL_FILE_NAME "RPM-DEFAULT")

# Package runtime (DEB)
set(ABLS_DEBIAN_GLIB_RUNTIME_DEPEND "libglib2.0-0t64 | libglib2.0-0")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME "abls-libs")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_DEPENDS "${ABLS_DEBIAN_GLIB_RUNTIME_DEPEND}, libjson-glib-1.0-0, libmosquitto1")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_SHLIBDEPS ON)

# Package devel (DEB)
set(CPACK_DEBIAN_DEVEL_PACKAGE_NAME "abls-libs-dev")
set(CPACK_DEBIAN_DEVEL_PACKAGE_DEPENDS "abls-libs (= ${CPACK_DEBIAN_PACKAGE_VERSION}), libglib2.0-dev, libjson-glib-dev, libmosquitto-dev")

include(CPack)
