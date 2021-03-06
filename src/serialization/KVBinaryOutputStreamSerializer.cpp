// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "KVBinaryOutputStreamSerializer.h"
#include "KVBinaryCommon.h"

#include <cassert>
#include <stdexcept>
#include <stream/writer.h>
#include <limits>

using namespace Common;
using namespace cryptonote;

namespace {

template <typename T>
void writePod(std::ostringstream& s, const T& value) {
  s.write((const char *)&value, sizeof(T));
}

template<class T>
size_t packVarint(std::ostringstream& s, uint8_t type_or, size_t pv) {
  T v = static_cast<T>(pv << 2);
  v |= type_or;
  s.write((const char *)&v, sizeof(T));
  return sizeof(T);
}

void writeElementName(std::ostringstream& s, Common::StringView name) {
  if (name.getSize() > std::numeric_limits<uint8_t>::max()) {
    throw std::runtime_error("Element name is too long");
  }

  uint8_t len = static_cast<uint8_t>(name.getSize());
  s.write((char *)&len, sizeof(len));
  s.write(name.getData(), len);
}

size_t writeArraySize(std::ostringstream& s, size_t val) {
  if (val <= 63) {
    return packVarint<uint8_t>(s, PORTABLE_RAW_SIZE_MARK_BYTE, val);
  } else if (val <= 16383) {
    return packVarint<uint16_t>(s, PORTABLE_RAW_SIZE_MARK_WORD, val);
  } else if (val <= 1073741823) {
    return packVarint<uint32_t>(s, PORTABLE_RAW_SIZE_MARK_DWORD, val);
  } else {
    if (val > 4611686018427387903) {
      throw std::runtime_error("failed to pack varint - too big amount");
    }
    return packVarint<uint64_t>(s, PORTABLE_RAW_SIZE_MARK_INT64, val);
  }
}

}

namespace cryptonote {

KVBinaryOutputStreamSerializer::KVBinaryOutputStreamSerializer() {
  beginObject(std::string());
}

void KVBinaryOutputStreamSerializer::dump(Writer& s) {
  assert(m_objectsStack.size() == 1);
  assert(m_stack.size() == 1);

  KVBinaryStorageBlockHeader hdr;
  hdr.m_signature_a = PORTABLE_STORAGE_SIGNATUREA;
  hdr.m_signature_b = PORTABLE_STORAGE_SIGNATUREB;
  hdr.m_ver = PORTABLE_STORAGE_FORMAT_VER;

  std::ostringstream oss;
  oss.write((char *)&hdr, sizeof(hdr));
  writeArraySize(oss, m_stack.front().count);
  oss.write(stream().str().c_str(), stream().str().length());
  s.write(oss.str().c_str(), oss.str().length());
}

ISerializer::SerializerType KVBinaryOutputStreamSerializer::type() const {
  return ISerializer::OUTPUT;
}

bool KVBinaryOutputStreamSerializer::beginObject(Common::StringView name) {
  checkArrayPreamble(BIN_KV_SERIALIZE_TYPE_OBJECT);
 
  m_stack.push_back(Level(name));
  m_objectsStack.push_back(std::ostringstream());

  return true;
}

void KVBinaryOutputStreamSerializer::endObject() {
  assert(m_objectsStack.size());

  auto level = std::move(m_stack.back());
  m_stack.pop_back();


  // Pop the last stream
  auto objStream = std::move(m_objectsStack.back());
  m_objectsStack.pop_back();

  // Get the last stream after pop
  auto& s = stream();

  writeElementPrefix(BIN_KV_SERIALIZE_TYPE_OBJECT, level.name);

  writeArraySize(s, level.count);

  // Append the popped stream into the last stream
  s.write(objStream.str().c_str(), objStream.str().length());
}

bool KVBinaryOutputStreamSerializer::beginArray(size_t& size, Common::StringView name) {
  m_stack.push_back(Level(name, size));
  return true;
}

void KVBinaryOutputStreamSerializer::endArray() {
  bool validArray = m_stack.back().state == State::Array;
  m_stack.pop_back();

  if (m_stack.back().state == State::Object && validArray) {
    ++m_stack.back().count;
  }
}

bool KVBinaryOutputStreamSerializer::operator()(uint8_t& value, Common::StringView name) {
  writeElementPrefix(BIN_KV_SERIALIZE_TYPE_UINT8, name);
  writePod(stream(), value);
  return true;
}

bool KVBinaryOutputStreamSerializer::operator()(uint16_t& value, Common::StringView name) {
  writeElementPrefix(BIN_KV_SERIALIZE_TYPE_UINT16, name);
  writePod(stream(), value);
  return true;
}

bool KVBinaryOutputStreamSerializer::operator()(int16_t& value, Common::StringView name) {
  writeElementPrefix(BIN_KV_SERIALIZE_TYPE_INT16, name);
  writePod(stream(), value);
  return true;
}

bool KVBinaryOutputStreamSerializer::operator()(uint32_t& value, Common::StringView name) {
  writeElementPrefix(BIN_KV_SERIALIZE_TYPE_UINT32, name);
  writePod(stream(), value);
  return true;
}

bool KVBinaryOutputStreamSerializer::operator()(int32_t& value, Common::StringView name) {
  writeElementPrefix(BIN_KV_SERIALIZE_TYPE_INT32, name);
  writePod(stream(), value);
  return true;
}

bool KVBinaryOutputStreamSerializer::operator()(int64_t& value, Common::StringView name) {
  writeElementPrefix(BIN_KV_SERIALIZE_TYPE_INT64, name);
  writePod(stream(), value);
  return true;
}

bool KVBinaryOutputStreamSerializer::operator()(uint64_t& value, Common::StringView name) {
  writeElementPrefix(BIN_KV_SERIALIZE_TYPE_UINT64, name);
  writePod(stream(), value);
  return true;
}

bool KVBinaryOutputStreamSerializer::operator()(bool& value, Common::StringView name) {
  writeElementPrefix(BIN_KV_SERIALIZE_TYPE_BOOL, name);
  writePod(stream(), value);
  return true;
}

bool KVBinaryOutputStreamSerializer::operator()(double& value, Common::StringView name) {
  writeElementPrefix(BIN_KV_SERIALIZE_TYPE_DOUBLE, name);
  writePod(stream(), value);
  return true;
}

bool KVBinaryOutputStreamSerializer::operator()(std::string& value, Common::StringView name) {
  writeElementPrefix(BIN_KV_SERIALIZE_TYPE_STRING, name);

  auto& s = stream();
  writeArraySize(s, value.size());
  s.write(value.data(), value.size());
  return true;
}

bool KVBinaryOutputStreamSerializer::binary(void* value, size_t size, Common::StringView name) {
  if (size > 0) {
    writeElementPrefix(BIN_KV_SERIALIZE_TYPE_STRING, name);
    auto& s = stream();
    writeArraySize(s, size);
    s.write((char *)value, size);
  }
  return true;
}

bool KVBinaryOutputStreamSerializer::binary(std::string& value, Common::StringView name) {
  return binary(const_cast<char*>(value.data()), value.size(), name);
}

void KVBinaryOutputStreamSerializer::writeElementPrefix(uint8_t type, Common::StringView name) {  
  assert(m_stack.size());

  checkArrayPreamble(type);
  Level& level = m_stack.back();
  
  if (level.state != State::Array) {
    if (!name.isEmpty()) {
      auto& s = stream();
      writeElementName(s, name);
      s.write((char *)&type, 1);
    }
    ++level.count;
  }
}

void KVBinaryOutputStreamSerializer::checkArrayPreamble(uint8_t type) {
  if (m_stack.empty()) {
    return;
  }

  Level& level = m_stack.back();

  if (level.state == State::ArrayPrefix) {
    auto& s = stream();
    writeElementName(s, level.name);
    char c = BIN_KV_SERIALIZE_FLAG_ARRAY | type;
    s.write(&c, 1);
    writeArraySize(s, level.count);
    level.state = State::Array;
  }
}


std::ostringstream& KVBinaryOutputStreamSerializer::stream() {
  assert(m_objectsStack.size());
  return m_objectsStack.back();
}

}
