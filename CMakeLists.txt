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
#  Vix.cpp

cmake_minimum_required(VERSION 3.20)
project(vix_webrpc VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ======================================================
# Dependency policy
# ======================================================

option(VIX_WEBRPC_FETCH_JSON "Auto-fetch vix::json if missing" ON)

if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
  set(VIX_WEBRPC_FETCH_JSON OFF CACHE BOOL "Auto-fetch vix::json if missing" FORCE)
endif()

if (DEFINED VIX_UMBRELLA_BUILD AND VIX_UMBRELLA_BUILD)
  if (NOT TARGET vix::json)
    message(FATAL_ERROR "[webrpc] Umbrella build: vix::json must be provided before webrpc.")
  endif()
else()
  if (NOT TARGET vix::json)
    if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../json/CMakeLists.txt")
      message(STATUS "[webrpc] Adding json from sibling module: ../json")
      add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../json" "json")
    elseif (VIX_WEBRPC_FETCH_JSON)
      include(FetchContent)
      message(STATUS "[webrpc] Fetching vix::json via FetchContent")
      FetchContent_Declare(vix_json
        GIT_REPOSITORY https://github.com/vixcpp/json.git
        GIT_TAG        dev
      )
      FetchContent_MakeAvailable(vix_json)
    else()
      message(FATAL_ERROR
        "vix::json not found.\n"
        "Enable VIX_WEBRPC_FETCH_JSON=ON or provide the target before webrpc."
      )
    endif()
  endif()
endif()

if (NOT TARGET vix::json)
  message(FATAL_ERROR "[webrpc] Required dependency target vix::json is still missing.")
endif()

# ======================================================
# Target (header-only)
# ======================================================

add_library(vix_webrpc INTERFACE)
add_library(vix::webrpc ALIAS vix_webrpc)

target_compile_features(vix_webrpc INTERFACE cxx_std_20)

target_include_directories(vix_webrpc INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(vix_webrpc INTERFACE
  vix::json
)

# Optional umbrella helpers (if the parent project provides them)
if (TARGET vix_warnings)
  target_link_libraries(vix_webrpc INTERFACE vix_warnings)
endif()

if (VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
  target_link_libraries(vix_webrpc INTERFACE vix_sanitizers)
endif()

# Install target + headers
install(TARGETS vix_webrpc
  EXPORT VixTargets
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# ======================================================
# Tests
# ======================================================

option(VIX_WEBRPC_BUILD_TESTS "Build webrpc module tests" OFF)

if (VIX_WEBRPC_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

# ======================================================
# Examples
# ======================================================

option(VIX_WEBRPC_BUILD_EXAMPLES "Build webrpc module examples" OFF)

if (VIX_WEBRPC_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# ======================================================
# Summary
# ======================================================

message(STATUS "------------------------------------------------------")
message(STATUS "vix::webrpc configured (${PROJECT_VERSION})")
message(STATUS "Mode:                 HEADER-ONLY")
message(STATUS "JSON target:          vix::json")
message(STATUS "Include dir:           ${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "Build type:            ${CMAKE_BUILD_TYPE}")
message(STATUS "Build tests:           ${VIX_WEBRPC_BUILD_TESTS}")
message(STATUS "Build examples:        ${VIX_WEBRPC_BUILD_EXAMPLES}")
message(STATUS "Fetch json:            ${VIX_WEBRPC_FETCH_JSON}")
message(STATUS "------------------------------------------------------")
