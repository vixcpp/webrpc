#include <cassert>
#include <iostream>

#include <vix/webrpc/Error.hpp>
#include <vix/json/Simple.hpp>

using namespace vix::webrpc;
using namespace vix::json;

static void test_basic_error_serialization()
{
  RpcError err{
      "METHOD_NOT_FOUND",
      "RPC method not found",
      obj({
          "method",
          "user.get",
      }),
  };

  token t = err.to_json();

  auto objp = t.as_object_ptr();
  assert(objp && "error must serialize to object");

  const kvs &o = *objp;

  assert(o.get_string_or("code", "") == "METHOD_NOT_FOUND");
  assert(o.get_string_or("message", "") == "RPC method not found");

  const token *details = o.get_ptr("details");
  assert(details && "details must exist");

  auto dp = details->as_object_ptr();
  assert(dp && "details must be an object");

  assert(dp->get_string_or("method", "") == "user.get");
}

static void test_error_without_details()
{
  RpcError err{
      "INVALID_PARAMS",
      "Invalid RPC parameters",
  };

  token t = err.to_json();

  auto objp = t.as_object_ptr();
  assert(objp);

  const kvs &o = *objp;

  assert(o.get_string_or("code", "") == "INVALID_PARAMS");
  assert(o.get_string_or("message", "") == "Invalid RPC parameters");
  assert(o.get_ptr("details") == nullptr && "details must be absent");
}

static void test_error_parse_roundtrip()
{
  RpcError original{
      "PARSE_ERROR",
      "Failed to parse RPC payload",
      obj({
          "reason",
          "invalid json",
      }),
  };

  token serialized = original.to_json();

  auto parsed = RpcError::parse(serialized);
  assert(parsed.ok() && "RpcError::parse should succeed");

  const RpcError &err = parsed.value();

  assert(err.code == "PARSE_ERROR");
  assert(err.message == "Failed to parse RPC payload");
  assert(err.has_details());

  auto dp = err.details.as_object_ptr();
  assert(dp);
  assert(dp->get_string_or("reason", "") == "invalid json");
}

int main()
{
  test_basic_error_serialization();
  test_error_without_details();
  test_error_parse_roundtrip();

  std::cout << "[webrpc] error_serialization OK\n";
  return 0;
}
