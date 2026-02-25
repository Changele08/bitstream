# bitstream
Header-only C++ bitstream library supporting arbitrary bit width operations, seeking, and stream syntax (&lt;&lt; / >>).

BitStream 是一个头文件-only 的 C++17 库，提供高效的比特级读写操作。它允许你以任意位宽度读写数据，支持内存缓冲区、std::vector 和固定大小缓冲区，并提供了流式语法（<</>>）和宽度设置。


<div align="right">
  <a href="README.en.md">English</a> | <a href="README.md">简体中文</a>
</div>

## Features

* Arbitrary bit-width read/write: Supports reading and writing from 0 to 64 bits.
* Multiple data sources/sinks: Built‑in MemorySource (raw memory block), VectorSource (std::vector), VectorSink (dynamically growing), and FixedSink (fixed‑size buffer).
* Stream‑like syntax: Use the << and >> operators together with setbw to set the current width.
* Random access: Input streams support absolute/relative positioning (seek_abs/seek_rel) and random bit access (operator[]).
* Peek and skip: peek_bits lets you look ahead without moving the pointer; skip jumps over a given number of bits.
* Byte alignment: align() moves the read/write position to the next byte boundary.
* Bulk read/write: bread/bwrite can read or write entire structures or arrays in one go.
* Extensible: By implementing a few simple interfaces (size(), operator[], emplace_back), you can support any custom data source or sink.

## Quick Start

```c++
#include "bstream.h"
...
int main()
{
  vector<BYTE> encoded;
  ...
  MemorySource src(encoded.data(), encoded.size());
  basic_ibstream<MemorySource> bin(src)

  BYTE ch;
  bin >> setbw(8) >> ch; // read 8 bit to ch
}
```

## Custom Source / Sink

The library already provides sources and sinks based on std::vector and raw pointers.

You can implement your own Source or Sink by satisfying the following requirements:

1. Source requirements
* size_t size() const – returns the total number of bytes in the source.
* BYTE operator[](size_t index) const – returns the byte at position index.
2. Sink requirements
* void emplace_back(BYTE byte) – appends a byte to the sink.
* size_t size() const – returns the number of bytes already written.
* BYTE& operator[](size_t index) – returns a reference to the byte at position index.

## Important Notes

*Bit order: Both reading and writing follow LSB first (least significant bit first). That is, the first bit written becomes the least significant bit of the byte, and the first bit read also comes from the least significant bit.
Error handling: An attempt to read beyond the end of the source throws std::out_of_range; writing beyond the capacity of a fixed‑size sink also throws std::out_of_range.
*Performance: Each operation directly reads or writes memory without using a large internal cache. If you need extreme performance, you can add your own buffering layer on top.
*Thread safety: Stream objects are not thread‑safe. Concurrent access from multiple threads must be synchronized externally.
