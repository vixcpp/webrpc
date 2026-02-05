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
 *  Vix.cpp
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
   * @brief Transport-agnostic WebRPC response envelope.
   *
   * @details
   * `RpcResponse` represents the result of an RPC call as an explicit value:
   * - a success payload (`result`)
   * - or a structured error (`error`)
   *
   * The envelope is independent from any transport (HTTP, WebSocket, P2P, CLI),
   * making it usable by beginners (simple shape) and reliable for experts
   * (explicit rules, no exceptions).
   *
   * Expected JSON shape (object):
   *
   * Success:
   * @code
   * { "id": <id|null>, "result": <any> }
   * @endcode
   *
   * Error:
   * @code
   * { "id": <id|null>, "error": { "code": "<string>", "message": "<string>", "details": <any?> } }
   * @endcode
   *
   * Rules:
   * - `result` XOR `error` (never both)
   * - `id` is optional and may be null (common for notifications / fire-and-forget)
   */
  struct RpcResponse
  {
    /// Echo of the request id (allowed: null|string|int).
    vix::json::token id{nullptr};

    /// Success payload (valid only when has_error == false).
    vix::json::token result{nullptr};

    /// Error payload (valid only when has_error == true).
    RpcError error{};

    /// True if this response represents an error.
    bool has_error{false};

    RpcResponse() = default;

    /**
     * @brief Build a success response.
     *
     * @param id_     Request id (may be null).
     * @param result_ Success payload.
     */
    static RpcResponse ok(vix::json::token id_, vix::json::token result_)
    {
      RpcResponse r;
      r.id = std::move(id_);
      r.result = std::move(result_);
      r.error = RpcError{};
      r.has_error = false;
      return r;
    }

    /**
     * @brief Build an error response.
     *
     * @param id_  Request id (may be null).
     * @param err  Structured error.
     */
    static RpcResponse fail(vix::json::token id_, RpcError err)
    {
      RpcResponse r;
      r.id = std::move(id_);
      r.result = vix::json::token(nullptr);
      r.error = std::move(err);
      r.has_error = true;
      return r;
    }

    /// True if id is null (typically a notification).
    bool is_notification() const noexcept { return id.is_null(); }

    /// True if this response is a success response.
    bool ok() const noexcept { return !has_error; }

    /**
     * @brief Serialize this response to a JSON object token.
     *
     * @return A `vix::json::token` holding {id, result} or {id, error}.
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

    /**
     * @brief Parse an RpcResponse from a JSON token.
     *
     * @param root Input JSON token.
     * @return On success: `RpcResponse`. On failure: `RpcError` (PARSE_ERROR / INVALID_PARAMS).
     *
     * @details
     * Validation performed:
     * - root must be an object
     * - "id" if present must be null, string, or int (i64)
     * - response must contain exactly one of: "result" or "error"
     * - "error" is validated by `RpcError::parse()`
     *
     * @note
     * This API is explicit and exception-free: callers must handle both cases.
     */
    static std::variant<RpcResponse, RpcError> parse(const vix::json::token &root)
    {
      using namespace vix::json;

      const auto objp = root.as_object_ptr();
      if (!objp)
        return RpcError::parse_error("response must be an object");

      const kvs &o = *objp;

      token id_tok{nullptr};
      if (const token *idp = o.get_ptr("id"))
      {
        id_tok = *idp;
        if (!(id_tok.is_null() || id_tok.is_string() || id_tok.is_i64()))
          return RpcError::invalid_params("id must be string, int, or null");
      }

      const token *res_p = o.get_ptr("result");
      const token *err_p = o.get_ptr("error");

      if (res_p && err_p)
        return RpcError::invalid_params("response cannot contain both result and error");

      if (!res_p && !err_p)
        return RpcError::invalid_params("response must contain result or error");

      if (err_p)
      {
        const auto pr = RpcError::parse(*err_p);
        if (!pr.ok())
          return pr.error();

        RpcResponse out;
        out.id = std::move(id_tok);
        out.result = token(nullptr);
        out.error = pr.value();
        out.has_error = true;
        return out;
      }

      RpcResponse out;
      out.id = std::move(id_tok);
      out.result = *res_p;
      out.error = RpcError{};
      out.has_error = false;
      return out;
    }
  };

} // namespace vix::webrpc

#endif // VIX_WEBRPC_RESPONSE_HPP
