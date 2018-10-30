// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <atomic>

#include "google/sparse_hash_set"
#include "google/sparse_hash_map"

#include "common/ObserverManager.h"
#include "cryptonote/core/BlockIndex.h"
#include "cryptonote/core/Checkpoints.h"
#include "cryptonote/core/Currency.h"
#include "cryptonote/core/blockchain/serializer/exports.h"
#include "cryptonote/core/IBlockchainStorageObserver.h"
#include "cryptonote/core/ITransactionValidator.h"
#include "cryptonote/core/SwappedVector.h"
#include "cryptonote/core/CryptoNoteFormatUtils.h"
#include "cryptonote/core/tx_memory_pool.h"
#include "cryptonote/core/blockchain/indexing/exports.h"

#include "cryptonote/core/template/MessageQueue.h"
#include "cryptonote/core/BlockchainMessages.h"
#include "cryptonote/core/template/IntrusiveLinkedList.h"

#include <logging/LoggerRef.h>

#undef ERROR

namespace cryptonote {
  struct NOTIFY_REQUEST_GET_OBJECTS_request;
  struct NOTIFY_RESPONSE_GET_OBJECTS_request;
  struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request;
  struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response;
  struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_outs_for_amount;

  using cryptonote::BlockInfo;
  
  class LockedBlockchainStorage;

  class Blockchain : public cryptonote::ITransactionValidator {
  public:
    Blockchain(const Currency& currency, TxMemoryPool& tx_pool, Logging::ILogger& logger);

    bool addObserver(IBlockchainStorageObserver* observer);
    bool removeObserver(IBlockchainStorageObserver* observer);

    // ITransactionValidator
    virtual bool checkTransactionInputs(const cryptonote::Transaction& tx, BlockInfo& maxUsedBlock) override;
    virtual bool checkTransactionInputs(const cryptonote::Transaction& tx, BlockInfo& maxUsedBlock, BlockInfo& lastFailed) override;
    virtual bool haveSpentKeyImages(const cryptonote::Transaction& tx) override;
    virtual bool checkTransactionSize(size_t blobSize) override;

    bool init(bool load_existing = true);
    bool deinit();

    bool getLowerBound(uint64_t timestamp, uint64_t startOffset, uint32_t& height);
    std::vector<crypto::Hash> getBlockIds(uint32_t startHeight, uint32_t maxCount);

    void setCheckpoints(Checkpoints&& chk_pts) { m_checkpoints = chk_pts; }
    bool getBlocks(uint32_t start_offset, uint32_t count, std::list<Block>& blocks, std::list<Transaction>& txs);
    bool getBlocks(uint32_t start_offset, uint32_t count, std::list<Block>& blocks);
    bool getAlternativeBlocks(std::list<Block>& blocks);
    uint32_t getAlternativeBlocksCount();
    crypto::Hash getBlockIdByHeight(uint32_t height);
    bool getBlockByHash(const crypto::Hash &h, Block &blk);
    bool getBlockHeight(const crypto::Hash& blockId, uint32_t& blockHeight);

    template<class archive_t> void serialize(archive_t & ar, const unsigned int version);

    bool haveTransaction(const crypto::Hash &id);
    bool haveTransactionKeyImagesAsSpent(const Transaction &tx);

    uint32_t getCurrentBlockchainHeight(); //TODO rename to getCurrentBlockchainSize
    crypto::Hash getTailId();
    crypto::Hash getTailId(uint32_t& height);
    difficulty_type getDifficultyForNextBlock();
    uint64_t getCoinsInCirculation();
    bool addNewBlock(const Block& bl_, BlockVerificationContext& bvc);
    bool resetAndSetGenesisBlock(const Block& b);
    bool haveBlock(const crypto::Hash& id);
    size_t getTotalTransactions();
    std::vector<crypto::Hash> buildSparseChain();
    std::vector<crypto::Hash> buildSparseChain(const crypto::Hash& startBlockId);
    uint32_t findBlockchainSupplement(const std::vector<crypto::Hash>& qblock_ids); // !!!!
    std::vector<crypto::Hash> findBlockchainSupplement(const std::vector<crypto::Hash>& remoteBlockIds, size_t maxCount,
      uint32_t& totalBlockCount, uint32_t& startBlockIndex);
    bool handleGetObjects(NOTIFY_REQUEST_GET_OBJECTS_request& arg, NOTIFY_RESPONSE_GET_OBJECTS_request& rsp); //Deprecated. Should be removed with CryptoNoteProtocolHandler.
    bool getRandomOutsByAmount(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response& res);
    bool getBackwardBlocksSize(size_t from_height, std::vector<size_t>& sz, size_t count);
    bool getTransactionOutputGlobalIndexes(const crypto::Hash& tx_id, std::vector<uint32_t>& indexs);
    bool get_out_by_msig_gindex(uint64_t amount, uint64_t gindex, MultisignatureOutput& out);
    bool checkTransactionInputs(const Transaction& tx, uint32_t& pmax_used_block_height, crypto::Hash& max_used_block_id, BlockInfo* tail = 0);
    uint64_t getCurrentCumulativeBlocksizeLimit();
    uint64_t blockDifficulty(size_t i);
    bool getBlockContainingTransaction(const crypto::Hash& txId, crypto::Hash& blockId, uint32_t& blockHeight);
    bool getAlreadyGeneratedCoins(const crypto::Hash& hash, uint64_t& generatedCoins);
    bool getBlockSize(const crypto::Hash& hash, size_t& size);
    bool getMultisigOutputReference(const MultisignatureInput& txInMultisig, std::pair<crypto::Hash, size_t>& outputReference);
    bool getGeneratedTransactionsNumber(uint32_t height, uint64_t& generatedTransactions);
    bool getOrphanBlockIdsByHeight(uint32_t height, std::vector<crypto::Hash>& blockHashes);
    bool getBlockIdsByTimestamp(uint64_t timestampBegin, uint64_t timestampEnd, uint32_t blocksNumberLimit, std::vector<crypto::Hash>& hashes, uint32_t& blocksNumberWithinTimestamps);
    bool getTransactionIdsByPaymentId(const crypto::Hash& paymentId, std::vector<crypto::Hash>& transactionHashes);
    bool isBlockInMainChain(const crypto::Hash& blockId);

    template<class visitor_t> bool scanOutputKeysForIndexes(const KeyInput& tx_in_to_key, visitor_t& vis, uint32_t* pmax_related_block_height = NULL);

    bool addMessageQueue(MessageQueue<BlockchainMessage>& messageQueue);
    bool removeMessageQueue(MessageQueue<BlockchainMessage>& messageQueue);

    template<class t_ids_container, class t_blocks_container, class t_missed_container>
    bool getBlocks(const t_ids_container& block_ids, t_blocks_container& blocks, t_missed_container& missed_bs) {
      std::lock_guard<std::recursive_mutex> lk(m_blockchain_lock);

      for (const auto& bl_id : block_ids) {
        uint32_t height = 0;
        if (!m_blockIndex.getBlockHeight(bl_id, height)) {
          missed_bs.push_back(bl_id);
        } else {
          if (!(height < m_blocks.size())) { logger(Logging::ERROR, Logging::BRIGHT_RED) << "Internal error: bl_id=" << Common::podToHex(bl_id)
            << " have index record with offset=" << height << ", bigger then m_blocks.size()=" << m_blocks.size(); return false; }
            blocks.push_back(m_blocks[height].bl);
        }
      }

      return true;
    }

    template<class t_ids_container, class t_tx_container, class t_missed_container>
    void getBlockchainTransactions(const t_ids_container& txs_ids, t_tx_container& txs, t_missed_container& missed_txs) {
      std::lock_guard<decltype(m_blockchain_lock)> bcLock(m_blockchain_lock);

      for (const auto& tx_id : txs_ids) {
        auto it = m_transactionMap.find(tx_id);
        if (it == m_transactionMap.end()) {
          missed_txs.push_back(tx_id);
        } else {
          txs.push_back(transactionByIndex(it->second).tx);
        }
      }
    }

    template<class t_ids_container, class t_tx_container, class t_missed_container>
    void getTransactions(const t_ids_container& txs_ids, t_tx_container& txs, t_missed_container& missed_txs, bool checkTxPool = false) {
      if (checkTxPool){
        std::lock_guard<decltype(m_tx_pool)> txLock(m_tx_pool);

        getBlockchainTransactions(txs_ids, txs, missed_txs);

        auto poolTxIds = std::move(missed_txs);
        missed_txs.clear();
        m_tx_pool.getTransactions(poolTxIds, txs, missed_txs);

      } else {
        getBlockchainTransactions(txs_ids, txs, missed_txs);
      }
    }

    //debug functions
    void print_blockchain(uint64_t start_index, uint64_t end_index);
    void print_blockchain_index();
    void print_blockchain_outs(const std::string& file);

  private:
    typedef google::sparse_hash_set<crypto::KeyImage> key_images_container;
    typedef std::unordered_map<crypto::Hash, BlockEntry> blocks_ext_by_hash;
    typedef google::sparse_hash_map<uint64_t, std::vector<std::pair<TransactionIndex, uint16_t>>> outputs_container; //crypto::Hash - tx hash, size_t - index of out in transaction
    typedef google::sparse_hash_map<uint64_t, std::vector<MultisignatureOutputUsage>> MultisignatureOutputsContainer;

    const Currency& m_currency;
    TxMemoryPool& m_tx_pool;
    std::recursive_mutex m_blockchain_lock; // TODO: add here reader/writer lock
    Tools::ObserverManager<IBlockchainStorageObserver> m_observerManager;

    key_images_container m_spent_keys;
    size_t m_current_block_cumul_sz_limit;
    blocks_ext_by_hash m_alternative_chains; // crypto::Hash -> block_extended_info
    outputs_container m_outputs;

    Checkpoints m_checkpoints;
    std::atomic<bool> m_is_in_checkpoint_zone;

    typedef SwappedVector<BlockEntry> Blocks;
    typedef std::unordered_map<crypto::Hash, uint32_t> BlockMap;
    typedef std::unordered_map<crypto::Hash, TransactionIndex> TransactionMap;

    friend class BlockCacheSerializer;
    friend class BlockchainIndicesSerializer;

    Blocks m_blocks;
    cryptonote::BlockIndex m_blockIndex;
    TransactionMap m_transactionMap;
    MultisignatureOutputsContainer m_multisignatureOutputs;

    PaymentIdIndex m_paymentIdIndex;
    TimestampBlocksIndex m_timestampIndex;
    GeneratedTransactionsIndex m_generatedTransactionsIndex;
    OrphanBlocksIndex m_orthanBlocksIndex;

    IntrusiveLinkedList<MessageQueue<BlockchainMessage>> m_messageQueueList;

    Logging::LoggerRef logger;

    void rebuildCache();
    bool storeCache();
    bool switch_to_alternative_blockchain(std::list<blocks_ext_by_hash::iterator>& alt_chain, bool discard_disconnected_chain);
    bool handle_alternative_block(const Block& b, const crypto::Hash& id, BlockVerificationContext& bvc, bool sendNewAlternativeBlockMessage = true);
    difficulty_type get_next_difficulty_for_alternative_chain(const std::list<blocks_ext_by_hash::iterator>& alt_chain, BlockEntry& bei);
    bool prevalidate_miner_transaction(const Block& b, uint32_t height);
    bool validate_miner_transaction(const Block& b, uint32_t height, size_t cumulativeBlockSize, uint64_t alreadyGeneratedCoins, uint64_t fee, uint64_t& reward, int64_t& emissionChange);
    bool rollback_blockchain_switching(std::list<Block>& original_chain, size_t rollback_height);
    bool get_last_n_blocks_sizes(std::vector<size_t>& sz, size_t count);
    bool add_out_to_get_random_outs(std::vector<std::pair<TransactionIndex, uint16_t>>& amount_outs, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_outs_for_amount& result_outs, uint64_t amount, size_t i);
    bool is_tx_spendtime_unlocked(uint64_t unlock_time);
    size_t find_end_of_allowed_index(const std::vector<std::pair<TransactionIndex, uint16_t>>& amount_outs);
    bool check_block_timestamp_main(const Block& b);
    bool check_block_timestamp(std::vector<uint64_t> timestamps, const Block& b);
    uint64_t get_adjusted_time();
    bool complete_timestamps_vector(uint64_t start_height, std::vector<uint64_t>& timestamps);
    bool checkCumulativeBlockSize(const crypto::Hash& blockId, size_t cumulativeBlockSize, uint64_t height);
    std::vector<crypto::Hash> doBuildSparseChain(const crypto::Hash& startBlockId) const;
    bool getBlockCumulativeSize(const Block& block, size_t& cumulativeSize);
    bool update_next_comulative_size_limit();
    bool check_tx_input(const KeyInput& txin, const crypto::Hash& tx_prefix_hash, const std::vector<crypto::Signature>& sig, uint32_t* pmax_related_block_height = NULL);
    bool checkTransactionInputs(const Transaction& tx, const crypto::Hash& tx_prefix_hash, uint32_t* pmax_used_block_height = NULL);
    bool checkTransactionInputs(const Transaction& tx, uint32_t* pmax_used_block_height = NULL);
    bool have_tx_keyimg_as_spent(const crypto::KeyImage &key_im);
    const TransactionEntry& transactionByIndex(TransactionIndex index);
    bool pushBlock(const Block& blockData, BlockVerificationContext& bvc);
    bool pushBlock(const Block& blockData, const std::vector<Transaction>& transactions, BlockVerificationContext& bvc);
    bool pushBlock(BlockEntry& block);
    void popBlock(const crypto::Hash& blockHash);
    bool pushTransaction(BlockEntry& block, const crypto::Hash& transactionHash, TransactionIndex transactionIndex);
    void popTransaction(const Transaction& transaction, const crypto::Hash& transactionHash);
    void popTransactions(const BlockEntry& block, const crypto::Hash& minerTransactionHash);
    bool validateInput(const MultisignatureInput& input, const crypto::Hash& transactionHash, const crypto::Hash& transactionPrefixHash, const std::vector<crypto::Signature>& transactionSignatures);

    bool storeBlockchainIndices();
    bool loadBlockchainIndices();

    bool loadTransactions(const Block& block, std::vector<Transaction>& transactions);
    void saveTransactions(const std::vector<Transaction>& transactions);

    void sendMessage(const BlockchainMessage& message);

    friend class LockedBlockchainStorage;
  };

  template<class visitor_t> bool Blockchain::scanOutputKeysForIndexes(const KeyInput& tx_in_to_key, visitor_t& vis, uint32_t* pmax_related_block_height) {
    std::lock_guard<std::recursive_mutex> lk(m_blockchain_lock);
    auto it = m_outputs.find(tx_in_to_key.amount);
    if (it == m_outputs.end() || !tx_in_to_key.outputIndexes.size())
      return false;

    std::vector<uint32_t> absolute_offsets = relative_output_offsets_to_absolute(tx_in_to_key.outputIndexes);
    std::vector<std::pair<TransactionIndex, uint16_t>>& amount_outs_vec = it->second;
    size_t count = 0;
    for (uint64_t i : absolute_offsets) {
      if(i >= amount_outs_vec.size() ) {
        logger(Logging::INFO) << "Wrong index in transaction inputs: " << i << ", expected maximum " << amount_outs_vec.size() - 1;
        return false;
      }

      //auto tx_it = m_transactionMap.find(amount_outs_vec[i].first);
      //if (!(tx_it != m_transactionMap.end())) { logger(ERROR, BRIGHT_RED) << "Wrong transaction id in output indexes: " << Common::podToHex(amount_outs_vec[i].first); return false; }

      const TransactionEntry& tx = transactionByIndex(amount_outs_vec[i].first);

      if (!(amount_outs_vec[i].second < tx.tx.outputs.size())) {
        logger(Logging::ERROR, Logging::BRIGHT_RED)
            << "Wrong index in transaction outputs: "
            << amount_outs_vec[i].second << ", expected less then "
            << tx.tx.outputs.size();
        return false;
      }

      if (!vis.handle_output(tx.tx, tx.tx.outputs[amount_outs_vec[i].second], amount_outs_vec[i].second)) {
        logger(Logging::INFO) << "Failed to handle_output for output no = " << count << ", with absolute offset " << i;
        return false;
      }

      if(count++ == absolute_offsets.size()-1 && pmax_related_block_height) {
        if (*pmax_related_block_height < amount_outs_vec[i].first.block) {
          *pmax_related_block_height = amount_outs_vec[i].first.block;
        }
      }
    }

    return true;
  }
}

