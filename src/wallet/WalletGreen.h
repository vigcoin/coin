// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "IWallet.h"

#include <queue>
#include <unordered_map>

#include "IFusionManager.h"
#include "WalletIndices.h"

#include <system/Dispatcher.h>
#include <system/Event.h>
#include "transfers/TransfersSynchronizer.h"
#include "transfers/BlockchainSynchronizer.h"

namespace cryptonote {

class WalletGreen : public IWallet,
                    ITransfersObserver,
                    IBlockchainSynchronizerObserver,
                    ITransfersSynchronizerObserver,
                    IFusionManager {
public:
  WalletGreen(System::Dispatcher& dispatcher, const Currency& currency, INode& node, uint32_t transactionSoftLockTime = 1);
  virtual ~WalletGreen();

  virtual void initialize(const std::string& password) override;
  virtual void initializeWithViewKey(const secret_key_t& viewSecretKey, const std::string& password) override;
  virtual void load(std::istream& source, const std::string& password) override;
  virtual void shutdown() override;

  virtual void changePassword(const std::string& oldPassword, const std::string& newPassword) override;
  virtual void save(std::ostream& destination, bool saveDetails = true, bool saveCache = true) override;

  virtual size_t getAddressCount() const override;
  virtual std::string getAddress(size_t index) const override;
  virtual key_pair_t getAddressSpendKey(size_t index) const override;
  virtual key_pair_t getAddressSpendKey(const std::string& address) const override;
  virtual key_pair_t getViewKey() const override;
  virtual std::string createAddress() override;
  virtual std::string createAddress(const secret_key_t& spendSecretKey) override;
  virtual std::string createAddress(const public_key_t& spendPublicKey) override;
  virtual void deleteAddress(const std::string& address) override;

  virtual uint64_t getActualBalance() const override;
  virtual uint64_t getActualBalance(const std::string& address) const override;
  virtual uint64_t getPendingBalance() const override;
  virtual uint64_t getPendingBalance(const std::string& address) const override;

  virtual size_t getTransactionCount() const override;
  virtual WalletTransaction getTransaction(size_t transactionIndex) const override;
  virtual size_t getTransactionTransferCount(size_t transactionIndex) const override;
  virtual WalletTransfer getTransactionTransfer(size_t transactionIndex, size_t transferIndex) const override;

  virtual WalletTransactionWithTransfers getTransaction(const hash_t& transactionHash) const override;
  virtual std::vector<TransactionsInBlockInfo> getTransactions(const hash_t& blockHash, size_t count) const override;
  virtual std::vector<TransactionsInBlockInfo> getTransactions(uint32_t blockIndex, size_t count) const override;
  virtual std::vector<hash_t> getBlockHashes(uint32_t blockIndex, size_t count) const override;
  virtual uint32_t getBlockCount() const override;
  virtual std::vector<WalletTransactionWithTransfers> getUnconfirmedTransactions() const override;
  virtual std::vector<size_t> getDelayedTransactionIds() const override;

  virtual size_t transfer(const TransactionParameters& sendingTransaction) override;

  virtual size_t makeTransaction(const TransactionParameters& sendingTransaction) override;
  virtual void commitTransaction(size_t) override;
  virtual void rollbackUncommitedTransaction(size_t) override;

  virtual void start() override;
  virtual void stop() override;
  virtual WalletEvent getEvent() override;

  virtual size_t createFusionTransaction(uint64_t threshold, uint64_t mixin) override;
  virtual bool isFusionTransaction(size_t transactionId) const override;
  virtual IFusionManager::EstimateResult estimate(uint64_t threshold) const override;

protected:
  void throwIfNotInitialized() const;
  void throwIfStopped() const;
  void throwIfTrackingMode() const;
  void doShutdown();
  void clearCaches();
  void initWithKeys(const public_key_t& viewPublicKey, const secret_key_t& viewSecretKey, const std::string& password);
  std::string doCreateAddress(const public_key_t& spendPublicKey, const secret_key_t& spendSecretKey, uint64_t creationTimestamp);

  struct InputInfo {
    input_key_info_t keyInfo;
    WalletRecord* walletRecord = nullptr;
    key_pair_t ephKeys;
  };

  struct OutputToTransfer {
    TransactionOutputInformation out;
    WalletRecord* wallet;
  };

  struct ReceiverAmounts {
    cryptonote::account_public_address_t receiver;
    std::vector<uint64_t> amounts;
  };

  struct WalletOuts {
    WalletRecord* wallet;
    std::vector<TransactionOutputInformation> outs;
  };

  typedef std::pair<WalletTransfers::const_iterator, WalletTransfers::const_iterator> TransfersRange;

  struct AddressAmounts {
    int64_t input = 0;
    int64_t output = 0;
  };

  struct ContainerAmounts {
    ITransfersContainer* container;
    AddressAmounts amounts;
  };

  typedef std::unordered_map<std::string, AddressAmounts> TransfersMap;

  virtual void onError(ITransfersSubscription* object, uint32_t height, std::error_code ec) override;

  virtual void onTransactionUpdated(ITransfersSubscription* object, const hash_t& transactionHash) override;
  virtual void onTransactionUpdated(const public_key_t& viewPublicKey, const hash_t& transactionHash,
    const std::vector<ITransfersContainer*>& containers) override;
  void transactionUpdated(const transaction_infomation_t& transactionInfo, const std::vector<ContainerAmounts>& containerAmountsList);

  virtual void onTransactionDeleted(ITransfersSubscription* object, const hash_t& transactionHash) override;
  void transactionDeleted(ITransfersSubscription* object, const hash_t& transactionHash);

  virtual void synchronizationProgressUpdated(uint32_t processedBlockCount, uint32_t totalBlockCount) override;
  virtual void synchronizationCompleted(std::error_code result) override;

  void onSynchronizationProgressUpdated(uint32_t processedBlockCount, uint32_t totalBlockCount);
  void onSynchronizationCompleted();

  virtual void onBlocksAdded(const public_key_t& viewPublicKey, const std::vector<hash_t>& blockHashes) override;
  void blocksAdded(const std::vector<hash_t>& blockHashes);

  virtual void onBlockchainDetach(const public_key_t& viewPublicKey, uint32_t blockIndex) override;
  void blocksRollback(uint32_t blockIndex);

  virtual void onTransactionDeleteBegin(const public_key_t& viewPublicKey, hash_t transactionHash) override;
  void transactionDeleteBegin(hash_t transactionHash);

  virtual void onTransactionDeleteEnd(const public_key_t& viewPublicKey, hash_t transactionHash) override;
  void transactionDeleteEnd(hash_t transactionHash);

  std::vector<WalletOuts> pickWalletsWithMoney() const;
  WalletOuts pickWallet(const std::string& address);
  std::vector<WalletOuts> pickWallets(const std::vector<std::string>& addresses);

  void updateBalance(cryptonote::ITransfersContainer* container);
  void unlockBalances(uint32_t height);

  const WalletRecord& getWalletRecord(const public_key_t& key) const;
  const WalletRecord& getWalletRecord(const std::string& address) const;
  const WalletRecord& getWalletRecord(cryptonote::ITransfersContainer* container) const;

  cryptonote::account_public_address_t parseAddress(const std::string& address) const;
  std::string addWallet(const public_key_t& spendPublicKey, const secret_key_t& spendSecretKey, uint64_t creationTimestamp);
  account_keys_t makeAccountKeys(const WalletRecord& wallet) const;
  size_t getTransactionId(const hash_t& transactionHash) const;
  void pushEvent(const WalletEvent& event);
  bool isFusionTransaction(const WalletTransaction& walletTx) const;

  struct PreparedTransaction {
    std::unique_ptr<ITransaction> transaction;
    std::vector<WalletTransfer> destinations;
    uint64_t neededMoney;
    uint64_t changeAmount;
  };

  void prepareTransaction(std::vector<WalletOuts>&& wallets,
    const std::vector<WalletOrder>& orders,
    uint64_t fee,
    uint64_t mixIn,
    const std::string& extra,
    uint64_t unlockTimestamp,
    const DonationSettings& donation,
    const cryptonote::account_public_address_t& changeDestinationAddress,
    PreparedTransaction& preparedTransaction);

  void validateTransactionParameters(const TransactionParameters& transactionParameters);
  size_t doTransfer(const TransactionParameters& transactionParameters);

  void requestMixinOuts(const std::vector<OutputToTransfer>& selectedTransfers,
    uint64_t mixIn,
    std::vector<cryptonote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>& mixinResult);

  void prepareInputs(const std::vector<OutputToTransfer>& selectedTransfers,
    std::vector<cryptonote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>& mixinResult,
    uint64_t mixIn,
    std::vector<InputInfo>& keysInfo);

  uint64_t selectTransfers(uint64_t needeMoney,
    bool dust,
    uint64_t dustThreshold,
    std::vector<WalletOuts>&& wallets,
    std::vector<OutputToTransfer>& selectedTransfers);

  std::vector<ReceiverAmounts> splitDestinations(const std::vector<WalletTransfer>& destinations,
    uint64_t dustThreshold, const Currency& currency);
  ReceiverAmounts splitAmount(uint64_t amount, const account_public_address_t& destination, uint64_t dustThreshold);

  std::unique_ptr<cryptonote::ITransaction> makeTransaction(const std::vector<ReceiverAmounts>& decomposedOutputs,
    std::vector<InputInfo>& keysInfo, const std::string& extra, uint64_t unlockTimestamp);

  void sendTransaction(const cryptonote::transaction_t& cryptoNoteTransaction);
  size_t validateSaveAndSendTransaction(const ITransactionReader& transaction, const std::vector<WalletTransfer>& destinations, bool isFusion, bool send);

  size_t insertBlockchainTransaction(const transaction_infomation_t& info, int64_t txBalance);
  size_t insertOutgoingTransactionAndPushEvent(const hash_t& transactionHash, uint64_t fee, const binary_array_t& extra, uint64_t unlockTimestamp);
  void updateTransactionStateAndPushEvent(size_t transactionId, WalletTransactionState state);
  bool updateWalletTransactionInfo(size_t transactionId, const cryptonote::transaction_infomation_t& info, int64_t totalAmount);
  bool updateTransactionTransfers(size_t transactionId, const std::vector<ContainerAmounts>& containerAmountsList,
    int64_t allInputsAmount, int64_t allOutputsAmount);
  TransfersMap getKnownTransfersMap(size_t transactionId, size_t firstTransferIdx) const;
  bool updateAddressTransfers(size_t transactionId, size_t firstTransferIdx, const std::string& address, int64_t knownAmount, int64_t targetAmount);
  bool updateUnknownTransfers(size_t transactionId, size_t firstTransferIdx, const std::unordered_set<std::string>& myAddresses,
    int64_t knownAmount, int64_t myAmount, int64_t totalAmount, bool isOutput);
  void appendTransfer(size_t transactionId, size_t firstTransferIdx, const std::string& address, int64_t amount);
  bool adjustTransfer(size_t transactionId, size_t firstTransferIdx, const std::string& address, int64_t amount);
  bool eraseTransfers(size_t transactionId, size_t firstTransferIdx, std::function<bool(bool, const std::string&)>&& predicate);
  bool eraseTransfersByAddress(size_t transactionId, size_t firstTransferIdx, const std::string& address, bool eraseOutputTransfers);
  bool eraseForeignTransfers(size_t transactionId, size_t firstTransferIdx, const std::unordered_set<std::string>& knownAddresses, bool eraseOutputTransfers);
  void pushBackOutgoingTransfers(size_t txId, const std::vector<WalletTransfer>& destinations);
  void insertUnlockTransactionJob(const hash_t& transactionHash, uint32_t blockHeight, cryptonote::ITransfersContainer* container);
  void deleteUnlockTransactionJob(const hash_t& transactionHash);
  void startBlockchainSynchronizer();
  void stopBlockchainSynchronizer();
  void addUnconfirmedTransaction(const ITransactionReader& transaction);
  void removeUnconfirmedTransaction(const hash_t& transactionHash);

  void unsafeLoad(std::istream& source, const std::string& password);
  void unsafeSave(std::ostream& destination, bool saveDetails, bool saveCache);

  std::vector<OutputToTransfer> pickRandomFusionInputs(uint64_t threshold, size_t minInputCount, size_t maxInputCount);
  ReceiverAmounts decomposeFusionOutputs(uint64_t inputsAmount);

  enum class WalletState {
    INITIALIZED,
    NOT_INITIALIZED
  };

  enum class WalletTrackingMode {
    TRACKING,
    NOT_TRACKING,
    NO_ADDRESSES
  };

  WalletTrackingMode getTrackingMode() const;

  TransfersRange getTransactionTransfersRange(size_t transactionIndex) const;
  std::vector<TransactionsInBlockInfo> getTransactionsInBlocks(uint32_t blockIndex, size_t count) const;
  hash_t getBlockHashByIndex(uint32_t blockIndex) const;

  std::vector<WalletTransfer> getTransactionTransfers(const WalletTransaction& transaction) const;
  void filterOutTransactions(WalletTransactions& transactions, WalletTransfers& transfers, std::function<bool (const WalletTransaction&)>&& pred) const;
  void getViewKeyKnownBlocks(const public_key_t& viewPublicKey);
  cryptonote::account_public_address_t getChangeDestination(const std::string& changeDestinationAddress, const std::vector<std::string>& sourceAddresses) const;
  bool isMyAddress(const std::string& address) const;

  void deleteContainerFromUnlockTransactionJobs(const ITransfersContainer* container);
  std::vector<size_t> deleteTransfersForAddress(const std::string& address, std::vector<size_t>& deletedTransactions);
  void deleteFromUncommitedTransactions(const std::vector<size_t>& deletedTransactions);

  System::Dispatcher& m_dispatcher;
  const Currency& m_currency;
  INode& m_node;
  bool m_stopped;

  WalletsContainer m_walletsContainer;
  UnlockTransactionJobs m_unlockTransactionsJob;
  WalletTransactions m_transactions;
  WalletTransfers m_transfers; //sorted
  mutable std::unordered_map<size_t, bool> m_fusionTxsCache; // txIndex -> isFusion
  UncommitedTransactions m_uncommitedTransactions;

  bool m_blockchainSynchronizerStarted;
  BlockchainSynchronizer m_blockchainSynchronizer;
  TransfersSyncronizer m_synchronizer;

  System::Event m_eventOccurred;
  std::queue<WalletEvent> m_events;
  mutable System::Event m_readyEvent;

  WalletState m_state;

  std::string m_password;

  public_key_t m_viewPublicKey;
  secret_key_t m_viewSecretKey;

  uint64_t m_actualBalance;
  uint64_t m_pendingBalance;

  uint64_t m_upperTransactionSizeLimit;
  uint32_t m_transactionSoftLockTime;

  BlockHashesContainer m_blockchain;
};

} //namespace cryptonote
