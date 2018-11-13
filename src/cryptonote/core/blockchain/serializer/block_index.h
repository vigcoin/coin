// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

#include "crypto/hash.h"
#include <vector>

namespace cryptonote
{
  class ISerializer;

  class BlockIndex {

  public:

    BlockIndex() : m_index(m_container.get<1>()) {}

    void pop() {
      m_container.pop_back();
    }

    // returns true if new element was inserted, false if already exists
    bool push(const crypto::hash_t& h) {
      auto result = m_container.push_back(h);
      return result.second;
    }

    bool hasBlock(const crypto::hash_t& h) const {
      return m_index.find(h) != m_index.end();
    }

    bool getBlockHeight(const crypto::hash_t& h, uint32_t& height) const {
      auto hi = m_index.find(h);
      if (hi == m_index.end())
        return false;

      height = static_cast<uint32_t>(std::distance(m_container.begin(), m_container.project<0>(hi)));
      return true;
    }

    uint32_t size() const {
      return static_cast<uint32_t>(m_container.size());
    }

    void clear() {
      m_container.clear();
    }

    crypto::hash_t getBlockId(uint32_t height) const;
    std::vector<crypto::hash_t> getBlockIds(uint32_t startBlockIndex, uint32_t maxCount) const;
    bool findSupplement(const std::vector<crypto::hash_t>& ids, uint32_t& offset) const;
    std::vector<crypto::hash_t> buildSparseChain(const crypto::hash_t& startBlockId) const;
    crypto::hash_t getTailId() const;

    void serialize(ISerializer& s);

  private:

    typedef boost::multi_index_container <
      crypto::hash_t,
      boost::multi_index::indexed_by<
        boost::multi_index::random_access<>,
        boost::multi_index::hashed_unique<boost::multi_index::identity<crypto::hash_t>>
      >
    > ContainerT;

    ContainerT m_container;
    ContainerT::nth_index<1>::type& m_index;

  };
}
