// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "RPCTestNode.h"

#include <future>
#include <vector>
#include <thread>

#include "common/StringTools.h"
#include "cryptonote/core/CryptoNoteTools.h"
#include "cryptonote/structures/array.hpp"
#include "NodeRpcProxy/NodeRpcProxy.h"
#include "rpc/CoreRpcServerCommandsDefinitions.h"
#include "rpc/HttpClient.h"
#include "rpc/JsonRpc.h"

#include "Logger.h"
#include "NodeCallback.h"

using namespace cryptonote;
using namespace System;

namespace Tests {

RPCTestNode::RPCTestNode(uint16_t port, System::Dispatcher& d) : 
  m_rpcPort(port), m_dispatcher(d), m_httpClient(d, "127.0.0.1", port) {
}

bool RPCTestNode::startMining(size_t threadsCount, const std::string& address) { 
  LOG_DEBUG("startMining()");

  try {
    COMMAND_RPC_START_MINING::request req;
    COMMAND_RPC_START_MINING::response resp;
    req.miner_address = address;
    req.threads_count = threadsCount;

    invokeJsonCommand(m_httpClient, "/start_mining", req, resp);
    if (resp.status != CORE_RPC_STATUS_OK) {
      throw std::runtime_error(resp.status);
    }
  } catch (std::exception& e) {
    std::cout << "startMining() RPC call fail: " << e.what();
    return false;
  }

  return true;
}

bool RPCTestNode::getBlockTemplate(const std::string& minerAddress, cryptonote::block_t& blockTemplate, uint64_t& difficulty) {
  LOG_DEBUG("getBlockTemplate()");

  try {
    COMMAND_RPC_GETBLOCKTEMPLATE::request req;
    COMMAND_RPC_GETBLOCKTEMPLATE::response rsp;
    req.wallet_address = minerAddress;
    req.reserve_size = 0;

    JsonRpc::invokeJsonRpcCommand(m_httpClient, "getblocktemplate", req, rsp);
    if (rsp.status != CORE_RPC_STATUS_OK) {
      throw std::runtime_error(rsp.status);
    }

    difficulty = rsp.difficulty;

    binary_array_t blockBlob = (::hex::from(rsp.blocktemplate_blob));
    return BinaryArray::from(blockTemplate, blockBlob);
  } catch (std::exception& e) {
    LOG_ERROR("JSON-RPC call startMining() failed: " + std::string(e.what()));
    return false;
  }

  return true;
}

bool RPCTestNode::submitBlock(const std::string& block) {
  LOG_DEBUG("submitBlock()");

  try {
    COMMAND_RPC_SUBMITBLOCK::request req;
    COMMAND_RPC_SUBMITBLOCK::response res;
    req.push_back(block);
    JsonRpc::invokeJsonRpcCommand(m_httpClient, "submitblock", req, res);
    if (res.status != CORE_RPC_STATUS_OK) {
      throw std::runtime_error(res.status);
    }
  } catch (std::exception& e) {
    LOG_ERROR("RPC call of submit_block returned error: " + std::string(e.what()));
    return false;
  }

  return true;
}

bool RPCTestNode::stopMining() { 
  LOG_DEBUG("stopMining()");

  try {
    COMMAND_RPC_STOP_MINING::request req;
    COMMAND_RPC_STOP_MINING::response resp;
    invokeJsonCommand(m_httpClient, "/stop_mining", req, resp);
    if (resp.status != CORE_RPC_STATUS_OK) {
      throw std::runtime_error(resp.status);
    }
  } catch (std::exception& e) {
    std::cout << "stopMining() RPC call fail: " << e.what();
    return false;
  }

  return true;
}

bool RPCTestNode::getTailBlockId(hash_t& tailBlockId) {
  LOG_DEBUG("getTailBlockId()");

  try {
    COMMAND_RPC_GET_LAST_BLOCK_HEADER::request req;
    COMMAND_RPC_GET_LAST_BLOCK_HEADER::response rsp;
    JsonRpc::invokeJsonRpcCommand(m_httpClient, "getlastblockheader", req, rsp);
    if (rsp.status != CORE_RPC_STATUS_OK) {
      throw std::runtime_error(rsp.status);
    }

    return ::hex::podFrom(rsp.block_header.hash, tailBlockId);
  } catch (std::exception& e) {
    LOG_ERROR("JSON-RPC call getTailBlockId() failed: " + std::string(e.what()));
    return false;
  }

  return true;
}

bool RPCTestNode::makeINode(std::unique_ptr<cryptonote::INode>& node) {
  std::unique_ptr<cryptonote::INode> newNode(new cryptonote::NodeRpcProxy("127.0.0.1", m_rpcPort));
  NodeCallback cb;
  newNode->init(cb.callback());
  auto ec = cb.get();

  if (ec) {
    LOG_ERROR("init error: " + ec.message() + ':' + TO_STRING(ec.value()));
    return false;
  }

  LOG_DEBUG("NodeRPCProxy on port " + TO_STRING(m_rpcPort) + " initialized");
  node = std::move(newNode);
  return true;
}

bool RPCTestNode::stopDaemon() {
  try {
    LOG_DEBUG("stopDaemon()");
    COMMAND_RPC_STOP_DAEMON::request req;
    COMMAND_RPC_STOP_DAEMON::response resp;
    invokeJsonCommand(m_httpClient, "/stop_daemon", req, resp);
    if (resp.status != CORE_RPC_STATUS_OK) {
      throw std::runtime_error(resp.status);
    }
  } catch (std::exception& e) {
    std::cout << "stopDaemon() RPC call fail: " << e.what();
    return false;
  }

  return true;
}

uint64_t RPCTestNode::getLocalHeight() {
  try {
    cryptonote::COMMAND_RPC_GET_INFO::request req;
    cryptonote::COMMAND_RPC_GET_INFO::response rsp;
    invokeJsonCommand(m_httpClient, "/getinfo", req, rsp);
    if (rsp.status == CORE_RPC_STATUS_OK) {
      return rsp.height;
    }
  } catch (std::exception&) {
  }

  return 0;
}

}
