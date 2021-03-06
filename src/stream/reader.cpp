
#include "reader.h"
#include <limits>

Reader::Reader(std::istream &in) : in(in)
{
}

size_t Reader::readSome(void *data, size_t size)
{
  in.read(static_cast<char *>(data), size);
  return in.gcount();
}

size_t Reader::getPosition() const
{
  return in.tellg();
}

bool Reader::endOfStream() const
{
  return in.peek() == EOF;
}

void Reader::read(void *data, size_t size)
{
  while (size > 0)
  {
    size_t readSize = readSome(data, size);
    if (readSize == 0)
    {
      throw std::runtime_error("Failed to read from IInputStream");
    }

    data = static_cast<uint8_t *>(data) + readSize;
    size -= readSize;
  }
}

void Reader::read(std::vector<uint8_t> &data, size_t size)
{
  data.resize(size);
  read(data.data(), size);
}

void Reader::read(std::string &data, size_t size)
{
  std::vector<char> temp(size);
  read(temp.data(), size);
  data.assign(temp.data(), size);
}

void Reader::readHeight(size_t &blockHeight)
{
  uint64_t height;
  *this >> height;
  if (height == std::numeric_limits<uint64_t>::max())
  {
    blockHeight = std::numeric_limits<uint32_t>::max();
  }
  else if (height > std::numeric_limits<uint32_t>::max() && height < std::numeric_limits<uint64_t>::max())
  {
    throw std::runtime_error("Deserialization error: wrong value");
  }
  else
  {
    blockHeight = static_cast<uint32_t>(height);
  }
}

Reader &operator>>(Reader &i, std::string &v)
{
  uint64_t size;

  i >> size;

  if (size > 0)
  {
    std::vector<char> temp;
    temp.resize(size);
    i.read(&temp[0], size);
    v.reserve(size);
    v.assign(&temp[0], size);
  }
  else
  {
    v.clear();
  }
  return i;
}