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
 *  Vix.cpp
 */

#ifndef VIX_WEBRPC_ERROR_HPP
#define VIX_WEBRPC_ERROR_HPP

#include <string>
#include <string_view>
#include <utility>

#include <vix/json/Simple.hpp>

namespace vix::webrpc
{
  struct RpcErrorParseResult;

  /**
   * @brief Structured error returned by a WebRPC call.
   *
   * @details
   * `RpcError` represents an explicit failure in an RPC operation.
   * Errors are **values**, not exceptions, and must be returned as part
   * of an RPC response.
   *
   * JSON representation:
   * @code
   * {
   *   "code":    "<string>",
   *   "message": "<string>",
   *   "details": <any?>   // optional
   * }
   * @endcode
   *
   * @note
   * - `code` is machine-readable and stable.
   * - `message` is human-readable.
   * - `details` is optional and may contain structured data.
   */
  struct RpcError
  {
    /// Machine-readable error code (e.g. "METHOD_NOT_FOUND").
    std::string code{};

    /// Human-readable error description.
    std::string message{};

    /// Optional structured details.
    vix::json::token details{nullptr};

    RpcError() = default;

    RpcError(std::string code_, std::string message_)
        : code(std::move(code_)), message(std::move(message_)), details(nullptr)
    {
    }

    RpcError(std::string code_, std::string message_, vix::json::token details_)
        : code(std::move(code_)),
          message(std::move(message_)),
          details(std::move(details_))
    {
    }

    /// True if this error is valid (non-empty code).
    bool valid() const noexcept { return !code.empty(); }

    /// True if this error contains structured details.
    bool has_details() const noexcept { return !details.is_null(); }

    /**
     * @brief Convert this error to a JSON object.
     *
     * @return A `vix::json::token` representing the error.
     */
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

    /// Alias for to_json_token().
    vix::json::token to_json() const { return to_json_token(); }

    /**
     * @brief Parse an RpcError from a JSON token.
     *
     * @param root JSON token representing an error object.
     * @return RpcErrorParseResult describing success or failure.
     */
    static RpcErrorParseResult parse(const vix::json::token &root);

    /**
     * @brief Error: RPC method not found.
     */
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

    /**
     * @brief Error: invalid parameters.
     */
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

    /**
     * @brief Error: malformed or invalid RPC payload.
     */
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

    /**
     * @brief Error: internal server failure.
     */
    static RpcError internal_error(std::string_view msg)
    {
      return RpcError{
          "INTERNAL_ERROR",
          std::string(msg),
      };
    }
  };

  /**
   * @brief Result type returned by RpcError::parse().
   *
   * @details
   * This type avoids `std::variant` to keep the API explicit
   * and to prevent incomplete-type issues during parsing.
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

    /// Valid only if ok() == true
    const RpcError &value() const noexcept { return value_; }
    RpcError &value() noexcept { return value_; }

    /// Valid only if ok() == false
    const RpcError &error() const noexcept { return error_; }
    RpcError &error() noexcept { return error_; }
  };

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
