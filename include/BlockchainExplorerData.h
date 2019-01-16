// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <array>
#include <string>
#include <vector>

#include "crypto.h"

#include <boost/variant.hpp>

namespace cryptonote {

enum class transaction_remove_reason_t : uint8_t 
{ 
  INCLUDED_IN_BLOCK = 0, 
  TIMEOUT = 1
};

struct transaction_output_to_key_details_t {
  crypto::public_key_t txOutKey;
};

struct transaction_output_multi_signature_details_t {
  std::vector<crypto::public_key_t> keys;
  uint32_t requiredSignatures;
};

struct transaction_output_details_t {
  uint64_t amount;
  uint32_t globalIndex;

  boost::variant<
    transaction_output_to_key_details_t,
    transaction_output_multi_signature_details_t> output;
};

struct transaction_output_reference_details_t {
  crypto::hash_t transactionHash;
  size_t number;
};

struct TransactionInputGenerateDetails {
  uint32_t height;
};

struct TransactionInputToKeyDetails {
  std::vector<uint32_t> outputIndexes;
  crypto::key_image_t keyImage;
  uint64_t mixin;
  transaction_output_reference_details_t output;
};

struct TransactionInputMultisignatureDetails {
  uint32_t signatures;
  transaction_output_reference_details_t output;
};

struct transaction_input_details_t {
  uint64_t amount;

  boost::variant<
    TransactionInputGenerateDetails,
    TransactionInputToKeyDetails,
    TransactionInputMultisignatureDetails> input;
};

struct transaction_extra_details_t {
  std::vector<size_t> padding;
  std::vector<crypto::public_key_t> publicKey; 
  std::vector<std::string> nonce;
  std::vector<uint8_t> raw;
};

struct transaction_details_t {
  crypto::hash_t hash;
  uint64_t size;
  uint64_t fee;
  uint64_t totalInputsAmount;
  uint64_t totalOutputsAmount;
  uint64_t mixin;
  uint64_t unlockTime;
  uint64_t timestamp;
  crypto::hash_t paymentId;
  bool inBlockchain;
  crypto::hash_t blockHash;
  uint32_t blockHeight;
  transaction_extra_details_t extra;
  std::vector<std::vector<crypto::signature_t>> signatures;
  std::vector<transaction_input_details_t> inputs;
  std::vector<transaction_output_details_t> outputs;
};

struct block_details_t {
  uint8_t majorVersion;
  uint8_t minorVersion;
  uint64_t timestamp;
  crypto::hash_t prevBlockHash;
  uint32_t nonce;
  bool isOrphaned;
  uint32_t height;
  crypto::hash_t hash;
  uint64_t difficulty;
  uint64_t reward;
  uint64_t baseReward;
  uint64_t blockSize;
  uint64_t transactionsCumulativeSize;
  uint64_t alreadyGeneratedCoins;
  uint64_t alreadyGeneratedTransactions;
  uint64_t sizeMedian;
  double penalty;
  uint64_t totalFeeAmount;
  std::vector<transaction_details_t> transactions;
};

}
