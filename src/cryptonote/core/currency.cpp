// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>
#include <fstream>
#include "currency.h"
#include <cctype>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include "common/base58.h"
#include "common/int-util.h"
#include "common/StringTools.h"
#include "cryptonote/structures/block.h"

#include "account.h"
#include "CryptoNoteFormatUtils.h"
#include "CryptoNoteTools.h"
#include "TransactionExtra.h"
#include "../structures/block_entry.h"
#include "cryptonote/structures/array.hpp"
#include "cryptonote/core/util/amount.h"
#include "difficulty.h"

#undef ERROR

using namespace Logging;
using namespace Common;

namespace cryptonote
{
const std::vector<uint64_t> Currency::PRETTY_AMOUNTS = {
    1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 20, 30, 40, 50, 60, 70, 80, 90,
    100, 200, 300, 400, 500, 600, 700, 800, 900,
    1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
    10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000,
    100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000,
    1000000, 2000000, 3000000, 4000000, 5000000, 6000000, 7000000, 8000000, 9000000,
    10000000, 20000000, 30000000, 40000000, 50000000, 60000000, 70000000, 80000000, 90000000,
    100000000, 200000000, 300000000, 400000000, 500000000, 600000000, 700000000, 800000000, 900000000,
    1000000000, 2000000000, 3000000000, 4000000000, 5000000000, 6000000000, 7000000000, 8000000000, 9000000000,
    10000000000, 20000000000, 30000000000, 40000000000, 50000000000, 60000000000, 70000000000, 80000000000, 90000000000,
    100000000000, 200000000000, 300000000000, 400000000000, 500000000000, 600000000000, 700000000000, 800000000000, 900000000000,
    1000000000000, 2000000000000, 3000000000000, 4000000000000, 5000000000000, 6000000000000, 7000000000000, 8000000000000, 9000000000000,
    10000000000000, 20000000000000, 30000000000000, 40000000000000, 50000000000000, 60000000000000, 70000000000000, 80000000000000, 90000000000000,
    100000000000000, 200000000000000, 300000000000000, 400000000000000, 500000000000000, 600000000000000, 700000000000000, 800000000000000, 900000000000000,
    1000000000000000, 2000000000000000, 3000000000000000, 4000000000000000, 5000000000000000, 6000000000000000, 7000000000000000, 8000000000000000, 9000000000000000,
    10000000000000000, 20000000000000000, 30000000000000000, 40000000000000000, 50000000000000000, 60000000000000000, 70000000000000000, 80000000000000000, 90000000000000000,
    100000000000000000, 200000000000000000, 300000000000000000, 400000000000000000, 500000000000000000, 600000000000000000, 700000000000000000, 800000000000000000, 900000000000000000,
    1000000000000000000, 2000000000000000000, 3000000000000000000, 4000000000000000000, 5000000000000000000, 6000000000000000000, 7000000000000000000, 8000000000000000000, 9000000000000000000,
    10000000000000000000ull};

bool Currency::init()
{
  if (!generateGenesisBlock())
  {
    logger(ERROR, BRIGHT_RED) << "Failed to generate genesis block";
    return false;
  }

  if (!Block::getHash(m_genesisBlock, m_genesisBlockHash))
  {
    logger(ERROR, BRIGHT_RED) << "Failed to get genesis block hash";
    return false;
  }

  if (!getPathAndFilesReady())
  {
    logger(ERROR, BRIGHT_RED) << "Failed to get directory working.";

    return false;
  }

  return true;
}

bool Currency::getPathAndFilesReady()
{
  if (!boost::filesystem::exists(m_path))
  {
    std::cout << "Dir: `" << m_path << "` not found! Creating..." << std::endl;
    if (!boost::filesystem::create_directory(m_path))
    {
      std::cout << "Failed to create dir: " << m_path << "." << std::endl;
      return false;
    }
    std::cout << "Success creating dir: " << m_path << "." << std::endl;
  }
  std::string files[] = {
      // blocksFileName(),
      // blocksCacheFileName(),
      // blockIndexesFileName(),
      // txPoolFileName(),
      blockchainIndexesFileName()};
  for (std::string file : files)
  {
    if (!boost::filesystem::exists(file))
    {
      std::cout << "Creating File: " << file << "." << std::endl;
      std::ofstream savefile(file);
      savefile.close();
    }
  }
  return true;
}

bool Currency::generateGenesisBlock()
{
  m_genesisBlock = Block::genesis(m_config);
  return true;
}

bool Currency::getBlockReward(size_t medianSize, size_t currentBlockSize, uint64_t alreadyGeneratedCoins,
                              uint64_t fee, uint64_t &reward, int64_t &emissionChange) const
{
  assert(alreadyGeneratedCoins <= m_moneySupply);
  assert(m_emissionSpeedFactor > 0 && m_emissionSpeedFactor <= 8 * sizeof(uint64_t));

  uint64_t baseReward = (m_moneySupply - alreadyGeneratedCoins) >> m_emissionSpeedFactor;

  if (alreadyGeneratedCoins == 0)
  {
    baseReward = 1;
  }

  if (alreadyGeneratedCoins == 1)
  {
    baseReward = m_moneySupply * 0.20;
  }

  if (alreadyGeneratedCoins + baseReward >= m_moneySupply)
  {
    baseReward = 0;
  }

  medianSize = std::max(medianSize, m_blockGrantedFullRewardZone);
  if (currentBlockSize > UINT64_C(2) * medianSize)
  {
    logger(TRACE) << "Block cumulative size is too big: " << currentBlockSize << ", expected less than " << 2 * medianSize;
    return false;
  }

  uint64_t penalizedBaseReward = get_penalized_amount(baseReward, medianSize, currentBlockSize);
  uint64_t penalizedFee = get_penalized_amount(fee, medianSize, currentBlockSize);

  emissionChange = penalizedBaseReward - (fee - penalizedFee);
  reward = penalizedBaseReward + penalizedFee;

  return true;
}

size_t Currency::maxBlockCumulativeSize(uint64_t height) const
{
  assert(height <= std::numeric_limits<uint64_t>::max() / m_maxBlockSizeGrowthSpeedNumerator);
  size_t maxSize = static_cast<size_t>(m_maxBlockSizeInitial +
                                       (height * m_maxBlockSizeGrowthSpeedNumerator) / m_maxBlockSizeGrowthSpeedDenominator);
  assert(maxSize >= m_maxBlockSizeInitial);
  return maxSize;
}

bool Currency::constructMinerTx(uint32_t height, size_t medianSize, uint64_t alreadyGeneratedCoins, size_t currentBlockSize,
                                uint64_t fee, const account_public_address_t &minerAddress, transaction_t &tx,
                                const binary_array_t &extraNonce /* = binary_array_t()*/, size_t maxOuts /* = 1*/) const
{
  tx.inputs.clear();
  tx.outputs.clear();
  tx.extra.clear();

  key_pair_t txkey = Key::generate();
  addTransactionPublicKeyToExtra(tx.extra, txkey.publicKey);
  if (!extraNonce.empty())
  {
    if (!addExtraNonceToTransactionExtra(tx.extra, extraNonce))
    {
      return false;
    }
  }

  base_input_t in;
  in.blockIndex = height;

  uint64_t blockReward;
  int64_t emissionChange;
  if (!getBlockReward(medianSize, currentBlockSize, alreadyGeneratedCoins, fee, blockReward, emissionChange))
  {
    logger(INFO) << "Block is too big";
    return false;
  }

  std::vector<uint64_t> outAmounts;
  decompose_amount_into_digits(blockReward, m_defaultDustThreshold,
                               [&outAmounts](uint64_t a_chunk) { outAmounts.push_back(a_chunk); },
                               [&outAmounts](uint64_t a_dust) { outAmounts.push_back(a_dust); });

  if (!(1 <= maxOuts))
  {
    logger(ERROR, BRIGHT_RED) << "max_out must be non-zero";
    return false;
  }
  while (maxOuts < outAmounts.size())
  {
    outAmounts[outAmounts.size() - 2] += outAmounts.back();
    outAmounts.resize(outAmounts.size() - 1);
  }

  uint64_t summaryAmounts = 0;
  for (size_t no = 0; no < outAmounts.size(); no++)
  {
    key_derivation_t derivation = boost::value_initialized<key_derivation_t>();
    public_key_t outEphemeralPubKey = boost::value_initialized<public_key_t>();

    bool r = generate_key_derivation((const uint8_t *)&minerAddress.viewPublicKey, (const uint8_t *)&txkey.secretKey, (uint8_t *)&derivation);

    if (!(r))
    {
      logger(ERROR, BRIGHT_RED)
          << "while creating outs: failed to generate_key_derivation("
          << minerAddress.viewPublicKey << ", " << txkey.secretKey << ")";
      return false;
    }

    r = derive_public_key((const uint8_t *)&derivation, no, (const uint8_t *)&minerAddress.spendPublicKey, (uint8_t *)&outEphemeralPubKey);

    if (!(r))
    {
      logger(ERROR, BRIGHT_RED)
          << "while creating outs: failed to derive_public_key("
          << derivation << ", " << no << ", "
          << minerAddress.spendPublicKey << ")";
      return false;
    }

    key_output_t tk;
    tk.key = outEphemeralPubKey;

    transaction_output_t out;
    summaryAmounts += out.amount = outAmounts[no];
    out.target = tk;
    tx.outputs.push_back(out);
  }

  if (!(summaryAmounts == blockReward))
  {
    logger(ERROR, BRIGHT_RED) << "Failed to construct miner tx, summaryAmounts = " << summaryAmounts << " not equal blockReward = " << blockReward;
    return false;
  }

  tx.version = config::get().transaction.version.major;
  //lock
  tx.unlockTime = height + m_minedMoneyUnlockWindow;
  tx.inputs.push_back(in);
  return true;
}

bool Currency::isFusionTransaction(const std::vector<uint64_t> &inputsAmounts, const std::vector<uint64_t> &outputsAmounts, size_t size) const
{
  if (size > fusionTxMaxSize())
  {
    return false;
  }

  if (inputsAmounts.size() < fusionTxMinInputCount())
  {
    return false;
  }

  if (inputsAmounts.size() < outputsAmounts.size() * fusionTxMinInOutCountRatio())
  {
    return false;
  }

  uint64_t inputAmount = 0;
  for (auto amount : inputsAmounts)
  {
    if (amount < defaultDustThreshold())
    {
      return false;
    }

    inputAmount += amount;
  }

  std::vector<uint64_t> expectedOutputsAmounts;
  expectedOutputsAmounts.reserve(outputsAmounts.size());
  decomposeAmount(inputAmount, defaultDustThreshold(), expectedOutputsAmounts);
  std::sort(expectedOutputsAmounts.begin(), expectedOutputsAmounts.end());

  return expectedOutputsAmounts == outputsAmounts;
}

bool Currency::isFusionTransaction(const transaction_t &transaction, size_t size) const
{
  assert(BinaryArray::size(transaction) == size);

  std::vector<uint64_t> outputsAmounts;
  outputsAmounts.reserve(transaction.outputs.size());
  for (const transaction_output_t &output : transaction.outputs)
  {
    outputsAmounts.push_back(output.amount);
  }

  return isFusionTransaction(getInputsAmounts(transaction), outputsAmounts, size);
}

bool Currency::isFusionTransaction(const transaction_t &transaction) const
{
  return isFusionTransaction(transaction, BinaryArray::size(transaction));
}

bool Currency::isAmountApplicableInFusionTransactionInput(uint64_t amount, uint64_t threshold) const
{
  uint8_t ignore;
  return isAmountApplicableInFusionTransactionInput(amount, threshold, ignore);
}

bool Currency::isAmountApplicableInFusionTransactionInput(uint64_t amount, uint64_t threshold, uint8_t &amountPowerOfTen) const
{
  if (amount >= threshold)
  {
    return false;
  }

  if (amount < defaultDustThreshold())
  {
    return false;
  }

  auto it = std::lower_bound(PRETTY_AMOUNTS.begin(), PRETTY_AMOUNTS.end(), amount);
  if (it == PRETTY_AMOUNTS.end() || amount != *it)
  {
    return false;
  }

  amountPowerOfTen = static_cast<uint8_t>(std::distance(PRETTY_AMOUNTS.begin(), it) / 9);
  return true;
}

std::string Currency::formatAmount(uint64_t amount) const
{
  std::string s = std::to_string(amount);
  if (s.size() < m_numberOfDecimalPlaces + 1)
  {
    s.insert(0, m_numberOfDecimalPlaces + 1 - s.size(), '0');
  }
  s.insert(s.size() - m_numberOfDecimalPlaces, ".");
  return s;
}

std::string Currency::formatAmount(int64_t amount) const
{
  std::string s = formatAmount(static_cast<uint64_t>(std::abs(amount)));

  if (amount < 0)
  {
    s.insert(0, "-");
  }

  return s;
}

bool Currency::parseAmount(const std::string &str, uint64_t &amount) const
{
  std::string strAmount = str;
  boost::algorithm::trim(strAmount);

  size_t pointIndex = strAmount.find_first_of('.');
  size_t fractionSize;
  if (std::string::npos != pointIndex)
  {
    fractionSize = strAmount.size() - pointIndex - 1;
    while (m_numberOfDecimalPlaces < fractionSize && '0' == strAmount.back())
    {
      strAmount.erase(strAmount.size() - 1, 1);
      --fractionSize;
    }
    if (m_numberOfDecimalPlaces < fractionSize)
    {
      return false;
    }
    strAmount.erase(pointIndex, 1);
  }
  else
  {
    fractionSize = 0;
  }

  if (strAmount.empty())
  {
    return false;
  }

  if (!std::all_of(strAmount.begin(), strAmount.end(), ::isdigit))
  {
    return false;
  }

  if (fractionSize < m_numberOfDecimalPlaces)
  {
    strAmount.append(m_numberOfDecimalPlaces - fractionSize, '0');
  }

  return stream::fromString(strAmount, amount);
}

difficulty_t Currency::nextDifficulty(std::vector<uint64_t> timestamps,
                                      std::vector<difficulty_t> cumulativeDifficulties) const
{
  difficulty_config_t config;
  config.window = m_difficultyWindow;
  config.target = m_difficultyTarget;
  config.cut = m_difficultyCut;
  config.lag = m_difficultyLag;
  return next_difficulty(timestamps.data(), timestamps.size(),
                         cumulativeDifficulties.data(),
                         (uint64_t *)&config);
}

size_t Currency::getApproximateMaximumInputCount(size_t transactionSize, size_t outputCount, size_t mixinCount) const
{
  const size_t KEY_IMAGE_SIZE = sizeof(key_image_t);
  const size_t OUTPUT_KEY_SIZE = sizeof(decltype(key_output_t::key));
  const size_t AMOUNT_SIZE = sizeof(uint64_t) + 2;                   //varint
  const size_t GLOBAL_INDEXES_VECTOR_SIZE_SIZE = sizeof(uint8_t);    //varint
  const size_t GLOBAL_INDEXES_INITIAL_VALUE_SIZE = sizeof(uint32_t); //varint
  const size_t GLOBAL_INDEXES_DIFFERENCE_SIZE = sizeof(uint32_t);    //varint
  const size_t SIGNATURE_SIZE = sizeof(signature_t);
  const size_t EXTRA_TAG_SIZE = sizeof(uint8_t);
  const size_t INPUT_TAG_SIZE = sizeof(uint8_t);
  const size_t OUTPUT_TAG_SIZE = sizeof(uint8_t);
  const size_t PUBLIC_KEY_SIZE = sizeof(public_key_t);
  const size_t TRANSACTION_VERSION_SIZE = sizeof(uint8_t);
  const size_t TRANSACTION_UNLOCK_TIME_SIZE = sizeof(uint64_t);

  const size_t outputsSize = outputCount * (OUTPUT_TAG_SIZE + OUTPUT_KEY_SIZE + AMOUNT_SIZE);
  const size_t headerSize = TRANSACTION_VERSION_SIZE + TRANSACTION_UNLOCK_TIME_SIZE + EXTRA_TAG_SIZE + PUBLIC_KEY_SIZE;
  const size_t inputSize = INPUT_TAG_SIZE + AMOUNT_SIZE + KEY_IMAGE_SIZE + SIGNATURE_SIZE + GLOBAL_INDEXES_VECTOR_SIZE_SIZE + GLOBAL_INDEXES_INITIAL_VALUE_SIZE +
                           mixinCount * (GLOBAL_INDEXES_DIFFERENCE_SIZE + SIGNATURE_SIZE);

  return (transactionSize - headerSize - outputsSize) / inputSize;
}

CurrencyBuilder::CurrencyBuilder(const std::string &path, config::config_t &config, Logging::ILogger &log) : m_currency(path, config, log)
{
  maxBlockNumber(parameters::CRYPTONOTE_MAX_BLOCK_NUMBER);
  maxBlockBlobSize(parameters::CRYPTONOTE_MAX_BLOCK_BLOB_SIZE);
  maxTxSize(parameters::CRYPTONOTE_MAX_TX_SIZE);
  publicAddressBase58Prefix(parameters::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX);
  minedMoneyUnlockWindow(parameters::CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW);

  timestampCheckWindow(parameters::BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW);
  blockFutureTimeLimit(parameters::CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT);

  moneySupply(parameters::MONEY_SUPPLY);
  emissionSpeedFactor(parameters::EMISSION_SPEED_FACTOR);

  rewardBlocksWindow(parameters::CRYPTONOTE_REWARD_BLOCKS_WINDOW);
  blockGrantedFullRewardZone(parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE);
  minerTxBlobReservedSize(parameters::CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE);

  numberOfDecimalPlaces(parameters::CRYPTONOTE_DISPLAY_DECIMAL_POINT);

  mininumFee(parameters::MINIMUM_FEE);
  defaultDustThreshold(parameters::DEFAULT_DUST_THRESHOLD);

  difficultyTarget(parameters::DIFFICULTY_TARGET);
  difficultyWindow(parameters::DIFFICULTY_WINDOW);
  difficultyLag(parameters::DIFFICULTY_LAG);
  difficultyCut(parameters::DIFFICULTY_CUT);

  maxBlockSizeInitial(parameters::MAX_BLOCK_SIZE_INITIAL);
  maxBlockSizeGrowthSpeedNumerator(parameters::MAX_BLOCK_SIZE_GROWTH_SPEED_NUMERATOR);
  maxBlockSizeGrowthSpeedDenominator(parameters::MAX_BLOCK_SIZE_GROWTH_SPEED_DENOMINATOR);

  lockedTxAllowedDeltaSeconds(parameters::CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS);
  lockedTxAllowedDeltaBlocks(parameters::CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS);

  mempoolTxLiveTime(parameters::CRYPTONOTE_MEMPOOL_TX_LIVETIME);
  mempoolTxFromAltBlockLiveTime(parameters::CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME);
  numberOfPeriodsToForgetTxDeletedFromPool(parameters::CRYPTONOTE_NUMBER_OF_PERIODS_TO_FORGET_TX_DELETED_FROM_POOL);

  fusionTxMaxSize(parameters::FUSION_TX_MAX_SIZE);
  fusionTxMinInputCount(parameters::FUSION_TX_MIN_INPUT_COUNT);
  fusionTxMinInOutCountRatio(parameters::FUSION_TX_MIN_IN_OUT_COUNT_RATIO);

  coin::storage_files_t files;

  files.blocks = config.filenames.block;
  files.blocksCache = config.filenames.blockCache;
  files.blocksIndexes = config.filenames.blockIndex;
  files.txPool = config.filenames.pool;
  files.blockchainIndexes = config.filenames.blockChainIndex;
  m_currency.setFiles(files);
}

transaction_t CurrencyBuilder::generateGenesisTransaction()
{
  cryptonote::transaction_t tx;
  cryptonote::account_public_address_t ac = boost::value_initialized<cryptonote::account_public_address_t>();
  m_currency.constructMinerTx(0, 0, 0, 0, 0, ac, tx); // zero fee in genesis
  return tx;
}

CurrencyBuilder &CurrencyBuilder::emissionSpeedFactor(unsigned int val)
{
  if (val <= 0 || val > 8 * sizeof(uint64_t))
  {
    throw std::invalid_argument("val at emissionSpeedFactor()");
  }

  m_currency.m_emissionSpeedFactor = val;
  return *this;
}

CurrencyBuilder &CurrencyBuilder::numberOfDecimalPlaces(size_t val)
{
  m_currency.m_numberOfDecimalPlaces = val;
  m_currency.m_coin = 1;
  for (size_t i = 0; i < m_currency.m_numberOfDecimalPlaces; ++i)
  {
    m_currency.m_coin *= 10;
  }

  return *this;
}

CurrencyBuilder &CurrencyBuilder::difficultyWindow(size_t val)
{
  if (val < 2)
  {
    throw std::invalid_argument("val at difficultyWindow()");
  }
  m_currency.m_difficultyWindow = val;
  return *this;
}

} // namespace cryptonote
