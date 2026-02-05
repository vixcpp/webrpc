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
 * ====================================================================
 * Vix.cpp - WebRPC
 * ====================================================================
 * Purpose:
 *   Public aggregation header for the WebRPC module.
 *
 *   Include this file to access:
 *   - RPC request / response envelopes
 *   - Structured errors
 *   - Execution context
 *   - Method router
 *   - Dispatcher (object + batch)
 *
 *   Transport adapters (HTTP / WS) live outside this module.
 * ====================================================================
 */

#ifndef VIX_WEBRPC_WEBRPC_HPP
#define VIX_WEBRPC_WEBRPC_HPP

// --------------------------------------------------------------------
// Core data model
// --------------------------------------------------------------------

#include <vix/webrpc/Error.hpp>
#include <vix/webrpc/Request.hpp>
#include <vix/webrpc/Response.hpp>

// --------------------------------------------------------------------
// Execution
// --------------------------------------------------------------------

#include <vix/webrpc/Context.hpp>
#include <vix/webrpc/Router.hpp>
#include <vix/webrpc/Dispatcher.hpp>

#endif // VIX_WEBRPC_WEBRPC_HPP
