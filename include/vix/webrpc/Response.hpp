/**
 *
 *  @file Response.hpp
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
 *   Transport-agnostic RPC response envelope.
 *   - id:     echoes request id (string|int|null)
 *   - result: success payload (any token)
 *   - error:  structured error (RpcError)
 *
 * Rules:
 *   - result XOR error (never both)
 *   - id is optional (null for notifications / fire-and-forget)
 * ====================================================================
 */

#ifndef VIX_WEBRPC_RESPONSE_HPP
#define VIX_WEBRPC_RESPONSE_HPP

#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include <vix/json/Simple.hpp>
#include <vix/webrpc/Error.hpp>

namespace vix::webrpc
{
  /**
   * @brief WebRPC response envelope.
   *
   * Expected JSON shape (object):
   * Success:
   * { "id": <id|null>, "result": <any> }
   *
   * Error:
   * { "id": <id|null>, "error": { "code": <string>, "message": <string>, "details": <any?> } }
   */
  struct RpcResponse
  {
    vix::json::token id{nullptr};

    // Exactly one branch should be active.
    vix::json::token result{nullptr};
    RpcError error{};
    bool has_error{false};

    RpcResponse() = default;

    static RpcResponse ok(vix::json::token id_, vix::json::token result_)
    {
      RpcResponse r;
      r.id = std::move(id_);
      r.result = std::move(result_);
      r.error = RpcError{};
      r.has_error = false;
      return r;
    }

    static RpcResponse fail(vix::json::token id_, RpcError err)
    {
      RpcResponse r;
      r.id = std::move(id_);
      r.result = vix::json::token(nullptr);
      r.error = std::move(err);
      r.has_error = true;
      return r;
    }

    bool is_notification() const noexcept { return id.is_null(); }
    bool ok() const noexcept { return !has_error; }

    // ------------------------------------------------------------
    // Serialization
    // ------------------------------------------------------------

    /**
     * @brief Convert response to vix::json::token (object).
     */
    vix::json::token to_json() const
    {
      using namespace vix::json;

      if (has_error)
      {
        return obj({
            "id",
            id,
            "error",
            error.to_json(),
        });
      }

      return obj({
          "id",
          id,
          "result",
          result,
      });
    }

    // ------------------------------------------------------------
    // Parsing
    // ------------------------------------------------------------

    /**
     * @brief Parse a RpcResponse from a vix::json::token.
     *
     * Returns:
     * - RpcResponse on success
     * - RpcError on malformed envelope (PARSE_ERROR / INVALID_PARAMS)
     *
     * Notes:
     * - We validate: id type, XOR rule (result vs error),
     *   and error shape using RpcError::parse().
     */
    static std::variant<RpcResponse, RpcError> parse(const vix::json::token &root)
    {
      using namespace vix::json;

      const auto objp = root.as_object_ptr();
      if (!objp)
        return RpcError::parse_error("response must be an object");

      const kvs &o = *objp;

      // id (optional: null|string|int)
      token id_tok{nullptr};
      if (const token *idp = o.get_ptr("id"))
      {
        id_tok = *idp;
        if (!(id_tok.is_null() || id_tok.is_string() || id_tok.is_i64()))
          return RpcError::invalid_params("id must be string, int, or null");
      }

      const token *res_p = o.get_ptr("result");
      const token *err_p = o.get_ptr("error");

      // result XOR error
      if (res_p && err_p)
        return RpcError::invalid_params("response cannot contain both result and error");

      if (!res_p && !err_p)
        return RpcError::invalid_params("response must contain result or error");

      // error branch
      if (err_p)
      {
        // Let RpcError validate its own schema
        const auto pr = RpcError::parse(*err_p);
        if (!pr.ok())
          return pr.error(); // malformed error object -> return parse_error RpcError

        RpcResponse out;
        out.id = std::move(id_tok);
        out.result = token(nullptr);
        out.error = pr.value();
        out.has_error = true;
        return out;
      }

      // success branch
      RpcResponse out;
      out.id = std::move(id_tok);
      out.result = *res_p; // copy token
      out.error = RpcError{};
      out.has_error = false;
      return out;
    }
  };

} // namespace vix::webrpc

#endif // VIX_WEBRPC_RESPONSE_HPP
