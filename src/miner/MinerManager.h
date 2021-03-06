// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <queue>

#include <system/ContextGroup.h>
#include <system/Event.h>

#include "BlockchainMonitor.h"
#include "logging/LoggerRef.h"
#include "Miner.h"
#include "MinerEvent.h"
#include "MiningConfig.h"

namespace System {
class Dispatcher;
}

namespace Miner {

class MinerManager {
public:
  MinerManager(System::Dispatcher& dispatcher, const cryptonote::MiningConfig& config, Logging::ILogger& logger);
  ~MinerManager();

  void start();

private:
  System::Dispatcher& m_dispatcher;
  Logging::LoggerRef m_logger;
  System::ContextGroup m_contextGroup;
  cryptonote::MiningConfig m_config;
  cryptonote::Miner m_miner;
  BlockchainMonitor m_blockchainMonitor;

  System::Event m_eventOccurred;
  System::Event m_httpEvent;
  std::queue<MinerEvent> m_events;

  cryptonote::block_t m_minedBlock;

  uint64_t m_lastBlockTimestamp;

  void eventLoop();
  MinerEvent waitEvent();
  void pushEvent(MinerEvent&& event);

  void startMining(const cryptonote::BlockMiningParameters& params);
  void stopMining();

  void startBlockchainMonitoring();
  void stopBlockchainMonitoring();

  bool submitBlock(const cryptonote::block_t& minedBlock, const std::string& daemonHost, uint16_t daemonPort);
  cryptonote::BlockMiningParameters requestMiningParameters(System::Dispatcher& dispatcher, const std::string& daemonHost, uint16_t daemonPort, const std::string& miningAddress);

  void adjustBlockTemplate(cryptonote::block_t& blockTemplate) const;
};

} //namespace Miner
