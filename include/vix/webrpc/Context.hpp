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
   * @brief Execution context for a single WebRPC call (read-only view).
   *
   * @details
   * `Context` is the object given to every RPC handler. It contains:
   * - the RPC method name (`method`)
   * - the request parameters (`params`)
   * - the optional request id (`id`)
   * - optional transport name (`transport`)
   * - optional metadata map (`meta`)
   *
   * @par Design goals (Vix style)
   * - **Explicit**: no hidden globals, no implicit transport behavior.
   * - **Predictable**: no exceptions required; accessors are safe and return null/empty on mismatch.
   * - **Beginner-friendly**: simple booleans and pointer helpers to inspect payload types.
   * - **Expert-friendly**: well-defined lifetime and ownership model (zero-copy views).
   *
   * @par Ownership & lifetime
   * `Context` does **not** own the request payload. It stores references/views:
   * - `method` is a `std::string_view` and must remain valid for the call.
   * - `params` and `id` are `const vix::json::token&` and must outlive `Context`.
   * - `meta` is an optional pointer; if provided, it must outlive `Context`.
   *
   * In WebRPC, this is typically guaranteed because the dispatcher/router builds `Context`
   * from a parsed in-memory request token and invokes the handler immediately.
   *
   * @note
   * `Context` is intended to be lightweight and copied by value if needed (it mainly holds refs/views).
   * It is **not** meant to be stored beyond the handler call.
   */
  struct Context
  {
    /// Metadata container (e.g. headers, peer id, request tags).
    using MetaMap = std::unordered_map<std::string, std::string>;

    /**
     * @brief RPC method name (example: `"user.get"`).
     *
     * @note This is a view. The underlying storage must outlive this `Context`.
     */
    std::string_view method{};

    /**
     * @brief RPC parameters token.
     *
     * @details
     * Usually an object or array, but WebRPC does not enforce a single shape.
     * Handlers can validate shape using `params_is_object()` / `params_is_array()`.
     *
     * @note This is a reference to an existing token (zero-copy).
     */
    const vix::json::token &params;

    /**
     * @brief Optional request id token.
     *
     * @details
     * If absent, `id` is expected to be `null` and the call is treated as a notification
     * (fire-and-forget). Use `has_id()` to test.
     *
     * @note This is a reference to an existing token (zero-copy).
     */
    const vix::json::token &id;

    /**
     * @brief Transport name (example: `"http"`, `"websocket"`, `"p2p"`).
     *
     * @details
     * This field is purely informational. WebRPC behavior must not depend on the transport.
     */
    std::string_view transport{};

    /**
     * @brief Optional metadata pointer (e.g. headers, peer info, tracing ids).
     *
     * @details
     * When present, it is expected to be owned by the caller/transport layer and to outlive
     * this `Context` (which is ephemeral).
     *
     * @note If `meta` is null, metadata accessors return empty/false.
     */
    const MetaMap *meta{nullptr};

    /**
     * @brief Construct a Context.
     *
     * @param method_    RPC method view (must remain valid during the call).
     * @param params_    Reference to params token (must outlive Context).
     * @param id_        Reference to id token (must outlive Context).
     * @param transport_ Optional transport name.
     * @param meta_      Optional metadata map pointer.
     */
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

    /**
     * @brief True if this call has an id (request/response semantics).
     *
     * @return `true` when `id` is not null, otherwise `false`.
     *
     * @note
     * A missing id typically indicates a notification (no response expected).
     */
    bool has_id() const noexcept { return !id.is_null(); }

    /**
     * @brief True if `params` is an object.
     */
    bool params_is_object() const noexcept { return params.is_object(); }

    /**
     * @brief True if `params` is an array.
     */
    bool params_is_array() const noexcept { return params.is_array(); }

    /**
     * @brief Get params as object pointer (nullptr if not object).
     *
     * @return Pointer to the underlying object, or nullptr.
     *
     * @code
     * const auto* obj = ctx.params_object_ptr();
     * if (!obj) return RpcError::invalid_params("params must be object");
     * auto id = obj->get_i64_or("id", 0);
     * @endcode
     */
    const vix::json::kvs *params_object_ptr() const noexcept
    {
      const auto p = params.as_object_ptr();
      return p ? p.get() : nullptr;
    }

    /**
     * @brief Get params as array pointer (nullptr if not array).
     *
     * @return Pointer to the underlying array, or nullptr.
     */
    const vix::json::array_t *params_array_ptr() const noexcept
    {
      const auto p = params.as_array_ptr();
      return p ? p.get() : nullptr;
    }

    /**
     * @brief Retrieve a metadata value if present.
     *
     * @param key Metadata key.
     * @return A view of the metadata value, or empty view if missing.
     *
     * @details
     * This function is intentionally explicit and safe:
     * - returns `{}` when `meta` is null
     * - returns `{}` when key is not present
     *
     * @warning
     * The returned `std::string_view` points into the stored `std::string` inside the map.
     * The metadata map must outlive the `Context` (and any view derived from it).
     *
     * @note
     * Current implementation uses `std::string(key)` because `std::unordered_map<std::string, ...>`
     * does not provide heterogeneous lookup by default. This is explicit and correct.
     */
    std::string_view meta_value(std::string_view key) const noexcept
    {
      if (!meta)
        return {};

      const auto it = meta->find(std::string(key));
      if (it == meta->end())
        return {};

      return it->second;
    }

    /**
     * @brief True if metadata exists and contains the given key.
     *
     * @param key Metadata key.
     * @return `true` if the map exists and contains `key`.
     */
    bool has_meta(std::string_view key) const noexcept
    {
      if (!meta)
        return false;
      return meta->find(std::string(key)) != meta->end();
    }
  };

} // namespace vix::webrpc

#endif // VIX_WEBRPC_CONTEXT_HPP
