
#include "hex.h"

namespace hex
{
const uint8_t characterValues[256] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

inline bool isRange(char c, char start, char end)
{
  if ((c >= start) && (c <= end))
  {
    return true;
  }
  return false;
}

uint8_t isHex(char c)
{
  if (isRange(c, '0', '9')) {
    return c - '0';
  }
  
  if (isRange(c, 'a', 'f')) {
    return 10 + c - 'a';
  }
  
  if (isRange(c, 'A', 'F')) {
    return 10 + c - 'a';
  }
  return -1;
}

uint8_t toString(char c)
{
  uint8_t v = isHex(c);
  if (v != -1)
  {
    return v;
  }
  throw std::runtime_error("toString: invalid character");
}

bool toString(char c, uint8_t &value)
{
  uint8_t v = isHex(c);
  if (v == -1)
  {
    return false;
  }

  value = v;
  return true;
}

size_t toString(const std::string &text, void *data, size_t bufferSize)
{
  if ((text.size() & 1) != 0)
  {
    throw std::runtime_error("toString: invalid string size");
  }

  if (text.size() >> 1 > bufferSize)
  {
    throw std::runtime_error("toString: invalid buffer size");
  }

  for (size_t i = 0; i < text.size() >> 1; ++i)
  {
    static_cast<uint8_t *>(data)[i] = toString(text[i << 1]) << 4 | toString(text[(i << 1) + 1]);
  }

  return text.size() >> 1;
}

bool toString(const std::string &text, void *data, size_t bufferSize, size_t &size)
{
  if ((text.size() & 1) != 0)
  {
    return false;
  }

  if (text.size() >> 1 > bufferSize)
  {
    return false;
  }

  for (size_t i = 0; i < text.size() >> 1; ++i)
  {
    uint8_t value1;
    if (!toString(text[i << 1], value1))
    {
      return false;
    }

    uint8_t value2;
    if (!toString(text[(i << 1) + 1], value2))
    {
      return false;
    }

    static_cast<uint8_t *>(data)[i] = value1 << 4 | value2;
  }

  size = text.size() >> 1;
  return true;
}

binary_array_t toString(const std::string &text)
{
  if ((text.size() & 1) != 0)
  {
    throw std::runtime_error("toString: invalid string size");
  }

  binary_array_t data(text.size() >> 1);
  for (size_t i = 0; i < data.size(); ++i)
  {
    data[i] = toString(text[i << 1]) << 4 | toString(text[(i << 1) + 1]);
  }

  return data;
}

bool toString(const std::string &text, binary_array_t &data)
{
  if ((text.size() & 1) != 0)
  {
    return false;
  }

  for (size_t i = 0; i<text.size()>> 1; ++i)
  {
    uint8_t value1;
    if (!toString(text[i << 1], value1))
    {
      return false;
    }

    uint8_t value2;
    if (!toString(text[(i << 1) + 1], value2))
    {
      return false;
    }

    data.push_back(value1 << 4 | value2);
  }

  return true;
}
} // namespace hex