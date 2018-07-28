// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "MultiWalletService.h"

#include <future>

#include "Common/SignalHandler.h"
#include "InProcessNode/InProcessNode.h"
#include "Logging/LoggerRef.h"
#include "MultiServiceJsonRpcServer.h"

#include "CryptoNoteCore/CoreConfig.h"
#include "CryptoNoteCore/Core.h"
#include "CryptoNoteProtocol/CryptoNoteProtocolHandler.h"
#include "P2p/NetNode.h"
#include "PaymentGate/WalletFactory.h"
#include <System/Context.h>

#include <iostream>

#ifdef ERROR
#undef ERROR
#endif

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

using namespace PaymentService;
using namespace std;

namespace MultiWalletService
{

void changeDirectory(const std::string &path)
{
  if (chdir(path.c_str()))
  {
    throw std::runtime_error("Couldn't change directory to \'" + path + "\': " + strerror(errno));
  }
}

void stopSignalHandler(PaymentGateService *pg)
{
  pg->stop();
}

bool PaymentGateService::init(int argc, char **argv)
{
  if (!config.init(argc, argv))
  {
    return false;
  }

  logger.setMaxLevel(static_cast<Logging::Level>(config.gateConfiguration.logLevel));
  logger.addLogger(consoleLogger);

  Logging::LoggerRef log(logger, "main");

  if (config.gateConfiguration.testnet)
  {
    log(Logging::INFO) << "Starting in testnet mode";
    currencyBuilder.testnet(true);
  }

  if (!config.gateConfiguration.serverRoot.empty())
  {
    changeDirectory(config.gateConfiguration.serverRoot);
    log(Logging::INFO) << "Current working directory now is " << config.gateConfiguration.serverRoot;
  }

  fileStream.open(config.gateConfiguration.logFile, std::ofstream::app);

  if (!fileStream)
  {
    throw std::runtime_error("Couldn't open log file");
  }

  fileLogger.attachToStream(fileStream);
  logger.addLogger(fileLogger);

  return true;
}

const CryptoNote::Currency PaymentGateService::getCurrency()
{
  return currencyBuilder.currency();
}

void PaymentGateService::run()
{

  System::Dispatcher localDispatcher;
  System::Event localStopEvent(localDispatcher);

  this->dispatcher = &localDispatcher;
  this->stopEvent = &localStopEvent;

  Tools::SignalHandler::install(std::bind(&stopSignalHandler, this));

  Logging::LoggerRef log(logger, "run");

  runRpcProxy(log);

  this->dispatcher = nullptr;
  this->stopEvent = nullptr;
}

void PaymentGateService::stop()
{
  Logging::LoggerRef log(logger, "stop");

  log(Logging::INFO) << "Stop signal caught";

  if (dispatcher != nullptr)
  {
    dispatcher->remoteSpawn([&]() {
      if (stopEvent != nullptr)
      {
        stopEvent->set();
      }
    });
  }
}

void PaymentGateService::runRpcProxy(Logging::LoggerRef &log)
{
  log(Logging::INFO) << "Starting Payment Gate with remote node";
  CryptoNote::Currency currency = currencyBuilder.currency();

  std::unique_ptr<CryptoNote::INode> node(
      PaymentService::NodeFactory::createNode(
          config.remoteNodeConfig.daemonHost,
          config.remoteNodeConfig.daemonPort));

  runWalletService(currency, *node);
}

void PaymentGateService::runWalletService(const CryptoNote::Currency &currency, CryptoNote::INode &node)
{
  MultiWalletService::MultiServiceJsonRpcServer rpcServer(*dispatcher, *stopEvent, logger);
  rpcServer.start(config.gateConfiguration.bindAddress, config.gateConfiguration.bindPort);
}

} // namespace MultiWalletService