# BSD 2-Clause License

#
# Copyright (c) 2015, 2018, 2020 CNRS Authors: Joseph Mirabel, Guilhem Saurel
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:

# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.

# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME "hpp-plot")
set(PROJECT_DESCRIPTION "Plotting tools for HPP")

set(PROJECT_USE_CMAKE_EXPORT TRUE)
set(PROJECT_USE_KEYWORD_LINK_LIBRARIES TRUE)
set(CXX_DISABLE_WERROR TRUE)

# Check if the submodule cmake have been initialized
set(JRL_CMAKE_MODULES "${CMAKE_CURRENT_LIST_DIR}/cmake")
if(EXISTS "${JRL_CMAKE_MODULES}/base.cmake")
  message(STATUS "JRL cmakemodules found in 'cmake/' git submodule")
else()
  find_package(jrl-cmakemodules QUIET CONFIG)
  if(jrl-cmakemodules_FOUND)
    get_property(
      JRL_CMAKE_MODULES
      TARGET jrl-cmakemodules::jrl-cmakemodules
      PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    message(STATUS "JRL cmakemodules found on system at ${JRL_CMAKE_MODULES}")
  else()
    message(STATUS "JRL cmakemodules not found. Let's fetch it.")
    include(FetchContent)
    FetchContent_Declare(
      "jrl-cmakemodules"
      GIT_REPOSITORY "https://github.com/jrl-umi3218/jrl-cmakemodules.git")
    FetchContent_MakeAvailable("jrl-cmakemodules")
    FetchContent_GetProperties("jrl-cmakemodules" SOURCE_DIR JRL_CMAKE_MODULES)
  endif()
endif()

include("${JRL_CMAKE_MODULES}/hpp.cmake")

# Tells pkg-config to read qtversion from pkg config file.
list(APPEND PKG_CONFIG_ADDITIONAL_VARIABLES qtversion cmake_plugin)

compute_project_args(PROJECT_ARGS LANGUAGES CXX)
project(${PROJECT_NAME} ${PROJECT_ARGS})

option(HPP_MANIPULATION_CORBA_CLIENT_ONLY
       "Disable functionnalities requiring the corba servers of HPP" OFF)

add_project_dependency("gepetto-viewer")
add_project_dependency("gepetto-viewer-corba")

if(gepetto-viewer-corba_FOUND)
  # Get desired Qt version
  string(REPLACE "." ";" DESIRED_QT_VERSION_LIST
                 ${GEPETTO_VIEWER_CORBA_QTVERSION})
  list(GET DESIRED_QT_VERSION_LIST 0 DESIRED_QT_VERSION_MAJOR)

  if(${DESIRED_QT_VERSION_MAJOR} EQUAL 4)
    set(USE_QT4 True)
  elseif(${DESIRED_QT_VERSION_MAJOR} EQUAL 5)
    set(USE_QT4 False)
  else()
    message(FATAL_ERROR "This package is only compatible with Qt 4 and Qt 5")
  endif()
else(gepetto-viewer-corba_FOUND)
  set(USE_QT4
      FALSE
      CACHE BOOL "Use Qt4 instead of Qt5")
endif(gepetto-viewer-corba_FOUND)

message(STATUS "Looking for Qt ${DESIRED_QT_VERSION_MAJOR}.")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(USE_QT4)
  find_package(Qt4 REQUIRED QtCore QtGui)
  set(PKG_CONFIG_EXTRA
      "qtversion: ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")
else(USE_QT4)
  find_package(
    Qt5 REQUIRED
    COMPONENTS Core
               Widgets
               Gui
               PrintSupport
               Concurrent
               OpenGL
               Network
               Xml)
  foreach(
    component
    "Core"
    "Widgets"
    "Gui"
    "PrintSupport"
    "Concurrent"
    OpenGL
    Network
    Xml)
    list(APPEND QT_INCLUDE_DIRS ${Qt5${component}_INCLUDE_DIRS})
    list(APPEND QT_LIBRARIES ${Qt5${component}_LIBRARIES})
  endforeach()
  set(PKG_CONFIG_EXTRA "qtversion=${Qt5Core_VERSION}")
endif(USE_QT4)

add_project_dependency("hpp-manipulation-corba" REQUIRED)
add_project_dependency("qgv" REQUIRED)

set(${PROJECT_NAME}_HEADERS include/hpp/plot/graph-widget.hh
                            include/hpp/plot/hpp-manipulation-graph.hh)
set(${PROJECT_NAME}_HEADERS_NOMOC)

set(${PROJECT_NAME}_FORMS)

set(${PROJECT_NAME}_RESOURCES)

if(USE_QT4)
  qt4_wrap_cpp(${PROJECT_NAME}_HEADERS_MOC ${${PROJECT_NAME}_HEADERS})
  qt4_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${${PROJECT_NAME}_FORMS})
  qt4_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${${PROJECT_NAME}_RESOURCES})
  include(${QT_USE_FILE})
else(USE_QT4)
  qt5_wrap_cpp(${PROJECT_NAME}_HEADERS_MOC ${${PROJECT_NAME}_HEADERS})
  qt5_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${${PROJECT_NAME}_FORMS})
  qt5_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${${PROJECT_NAME}_RESOURCES})
endif(USE_QT4)
add_definitions(${QT_DEFINITIONS})

set(${PROJECT_NAME}_SOURCES src/graph-widget.cc src/hpp-manipulation-graph.cc)

add_library(
  ${PROJECT_NAME} SHARED
  ${${PROJECT_NAME}_SOURCES} ${${PROJECT_NAME}_HEADERS_NOMOC}
  ${${PROJECT_NAME}_HEADERS_MOC} ${${PROJECT_NAME}_FORMS_HEADERS}
  ${${PROJECT_NAME}_RESOURCES_RCC})

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC ${QT_LIBRARIES} hpp-manipulation-corba::hpp-manipulation-corba
         qgv::qgvcore)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${TARGETS_EXPORT_NAME}
  DESTINATION lib)

add_subdirectory(bin)
add_subdirectory(plugins)
