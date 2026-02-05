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
 * ====================================================================
 * Vix.cpp - WebRPC
 * ====================================================================
 * Purpose:
 *   RPC method registry and dispatcher.
 *
 *   - Maps method name -> handler
 *   - Validates requests
 *   - Produces either result or RpcError
 *
 *   Transport-agnostic and synchronous by design.
 * ====================================================================
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
   * - token  -> success
   * - error  -> failure
   */
  using RpcResult = std::variant<vix::json::token, RpcError>;

  /**
   * @brief RPC method handler signature.
   *
   * Handlers:
   * - receive an execution Context
   * - return either a result token or an RpcError
   */
  using RpcHandler = std::function<RpcResult(const Context &)>;

  /**
   * @brief Registry and dispatcher for RPC methods.
   */
  class Router
  {
  public:
    Router() = default;

    /**
     * @brief Register a new RPC method.
     *
     * @param name     Method name (ex: "user.get")
     * @param handler  Callable handling the method
     *
     * If a method with the same name already exists,
     * it will be replaced.
     */
    void add(std::string name, RpcHandler handler)
    {
      handlers_[std::move(name)] = std::move(handler);
    }

    /**
     * @brief Remove a method. Returns true if removed.
     */
    bool remove(std::string_view name)
    {
      return handlers_.erase(std::string(name)) > 0;
    }

    /**
     * @brief Number of registered methods.
     */
    std::size_t size() const noexcept
    {
      return handlers_.size();
    }

    /**
     * @brief Check if a method exists.
     */
    bool has(std::string_view name) const noexcept
    {
      return handlers_.find(std::string(name)) != handlers_.end();
    }

    /**
     * @brief Dispatch a parsed RPC request.
     *
     * This function:
     * - validates the request
     * - resolves the method
     * - executes the handler
     */
    RpcResult dispatch(const RpcRequest &req,
                       std::string_view transport = {},
                       const Context::MetaMap *meta = nullptr) const
    {
      if (!req.valid())
        return RpcError::invalid_params("invalid rpc request");

      // Note: lookup is by std::string key type.
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
     * @brief Parse and dispatch from raw JSON token.
     *
     * This is a convenience helper.
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
