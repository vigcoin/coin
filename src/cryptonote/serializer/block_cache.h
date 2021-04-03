#include "cryptonote/types.h"
#include "stream/serialize.h"
#include "block_index.h"
#include "cryptonote.h"

using namespace serialize;

class BlockCacheStream : public serialize::IStream
{
    BlockCacheStream(BlockIndexStream &index);
    bool serialize(std::ostream &o);
    bool serialize(std::istream &i);
    uint8_t version;
    hash_t m_lastBlockHash;
    BlockIndexStream &index;
    transaction_map_t map;
    key_images_container_t m_spent_keys;
};