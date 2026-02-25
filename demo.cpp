#include "bstream.h"
#include <iostream>
#include <vector>
#include <iomanip>

int main() {
    // 示例1：使用内存输入流和向量输出流进行编码
    {
        std::cout << "示例1：编码数据到 vector" << std::endl;
        
        // 准备输出目标
        std::vector<uint8_t> encoded;
        VectorSink sink(encoded);
        basic_obstream<VectorSink> out(sink);
        
        // 设置默认宽度为 5 位
        out.set_width(5);
        
        // 写入一些数据（使用运算符）
        out << 0x1F << 0x0A << 0x15;   // 三个 5 位值
        
        // 改变宽度为 8 位，写入一个字节
        out.set_width(8);
        out << 0xAB;
        
        // 直接调用 write_bits 写入 12 位
        out.write_bits(0x123, 12);
        
        // 对齐到字节边界（填充0）
        out.align();
        
        // 写入剩余数据
        out.write_bits(0xDEADBEEF, 32);
        out.flush();
        
        std::cout << "编码后字节数: " << encoded.size() << std::endl;
        std::cout << "编码内容: ";
        for (auto b : encoded) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << ' ';
        }
        std::cout << std::dec << std::endl;
        
        // 现在用输入流解码
        MemorySource src(encoded.data(), encoded.size());
        basic_ibstream<MemorySource> in(src);
        
        uint64_t val;
        in.set_width(5);
        in >> val; std::cout << "读取 5 位: " << val << std::endl;
        in >> val; std::cout << "读取 5 位: " << val << std::endl;
        in >> val; std::cout << "读取 5 位: " << val << std::endl;
        
        in.set_width(8);
        in >> val; std::cout << "读取 8 位: " << std::hex << val << std::dec << std::endl;
        
        val = in.read_bits(12);
        std::cout << "读取 12 位: " << std::hex << val << std::dec << std::endl;
        
        in.align();
        
        val = in.read_bits(32);
        std::cout << "读取 32 位: " << std::hex << val << std::dec << std::endl;
    }
    
    // 示例2：使用固定缓冲区输出
    {
        std::cout << "\n示例2：固定缓冲区输出" << std::endl;
        uint8_t buffer[16];
        FixedSink sink(buffer, sizeof(buffer));
        basic_obstream<FixedSink> out(sink);
        
        out.write_bits(0x12345678, 32);
        out.write_bits(0x9ABC, 16);
        out.flush();
        
        std::cout << "固定缓冲区内容: ";
        for (size_t i = 0; i < 6; ++i) { // 32+16=48位=6字节
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[i] << ' ';
        }
        std::cout << std::dec << std::endl;
    }
    
    // 示例3：使用 setbw 语法
    {
        std::cout << "\n示例3：使用 setbw 设置宽度" << std::endl;
        std::vector<uint8_t> data;
        VectorSink sink(data);
        basic_obstream<VectorSink> out(sink);
        
        out << setbw(4) << 0xC << setbw(6) << 0x2A << setbw(8) << 0xFF;
        out.flush();
        
        MemorySource src(data.data(), data.size());
        basic_ibstream<MemorySource> in(src);
        
        uint64_t a, b, c;
        in >> setbw(4) >> a >> setbw(6) >> b >> setbw(8) >> c;
        std::cout << "a=" << a << " b=" << b << " c=" << c << std::endl;
    }
    
    return 0;
}
