project(QML)

cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_MACOSX_RPATH 1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_definitions(-DJULIA_ENABLE_THREADING)

find_package(Qt5Quick)
find_package(Qt5Core)
find_package(Qt5Widgets)
find_package(JlCxx)

get_target_property(QtCore_location Qt5::Core LOCATION)
get_filename_component(QtCore_location ${QtCore_location} DIRECTORY)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${JlCxx_DIR}/../;${QtCore_location}")

if(WIN32)
  if(DEFINED ENV{MSYSTEM})
    if($ENV{MSYSTEM} STREQUAL "MINGW32")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=pentium4 -static-libstdc++ -static-libgcc")
    endif()
  endif()
  add_definitions(-DWIN32_LEAN_AND_MEAN)
  get_filename_component(VC_DIR "${CMAKE_LINKER}" DIRECTORY) # Visual studio dir is 3 levels up from the linker
  get_filename_component(VC_DIR "${VC_DIR}" DIRECTORY)
  if(CMAKE_GENERATOR STREQUAL "Visual Studio 14 2015 Win64")
    get_filename_component(VC_DIR "${VC_DIR}" DIRECTORY)
  endif()
  get_filename_component(QML_DIR "${CMAKE_SOURCE_DIR}" DIRECTORY) # Go up to the package test dir for QML examples
  get_filename_component(QML_DIR "${QML_DIR}" DIRECTORY)
  get_filename_component(QML_DIR "${QML_DIR}" DIRECTORY)
  set(QML_DIR "${QML_DIR}/example/qml")
  if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    # set(CMAKE_AUTOSTATICPLUGINS ON) # Static build on MinGW
    set(LIB_PREFIX_STR "lib")
    # set(static_qt5_extralibs
    #   Qt5::QLocalClientConnectionFactory
    #   Qt5::QQmlDebugServerFactory
    #   Qt5::QQmlDebuggerServiceFactory
    #   Qt5::QQmlInspectorServiceFactory
    #   Qt5::QQmlNativeDebugConnectorFactory
    #   Qt5::QQmlProfilerServiceFactory
    #   Qt5::QTcpServerConnectionFactory
    #   "$ENV{QT_ROOT}/share/qt5/qml/QtQuick.2/libqtquick2plugin.a"
    # )
  endif()
  configure_file("${CMAKE_SOURCE_DIR}/cmake/WinDeployQt.cmake" "${CMAKE_BINARY_DIR}/WinDeployQt.cmake")
  qt5_add_resources(RESOURCES ${CMAKE_SOURCE_DIR}/resources/resources.qrc)
endif(WIN32)

add_library(qmlwrap SHARED
  application_manager.hpp
  application_manager.cpp
  glvisualize_viewport.hpp
  glvisualize_viewport.cpp
  julia_api.hpp
  julia_api.cpp
  julia_display.hpp
  julia_display.cpp
  julia_object.hpp
  julia_object.cpp
  julia_painteditem.hpp
  julia_painteditem.cpp
  julia_signals.hpp
  julia_signals.cpp
  listmodel.hpp
  listmodel.cpp
  opengl_viewport.hpp
  opengl_viewport.cpp
  type_conversion.hpp
  type_conversion.cpp
  wrap_qml.cpp
${MOC_BUILT_SOURCES} ${UI_BUILT_SOURCES} ${RESOURCES})

target_link_libraries(qmlwrap Qt5::Core Qt5::Quick Qt5::Widgets JlCxx::jlcxx)

install(TARGETS
  qmlwrap
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib
RUNTIME DESTINATION lib)
if(WIN32)
  install (SCRIPT "${CMAKE_BINARY_DIR}/WinDeployQt.cmake")
endif(WIN32)
