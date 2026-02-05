# ====================================================================
# Vix.cpp - WebRPC | Tests
# ====================================================================

cmake_minimum_required(VERSION 3.16)
project(vix_webrpc_tests LANGUAGES CXX)

add_executable(webrpc_error_serialization
  error_serialization.cpp
)

add_executable(webrpc_router_basic
  router_basic.cpp
)

foreach(target
  webrpc_error_serialization
  webrpc_router_basic
)
  target_compile_features(${target} PRIVATE cxx_std_20)
  target_link_libraries(${target} PRIVATE vix::webrpc)
endforeach()

# Register tests with CTest
add_test(NAME webrpc.error_serialization COMMAND webrpc_error_serialization)
add_test(NAME webrpc.router_basic        COMMAND webrpc_router_basic)
