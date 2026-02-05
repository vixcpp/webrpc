#include <cassert>
#include <iostream>

#include <vix/webrpc/Router.hpp>
#include <vix/webrpc/Request.hpp>
#include <vix/webrpc/Error.hpp>
#include <vix/json/Simple.hpp>

using namespace vix::webrpc;
using namespace vix::json;

static void test_router_dispatch_ok()
{
  Router r;

  r.add("math.add", [](const Context &ctx) -> RpcResult
        {
          const auto *p = ctx.params_object_ptr();
          if (!p)
            return RpcError::invalid_params("params must be an object");

          const token *a_t = p->get_ptr("a");
          const token *b_t = p->get_ptr("b");

          if (!a_t || !b_t || !a_t->is_i64() || !b_t->is_i64())
            return RpcError::invalid_params("a and b must be int");

          const long long a = static_cast<long long>(a_t->as_i64_or(0));
          const long long b = static_cast<long long>(b_t->as_i64_or(0));

          return obj({
              "sum", a + b,
          }); });

  RpcRequest req{
      token(42LL),
      "math.add",
      obj({
          "a",
          7LL,
          "b",
          5LL,
      }),
  };

  RpcResult out = r.dispatch(req, "test");

  assert(std::holds_alternative<token>(out));
  const token &res = std::get<token>(out);

  auto op = res.as_object_ptr();
  assert(op);

  assert(op->get_i64_or("sum", -1) == 12);
}

static void test_router_method_not_found()
{
  Router r;

  RpcRequest req{
      token("id1"),
      "missing.method",
      token(nullptr),
  };

  RpcResult out = r.dispatch(req, "test");

  assert(std::holds_alternative<RpcError>(out));
  const RpcError &err = std::get<RpcError>(out);

  assert(err.code == "METHOD_NOT_FOUND");
  assert(err.message == "RPC method not found");
  assert(err.has_details());

  auto dp = err.details.as_object_ptr();
  assert(dp);
  assert(dp->get_string_or("method", "") == "missing.method");
}

static void test_router_invalid_request()
{
  Router r;

  RpcRequest req;
  req.method = ""; // invalid

  RpcResult out = r.dispatch(req, "test");

  assert(std::holds_alternative<RpcError>(out));
  const RpcError &err = std::get<RpcError>(out);

  assert(err.code == "INVALID_PARAMS");
}

static void test_router_dispatch_from_raw_token()
{
  Router r;

  r.add("echo", [](const Context &ctx) -> RpcResult
        { return ctx.params; });

  token raw = obj({
      "id",
      1LL,
      "method",
      "echo",
      "params",
      obj({
          "ok",
          true,
      }),
  });

  RpcResult out = r.dispatch(raw, "test");

  assert(std::holds_alternative<token>(out));
  const token &res = std::get<token>(out);

  auto op = res.as_object_ptr();
  assert(op);
  assert(op->get_bool_or("ok", false) == true);
}

int main()
{
  test_router_dispatch_ok();
  test_router_method_not_found();
  test_router_invalid_request();
  test_router_dispatch_from_raw_token();

  std::cout << "[webrpc] router_basic OK\n";
  return 0;
}
