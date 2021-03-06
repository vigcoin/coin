// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "version.h"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "DaemonCommandsHandler.h"

#include "common/SignalHandler.h"
#include "cryptonote/crypto/hash.h"
#include "cryptonote/core/core.h"
#include "command_line/daemon.h"
#include "command_line/CoreConfig.h"
#include "cryptonote/core/CryptoNoteTools.h"
#include "cryptonote/core/currency.h"
#include "command_line/MinerConfig.h"
#include "cryptonote/protocol/handler.h"
#include "p2p/NetNode.h"
#include "command_line/NetNodeConfig.h"
#include "rpc/RpcServer.h"
#include "command_line/RpcServerConfig.h"
#include "version.h"
#include "cryptonote/structures/array.hpp"

#include "logging/ConsoleLogger.h"
#include <logging/LoggerManager.h>

#if defined(WIN32)
#include <crtdbg.h>
#endif

using Common::JsonValue;
using namespace command_line;
using namespace cryptonote;
using namespace Logging;

namespace fs = boost::filesystem;

JsonValue buildLoggerConfiguration(Level level, const std::string &logfile)
{
  JsonValue loggerConfiguration(JsonValue::OBJECT);
  loggerConfiguration.insert("globalLevel", static_cast<int64_t>(level));

  JsonValue &cfgLoggers = loggerConfiguration.insert("loggers", JsonValue::ARRAY);

  JsonValue &fileLogger = cfgLoggers.pushBack(JsonValue::OBJECT);
  fileLogger.insert("type", "file");
  fileLogger.insert("filename", logfile);
  fileLogger.insert("level", static_cast<int64_t>(TRACE));

  JsonValue &consoleLogger = cfgLoggers.pushBack(JsonValue::OBJECT);
  consoleLogger.insert("type", "console");
  consoleLogger.insert("level", static_cast<int64_t>(TRACE));
  consoleLogger.insert("pattern", "%T %L ");

  return loggerConfiguration;
}

int main(int argc, char *argv[])
{

#ifdef WIN32
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  LoggerManager logManager;
  LoggerRef logger(logManager, "daemon");

  OptionsNames names;
  names.command = "Command line options";
  names.setting = "Command line options and settings options";
  names.full = "Allowed options";
  Daemon cli(names);

  try
  {
    cli.init();

    po::options_description &desc_cmd_sett = cli.desc_cmd_sett;
    RpcServerConfig::initOptions(desc_cmd_sett);
    // CoreConfig::initOptions(desc_cmd_sett);
    NetNodeConfig::initOptions(desc_cmd_sett);
    MinerConfig::initOptions(desc_cmd_sett);

    cli.setup();

    po::variables_map &vm = cli.vm;
    bool r = cli.parse(argc, argv, [&]() {
      return true;
    });

    if (!r)
    {
      std::cerr << "Command parsing error!" << std::endl;
      return 1;
    }
    bool testnet_mode = get_arg(vm, arg_testnet_on);

    std::cout << "test net = " << testnet_mode << std::endl;

    if (testnet_mode)
    {
      config::setType(testnet_mode ? config::TESTNET : config::MAINNET);
      logger(INFO) << "Starting in testnet mode!";
    }

    CoreConfig coreConfig;
    coreConfig.init(vm);
    NetNodeConfig netNodeConfig;
    netNodeConfig.init(vm);
    MinerConfig minerConfig;
    minerConfig.init(vm);
    RpcServerConfig rpcConfig;
    rpcConfig.init(vm);

    coreConfig.checkDataDir();
    cli.parseConfigFile();

    auto cfgLogFile = cli.getLogFile();

    Level cfgLogLevel = static_cast<Level>(static_cast<int>(Logging::ERROR) + cli.get(arg_log_level));

    // configure logging
    logManager.configure(buildLoggerConfiguration(cfgLogLevel, cfgLogFile.string()));

    logger(INFO) << config::get().name << " v" << PROJECT_VERSION_LONG;

    if (cli.checkVersion())
    {
      return 0;
    }

    logger(INFO) << "Module folder: " << argv[0];



    //create objects and link them
    cryptonote::CurrencyBuilder currencyBuilder(coreConfig.getDir(), config::get(), logManager);

    try
    {
      currencyBuilder.currency();
    }
    catch (std::exception &)
    {
      std::cout << "GENESIS_COINBASE_TX_HEX constant has an incorrect value. Please launch: " << config::get().name << "d --" << arg_print_genesis_tx.name;
      return 1;
    }

    cryptonote::Currency currency = currencyBuilder.currency();
    cryptonote::core ccore(currency, nullptr, logManager);

    cryptonote::Checkpoints checkpoints(logManager);
    for (const auto &cp : config::get().checkpoints)
    {
      checkpoints.add(cp.height, cp.blockId);
    }


    ccore.set_checkpoints(std::move(checkpoints));

    System::Dispatcher dispatcher;

    cryptonote::CryptoNoteProtocolHandler cprotocol(currency, dispatcher, ccore, nullptr, logManager);
    cryptonote::NodeServer p2psrv(dispatcher, cprotocol, logManager);
    cryptonote::RpcServer rpcServer(dispatcher, logManager, ccore, p2psrv, cprotocol);

    cprotocol.set_p2p_endpoint(&p2psrv);
    ccore.set_cryptonote_protocol(&cprotocol);
    DaemonCommandsHandler dch(ccore, p2psrv, logManager);

    // initialize objects
    logger(INFO) << "Initializing p2p server...";
    if (!p2psrv.init(netNodeConfig))
    {
      logger(ERROR, BRIGHT_RED) << "Failed to initialize p2p server.";
      return 1;
    }
    logger(INFO) << "P2p server initialized OK";

    //logger(INFO) << "Initializing core rpc server...";
    //if (!rpc_server.init(vm)) {
    //  logger(ERROR, BRIGHT_RED) << "Failed to initialize core rpc server.";
    //  return 1;
    //}
    // logger(INFO, BRIGHT_GREEN) << "Core rpc server initialized OK on port: " << rpc_server.get_binded_port();

    // initialize core here
    logger(INFO) << "Initializing core...";
    if (!ccore.init(minerConfig, true))
    {
      logger(ERROR, BRIGHT_RED) << "Failed to initialize core";
      return 1;
    }
    logger(INFO) << "Core initialized OK";

    // start components
    if (!has_arg(vm, arg_console))
    {
      dch.start_handling();
    }

    logger(INFO) << "Starting core rpc server on address " << rpcConfig.getBindAddress();
    rpcServer.start(rpcConfig.bindIp, rpcConfig.bindPort);
    rpcServer.enableCors(rpcConfig.enableCORS);
    logger(INFO) << "Core rpc server started ok";

    Tools::SignalHandler::install([&dch, &p2psrv] {
      dch.stop_handling();
      p2psrv.sendStopSignal();
    });

    logger(INFO) << "Starting p2p net loop...";
    p2psrv.run();
    logger(INFO) << "p2p net loop stopped";

    dch.stop_handling();

    //stop components
    logger(INFO) << "Stopping core rpc server...";
    rpcServer.stop();

    //deinitialize components
    logger(INFO) << "Deinitializing core...";
    ccore.deinit();
    logger(INFO) << "Deinitializing p2p...";
    p2psrv.deinit();

    ccore.set_cryptonote_protocol(NULL);
    cprotocol.set_p2p_endpoint(NULL);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Exception catched: " << e.what() << std::endl;
    return 1;
  }

  logger(INFO) << "Node stopped.";
  return 0;
}
