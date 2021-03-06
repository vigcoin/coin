#include "timestamp_transaction.h"
#include "cryptonote/crypto/hash.h"
#include "cryptonote/core/key.h"
#include "cryptonote/core/CryptoNoteTools.h"

namespace cryptonote
{

  bool TimestampTransactionsIndex::add(uint64_t timestamp, const hash_t &hash)
  {
    index.emplace(timestamp, hash);
    return true;
  }

  bool TimestampTransactionsIndex::remove(uint64_t timestamp, const hash_t &hash)
  {
    auto range = index.equal_range(timestamp);
    for (auto iter = range.first; iter != range.second; ++iter)
    {
      if (iter->second == hash)
      {
        index.erase(iter);
        return true;
      }
    }

    return false;
  }

  bool TimestampTransactionsIndex::find(uint64_t timestampBegin, uint64_t timestampEnd, uint64_t hashesNumberLimit, std::vector<hash_t> &hashes, uint64_t &hashesNumberWithinTimestamps)
  {
    uint32_t hashesNumber = 0;
    if (timestampBegin > timestampEnd)
    {
      //std::swap(timestampBegin, timestampEnd);
      return false;
    }
    auto begin = index.lower_bound(timestampBegin);
    auto end = index.upper_bound(timestampEnd);

    hashesNumberWithinTimestamps = static_cast<uint32_t>(std::distance(begin, end));

    for (auto iter = begin; iter != end && hashesNumber < hashesNumberLimit; ++iter)
    {
      ++hashesNumber;
      hashes.emplace_back(iter->second);
    }
    return hashesNumber > 0;
  }

  void TimestampTransactionsIndex::clear()
  {
    index.clear();
  }

  Reader &operator>>(Reader &i, TimestampTransactionsIndex &v)
  {
    i >> v.index;
    return i;
  }

  Writer &operator<<(Writer &o, const TimestampTransactionsIndex &v)
  {
    o << v.index;
    return o;
  }
} // namespace cryptonote