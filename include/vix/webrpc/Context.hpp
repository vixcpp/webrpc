/**
 *
 *  @file Context.hpp
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
 *   Execution context for a single WebRPC call.
 *
 *   - Transport-agnostic
 *   - Zero-copy access to request data
 *   - Optional metadata (headers, peer info, transport)
 *
 *   Context is passed to RPC handlers and lives only
 *   for the duration of a single call.
 * ====================================================================
 */

#ifndef VIX_WEBRPC_CONTEXT_HPP
#define VIX_WEBRPC_CONTEXT_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

#include <vix/json/Simple.hpp>

namespace vix::webrpc
{
  /**
   * @brief Execution context for an RPC call.
   *
   * This object provides read-only access to:
   * - method name
   * - parameters
   * - request id
   * - optional metadata provided by the transport
   *
   * It does NOT own the transport.
   */
  struct Context
  {
    /// RPC method name (ex: "user.get")
    std::string_view method;

    /// RPC parameters (JSON-like object or array)
    const vix::json::token &params;

    /// Optional request id (null if absent)
    const vix::json::token &id;

    /// Arbitrary metadata (headers, peer id, etc.)
    using MetaMap = std::unordered_map<std::string, std::string>;
    const MetaMap *meta{nullptr};

    /// Transport name (ex: "http", "websocket", "p2p")
    std::string_view transport;

    Context(std::string_view method_,
            const vix::json::token &params_,
            const vix::json::token &id_,
            std::string_view transport_ = {},
            const MetaMap *meta_ = nullptr)
        : method(method_),
          params(params_),
          id(id_),
          meta(meta_),
          transport(transport_)
    {
    }

    /// True if the call has an id (request/response semantics)
    bool has_id() const noexcept
    {
      return !id.is_null();
    }

    /// True if params is an object
    bool params_is_object() const noexcept
    {
      return params.is_object();
    }

    /// True if params is an array
    bool params_is_array() const noexcept
    {
      return params.is_array();
    }

    /// Retrieve a metadata value if present
    std::string_view meta_value(std::string_view key) const noexcept
    {
      if (!meta)
        return {};

      auto it = meta->find(std::string(key));
      if (it == meta->end())
        return {};

      return it->second;
    }
  };

} // namespace vix::webrpc

#endif // VIX_WEBRPC_CONTEXT_HPP
