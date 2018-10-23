// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

#include "Chaingen.h"

struct GetRandomOutputs : public test_chain_unit_base
{
  GetRandomOutputs();

  // bool check_TxVerificationContext(const cryptonote::TxVerificationContext& tvc, bool tx_added, size_t event_idx, const cryptonote::Transaction& tx);
  // bool check_BlockVerificationContext(const cryptonote::BlockVerificationContext& bvc, size_t event_idx, const cryptonote::Block& block);
  // bool mark_last_valid_block(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  bool generate(std::vector<test_event_entry>& events) const;


private:

  bool checkHalfUnlocked(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool checkFullyUnlocked(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  bool request(cryptonote::core& c, uint64_t amount, size_t mixin, cryptonote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response& resp);

};
