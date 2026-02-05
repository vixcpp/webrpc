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
#   Minimal WebRPC layer for Vix:
#   - Typed-ish RPC envelope (request/response/error) over JSON
#   - Method router + dispatcher
#   - Bridges to vix::core HTTP (and optionally vix::websocket later)
#
# Public Targets:
#   - vix_webrpc   : The actual library target (STATIC or INTERFACE)
#   - vix::webrpc  : Namespaced alias for consumers
#
# Public Dependencies:
#   - vix::core
#   - vix::utils
#   - optional JSON backend (vix::json / vix_json)
#
# Options:
#   - VIX_WEBRPC_WITH_JSON   : AUTO|ON|OFF (default: AUTO)
#   - VIX_WEBRPC_WITH_WS     : Enable websocket bridge (default OFF)
#   - VIX_WEBRPC_FETCH_CORE  : Auto-fetch vix::core if missing (default ON)
#   - VIX_WEBRPC_FETCH_UTILS : Auto-fetch vix::utils if missing (default ON)
#   - VIX_WEBRPC_FETCH_WS    : Auto-fetch vix::websocket if missing (default ON)
#
# Installation/Export:
#   - Contributes to the umbrella export-set `VixTargets`
# ====================================================================

cmake_minimum_required(VERSION 3.20)
project(vix_webrpc VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Sources discovery
file(GLOB_RECURSE WEBRPC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Options
option(VIX_WEBRPC_FETCH_CORE  "Auto-fetch vix::core if missing" ON)
option(VIX_WEBRPC_FETCH_UTILS "Auto-fetch vix::utils if missing" ON)
option(VIX_WEBRPC_WITH_WS     "Enable websocket bridge (optional)" OFF)
option(VIX_WEBRPC_FETCH_WS    "Auto-fetch vix::websocket if missing (when WITH_WS=ON)" ON)

# ----------------------------- deps ----------------------------------

# utils
if (NOT TARGET vix::utils)
  if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../utils/CMakeLists.txt")
    message(STATUS "[webrpc] Adding utils from umbrella: ../utils")
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../utils" "utils")
  elseif (VIX_WEBRPC_FETCH_UTILS)
    include(FetchContent)
    message(STATUS "[webrpc] Fetching vix::utils via FetchContent")
    FetchContent_Declare(vix_utils
      GIT_REPOSITORY https://github.com/vixcpp/utils.git
      GIT_TAG        dev
    )
    FetchContent_MakeAvailable(vix_utils)
  else()
    message(FATAL_ERROR
      "vix::utils not found. Provide vix::utils before webrpc "
      "or enable VIX_WEBRPC_FETCH_UTILS=ON.")
  endif()
endif()

# core
if (NOT TARGET vix::core)
  if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../core/CMakeLists.txt")
    message(STATUS "[webrpc] Adding core from umbrella: ../core")
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../core" "core")
  elseif (VIX_WEBRPC_FETCH_CORE)
    include(FetchContent)
    message(STATUS "[webrpc] Fetching vix::core via FetchContent")
    FetchContent_Declare(vix_core
      GIT_REPOSITORY https://github.com/vixcpp/core.git
      GIT_TAG        dev
    )
    FetchContent_MakeAvailable(vix_core)
  else()
    message(FATAL_ERROR
      "vix::core not found. Provide vix::core before webrpc "
      "or enable VIX_WEBRPC_FETCH_CORE=ON.")
  endif()
endif()

set(VIX_CORE_TARGET  vix::core)
set(VIX_UTILS_TARGET vix::utils)

# optional websocket dependency (later)
if (VIX_WEBRPC_WITH_WS)
  if (NOT TARGET vix::websocket)
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../websocket/CMakeLists.txt")
      message(STATUS "[webrpc] Adding websocket from umbrella: ../websocket")
      add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../websocket" "websocket")
    elseif (VIX_WEBRPC_FETCH_WS)
      include(FetchContent)
      message(STATUS "[webrpc] Fetching vix::websocket via FetchContent")
      FetchContent_Declare(vix_websocket
        GIT_REPOSITORY https://github.com/vixcpp/websocket.git
        GIT_TAG        dev
      )
      FetchContent_MakeAvailable(vix_websocket)
    else()
      message(FATAL_ERROR
        "VIX_WEBRPC_WITH_WS=ON but vix::websocket not found. "
        "Provide vix::websocket or enable VIX_WEBRPC_FETCH_WS=ON.")
    endif()
  endif()
endif()

# JSON linkage policy
set(VIX_WEBRPC_WITH_JSON "AUTO" CACHE STRING "Link JSON backend (AUTO|ON|OFF)")
set_property(CACHE VIX_WEBRPC_WITH_JSON PROPERTY STRINGS AUTO ON OFF)

function(vix_webrpc_try_link_json onto link_scope)
  if (VIX_WEBRPC_WITH_JSON STREQUAL "OFF")
    message(STATUS "[webrpc] JSON disabled (OFF)")
    target_compile_definitions(${onto} ${link_scope} VIX_WEBRPC_NO_JSON=1)
    return()
  endif()

  set(_json_candidates vix::json vix_json)
  set(_json_found "")
  foreach(t IN LISTS _json_candidates)
    if (TARGET ${t})
      set(_json_found ${t})
      break()
    endif()
  endforeach()

  if (_json_found)
    message(STATUS "[webrpc] JSON backend: ${_json_found}")
    target_link_libraries(${onto} ${link_scope} ${_json_found})
  else()
    if (VIX_WEBRPC_WITH_JSON STREQUAL "ON")
      message(FATAL_ERROR "JSON required but not found (expected: vix::json or vix_json).")
    else()
      message(STATUS "[webrpc] No JSON backend detected; building without JSON.")
      target_compile_definitions(${onto} ${link_scope} VIX_WEBRPC_NO_JSON=1)
    endif()
  endif()
endfunction()

# ----------------------------- target --------------------------------

if (WEBRPC_SOURCES)
  message(STATUS "[webrpc] Building STATIC library with detected sources.")

  add_library(vix_webrpc STATIC ${WEBRPC_SOURCES})
  add_library(vix::webrpc ALIAS vix_webrpc)
  target_compile_features(vix_webrpc PUBLIC cxx_std_20)

  if (TARGET vix_warnings)
    target_link_libraries(vix_webrpc PUBLIC vix_warnings)
  endif()

  if (VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
    target_link_libraries(vix_webrpc PUBLIC vix_sanitizers)
  endif()

  target_include_directories(vix_webrpc
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_link_libraries(vix_webrpc
    PUBLIC
      ${VIX_CORE_TARGET}
      ${VIX_UTILS_TARGET}
  )

  if (VIX_WEBRPC_WITH_WS)
    target_link_libraries(vix_webrpc PUBLIC vix::websocket)
    target_compile_definitions(vix_webrpc PUBLIC VIX_WEBRPC_WITH_WS=1)
  else()
    target_compile_definitions(vix_webrpc PUBLIC VIX_WEBRPC_WITH_WS=0)
  endif()

  vix_webrpc_try_link_json(vix_webrpc PUBLIC)

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
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

else()
  message(STATUS "[webrpc] Building HEADER-ONLY library (no sources).")

  add_library(vix_webrpc INTERFACE)
  add_library(vix::webrpc ALIAS vix_webrpc)
  target_compile_features(vix_webrpc INTERFACE cxx_std_20)

  if (TARGET vix_warnings)
    target_link_libraries(vix_webrpc INTERFACE vix_warnings)
  endif()

  if (VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
    target_link_libraries(vix_webrpc INTERFACE vix_sanitizers)
  endif()

  target_include_directories(vix_webrpc INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_link_libraries(vix_webrpc INTERFACE
    ${VIX_CORE_TARGET}
    ${VIX_UTILS_TARGET}
  )

  if (VIX_WEBRPC_WITH_WS)
    target_link_libraries(vix_webrpc INTERFACE vix::websocket)
    target_compile_definitions(vix_webrpc INTERFACE VIX_WEBRPC_WITH_WS=1)
  else()
    target_compile_definitions(vix_webrpc INTERFACE VIX_WEBRPC_WITH_WS=0)
  endif()

  vix_webrpc_try_link_json(vix_webrpc INTERFACE)

  install(TARGETS vix_webrpc
    EXPORT VixTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
endif()

# ----------------------------- tests ---------------------------------
option(VIX_WEBRPC_BUILD_TESTS "Build webrpc module tests" OFF)

if (VIX_WEBRPC_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

# ----------------------------- summary -------------------------------
message(STATUS "------------------------------------------------------")
message(STATUS "vix::webrpc configured (${PROJECT_VERSION})")
if (WEBRPC_SOURCES)
  message(STATUS "Mode: STATIC / sources found")
else()
  message(STATUS "Mode: HEADER-ONLY / no sources")
endif()
message(STATUS "Core target:          ${VIX_CORE_TARGET}")
message(STATUS "Utils target:         ${VIX_UTILS_TARGET}")
message(STATUS "JSON policy:          ${VIX_WEBRPC_WITH_JSON}")
message(STATUS "WebSocket bridge:     ${VIX_WEBRPC_WITH_WS}")
message(STATUS "Include dir:          ${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "Fetch core enabled:   ${VIX_WEBRPC_FETCH_CORE}")
message(STATUS "Fetch utils enabled:  ${VIX_WEBRPC_FETCH_UTILS}")
message(STATUS "Fetch ws enabled:     ${VIX_WEBRPC_FETCH_WS}")
message(STATUS "------------------------------------------------------")
