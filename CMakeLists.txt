#  @file CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2026, Gaspard Kirira.
#  All rights reserved.
#  https://github.com/vixcpp/webrpc
#
#  Use of this source code is governed by a MIT license
#  that can be found in the LICENSE file.
#
# ====================================================================
# Vix.cpp - WebRPC Module
# ====================================================================
# Purpose:
#   Minimal transport-agnostic WebRPC layer for Vix:
#   - RPC envelope (request/response/error) over vix::json::Simple (header-only)
#   - Method router + dispatcher
#
# Public Targets:
#   - vix_webrpc   : The actual library target (STATIC or INTERFACE)
#   - vix::webrpc  : Namespaced alias for consumers
#
# Dependencies:
#   - None required at link time for the core (Simple.hpp is header-only)
#
# Options:
#   - VIX_WEBRPC_BUILD_TESTS    : Build module tests (default OFF)
#   - VIX_WEBRPC_BUILD_EXAMPLES : Build module examples (default OFF)
#
# Notes:
#   - WebRPC is deliberately independent from vix::core and vix::websocket.
#     Bridges (HTTP/WS) can live in a separate submodule later.
# ====================================================================

cmake_minimum_required(VERSION 3.20)
project(vix_webrpc VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------
# Sources discovery
# ------------------------------------------------------
file(GLOB_RECURSE WEBRPC_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# ------------------------------------------------------
# Target
# ------------------------------------------------------
if (WEBRPC_SOURCES)
  message(STATUS "[webrpc] Building STATIC library with detected sources.")

  add_library(vix_webrpc STATIC ${WEBRPC_SOURCES})
  add_library(vix::webrpc ALIAS vix_webrpc)

  target_compile_features(vix_webrpc PUBLIC cxx_std_20)

  target_include_directories(vix_webrpc
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  # Optional umbrella helpers if present
  if (TARGET vix_warnings)
    target_link_libraries(vix_webrpc PUBLIC vix_warnings)
  endif()

  if (VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
    target_link_libraries(vix_webrpc PUBLIC vix_sanitizers)
  endif()

  set_target_properties(vix_webrpc PROPERTIES
    OUTPUT_NAME vix_webrpc
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    EXPORT_NAME webrpc
  )

  install(TARGETS vix_webrpc
    EXPORT VixTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

else()
  message(STATUS "[webrpc] Building HEADER-ONLY library (no sources).")

  add_library(vix_webrpc INTERFACE)
  add_library(vix::webrpc ALIAS vix_webrpc)

  target_compile_features(vix_webrpc INTERFACE cxx_std_20)

  target_include_directories(vix_webrpc INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  # Optional umbrella helpers if present
  if (TARGET vix_warnings)
    target_link_libraries(vix_webrpc INTERFACE vix_warnings)
  endif()

  if (VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
    target_link_libraries(vix_webrpc INTERFACE vix_sanitizers)
  endif()

  install(TARGETS vix_webrpc
    EXPORT VixTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()

# Install headers
install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# ------------------------------------------------------
# Tests
# ------------------------------------------------------
option(VIX_WEBRPC_BUILD_TESTS "Build webrpc module tests" OFF)

if (VIX_WEBRPC_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

# ------------------------------------------------------
# Examples
# ------------------------------------------------------
option(VIX_WEBRPC_BUILD_EXAMPLES "Build webrpc module examples" OFF)

if (VIX_WEBRPC_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# ------------------------------------------------------
# Summary
# ------------------------------------------------------
message(STATUS "------------------------------------------------------")
message(STATUS "vix::webrpc configured (${PROJECT_VERSION})")
if (WEBRPC_SOURCES)
  message(STATUS "Mode: STATIC / sources found")
else()
  message(STATUS "Mode: HEADER-ONLY / no sources")
endif()
message(STATUS "Include dir:          ${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "Build tests:          ${VIX_WEBRPC_BUILD_TESTS}")
message(STATUS "Build examples:       ${VIX_WEBRPC_BUILD_EXAMPLES}")
message(STATUS "------------------------------------------------------")
