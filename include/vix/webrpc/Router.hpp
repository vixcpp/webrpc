/**
 *
 *  @file Router.hpp
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

#ifndef VIX_WEBRPC_ROUTER_HPP
#define VIX_WEBRPC_ROUTER_HPP

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

#include <vix/json/Simple.hpp>

#include <vix/webrpc/Context.hpp>
#include <vix/webrpc/Error.hpp>
#include <vix/webrpc/Request.hpp>

namespace vix::webrpc
{
  /**
   * @brief Result of an RPC handler.
   *
   * @details
   * Handlers return either:
   * - a JSON-like token (success payload)
   * - or a structured RpcError (failure)
   *
   * This keeps control flow explicit and exception-free.
   */
  using RpcResult = std::variant<vix::json::token, RpcError>;

  /**
   * @brief RPC method handler signature.
   *
   * @details
   * A handler receives an execution Context and returns an RpcResult.
   * Handlers should validate their own input schema (params shape and types).
   */
  using RpcHandler = std::function<RpcResult(const Context &)>;

  /**
   * @brief Registry and dispatcher for WebRPC methods.
   *
   * @details
   * `Router` maps method names to handlers and executes them synchronously.
   * It is transport-agnostic (HTTP/WebSocket/P2P are integration concerns above this layer).
   *
   * Typical flow:
   * - parse a raw request token into RpcRequest
   * - validate and resolve a handler by method name
   * - build a Context view
   * - execute the handler
   */
  class Router
  {
  public:
    Router() = default;

    /**
     * @brief Register (or replace) an RPC method handler.
     *
     * @param name    Method name (e.g. "user.get").
     * @param handler Callable handling this method.
     *
     * @note
     * If a method with the same name already exists, it is replaced.
     */
    void add(std::string name, RpcHandler handler)
    {
      handlers_[std::move(name)] = std::move(handler);
    }

    /**
     * @brief Remove a method by name.
     *
     * @param name Method name.
     * @return True if a handler was removed.
     */
    bool remove(std::string_view name)
    {
      return handlers_.erase(std::string(name)) > 0;
    }

    /**
     * @brief Get the number of registered methods.
     */
    std::size_t size() const noexcept
    {
      return handlers_.size();
    }

    /**
     * @brief Check whether a method is registered.
     *
     * @param name Method name.
     * @return True if the method exists.
     */
    bool has(std::string_view name) const noexcept
    {
      return handlers_.find(std::string(name)) != handlers_.end();
    }

    /**
     * @brief Dispatch a parsed request to its handler.
     *
     * @param req       Parsed request.
     * @param transport Optional transport label (e.g. "http", "websocket", "p2p").
     * @param meta      Optional metadata map (e.g. headers, peer id).
     * @return RpcResult containing either a success token or an RpcError.
     *
     * @details
     * This function:
     * - validates the request (method must be non-empty)
     * - resolves the method in the registry
     * - builds a Context view (zero-copy)
     * - executes the handler
     */
    RpcResult dispatch(const RpcRequest &req,
                       std::string_view transport = {},
                       const Context::MetaMap *meta = nullptr) const
    {
      if (!req.valid())
        return RpcError::invalid_params("invalid rpc request");

      auto it = handlers_.find(req.method);
      if (it == handlers_.end())
        return RpcError::method_not_found(req.method);

      Context ctx{
          req.method,
          req.params,
          req.id,
          transport,
          meta,
      };

      return it->second(ctx);
    }

    /**
     * @brief Convenience: parse a raw token and dispatch it.
     *
     * @param raw      Raw JSON token representing a request object.
     * @param transport Optional transport label.
     * @param meta     Optional metadata map.
     * @return RpcResult containing either a success token or an RpcError.
     *
     * @note
     * This helper only supports a single request object. Batch handling is typically
     * implemented at a higher layer (dispatcher).
     */
    RpcResult dispatch(const vix::json::token &raw,
                       std::string_view transport = {},
                       const Context::MetaMap *meta = nullptr) const
    {
      auto parsed = RpcRequest::parse(raw);
      if (std::holds_alternative<RpcError>(parsed))
        return std::get<RpcError>(std::move(parsed));

      return dispatch(std::get<RpcRequest>(parsed), transport, meta);
    }

  private:
    std::unordered_map<std::string, RpcHandler> handlers_;
  };

} // namespace vix::webrpc

#endif // VIX_WEBRPC_ROUTER_HPP
