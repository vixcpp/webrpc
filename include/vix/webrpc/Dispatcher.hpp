/**
 *
 *  @file Dispatcher.hpp
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
 *   Request dispatcher for WebRPC.
 *
 *   - Accepts a raw vix::json::token (object or batch array)
 *   - Parses RpcRequest
 *   - Executes Router dispatch
 *   - Produces RpcResponse envelope (or no response for notifications)
 *
 *   Transport-agnostic.
 * ====================================================================
 */

#ifndef VIX_WEBRPC_DISPATCHER_HPP
#define VIX_WEBRPC_DISPATCHER_HPP

#include <optional>
#include <string_view>
#include <utility>

#include <vix/json/Simple.hpp>

#include <vix/webrpc/Error.hpp>
#include <vix/webrpc/Request.hpp>
#include <vix/webrpc/Response.hpp>
#include <vix/webrpc/Router.hpp>

namespace vix::webrpc
{
  /**
   * @brief Transport-agnostic dispatcher.
   *
   * The Dispatcher is a thin layer:
   * - parse -> dispatch -> envelope
   * It does not own the Router.
   */
  class Dispatcher
  {
  public:
    explicit Dispatcher(const Router &router) noexcept : router_(router) {}

    /**
     * @brief Handle one payload (object or batch array).
     *
     * Returns:
     * - nullopt if it's a notification (or a batch of only notifications)
     * - a token representing RpcResponse object
     * - or a token representing an array of RpcResponse objects (batch)
     */
    std::optional<vix::json::token> handle(
        const vix::json::token &payload,
        std::string_view transport = {},
        const Context::MetaMap *meta = nullptr) const
    {
      if (payload.is_array())
        return handle_batch(payload, transport, meta);

      auto r = handle_one(payload, transport, meta);
      if (!r.has_value())
        return std::nullopt;

      return r->to_json();
    }

    /**
     * @brief Handle a single call (must be an object).
     *
     * Returns:
     * - nullopt if notification (id is null)
     * - RpcResponse otherwise
     */
    std::optional<RpcResponse> handle_one(
        const vix::json::token &payload,
        std::string_view transport = {},
        const Context::MetaMap *meta = nullptr) const
    {
      // Parse request envelope
      auto parsed = RpcRequest::parse(payload);
      if (std::holds_alternative<RpcError>(parsed))
      {
        // Parse errors have no id to echo.
        return RpcResponse::fail(vix::json::token{nullptr}, std::get<RpcError>(std::move(parsed)));
      }

      const RpcRequest req = std::get<RpcRequest>(std::move(parsed));

      // Notification: no response at all
      if (req.id.is_null())
      {
        // Still dispatch if you want side effects.
        (void)router_.dispatch(req, transport, meta);
        return std::nullopt;
      }

      // Dispatch
      auto out = router_.dispatch(req, transport, meta);

      if (std::holds_alternative<RpcError>(out))
        return RpcResponse::fail(req.id, std::get<RpcError>(std::move(out)));

      return RpcResponse::ok(req.id, std::get<vix::json::token>(std::move(out)));
    }

  private:
    const Router &router_;

    std::optional<vix::json::token> handle_batch(
        const vix::json::token &payload,
        std::string_view transport,
        const Context::MetaMap *meta) const
    {
      const auto ap = payload.as_array_ptr();
      if (!ap)
      {
        // Should never happen if is_array() is correct, but keep it strict.
        RpcResponse err = RpcResponse::fail(vix::json::token{nullptr}, RpcError::parse_error("batch must be an array"));
        return err.to_json();
      }

      vix::json::array_t out_arr;
      out_arr.elems.reserve(ap->elems.size());

      for (const auto &item : ap->elems)
      {
        auto resp = handle_one(item, transport, meta);
        if (!resp.has_value())
          continue; // notification

        out_arr.elems.push_back(resp->to_json());
      }

      // If batch produced no responses, return nothing (all notifications)
      if (out_arr.elems.empty())
        return std::nullopt;

      return vix::json::token(out_arr);
    }
  };

} // namespace vix::webrpc

#endif // VIX_WEBRPC_DISPATCHER_HPP
