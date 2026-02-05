# Vix.cpp — WebRPC

**WebRPC** is a minimal, transport-agnostic C++ RPC layer designed for explicit, predictable systems.

It provides:
- a clear **RPC envelope** (request / response / error),
- a simple **method router**,
- a **dispatcher** supporting single calls, notifications, and batch requests,
- built on top of `vix::json::Simple` (header-only, no text parsing).

No network assumptions.
No exceptions.
No magic.

---

## ✨ Goals

WebRPC is designed to:
- be **explicit and predictable**,
- integrate easily with any transport (HTTP, WebSocket, P2P, CLI, etc.),
- remain **independent from vix::core** and networking layers,
- act as a **foundational RPC building block** in the Vix ecosystem.

---

## 📦 Features

- RPC request / response / error model
- Explicit `RpcError`
- Notifications (fire-and-forget)
- Batch requests
- Synchronous router
- Transport-agnostic dispatcher
- Zero runtime dependencies

---

## 🧱 RPC Model

### Request

```json
{
  "id": 1,
  "method": "math.add",
  "params": { "a": 7, "b": 5 }
}
```

### Success Response

```json
{
  "id": 1,
  "result": { "sum": 12 }
}
```

### Error Response

```json
{
  "id": 1,
  "error": {
    "code": "INVALID_PARAMS",
    "message": "a and b must be int",
    "details": {}
  }
}
```

---

## 🚀 Minimal Example

```cpp
#include <vix/webrpc/Router.hpp>
#include <vix/json/Simple.hpp>

using namespace vix::webrpc;
using namespace vix::json;

int main()
{
  Router router;

  router.add("math.add", [](const Context& ctx) -> RpcResult {
    const auto* p = ctx.params.as_object_ptr();
    if (!p)
      return RpcError::invalid_params("params must be object");

    const auto a = p->get_i64_or("a", 0);
    const auto b = p->get_i64_or("b", 0);

    return obj({ "sum", a + b });
  });

  token req = obj({
    "id", 1,
    "method", "math.add",
    "params", obj({ "a", 7, "b", 5 })
  });

  RpcResult out = router.dispatch(req);
  return 0;
}
```

---

## 📁 Examples

The `examples/` directory contains ready-to-run executables:

- `01_basic_request.cpp` — basic RPC request parsing
- `02_router_dispatch.cpp` — router + handler execution
- `03_notification.cpp` — notification without response
- `04_batch_requests.cpp` — mixed batch handling
- `run_all.cpp` — runs all examples

### Build examples

```bash
cmake -S . -B build -DVIX_WEBRPC_BUILD_EXAMPLES=ON
cmake --build build -j
```

### Run

```bash
cd build/examples
./webrpc_examples
```

---

## 🧪 Tests

```bash
cmake -S . -B build -DVIX_WEBRPC_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

---

## 🧩 Design Philosophy

- No exceptions
- No JSON text parsing
- Explicit error handling
- Minimal surface API
- Transport is an integration concern, not a dependency

WebRPC is intentionally small.
HTTP / WebSocket / P2P bridges live above it, not inside it.

---

## 📜 License

MIT © 2026 — Gaspard Kirira
https://github.com/vixcpp/webrpc
