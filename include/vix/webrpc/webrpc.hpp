/**
 *
 *  @file webrpc.hpp
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

#ifndef VIX_WEBRPC_WEBRPC_HPP
#define VIX_WEBRPC_WEBRPC_HPP

/**
 * @brief Public aggregation header for the WebRPC module.
 *
 * @details
 * Include this header to access the full WebRPC surface:
 * - request / response envelopes
 * - structured errors
 * - execution context
 * - router (method registry + dispatch)
 * - dispatcher (single call + batch handling)
 *
 * WebRPC is transport-agnostic by design. Transport adapters (HTTP/WebSocket/P2P)
 * are expected to live above this module.
 *
 * @code
 * #include <vix/webrpc/webrpc.hpp>
 * @endcode
 */

// Core data model
#include <vix/webrpc/Error.hpp>
#include <vix/webrpc/Request.hpp>
#include <vix/webrpc/Response.hpp>

// Execution
#include <vix/webrpc/Context.hpp>
#include <vix/webrpc/Router.hpp>
#include <vix/webrpc/Dispatcher.hpp>

#endif // VIX_WEBRPC_WEBRPC_HPP
