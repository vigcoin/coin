// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "WalletLegacySerializer.h"

#include <stdexcept>

#include "stream/reader.h"
#include "stream/reader.h"
#include "stream/writer.h"
#include "serialization/BinaryOutputStreamSerializer.h"
#include "serialization/BinaryInputStreamSerializer.h"
#include "cryptonote/core/account.h"
#include "cryptonote/core/transaction/serializer/basics.h"
#include "wallet_legacy/WalletUserTransactionsCache.h"
#include "wallet/WalletErrors.h"
#include "wallet_legacy/KeysStorage.h"

using namespace Common;

namespace {

bool verifyKeys(const secret_key_t& sec, const public_key_t& expected_pub) {
  public_key_t pub;
  bool r = secret_key_to_public_key((const uint8_t*)&sec, (uint8_t*)&pub);
  return r && expected_pub == pub;
}

void throwIfKeysMissmatch(const secret_key_t& sec, const public_key_t& expected_pub) {
  if (!verifyKeys(sec, expected_pub))
    throw std::system_error(make_error_code(cryptonote::error::WRONG_PASSWORD));
}

}

namespace cryptonote {

WalletLegacySerializer::WalletLegacySerializer(cryptonote::Account& account, WalletUserTransactionsCache& transactionsCache) :
  account(account),
  transactionsCache(transactionsCache),
  walletSerializationVersion(1)
{
}

void WalletLegacySerializer::serialize(std::ostream& stream, const std::string& password, bool saveDetailed, const std::string& cache) {
  std::stringstream plainArchive;
  Writer plainStream(plainArchive);
  cryptonote::BinaryOutputStreamSerializer serializer(plainStream);
  saveKeys(serializer);

  serializer(saveDetailed, "has_details");

  if (saveDetailed) {
    serializer(transactionsCache, "details");
  }

  serializer.binary(const_cast<std::string&>(cache), "cache");

  std::string plain = plainArchive.str();
  std::string cipher;

  chacha_iv_t iv = encrypt(plain, password, cipher);

  uint32_t version = walletSerializationVersion;
  Writer output(stream);
  cryptonote::BinaryOutputStreamSerializer s(output);
  s.beginObject("wallet");
  s(version, "version");
  s(iv, "iv");
  s(cipher, "data");
  s.endObject();

  stream.flush();
}

void WalletLegacySerializer::saveKeys(cryptonote::ISerializer& serializer) {
  cryptonote::KeysStorage keys;
  cryptonote::account_keys_t acc = account.getAccountKeys();

  keys.creationTimestamp = account.getCreatetime();
  keys.spendPublicKey = acc.address.spendPublicKey;
  keys.spendSecretKey = acc.spendSecretKey;
  keys.viewPublicKey = acc.address.viewPublicKey;
  keys.viewSecretKey = acc.viewSecretKey;

  keys.serialize(serializer, "keys");
}

chacha_iv_t WalletLegacySerializer::encrypt(const std::string& plain, const std::string& password, std::string& cipher) {
  chacha_key_t key;
  generate_chacha_key(password, key);

  cipher.resize(plain.size());

  chacha_iv_t iv = rand<chacha_iv_t>();
  chacha8(plain.data(), plain.size(), key, iv, &cipher[0]);

  return iv;
}


void WalletLegacySerializer::deserialize(std::istream& stream, const std::string& password, std::string& cache) {
  Reader stdStream(stream);
  cryptonote::BinaryInputStreamSerializer serializerEncrypted(stdStream);

  serializerEncrypted.beginObject("wallet");

  uint32_t version;
  serializerEncrypted(version, "version");

  chacha_iv_t iv;
  serializerEncrypted(iv, "iv");

  std::string cipher;
  serializerEncrypted(cipher, "data");

  serializerEncrypted.endObject();

  std::string plain;
  decrypt(cipher, plain, iv, password);

  const char * b = static_cast<const char *>(plain.data());
  membuf mem((char *)(b), (char *)(b + plain.size()));
  std::istream istream(&mem);
  Reader decrypted(istream);
  cryptonote::BinaryInputStreamSerializer serializer(decrypted);

  loadKeys(serializer);
  throwIfKeysMissmatch(account.getAccountKeys().viewSecretKey, account.getAccountKeys().address.viewPublicKey);

  if (account.getAccountKeys().spendSecretKey != NULL_SECRET_KEY) {
    throwIfKeysMissmatch(account.getAccountKeys().spendSecretKey, account.getAccountKeys().address.spendPublicKey);
  } else {
    if (!check_key((uint8_t*)&account.getAccountKeys().address.spendPublicKey)) {
      throw std::system_error(make_error_code(cryptonote::error::WRONG_PASSWORD));
    }
  }

  bool detailsSaved;

  serializer(detailsSaved, "has_details");

  if (detailsSaved) {
    serializer(transactionsCache, "details");
  }

  serializer.binary(cache, "cache");
}

void WalletLegacySerializer::decrypt(const std::string& cipher, std::string& plain, chacha_iv_t iv, const std::string& password) {
  chacha_key_t key;
  generate_chacha_key(password, key);

  plain.resize(cipher.size());

  chacha8(cipher.data(), cipher.size(), key, iv, &plain[0]);
}

void WalletLegacySerializer::loadKeys(cryptonote::ISerializer& serializer) {
  cryptonote::KeysStorage keys;

  keys.serialize(serializer, "keys");

  cryptonote::account_keys_t acc;
  acc.address.spendPublicKey = keys.spendPublicKey;
  acc.spendSecretKey = keys.spendSecretKey;
  acc.address.viewPublicKey = keys.viewPublicKey;
  acc.viewSecretKey = keys.viewSecretKey;

  account.setAccountKeys(acc);
  account.setCreatetime(keys.creationTimestamp);
}

}
