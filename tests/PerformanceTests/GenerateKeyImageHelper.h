// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "cryptonote/core/key.h"
#include "cryptonote/core/CryptoNoteFormatUtils.h"

#include "SingleTransactionTestBase.h"

class test_generate_key_image_helper : public single_tx_test_base
{
public:
  static const size_t loop_count = 500;

  bool test()
  {
    cryptonote::key_pair_t in_ephemeral;
    key_image_t ki;
    return cryptonote::generate_key_image_helper(m_bob.getAccountKeys(), m_tx_pub_key, 0, in_ephemeral, ki);
  }
};
