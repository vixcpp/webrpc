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
 * ====================================================================
 * Vix.cpp - WebRPC
 * ====================================================================
 * Purpose:
 *   Transport-agnostic RPC request model.
 *   - method: string (required)
 *   - params: JSON-like token (optional)
 *   - id: JSON-like token (optional, can be string/int/null)
 * ====================================================================
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
   * @brief WebRPC request envelope.
   *
   * Expected JSON shape (object):
   * {
   *   "id":     <string|int|null> (optional),
   *   "method": <string>          (required),
   *   "params": <any>             (optional)
   * }
   */
  struct RpcRequest
  {
    vix::json::token id{nullptr};
    std::string method{};
    vix::json::token params{nullptr};

    RpcRequest() = default;

    RpcRequest(vix::json::token id_,
               std::string method_,
               vix::json::token params_)
        : id(std::move(id_)),
          method(std::move(method_)),
          params(std::move(params_))
    {
    }

    bool has_id() const noexcept { return !id.is_null(); }
    bool valid() const noexcept { return !method.empty(); }

    // ------------------------------------------------------------
    // Serialization
    // ------------------------------------------------------------

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

    // ------------------------------------------------------------
    // Parsing
    // ------------------------------------------------------------

    /**
     * @brief Parse a RpcRequest from a vix::json::token.
     *
     * Returns:
     * - RpcRequest on success
     * - RpcError on failure (PARSE_ERROR or INVALID_PARAMS)
     */
    static std::variant<RpcRequest, RpcError> parse(const vix::json::token &root)
    {
      using namespace vix::json;

      const auto objp = root.as_object_ptr();
      if (!objp)
        return RpcError::parse_error("request must be an object");

      const kvs &o = *objp;

      // method (required, non-empty string)
      const token *m = o.get_ptr("method");
      if (!m)
        return RpcError::invalid_params("missing field: method");

      const std::string *ms = m->as_string();
      if (!ms || ms->empty())
        return RpcError::invalid_params("method must be a non-empty string");

      // id (optional: null|string|int)
      token id_tok{nullptr};
      if (const token *idp = o.get_ptr("id"))
      {
        id_tok = *idp;
        if (!(id_tok.is_null() || id_tok.is_string() || id_tok.is_i64()))
          return RpcError::invalid_params("id must be string, int, or null");
      }

      // params (optional: any)
      token params_tok{nullptr};
      if (const token *pp = o.get_ptr("params"))
        params_tok = *pp;

      return RpcRequest{std::move(id_tok), *ms, std::move(params_tok)};
    }

    // ------------------------------------------------------------
    // Params helpers
    // ------------------------------------------------------------

    /**
     * @brief Extract params as object if possible.
     * @return pointer to object storage or nullptr.
     *
     * Note: vix::json::token stores object/array via shared_ptr internally,
     * so this pointer remains valid as long as *this is alive and params
     * is not reassigned.
     */
    const vix::json::kvs *params_object_ptr() const noexcept
    {
      const auto p = params.as_object_ptr();
      return p ? p.get() : nullptr;
    }

    /**
     * @brief Extract params as array if possible.
     * @return pointer to array storage or nullptr.
     */
    const vix::json::array_t *params_array_ptr() const noexcept
    {
      const auto p = params.as_array_ptr();
      return p ? p.get() : nullptr;
    }

    /**
     * @brief Convenience: get param token by key when params is an object.
     * @return nullptr if params is not object or key missing.
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
