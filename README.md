# bitstream
Header-only C++ bitstream library supporting arbitrary bit width operations, seeking, and stream syntax (&lt;&lt; / >>).

BitStream 是一个头文件-only 的 C++17 库，提供高效的比特级读写操作。它允许你以任意位宽度读写数据，支持内存缓冲区、std::vector 和固定大小缓冲区，并提供了流式语法（<</>>）和宽度设置。

<div align="right">
  <a href="README.en.md">English</a> | <a href="README.md">简体中文</a>
</div>

## 特性

* 任意位宽读写：支持 0~64 位的读取和写入。
* 多种数据源/目的：内置 MemorySource（内存块）、VectorSource（std::vector）、VectorSink（动态增长）、FixedSink（固定缓冲区）。
* 流式语法：通过 << 和 >> 操作符，配合 setbw 设置当前宽度。
* 随机访问：输入流支持绝对/相对定位（seek_abs/seek_rel）和随机位访问（operator[]）。
* 预读与跳过：peek_bits 预读不移动指针，skip 跳过指定位。
* 字节对齐：align() 将读写位置对齐到字节边界。
* 批量读写：bread/bwrite 一次性读写结构体或数组。
* 可扩展：通过实现简单的接口（size()、operator[]、emplace_back），你可以支持任意自定义数据源/目标。

## 快速开始

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
## 自定义 Source/Sink

已经实现了基于 vector 和 裸指针的 source & sink

你可以实现自己的 Source 或 Sink，只需满足以下接口：

1. Source 要求
- size_t size() const：返回数据总字节数。
- BYTE operator[](size_t index) const：返回第 index 字节。

2. Sink 要求
- void emplace_back(BYTE byte)：追加一个字节。
- size_t size() const：返回当前已写入字节数。
- BYTE& operator[](size_t index): 返回第 index 字节。

## 注意事项
* 位序：读写均遵循 LSB 优先（低位在前）。即第一个写入的位成为字节的最低位，第一个读取的位也来自最低位。
* 错误处理：当读取超出范围时抛出 std::out_of_range；写入固定缓冲区溢出时同样抛出异常。
* 性能：每次操作均直接读写内存，未使用大缓存。若需极致性能，可自行在外层增加缓冲。
* 线程安全：每个流对象不是线程安全的，多线程访问需外部同步。
