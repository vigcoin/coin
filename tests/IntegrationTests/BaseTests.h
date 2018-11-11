// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <gtest/gtest.h>
#include <future>

#include <logging/ConsoleLogger.h>
#include <system/Dispatcher.h>
#include "cryptonote/core/currency.h"

#include "../IntegrationTestLib/TestNetwork.h"

namespace Tests {

class BaseTest : public testing::Test {
public:

  BaseTest() :
    currency(cryptonote::CurrencyBuilder(os::appdata::path(), config::testnet::data, logger)
    // .testnet(true)
    .currency()),
    network(dispatcher, currency) {
  }

protected:

  virtual void TearDown() override {
    network.shutdown();
  }

  System::Dispatcher& getDispatcher() {
    return dispatcher;
  }

  System::Dispatcher dispatcher;
  Logging::ConsoleLogger logger;
  cryptonote::Currency currency;
  TestNetwork network;
};

}
