/**
 *
 *  @file Request.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/webrpc
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix.cpp
 */

#ifndef VIX_WEBRPC_REQUEST_HPP
#define VIX_WEBRPC_REQUEST_HPP

#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include <vix/json/Simple.hpp>
#include <vix/webrpc/Error.hpp>

namespace vix::webrpc
{
  /**
   * @brief Transport-agnostic WebRPC request envelope.
   *
   * @details
   * `RpcRequest` represents one RPC call as a value object. It is independent from
   * any transport (HTTP, WebSocket, P2P, CLI). This makes it usable for beginners
   * (simple shape, simple helpers) and for experts (explicit parsing rules, no exceptions).
   *
   * Expected JSON shape (object):
   * @code
   * {
   *   "id":     <string|int|null> (optional),
   *   "method": <string>          (required),
   *   "params": <any>             (optional)
   * }
   * @endcode
   *
   * @note
   * - Missing `id` typically indicates a notification (no response expected).
   * - `params` can be any JSON-like value; handlers decide how to interpret it.
   */
  struct RpcRequest
  {
    /// Optional request id (null if absent). Allowed: null|string|int.
    vix::json::token id{nullptr};

    /// RPC method name (required, non-empty).
    std::string method{};

    /// Optional parameters payload (any JSON-like value).
    vix::json::token params{nullptr};

    RpcRequest() = default;

    /**
     * @brief Construct a request.
     *
     * @param id_     Request id token (null if absent).
     * @param method_ RPC method name.
     * @param params_ Parameters token (null if absent).
     */
    RpcRequest(vix::json::token id_,
               std::string method_,
               vix::json::token params_)
        : id(std::move(id_)),
          method(std::move(method_)),
          params(std::move(params_))
    {
    }

    /// True if `id` is present (request/response semantics).
    bool has_id() const noexcept { return !id.is_null(); }

    /// True if this request has a non-empty method.
    bool valid() const noexcept { return !method.empty(); }

    /**
     * @brief Serialize this request to a JSON object token.
     *
     * @return A `vix::json::token` holding an object with fields {method, id?, params?}.
     */
    vix::json::token to_json() const
    {
      using namespace vix::json;

      kvs o;
      o.set_string("method", method);

      if (!id.is_null())
        o.set("id", id);

      if (!params.is_null())
        o.set("params", params);

      return token(o);
    }

    /**
     * @brief Parse an RpcRequest from a JSON token.
     *
     * @param root Input JSON token.
     * @return On success: `RpcRequest`. On failure: `RpcError` (PARSE_ERROR / INVALID_PARAMS).
     *
     * @details
     * Parsing rules:
     * - root must be an object
     * - "method" must exist and be a non-empty string
     * - "id" if present must be null, string, or int (i64)
     * - "params" if present can be any value
     *
     * @note
     * This API is explicit and exception-free: callers must handle both cases.
     */
    static std::variant<RpcRequest, RpcError> parse(const vix::json::token &root)
    {
      using namespace vix::json;

      const auto objp = root.as_object_ptr();
      if (!objp)
        return RpcError::parse_error("request must be an object");

      const kvs &o = *objp;

      const token *m = o.get_ptr("method");
      if (!m)
        return RpcError::invalid_params("missing field: method");

      const std::string *ms = m->as_string();
      if (!ms || ms->empty())
        return RpcError::invalid_params("method must be a non-empty string");

      token id_tok{nullptr};
      if (const token *idp = o.get_ptr("id"))
      {
        id_tok = *idp;
        if (!(id_tok.is_null() || id_tok.is_string() || id_tok.is_i64()))
          return RpcError::invalid_params("id must be string, int, or null");
      }

      token params_tok{nullptr};
      if (const token *pp = o.get_ptr("params"))
        params_tok = *pp;

      return RpcRequest{std::move(id_tok), *ms, std::move(params_tok)};
    }

    /**
     * @brief Get params as an object pointer when params is an object.
     *
     * @return Pointer to the object storage, or nullptr if params is not an object.
     *
     * @note
     * `vix::json::token` stores objects/arrays via shared ownership internally.
     * The returned pointer is valid as long as `params` is not reassigned and
     * this `RpcRequest` remains alive.
     */
    const vix::json::kvs *params_object_ptr() const noexcept
    {
      const auto p = params.as_object_ptr();
      return p ? p.get() : nullptr;
    }

    /**
     * @brief Get params as an array pointer when params is an array.
     *
     * @return Pointer to the array storage, or nullptr if params is not an array.
     */
    const vix::json::array_t *params_array_ptr() const noexcept
    {
      const auto p = params.as_array_ptr();
      return p ? p.get() : nullptr;
    }

    /**
     * @brief Convenience: retrieve a parameter by key when params is an object.
     *
     * @param key Parameter name.
     * @return Pointer to the param token, or nullptr if params is not an object or key is missing.
     */
    const vix::json::token *param_ptr(std::string_view key) const noexcept
    {
      if (const auto *o = params_object_ptr())
        return o->get_ptr(key);
      return nullptr;
    }
  };

} // namespace vix::webrpc

#endif // VIX_WEBRPC_REQUEST_HPP
