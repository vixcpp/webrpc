# ====================================================================
# Vix.cpp - WebRPC | Examples
# ====================================================================

cmake_minimum_required(VERSION 3.16)

add_executable(webrpc_example_basic_request
  01_basic_request.cpp
)

add_executable(webrpc_example_router_dispatch
  02_router_dispatch.cpp
)

add_executable(webrpc_example_notification
  03_notification.cpp
)

add_executable(webrpc_example_batch_requests
  04_batch_requests.cpp
)

# Runner that executes all examples
add_executable(webrpc_examples
  run_all.cpp
)

# ------------------------------------------------------
# Common compile & link settings
# ------------------------------------------------------

foreach(target
  webrpc_example_basic_request
  webrpc_example_router_dispatch
  webrpc_example_notification
  webrpc_example_batch_requests
  webrpc_examples
)
  target_compile_features(${target} PRIVATE cxx_std_20)
  target_link_libraries(${target} PRIVATE vix::webrpc)
endforeach()

# ------------------------------------------------------
# Register runner as a CTest test (optional but recommended)
# ------------------------------------------------------

add_test(NAME webrpc.examples COMMAND webrpc_examples)
