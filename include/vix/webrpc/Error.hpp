/**
 *
 *  @file Error.hpp
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
 *   Structured RPC error model for WebRPC.
 *   - Explicit error codes
 *   - Human-readable messages
 *   - Optional JSON details (vix::json::Simple token)
 *
 *   This type is transport-agnostic and can be serialized
 *   over HTTP or WebSocket.
 * ====================================================================
 */

#ifndef VIX_WEBRPC_ERROR_HPP
#define VIX_WEBRPC_ERROR_HPP

#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include <vix/json/Simple.hpp>

namespace vix::webrpc
{
  /**
   * @brief Structured WebRPC error.
   *
   * An RpcError represents a failure in an RPC call.
   * Errors are explicit and must be returned as part of the RPC response (never thrown).
   *
   * JSON shape:
   * {
   *   "code":    <string>,
   *   "message": <string>,
   *   "details": <any?>      // optional
   * }
   */
  struct RpcError
  {
    /// Machine-readable error code (example: "METHOD_NOT_FOUND")
    std::string code;

    /// Human-readable description
    std::string message;

    /// Optional structured details (JSON-like token)
    vix::json::token details{nullptr};

    RpcError() = default;

    RpcError(std::string code_, std::string message_)
        : code(std::move(code_)), message(std::move(message_)), details(nullptr)
    {
    }

    RpcError(std::string code_, std::string message_, vix::json::token details_)
        : code(std::move(code_)), message(std::move(message_)), details(std::move(details_))
    {
    }

    bool valid() const noexcept
    {
      return !code.empty();
    }

    bool has_details() const noexcept
    {
      return !details.is_null();
    }

    /// Convert error to JSON object (vix::json::Simple).
    vix::json::token to_json_token() const
    {
      using namespace vix::json;

      if (details.is_null())
      {
        return obj({
            "code",
            code,
            "message",
            message,
        });
      }

      return obj({
          "code",
          code,
          "message",
          message,
          "details",
          details,
      });
    }

    /// Alias kept for convenience.
    vix::json::token to_json() const
    {
      return to_json_token();
    }

    /**
     * @brief Parse an RpcError from a token.
     *
     * Returns:
     * - RpcError (valid) if input is well-formed
     * - RpcError::parse_error(...) if malformed
     */
    static std::variant<RpcError, RpcError> parse(const vix::json::token &root)
    {
      using namespace vix::json;

      auto objp = root.as_object_ptr();
      if (!objp)
        return parse_error("error must be an object");

      const kvs &o = *objp;

      const token *code_t = o.get_ptr("code");
      const token *msg_t = o.get_ptr("message");

      if (!code_t || !msg_t)
        return parse_error("error object must contain code and message");

      if (!code_t->is_string() || !msg_t->is_string())
        return parse_error("code and message must be strings");

      RpcError out;
      out.code = code_t->as_string_or("");
      out.message = msg_t->as_string_or("");

      if (out.code.empty())
        return parse_error("code must not be empty");

      if (const token *d = o.get_ptr("details"))
        out.details = *d;

      return out;
    }

    // ------------------------------------------------------------
    // Common errors (string codes for now, stable ABI)
    // ------------------------------------------------------------

    static RpcError method_not_found(std::string_view method)
    {
      using namespace vix::json;

      return RpcError{
          "METHOD_NOT_FOUND",
          "RPC method not found",
          obj({
              "method",
              std::string(method),
          }),
      };
    }

    static RpcError invalid_params(std::string_view reason)
    {
      using namespace vix::json;

      return RpcError{
          "INVALID_PARAMS",
          "Invalid RPC parameters",
          obj({
              "reason",
              std::string(reason),
          }),
      };
    }

    static RpcError parse_error(std::string_view reason)
    {
      using namespace vix::json;

      return RpcError{
          "PARSE_ERROR",
          "Failed to parse RPC payload",
          obj({
              "reason",
              std::string(reason),
          }),
      };
    }

    static RpcError internal_error(std::string_view msg)
    {
      return RpcError{
          "INTERNAL_ERROR",
          std::string(msg),
      };
    }
  };

} // namespace vix::webrpc

#endif // VIX_WEBRPC_ERROR_HPP
