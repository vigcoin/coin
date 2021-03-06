// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "BinaryInputStreamSerializer.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <stream/writer.h>
#include "SerializationOverloads.h"

using namespace Common;

namespace cryptonote
{

  ISerializer::SerializerType BinaryInputStreamSerializer::type() const
  {
    return ISerializer::INPUT;
  }

  bool BinaryInputStreamSerializer::beginObject(Common::StringView name)
  {
    return true;
  }

  void BinaryInputStreamSerializer::endObject()
  {
  }

  bool BinaryInputStreamSerializer::beginArray(size_t &size, Common::StringView name)
  {
    uint64_t temp;
    i >> temp;
    size = temp;

    return true;
  }

  void BinaryInputStreamSerializer::endArray()
  {
  }


  bool BinaryInputStreamSerializer::operator()(uint8_t &value, Common::StringView name)
  {
    i >> value;
    return true;
  }

  bool BinaryInputStreamSerializer::operator()(uint16_t &value, Common::StringView name)
  {
    i >> value;
    return true;
  }

  bool BinaryInputStreamSerializer::operator()(int16_t &value, Common::StringView name)
  {
    i >> value;
    return true;
  }

  bool BinaryInputStreamSerializer::operator()(uint32_t &value, Common::StringView name)
  {
    i >> value;
    return true;
  }

  bool BinaryInputStreamSerializer::operator()(int32_t &value, Common::StringView name)
  {
    i >> value;
    return true;
  }

  bool BinaryInputStreamSerializer::operator()(int64_t &value, Common::StringView name)
  {
    i >> value;
    return true;
  }

  bool BinaryInputStreamSerializer::operator()(uint64_t &value, Common::StringView name)
  {
    i >> value;
    return true;
  }

  bool BinaryInputStreamSerializer::operator()(bool &value, Common::StringView name)
  {
    i >> value;
    return true;
  }

  bool BinaryInputStreamSerializer::operator()(std::string &value, Common::StringView name)
  {
    uint64_t size;

    i >> size;

    if (size > 0)
    {
      std::vector<char> temp;
      temp.resize(size);
      i.read(&temp[0], size);
      value.reserve(size);
      value.assign(&temp[0], size);
    }
    else
    {
      value.clear();
    }

    return true;
  }

  bool BinaryInputStreamSerializer::binary(void *value, size_t size, Common::StringView name)
  {
    i.read(static_cast<char *>(value), size);
    return true;
  }

  bool BinaryInputStreamSerializer::binary(std::string &value, Common::StringView name)
  {
    return (*this)(value, name);
  }

  bool BinaryInputStreamSerializer::operator()(double &value, Common::StringView name)
  {
    assert(false); //the method is not supported for this type of serialization
    throw std::runtime_error("double serialization is not supported in BinaryInputStreamSerializer");
    return false;
  }

}
