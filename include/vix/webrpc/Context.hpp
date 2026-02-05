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

#include <string>
#include <string_view>
#include <unordered_map>

#include <vix/json/Simple.hpp>

namespace vix::webrpc
{
  /**
   * @brief Execution context for an RPC call (read-only).
   *
   * Context does not own request payload. It only references:
   * - method name
   * - params token
   * - id token
   *
   * Optional:
   * - transport name
   * - metadata map (headers, peer id, etc.)
   */
  struct Context
  {
    using MetaMap = std::unordered_map<std::string, std::string>;

    /// RPC method name (ex: "user.get")
    std::string_view method{};

    /// RPC parameters (JSON-like object or array)
    const vix::json::token &params;

    /// Optional request id (null if absent)
    const vix::json::token &id;

    /// Transport name (ex: "http", "websocket", "p2p")
    std::string_view transport{};

    /// Optional metadata (headers, peer id, etc.)
    const MetaMap *meta{nullptr};

    Context(std::string_view method_,
            const vix::json::token &params_,
            const vix::json::token &id_,
            std::string_view transport_ = {},
            const MetaMap *meta_ = nullptr) noexcept
        : method(method_),
          params(params_),
          id(id_),
          transport(transport_),
          meta(meta_)
    {
    }

    /// True if the call has an id (request/response semantics)
    bool has_id() const noexcept { return !id.is_null(); }

    /// True if params is an object
    bool params_is_object() const noexcept { return params.is_object(); }

    /// True if params is an array
    bool params_is_array() const noexcept { return params.is_array(); }

    /// Get params as object (nullptr if not object)
    const vix::json::kvs *params_object_ptr() const noexcept
    {
      const auto p = params.as_object_ptr();
      return p ? p.get() : nullptr;
    }

    /// Get params as array (nullptr if not array)
    const vix::json::array_t *params_array_ptr() const noexcept
    {
      const auto p = params.as_array_ptr();
      return p ? p.get() : nullptr;
    }

    /**
     * @brief Retrieve a metadata value if present.
     *
     * Important: meta_value() returns a view into the stored string.
     * The map must outlive this Context (it should, because Context is ephemeral).
     */
    std::string_view meta_value(std::string_view key) const noexcept
    {
      if (!meta)
        return {};

      // Avoid allocating a std::string for common cases by doing a transparent lookup
      // when possible. Since std::unordered_map<string,string> doesn't support
      // heterogenous lookup by default, we keep it simple and explicit here.
      const auto it = meta->find(std::string(key));
      if (it == meta->end())
        return {};

      return it->second;
    }

    /// Convenience: true if metadata exists and contains key
    bool has_meta(std::string_view key) const noexcept
    {
      if (!meta)
        return false;
      return meta->find(std::string(key)) != meta->end();
    }
  };

} // namespace vix::webrpc

#endif // VIX_WEBRPC_CONTEXT_HPP
