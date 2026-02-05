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

#include <vix/json/Simple.hpp>

namespace vix::webrpc
{
  struct RpcErrorParseResult; // defined after RpcError (avoids incomplete-type issues)

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
    std::string code{};

    /// Human-readable description
    std::string message{};

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

    bool valid() const noexcept { return !code.empty(); }
    bool has_details() const noexcept { return !details.is_null(); }

    // ------------------------------------------------------------
    // Serialization
    // ------------------------------------------------------------

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
    vix::json::token to_json() const { return to_json_token(); }

    // ------------------------------------------------------------
    // Parsing
    // ------------------------------------------------------------

    /**
     * @brief Parse an RpcError from a token.
     *
     * Returns:
     * - ok():     true  -> value() is the parsed error
     * - ok():     false -> error() is a PARSE_ERROR RpcError (with reason)
     */
    static RpcErrorParseResult parse(const vix::json::token &root);

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

  /**
   * @brief Result object for RpcError::parse().
   *
   * We intentionally do NOT use std::variant here, to avoid the "incomplete type"
   * problem inside RpcError, and to keep call sites simple and explicit.
   */
  struct RpcErrorParseResult
  {
    bool ok_{false};
    RpcError value_{};
    RpcError error_{};

    static RpcErrorParseResult ok(RpcError v)
    {
      RpcErrorParseResult r;
      r.ok_ = true;
      r.value_ = std::move(v);
      return r;
    }

    static RpcErrorParseResult fail(RpcError e)
    {
      RpcErrorParseResult r;
      r.ok_ = false;
      r.error_ = std::move(e);
      return r;
    }

    bool ok() const noexcept { return ok_; }

    /// Valid only if ok()==true
    const RpcError &value() const noexcept { return value_; }
    RpcError &value() noexcept { return value_; }

    /// Valid only if ok()==false
    const RpcError &error() const noexcept { return error_; }
    RpcError &error() noexcept { return error_; }
  };

  // ------------------------------------------------------------
  // RpcError::parse() implementation (after RpcErrorParseResult is complete)
  // ------------------------------------------------------------

  inline RpcErrorParseResult RpcError::parse(const vix::json::token &root)
  {
    using namespace vix::json;

    const auto objp = root.as_object_ptr();
    if (!objp)
      return RpcErrorParseResult::fail(parse_error("error must be an object"));

    const kvs &o = *objp;

    const token *code_t = o.get_ptr("code");
    const token *msg_t = o.get_ptr("message");

    if (!code_t || !msg_t)
      return RpcErrorParseResult::fail(parse_error("error object must contain code and message"));

    if (!code_t->is_string() || !msg_t->is_string())
      return RpcErrorParseResult::fail(parse_error("code and message must be strings"));

    RpcError out;
    out.code = code_t->as_string_or("");
    out.message = msg_t->as_string_or("");

    if (out.code.empty())
      return RpcErrorParseResult::fail(parse_error("code must not be empty"));

    if (const token *d = o.get_ptr("details"))
      out.details = *d;

    return RpcErrorParseResult::ok(std::move(out));
  }

} // namespace vix::webrpc

#endif // VIX_WEBRPC_ERROR_HPP
