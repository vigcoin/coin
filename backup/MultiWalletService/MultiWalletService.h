// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <fstream>

#include "ConfigurationManager.h"
#include "GateConfiguration.h"

#include "logging/ConsoleLogger.h"
#include "logging/LoggerGroup.h"
#include "logging/StreamLogger.h"

#include "payment_gate/NodeFactory.h"

#include "cryptonote/core/currency.h"
#include "system/Event.h"
#include "WalletInterface.h"

#include <system/ContextGroup.h>

using namespace std;

namespace MultiWalletService
{
class MultiWallet
{
public:
  MultiWallet() : dispatcher(nullptr), stopEvent(nullptr), config(), logger(), currencyBuilder(logger)
  {
  }

  bool init(int argc, char **argv);

  const MultiWalletService::ConfigurationManager &getConfig() const { return config; }
  const cryptonote::Currency getCurrency();

  void run();
  void stop();

  Logging::ILogger &getLogger() { return logger; }

private:
  void runRpcProxy(Logging::LoggerRef &log);

  void runWalletService(const cryptonote::Currency &currency, cryptonote::INode &node, Logging::LoggerRef &log);

  System::Dispatcher *dispatcher;
  System::Event *stopEvent;
  MultiWalletService::ConfigurationManager config;
  cryptonote::CurrencyBuilder currencyBuilder;

  Logging::LoggerGroup logger;
  ofstream fileStream;
  Logging::StreamLogger fileLogger;
  Logging::ConsoleLogger consoleLogger;
};
} // namespace MultiWalletService
