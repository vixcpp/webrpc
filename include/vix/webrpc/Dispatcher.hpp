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
 *  Vix.cpp
 */

#ifndef VIX_WEBRPC_DISPATCHER_HPP
#define VIX_WEBRPC_DISPATCHER_HPP

#include <optional>
#include <string_view>
#include <utility>
#include <variant>

#include <vix/json/Simple.hpp>

#include <vix/webrpc/Error.hpp>
#include <vix/webrpc/Request.hpp>
#include <vix/webrpc/Response.hpp>
#include <vix/webrpc/Router.hpp>

namespace vix::webrpc
{
  /**
   * @brief Transport-agnostic request dispatcher.
   *
   * @details
   * `Dispatcher` is a thin orchestration layer:
   * - parse request envelope (`RpcRequest`)
   * - dispatch to `Router`
   * - wrap output into a response envelope (`RpcResponse`)
   *
   * It supports:
   * - single call payloads (object)
   * - batch payloads (array)
   * - notifications (no id -> no response)
   *
   * @note
   * `Dispatcher` does not own the router. It holds a reference and assumes the router
   * outlives the dispatcher.
   */
  class Dispatcher
  {
  public:
    /**
     * @brief Construct a dispatcher bound to an existing router.
     *
     * @param router Router used to resolve and execute RPC methods.
     */
    explicit Dispatcher(const Router &router) noexcept : router_(router) {}

    /**
     * @brief Handle one payload (single call or batch).
     *
     * @param payload   Request token (object or array).
     * @param transport Optional transport label (e.g. "http", "websocket", "p2p").
     * @param meta      Optional metadata map (e.g. headers, peer id).
     *
     * @return
     * - `std::nullopt` if the payload is a notification (or a batch of only notifications)
     * - a token representing one `RpcResponse` object
     * - or a token representing an array of `RpcResponse` objects (batch)
     */
    std::optional<vix::json::token> handle(
        const vix::json::token &payload,
        std::string_view transport = {},
        const Context::MetaMap *meta = nullptr) const
    {
      if (payload.is_array())
        return handle_batch(payload, transport, meta);

      const auto r = handle_one(payload, transport, meta);
      if (!r.has_value())
        return std::nullopt;

      return r->to_json();
    }

    /**
     * @brief Handle a single call payload (request object).
     *
     * @param payload   Request token (must be an object).
     * @param transport Optional transport label.
     * @param meta      Optional metadata map.
     *
     * @return
     * - `std::nullopt` if the call is a notification (id is null)
     * - `RpcResponse` otherwise
     *
     * @note
     * If the envelope is malformed, an error response is returned with `id = null`,
     * because there is no reliable id to echo back.
     */
    std::optional<RpcResponse> handle_one(
        const vix::json::token &payload,
        std::string_view transport = {},
        const Context::MetaMap *meta = nullptr) const
    {
      auto parsed = RpcRequest::parse(payload);
      if (std::holds_alternative<RpcError>(parsed))
      {
        return RpcResponse::fail(vix::json::token{nullptr},
                                 std::get<RpcError>(std::move(parsed)));
      }

      RpcRequest req = std::get<RpcRequest>(std::move(parsed));

      if (req.id.is_null())
      {
        (void)router_.dispatch(req, transport, meta);
        return std::nullopt;
      }

      auto out = router_.dispatch(req, transport, meta);

      if (std::holds_alternative<RpcError>(out))
        return RpcResponse::fail(req.id, std::get<RpcError>(std::move(out)));

      return RpcResponse::ok(req.id, std::get<vix::json::token>(std::move(out)));
    }

  private:
    const Router &router_;

    /**
     * @brief Handle a batch payload (array of calls).
     *
     * @param payload   Batch token (must be an array).
     * @param transport Optional transport label.
     * @param meta      Optional metadata map.
     *
     * @return
     * - `std::nullopt` if all calls are notifications
     * - otherwise an array token of response objects
     *
     * Rules:
     * - empty batch is invalid (returns an error response with id = null)
     * - non-object items produce an error response entry (id = null)
     * - processing continues for remaining items (best-effort batch)
     */
    std::optional<vix::json::token> handle_batch(
        const vix::json::token &payload,
        std::string_view transport,
        const Context::MetaMap *meta) const
    {
      using namespace vix::json;

      const auto ap = payload.as_array_ptr();
      if (!ap)
      {
        RpcResponse err = RpcResponse::fail(token{nullptr},
                                            RpcError::parse_error("batch must be an array"));
        return err.to_json();
      }

      if (ap->elems.empty())
      {
        RpcResponse err = RpcResponse::fail(token{nullptr},
                                            RpcError::invalid_params("batch must not be empty"));
        return err.to_json();
      }

      array_t out_arr;
      out_arr.elems.reserve(ap->elems.size());

      for (const auto &item : ap->elems)
      {
        if (!item.is_object())
        {
          RpcResponse r = RpcResponse::fail(token{nullptr},
                                            RpcError::parse_error("batch item must be an object"));
          out_arr.elems.push_back(r.to_json());
          continue;
        }

        auto resp = handle_one(item, transport, meta);
        if (!resp.has_value())
          continue;

        out_arr.elems.push_back(resp->to_json());
      }

      if (out_arr.elems.empty())
        return std::nullopt;

      return token(out_arr);
    }
  };

} // namespace vix::webrpc

#endif // VIX_WEBRPC_DISPATCHER_HPP
