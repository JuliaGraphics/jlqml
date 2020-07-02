project(QML)

cmake_minimum_required(VERSION 3.1)
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Quick)
find_package(Qt5Core)
find_package(Qt5Svg)
find_package(Qt5Widgets)
find_package(JlCxx)

get_target_property(QtCore_location Qt5::Core LOCATION)
get_filename_component(QtCore_location ${QtCore_location} DIRECTORY)
get_target_property(JlCxx_location JlCxx::cxxwrap_julia LOCATION)
get_filename_component(JlCxx_location ${JlCxx_location} DIRECTORY)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${JlCxx_location};${QtCore_location}")

message(STATUS "Found JlCxx at ${JlCxx_location}")

if(WIN32)
	set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
	set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/bin")
  add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

add_library(jlqml SHARED
  application_manager.hpp
  application_manager.cpp
  julia_api.hpp
  julia_api.cpp
  julia_canvas.hpp
  julia_canvas.cpp
  julia_display.hpp
  julia_display.cpp
  julia_function.hpp
  julia_function.cpp
  julia_painteditem.hpp
  julia_painteditem.cpp
  julia_property_map.hpp
  julia_property_map.cpp
  julia_signals.hpp
  julia_signals.cpp
  listmodel.hpp
  listmodel.cpp
  makie_viewport.hpp
  makie_viewport.cpp
  opengl_viewport.hpp
  opengl_viewport.cpp
  jlqml.hpp
  wrap_qml.cpp
${MOC_BUILT_SOURCES} ${UI_BUILT_SOURCES} ${RESOURCES})

target_link_libraries(jlqml Qt5::Core Qt5::Quick Qt5::Svg Qt5::Widgets JlCxx::cxxwrap_julia JlCxx::cxxwrap_julia_stl)

install(TARGETS
  jlqml
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib
RUNTIME DESTINATION bin)
