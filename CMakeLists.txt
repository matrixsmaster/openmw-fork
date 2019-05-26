# Apps and tools
option(BUILD_OPENMW "build OpenMW" ON)
option(BUILD_BSATOOL "build BSA extractor" ON)
option(BUILD_ESMTOOL "build ESM inspector" ON)
option(BUILD_LAUNCHER "build Launcher" ON)
option(BUILD_MWINIIMPORTER "build MWiniImporter" ON)
option(BUILD_ESSIMPORTER "build ESS (Morrowind save game) importer" ON)
option(BUILD_OPENCS "build OpenMW Construction Set" ON)
option(BUILD_WIZARD "build Installation Wizard" ON)
option(BUILD_WITH_CODE_COVERAGE "Enable code coverage with gconv" OFF)
option(BUILD_UNITTESTS "Enable Unittests with Google C++ Unittest" OFF)
option(BUILD_NIFTEST "build nif file tester" OFF)
option(BUILD_MYGUI_PLUGIN "build MyGUI plugin for OpenMW resources, to use with MyGUI tools" ON)
option(BUILD_DOCS        "build documentation." OFF )

if (NOT BUILD_LAUNCHER AND NOT BUILD_OPENCS AND NOT BUILD_WIZARD)
   set(USE_QT FALSE)
else()
   set(USE_QT TRUE)
endif()

if (USE_QT)
    set(DESIRED_QT_VERSION 4 CACHE STRING "The QT version OpenMW should use (4 or 5)")
    set_property(CACHE DESIRED_QT_VERSION PROPERTY STRINGS 4 5)
endif()

# set the minimum required version across the board
cmake_minimum_required(VERSION 3.1.0)

project(OpenMW)

# If the user doesn't supply a CMAKE_BUILD_TYPE via command line, choose one for them.
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS None Debug Release RelWithDebInfo MinSizeRel)
ENDIF()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

# Version
message(STATUS "Configuring OpenMW...")

set(OPENMW_VERSION_MAJOR 0)
set(OPENMW_VERSION_MINOR 45)
set(OPENMW_VERSION_RELEASE 0)

set(OPENMW_VERSION_COMMITHASH "")
set(OPENMW_VERSION_TAGHASH "")

set(OPENMW_VERSION "${OPENMW_VERSION_MAJOR}.${OPENMW_VERSION_MINOR}.${OPENMW_VERSION_RELEASE}")

set(GIT_CHECKOUT FALSE)
if(EXISTS ${PROJECT_SOURCE_DIR}/.git)
    find_package(Git)

    if(GIT_FOUND)
        set(GIT_CHECKOUT TRUE)
    else(GIT_FOUND)
        message(WARNING "Git executable not found")
    endif(GIT_FOUND)
endif(EXISTS ${PROJECT_SOURCE_DIR}/.git)

# Macros
include(OpenMWMacros)

# doxygen main page

configure_file ("${OpenMW_SOURCE_DIR}/docs/mainpage.hpp.cmake" "${OpenMW_BINARY_DIR}/docs/mainpage.hpp")

option(MYGUI_STATIC "Link static build of Mygui into the binaries" FALSE)
option(BOOST_STATIC "Link static build of Boost into the binaries" FALSE)
option(SDL2_STATIC "Link static build of SDL into the binaries" FALSE)
option(OSG_STATIC "Link static build of OpenSceneGraph into the binaries" FALSE)
option(QT_STATIC "Link static build of QT into the binaries" FALSE)

option(OPENMW_UNITY_BUILD "Use fewer compilation units to speed up compile time" FALSE)

# what is necessary to build documentation
IF( BUILD_DOCS )
    # Builds the documentation.
    FIND_PACKAGE( Sphinx REQUIRED )
    FIND_PACKAGE( Doxygen REQUIRED )
ENDIF()

# Set up common paths
SET(BINDIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Where to install binaries")
SET(LIBDIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE PATH "Where to install libraries")
SET(DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share" CACHE PATH "Sets the root of data directories to a non-default location")
SET(GLOBAL_DATA_PATH "${DATAROOTDIR}/games/" CACHE PATH "Set data path prefix")
SET(DATADIR "${GLOBAL_DATA_PATH}/openmw" CACHE PATH "Sets the openmw data directories to a non-default location")
SET(ICONDIR "${DATAROOTDIR}/pixmaps" CACHE PATH "Set icon dir")
SET(LICDIR "${DATAROOTDIR}/licenses/openmw" CACHE PATH "Sets the openmw license directory to a non-default location.")
IF("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
    SET(GLOBAL_CONFIG_PATH "/etc/" CACHE PATH "Set config dir prefix")
ELSE()
    SET(GLOBAL_CONFIG_PATH "${CMAKE_INSTALL_PREFIX}/etc/" CACHE PATH "Set config dir prefix")
ENDIF()
SET(SYSCONFDIR "${GLOBAL_CONFIG_PATH}/openmw" CACHE PATH "Set config dir")

set(MORROWIND_DATA_FILES "${DATADIR}/data" CACHE PATH "location of Morrowind data files")
set(OPENMW_RESOURCE_FILES "${DATADIR}/resources" CACHE PATH "location of OpenMW resources files")

# Dependencies
find_package(OpenGL REQUIRED)

if (USE_QT)
    message(STATUS "Using Qt${DESIRED_QT_VERSION}")

    if (DESIRED_QT_VERSION MATCHES 4)
        find_package(Qt4 REQUIRED COMPONENTS QtCore QtGui QtNetwork QtOpenGL)
    else()
        find_package(Qt5Widgets REQUIRED)
        find_package(Qt5Core REQUIRED)
        find_package(Qt5Network REQUIRED)
        find_package(Qt5OpenGL REQUIRED)
    # Instruct CMake to run moc automatically when needed.
    #set(CMAKE_AUTOMOC ON)
    endif()
endif()

# Sound setup
find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL SWSCALE SWRESAMPLE)
# Required for building the FFmpeg headers
add_definitions(-D__STDC_CONSTANT_MACROS)

# TinyXML
#option(USE_SYSTEM_TINYXML "Use system TinyXML library instead of internal." OFF)
#if (USE_SYSTEM_TINYXML)
#    find_package(TinyXML REQUIRED)
#    add_definitions (-DTIXML_USE_STL)
#    include_directories(SYSTEM ${TinyXML_INCLUDE_DIRS})
#endif()

# Platform specific
if (WIN32)
    if(NOT MINGW)
    set(Boost_USE_STATIC_LIBS   ON)
    add_definitions(-DBOOST_ALL_NO_LIB)
    endif(NOT MINGW)

    # Suppress WinMain(), provided by SDL
    add_definitions(-DSDL_MAIN_HANDLED)

    # Get rid of useless crud from windows.h
    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
endif()

if (NOT WIN32 AND BUILD_WIZARD) # windows users can just run the morrowind installer
    find_package(LIBUNSHIELD REQUIRED) # required only for non win32 when building openmw-wizard
    set(OPENMW_USE_UNSHIELD TRUE)
endif()

# Fix for not visible pthreads functions for linker with glibc 2.15
if (UNIX AND NOT APPLE)
    find_package (Threads)
endif()

# Look for stdint.h
include(CheckIncludeFile)
check_include_file(stdint.h HAVE_STDINT_H)
if(NOT HAVE_STDINT_H)
    unset(HAVE_STDINT_H CACHE)
    message(FATAL_ERROR "stdint.h was not found" )
endif()


find_package(OpenSceneGraph 3.3.4 REQUIRED osgDB osgViewer osgText osgGA osgParticle osgUtil osgFX)
include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS})

set(USED_OSG_PLUGINS
                    osgdb_bmp
                    osgdb_dds
                    osgdb_freetype
                    osgdb_jpeg
                    osgdb_osg
                    osgdb_png
                    osgdb_serializers_osg
                    osgdb_tga
                  )

get_filename_component(OSG_LIB_DIR ${OSGDB_LIBRARY} DIRECTORY)
set(OSGPlugins_LIB_DIR "${OSG_LIB_DIR}/osgPlugins-${OPENSCENEGRAPH_VERSION}")

if(OSG_STATIC)
    add_definitions(-DOSG_LIBRARY_STATIC)

    find_package(OSGPlugins REQUIRED COMPONENTS ${USED_OSG_PLUGINS})
    list(APPEND OPENSCENEGRAPH_LIBRARIES ${OSGPlugins_LIBRARIES})
endif()

if(QT_STATIC)
   if(WIN32)
      if(DESIRED_QT_VERSION MATCHES 4)
         # QtCore needs WSAAsyncSelect from Ws2_32.lib
         set(QT_QTCORE_LIBRARY ${QT_QTCORE_LIBRARY} Ws2_32.lib)
         message("QT_QTCORE_LIBRARY: ${QT_QTCORE_LIBRARY}")
      endif()
   endif()
endif()


set(BOOST_COMPONENTS system filesystem program_options)
if(WIN32)
    set(BOOST_COMPONENTS ${BOOST_COMPONENTS} locale)
endif(WIN32)

IF(BOOST_STATIC)
    set(Boost_USE_STATIC_LIBS   ON)
endif()

set(REQUIRED_BULLET_VERSION 286) # Bullet 286 required due to runtime bugfixes for btCapsuleShape
if (DEFINED ENV{TRAVIS_BRANCH} OR DEFINED ENV{APPVEYOR})
    set(REQUIRED_BULLET_VERSION 283) # but for build testing, 283 is fine
endif()

find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
find_package(MyGUI 3.2.1 REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Bullet ${REQUIRED_BULLET_VERSION} REQUIRED COMPONENTS BulletCollision LinearMath)

include_directories("."
    SYSTEM
    ${SDL2_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${MyGUI_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
    ${Bullet_INCLUDE_DIRS}
)

link_directories(${SDL2_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS})

if(MYGUI_STATIC)
   add_definitions(-DMYGUI_STATIC)
endif (MYGUI_STATIC)

# Set up DEBUG define
set_directory_properties(PROPERTIES COMPILE_DEFINITIONS_DEBUG DEBUG=1)

if (NOT APPLE)
    set(OPENMW_MYGUI_FILES_ROOT ${OpenMW_BINARY_DIR})
    set(OPENMW_SHADERS_ROOT ${OpenMW_BINARY_DIR})
endif ()

add_subdirectory(files/)

# Specify build paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OpenMW_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OpenMW_BINARY_DIR}")

# Other files

configure_resource_file(${OpenMW_SOURCE_DIR}/files/settings-default.cfg
    "${OpenMW_BINARY_DIR}" "settings-default.cfg")

if (NOT APPLE)
    configure_resource_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg.local
        "${OpenMW_BINARY_DIR}" "openmw.cfg")
    configure_resource_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg
        "${OpenMW_BINARY_DIR}" "openmw.cfg.install")
else ()
    configure_file(${OpenMW_SOURCE_DIR}/files/openmw.cfg
        "${OpenMW_BINARY_DIR}/openmw.cfg")
endif ()

configure_resource_file(${OpenMW_SOURCE_DIR}/files/openmw-cs.cfg
    "${OpenMW_BINARY_DIR}" "openmw-cs.cfg")

# Needs the copy version because the configure version assumes the end of the file has been reached when a null character is reached and there are no CMake expressions to evaluate.
copy_resource_file(${OpenMW_SOURCE_DIR}/files/opencs/defaultfilters
    "${OpenMW_BINARY_DIR}" "resources/defaultfilters")

if (NOT WIN32 AND NOT APPLE)
    configure_file(${OpenMW_SOURCE_DIR}/files/openmw.desktop
        "${OpenMW_BINARY_DIR}/openmw.desktop")
    configure_file(${OpenMW_SOURCE_DIR}/files/openmw.appdata.xml
        "${OpenMW_BINARY_DIR}/openmw.appdata.xml")
    configure_file(${OpenMW_SOURCE_DIR}/files/openmw-cs.desktop
        "${OpenMW_BINARY_DIR}/openmw-cs.desktop")
endif()

# CXX Compiler settings
set(CMAKE_CXX_STANDARD 11)
if (CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wundef -Wno-unused-parameter -Wno-deprecated-declarations -std=c++11 -pedantic -Wno-long-long")
    add_definitions( -DBOOST_NO_CXX11_SCOPED_ENUMS=ON )

    if (APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL Clang AND NOT APPLE)
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.6 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 3.6)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-potentially-evaluated-expression")
        endif ()
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.6 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.6)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-parameter")
    endif()
endif (CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)

IF(NOT WIN32 AND NOT APPLE)
    # Linux installation

    # Install binaries
    IF(BUILD_OPENMW)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/openmw" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_OPENMW)
    IF(BUILD_LAUNCHER)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/openmw-launcher" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_LAUNCHER)
    IF(BUILD_BSATOOL)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/bsatool" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_BSATOOL)
    IF(BUILD_ESMTOOL)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/esmtool" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_ESMTOOL)
    IF(BUILD_NIFTEST)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/niftest" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_NIFTEST)
    IF(BUILD_MWINIIMPORTER)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/openmw-iniimporter" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_MWINIIMPORTER)
    IF(BUILD_ESSIMPORTER)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/openmw-essimporter" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_ESSIMPORTER)
    IF(BUILD_OPENCS)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/openmw-cs" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_OPENCS)
    IF(BUILD_WIZARD)
        INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/openmw-wizard" DESTINATION "${BINDIR}" )
    ENDIF(BUILD_WIZARD)
    #if(BUILD_MYGUI_PLUGIN)
    #    INSTALL(PROGRAMS "${OpenMW_BINARY_DIR}/Plugin_MyGUI_OpenMW_Resources.so" DESTINATION "${LIBDIR}" )
    #ENDIF(BUILD_MYGUI_PLUGIN)

    # Install licenses
    INSTALL(FILES "files/mygui/DejaVu Font License.txt" DESTINATION "${LICDIR}" )

    # Install icon and desktop file
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.desktop" DESTINATION "${DATAROOTDIR}/applications" COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_SOURCE_DIR}/files/launcher/images/openmw.png" DESTINATION "${ICONDIR}" COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.appdata.xml" DESTINATION "${DATAROOTDIR}/metainfo" COMPONENT "openmw")
    IF(BUILD_OPENCS)
        INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw-cs.desktop" DESTINATION "${DATAROOTDIR}/applications" COMPONENT "opencs")
        INSTALL(FILES "${OpenMW_SOURCE_DIR}/files/opencs/openmw-cs.png" DESTINATION "${ICONDIR}" COMPONENT "opencs")
    ENDIF(BUILD_OPENCS)

    # Install global configuration files
    INSTALL(FILES "${OpenMW_BINARY_DIR}/settings-default.cfg" DESTINATION "${SYSCONFDIR}" COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw.cfg.install" DESTINATION "${SYSCONFDIR}" RENAME "openmw.cfg" COMPONENT "openmw")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/resources/version" DESTINATION "${SYSCONFDIR}" COMPONENT "openmw")

    IF(BUILD_OPENCS)
        INSTALL(FILES "${OpenMW_BINARY_DIR}/openmw-cs.cfg" DESTINATION "${SYSCONFDIR}" COMPONENT "opencs")
    ENDIF(BUILD_OPENCS)

    # Install resources
    INSTALL(DIRECTORY "${OpenMW_BINARY_DIR}/resources" DESTINATION "${DATADIR}" COMPONENT "Resources")
    INSTALL(DIRECTORY DESTINATION "${DATADIR}/data" COMPONENT "Resources")
ENDIF(NOT WIN32 AND NOT APPLE)

# Extern
add_subdirectory (extern/osg-ffmpeg-videoplayer)
add_subdirectory (extern/xswrapper)

if (BUILD_OPENCS)
    add_subdirectory (extern/osgQt)
endif()

# Components
add_subdirectory (components)

# Apps and tools
if (BUILD_OPENMW)
    add_subdirectory( apps/openmw )
endif()

if (BUILD_BSATOOL)
  add_subdirectory( apps/bsatool )
endif()

if (BUILD_ESMTOOL)
  add_subdirectory( apps/esmtool )
endif()

if (BUILD_LAUNCHER)
   add_subdirectory( apps/launcher )
endif()

if (BUILD_MWINIIMPORTER)
   add_subdirectory( apps/mwiniimporter )
endif()

if (BUILD_ESSIMPORTER)
   add_subdirectory (apps/essimporter )
endif()

if (BUILD_OPENCS)
   add_subdirectory (apps/opencs)
endif()

if (BUILD_WIZARD)
   add_subdirectory(apps/wizard)
endif()

if (BUILD_NIFTEST)
    add_subdirectory(apps/niftest)
endif(BUILD_NIFTEST)

# UnitTests
if (BUILD_UNITTESTS)
  add_subdirectory( apps/openmw_test_suite )
endif()

# Doxygen Target -- simply run 'make doc' or 'make doc_pages'
# output directory for 'make doc'       is "${OpenMW_BINARY_DIR}/docs/Doxygen"
# output directory for 'make doc_pages' is "${DOXYGEN_PAGES_OUTPUT_DIR}" if defined
#                                       or "${OpenMW_BINARY_DIR}/docs/Pages" otherwise
find_package(Doxygen)
if (DOXYGEN_FOUND)
    # determine output directory for doc_pages
    if (NOT DEFINED DOXYGEN_PAGES_OUTPUT_DIR)
        set(DOXYGEN_PAGES_OUTPUT_DIR "${OpenMW_BINARY_DIR}/docs/Pages")
    endif ()
    configure_file(${OpenMW_SOURCE_DIR}/docs/Doxyfile.cmake ${OpenMW_BINARY_DIR}/docs/Doxyfile @ONLY)
    configure_file(${OpenMW_SOURCE_DIR}/docs/DoxyfilePages.cmake ${OpenMW_BINARY_DIR}/docs/DoxyfilePages @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${OpenMW_BINARY_DIR}/docs/Doxyfile
        WORKING_DIRECTORY ${OpenMW_BINARY_DIR}
        COMMENT "Generating Doxygen documentation at ${OpenMW_BINARY_DIR}/docs/Doxygen"
        VERBATIM)
    add_custom_target(doc_pages
        ${DOXYGEN_EXECUTABLE} ${OpenMW_BINARY_DIR}/docs/DoxyfilePages
        WORKING_DIRECTORY ${OpenMW_BINARY_DIR}
        COMMENT "Generating documentation for the github-pages at ${DOXYGEN_PAGES_OUTPUT_DIR}" VERBATIM)
endif ()

