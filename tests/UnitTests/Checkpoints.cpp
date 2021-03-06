// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "cryptonote/core/checkpoints.h"
#include <logging/LoggerGroup.h>

using namespace cryptonote;

TEST(checkpoints_isAllowed, handles_empty_checkpoins)
{
  Logging::LoggerGroup logger;
  Checkpoints cp(logger);

  ASSERT_FALSE(cp.isAllowed(0, 0));

  ASSERT_TRUE(cp.isAllowed(1, 1));
  ASSERT_TRUE(cp.isAllowed(1, 9));
  ASSERT_TRUE(cp.isAllowed(9, 1));
}

TEST(checkpoints_isAllowed, handles_one_checkpoint)
{
  Logging::LoggerGroup logger;
  Checkpoints cp(logger);
  cp.add(5, "0000000000000000000000000000000000000000000000000000000000000000");

  ASSERT_FALSE(cp.isAllowed(0, 0));

  ASSERT_TRUE (cp.isAllowed(1, 1));
  ASSERT_TRUE (cp.isAllowed(1, 4));
  ASSERT_TRUE (cp.isAllowed(1, 5));
  ASSERT_TRUE (cp.isAllowed(1, 6));
  ASSERT_TRUE (cp.isAllowed(1, 9));

  ASSERT_TRUE (cp.isAllowed(4, 1));
  ASSERT_TRUE (cp.isAllowed(4, 4));
  ASSERT_TRUE (cp.isAllowed(4, 5));
  ASSERT_TRUE (cp.isAllowed(4, 6));
  ASSERT_TRUE (cp.isAllowed(4, 9));

  ASSERT_FALSE(cp.isAllowed(5, 1));
  ASSERT_FALSE(cp.isAllowed(5, 4));
  ASSERT_FALSE(cp.isAllowed(5, 5));
  ASSERT_TRUE (cp.isAllowed(5, 6));
  ASSERT_TRUE (cp.isAllowed(5, 9));

  ASSERT_FALSE(cp.isAllowed(6, 1));
  ASSERT_FALSE(cp.isAllowed(6, 4));
  ASSERT_FALSE(cp.isAllowed(6, 5));
  ASSERT_TRUE (cp.isAllowed(6, 6));
  ASSERT_TRUE (cp.isAllowed(6, 9));

  ASSERT_FALSE(cp.isAllowed(9, 1));
  ASSERT_FALSE(cp.isAllowed(9, 4));
  ASSERT_FALSE(cp.isAllowed(9, 5));
  ASSERT_TRUE (cp.isAllowed(9, 6));
  ASSERT_TRUE (cp.isAllowed(9, 9));
}

TEST(checkpoints_isAllowed, handles_two_and_more_checkpoints)
{
  Logging::LoggerGroup logger;
  Checkpoints cp(logger);
  cp.add(5, "0000000000000000000000000000000000000000000000000000000000000000");
  cp.add(9, "0000000000000000000000000000000000000000000000000000000000000000");

  ASSERT_FALSE(cp.isAllowed(0, 0));

  ASSERT_TRUE (cp.isAllowed(1, 1));
  ASSERT_TRUE (cp.isAllowed(1, 4));
  ASSERT_TRUE (cp.isAllowed(1, 5));
  ASSERT_TRUE (cp.isAllowed(1, 6));
  ASSERT_TRUE (cp.isAllowed(1, 8));
  ASSERT_TRUE (cp.isAllowed(1, 9));
  ASSERT_TRUE (cp.isAllowed(1, 10));
  ASSERT_TRUE (cp.isAllowed(1, 11));

  ASSERT_TRUE (cp.isAllowed(4, 1));
  ASSERT_TRUE (cp.isAllowed(4, 4));
  ASSERT_TRUE (cp.isAllowed(4, 5));
  ASSERT_TRUE (cp.isAllowed(4, 6));
  ASSERT_TRUE (cp.isAllowed(4, 8));
  ASSERT_TRUE (cp.isAllowed(4, 9));
  ASSERT_TRUE (cp.isAllowed(4, 10));
  ASSERT_TRUE (cp.isAllowed(4, 11));

  ASSERT_FALSE(cp.isAllowed(5, 1));
  ASSERT_FALSE(cp.isAllowed(5, 4));
  ASSERT_FALSE(cp.isAllowed(5, 5));
  ASSERT_TRUE (cp.isAllowed(5, 6));
  ASSERT_TRUE (cp.isAllowed(5, 8));
  ASSERT_TRUE (cp.isAllowed(5, 9));
  ASSERT_TRUE (cp.isAllowed(5, 10));
  ASSERT_TRUE (cp.isAllowed(5, 11));

  ASSERT_FALSE(cp.isAllowed(6, 1));
  ASSERT_FALSE(cp.isAllowed(6, 4));
  ASSERT_FALSE(cp.isAllowed(6, 5));
  ASSERT_TRUE (cp.isAllowed(6, 6));
  ASSERT_TRUE (cp.isAllowed(6, 8));
  ASSERT_TRUE (cp.isAllowed(6, 9));
  ASSERT_TRUE (cp.isAllowed(6, 10));
  ASSERT_TRUE (cp.isAllowed(6, 11));

  ASSERT_FALSE(cp.isAllowed(8, 1));
  ASSERT_FALSE(cp.isAllowed(8, 4));
  ASSERT_FALSE(cp.isAllowed(8, 5));
  ASSERT_TRUE (cp.isAllowed(8, 6));
  ASSERT_TRUE (cp.isAllowed(8, 8));
  ASSERT_TRUE (cp.isAllowed(8, 9));
  ASSERT_TRUE (cp.isAllowed(8, 10));
  ASSERT_TRUE (cp.isAllowed(8, 11));

  ASSERT_FALSE(cp.isAllowed(9, 1));
  ASSERT_FALSE(cp.isAllowed(9, 4));
  ASSERT_FALSE(cp.isAllowed(9, 5));
  ASSERT_FALSE(cp.isAllowed(9, 6));
  ASSERT_FALSE(cp.isAllowed(9, 8));
  ASSERT_FALSE(cp.isAllowed(9, 9));
  ASSERT_TRUE (cp.isAllowed(9, 10));
  ASSERT_TRUE (cp.isAllowed(9, 11));

  ASSERT_FALSE(cp.isAllowed(10, 1));
  ASSERT_FALSE(cp.isAllowed(10, 4));
  ASSERT_FALSE(cp.isAllowed(10, 5));
  ASSERT_FALSE(cp.isAllowed(10, 6));
  ASSERT_FALSE(cp.isAllowed(10, 8));
  ASSERT_FALSE(cp.isAllowed(10, 9));
  ASSERT_TRUE (cp.isAllowed(10, 10));
  ASSERT_TRUE (cp.isAllowed(10, 11));

  ASSERT_FALSE(cp.isAllowed(11, 1));
  ASSERT_FALSE(cp.isAllowed(11, 4));
  ASSERT_FALSE(cp.isAllowed(11, 5));
  ASSERT_FALSE(cp.isAllowed(11, 6));
  ASSERT_FALSE(cp.isAllowed(11, 8));
  ASSERT_FALSE(cp.isAllowed(11, 9));
  ASSERT_TRUE (cp.isAllowed(11, 10));
  ASSERT_TRUE (cp.isAllowed(11, 11));
}
