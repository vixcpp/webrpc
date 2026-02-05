# ====================================================================
# Vix.cpp - WebRPC | Tests
# ====================================================================

cmake_minimum_required(VERSION 3.16)

project(vix_webrpc_tests LANGUAGES CXX)

# --------------------------------------------------------------------
# Test executables
# --------------------------------------------------------------------

add_executable(webrpc_error_serialization
  error_serialization.cpp
)

add_executable(webrpc_router_basic
  router_basic.cpp
)

# --------------------------------------------------------------------
# Common compile settings
# --------------------------------------------------------------------

foreach(target
  webrpc_error_serialization
  webrpc_router_basic
)
  target_compile_features(${target} PRIVATE cxx_std_20)

  target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
  )

  # Header-only dependencies used by WebRPC
  target_link_libraries(${target} PRIVATE
    vix_json
  )
endforeach()
