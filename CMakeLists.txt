project(JlQML)

cmake_minimum_required(VERSION 3.16.0)

set(JlQML_VERSION 0.4.0)
message(STATUS "Project version: v${JlQML_VERSION}")

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Core Gui OpenGL Qml Quick Svg REQUIRED)
find_package(JlCxx)

get_target_property(QtCore_location Qt6::Core LOCATION)
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

add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050F00)

qt6_add_qml_module(jlqml
  URI org.julialang.Julia
  VERSION 1.0
  NO_GENERATE_PLUGIN_SOURCE
  SOURCES
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
    julia_itemmodel.hpp
    julia_itemmodel.cpp
    julia_painteditem.hpp
    julia_painteditem.cpp
    julia_property_map.hpp
    julia_property_map.cpp
    julia_signals.hpp
    julia_signals.cpp
    makie_viewport.hpp
    makie_viewport.cpp
    opengl_viewport.hpp
    opengl_viewport.cpp
    jlqml.hpp
    wrap_qml.cpp)

set_property(TARGET jlqml PROPERTY VERSION ${JlQML_VERSION})
target_link_libraries(jlqml PRIVATE Qt6::Core Qt6::Gui Qt6::OpenGL Qt6::Qml Qt6::Quick Qt6::Svg JlCxx::cxxwrap_julia JlCxx::cxxwrap_julia_stl)

install(TARGETS
  jlqml
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib
RUNTIME DESTINATION bin)
