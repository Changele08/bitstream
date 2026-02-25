#include <stdexcept>
#include <type_traits>
#include <algorithm>
#include <vector> 

#define BYTE unsigned char
#define WORD unsiged short
#define DWORD unsigned
#define QWORD unsigned long long

constexpr int mask[9] = {0, 1, 3, 7, 15, 31, 63, 127, 255};

template<typename Source>
// Source 需提供如下操作：
// - size() 以获取总大小 
// - 重载 [] 以访问流内任意字节。

// basic_ibstream : 提供比特流读取、随机比特读取、绝对|相对偏移定位。
// 允许使用 >> 读取 bit

// ##NOTE 读取 bit 的顺序为：buffer[0] -> buffer[n], 每字节从低比特位到高比特位。 
class basic_ibstream
{
private:
	Source buffer;
	QWORD bufp;
	int bitp, width;// width: 一次读取多少个 bit 
	int curByte;	// curbyte: 当前读取到的字节 
public:
	
	basic_ibstream() : bufp(0), width(0), curByte(0), bitp(0) {}
	basic_ibstream(const Source &data)
	{
		set_buffer(data);
	}
	basic_ibstream(const basic_ibstream<Source> &equ)
	{
		buffer = equ.get_buffer();
		curByte = equ.get_curbyte();
		bitp = equ.get_bitptr();
		bufp = equ.get_bufptr();
		width = equ.get_width();
	}
	
	basic_ibstream<Source> &operator = (const basic_ibstream &equ)
	{
		buffer = equ.get_buffer();
		curByte = equ.get_curbyte();
		bitp = equ.get_bitptr();
		bufp = equ.get_bufptr();
		width = equ.get_width();
		return *this;
	}
	
	void set_buffer(const Source &data)
	{
		buffer = data;
		curByte = data[0];
		bitp = 0;
		bufp = 1;
		width = 0;
	}
	
	Source get_buffer() const
	{
		return buffer;
	}
	
	void set_width(int wd)
	{
		width = wd;
	}
	
	int get_width() const
	{
		return width;
	}
	
	int get_curbyte() const
	{
		return curByte;
	}
	int get_bitptr() const
	{
		return bitp;
	}
	int get_bufptr() const
	{
		return bufp;
	}
	
	// 读取 wd 个 bit 位 
	QWORD read_bits(int wd)
	{
		int count = wd, ti = wd;
		QWORD ret = 0;
		while (count)
		{
			if (bitp == 8)
			{
				if (bufp >= buffer.size())
					throw std::out_of_range("basic_ibstream::read_bits index out of range");
				curByte = buffer[bufp++];
				bitp = 0;
			}
			
			int mov = std::min(8 - bitp, count);
			ret |= (QWORD)((curByte & mask[mov])) << (ti - count);
			
			curByte >>= mov;
			bitp += mov;
			count -= mov;
		}
		return ret;
	}
	
	// 快速跳过 wd 个 bit 位 
	void skip(int wd)
	{
		if (wd <= 0)
			return;
		
		bitp += wd;
		if (bitp < 8)
			curByte >>= wd;
		else
		{
			if ((bufp + bitp >> 3) - 1 >= buffer.size())
				throw std::out_of_range("basic_ibstream::skip index out of range");
			curByte = buffer[(bufp += bitp >> 3) - 1] >> (bitp &= 7);
		}
	}
	
	// 预取 wd 个 bit 位，不移动指针
	QWORD peek_bits(int wd)
	{
		int bufp_back = bufp, bitp_back = bitp, curByte_back = curByte;
		QWORD ret = read_bits(wd);
		bufp = bufp_back;
		bitp = bitp_back;
		curByte = curByte_back;
		return ret; 
	}
	
	// 对字节对齐 
	void align()
	{
		if (bitp)
		{
			if (bufp >= buffer.size())
				throw std::out_of_range("basic_ibstream::align index out of range");
			curByte = buffer[bufp++];
			bitp = 0;
		}
	}
	
	// 定位到流中的第 n 个 bit，从 0 开始计数 
	void seek_abs(QWORD n)
	{
		if ((n >> 3) >= buffer.size())
			throw std::out_of_range("basic_ibstream::seek_abs index out of range");
		curByte = buffer[(bufp = (n >> 3) + 1) - 1] >> (bitp = (n & 7));
	}
	
	// 从当下开始，偏移定位 bit 
	void seek_rel(long long offset)
	{
		seek_abs(((bufp - 1) << 3) + bitp + offset);
	}
	
	// 获取当前位位置（从 0 开始）
	QWORD tell_bits() const
	{
	    return ((bufp - 1) << 3) + bitp;
	}
	
	// 判断是否已到流末尾（尝试读取时才会抛异常）
	bool eof() const
	{
	    return bufp >= buffer.size();
	}
	
	// 获取流中的任意比特 
	int operator[](QWORD index)
	{
		if ((index >> 3) >= buffer.size())
			throw std::out_of_range("basic_ibstream::operator[] index out of range");
		return (buffer[index >> 3] >> (index & 7)) & 1;
	}
	
	// 类似 fread ，将 n 个 bit（必须是 8 的倍数） 读入到一段内存。由此支持在流中读写结构体等 
	void bread(void *dst, int count)
	{
		BYTE *bytedst = (BYTE*)dst;
		int p = 0;
		count >>= 3;
		while (count --> 0)
			bytedst[p++] = read_bits(8);
	}
	
    basic_ibstream<Source>& operator >> (auto &get)
	{
        get = read_bits(width);
        return *this;
    }
};

template<typename Sink>
// Source 需提供如下操作：
// - size() 以获取总大小 
// - emplace_back() 向后追加一个字节（需要自动扩容） 
// - 重载 [] 以访问流内任意字节。

class basic_obstream
{
private:
	Sink &buffer;
	int curByte;
	int bitp, bufp, width;

public:
	basic_obstream() : bufp(0), width(0), curByte(0), bitp(0) {}
	basic_obstream(Sink &sk) : buffer(sk), curByte(0), bufp(0), width(0), bitp(0){}
	basic_obstream(const basic_obstream<Sink> &sk)
	{
		buffer = sk.get_buffer();
		curByte = sk.get_curbyte();
		bitp = sk.get_bitptr();
		bufp = sk.get_bufptr();
		width = sk.get_width();
	}
	
	basic_obstream<Sink> &operator = (const basic_obstream<Sink> &sk)
	{
		buffer = sk.get_buffer();
		curByte = sk.get_curbyte();
		bitp = sk.get_bitptr();
		bufp = sk.get_bufptr();
		width = sk.get_width();
		return *this;
	}
	
	~basic_obstream()
	{
		if (bufp == buffer.size())
			buffer.emplace_back(0);
		buffer[bufp++] = (BYTE)(curByte);
	}
	
	
	void set_buffer(Sink &sk)
	{
		buffer = sk;
		curByte = bufp = width = 0;
		bitp = 0;
	}
	
	Sink get_buffer() const
	{
		return buffer;
	}
	
	void set_width(int wd)
	{
		width = wd;
	}
	
	int get_width()
	{
		return width;
	}
	
	int get_curbyte() const
	{
		return curByte;
	}
	
	int get_bitptr() const
	{
		return bitp;
	}
	
	int get_bufptr() const
	{
		return bufp;
	}
	
	void set_bufptr(int bp)
	{
		bufp = bp;
	}
	
	void write_bits(QWORD write, int wd)
	{
		int count = wd;

		while (count)
		{
			int mov = std::min(8 - bitp, count);
			curByte |= (write & mask[mov]) << (bitp);

			write >>= mov;
			bitp += mov;
			count -= mov;

			if (bitp == 8)
			{
				if (bufp == buffer.size())
					buffer.emplace_back(0);
				buffer[bufp++] = (BYTE)(curByte);
				curByte = 0;
				bitp = 0;
			}
		}
	}
	
	void flush()
	{
		if (bitp)
		{
			if (bufp == buffer.size())
				buffer.emplace_back(0);
			buffer[bufp++] = (BYTE)(curByte);
			curByte = 0;
			bitp = 0;
		}
	}
	
	void align()
	{
		flush();
	}
	
	// 获取当前位位置（从 0 开始）
	QWORD tell_bits() const
	{
	    return ((bufp - 1) << 3) + bitp;
	}
	
	/*
	// ##NOTE : 定位到一个 bit 后，即使只更改一个位，所在的字节的低位都将丢失。
	// 细节实现的话太过复杂，将在后续版本中实现 seek_abs 和 [] 重载。 
	void seek_abs(QWORD n)
	{
		if ((n >> 3) + 1 >= buffer.size())
			throw std::out_of_range("basic_obstream::SeekAbs index out of range");
		// 保存当前的 bit
		if (bufp == buffer.size() - 1)
			buffer.emplace_back(0);
		buffer[bufp++] = (BYTE)(curByte);
		curByte = buffer[(bufp = (n >> 3) + 1) - 1] >> (bitp = 8 - (n & 7));
	}
	
	void seek_rel(long long offset)
	{
		seek_abs(((bufp - 1) << 3) + bitp + offset);
	}*/
	
	void bwrite(void *src, int count)
	{
		BYTE *bytesrc = (BYTE*)src;
		int p = 0;
		count >>= 3;
		while (count --> 0)
			write_bits(bytesrc[p++], 8);
	}
	
	basic_obstream<Sink>& operator << (auto wt)
	{
        write_bits(wt, width);
        return *this;
    }
};

class MemorySource
{
private:
	const BYTE *data;
	QWORD sizes;
public:
	MemorySource() : data(nullptr), sizes(0){}
	MemorySource(const BYTE *dat, QWORD sz) : data(dat), sizes(sz){}
	MemorySource(const MemorySource &equ) : data(equ.get_buffer()), sizes(equ.size()){}
	
	
	QWORD size() const
	{
		return sizes;
	}
	
	const BYTE *get_buffer() const
	{
		return data;
	}
	
	BYTE operator[](QWORD index) const
	{
		if (index >= sizes)
			throw std::out_of_range("MemorySource::operator[] index out of range");
		return data[index]; 
	}
};
class VectorSource
{
private:
	const std::vector<BYTE> data;
	QWORD sizes;
public:
	VectorSource() : sizes(0){}
	VectorSource(const std::vector<BYTE> dat) : data(dat), sizes(data.size()){}
	VectorSource(const VectorSource &equ) : data(equ.GetBuffer()), sizes(equ.size()){}
	
	
	QWORD size() const
	{
		return sizes;
	}
	
	const std::vector<BYTE>GetBuffer() const
	{
		return data;
	}
	
	BYTE operator[](QWORD index)
	{
		if (index >= sizes)
			throw std::out_of_range("MemorySource::operator[] index out of range");
		return data[index]; 
	}
};

class FixedSink
{
private:
    uint8_t* data_;
    size_t capacity_;
    size_t pos_;
public:
	FixedSink() : data_(nullptr), capacity_(0), pos_(0) {}
    FixedSink(uint8_t* data, size_t cap) : data_(data), capacity_(cap), pos_(0) {}
    void emplace_back(uint8_t byte)
	{
        if (pos_ >= capacity_) throw std::out_of_range("FixedSink::emplace_back overflow");
        data_[pos_++] = byte;
    }
    size_t size() const
	{
		return pos_;
	}
	
	BYTE &operator[](QWORD index)
	{
		if (index >= pos_)
			throw std::out_of_range("FixedSink::operator[] index out of range");
		return data_[index];
	}
};

class VectorSink
{
private:
    std::vector<uint8_t>& vec_;
public:
	VectorSink(std::vector<uint8_t>& vec) : vec_(vec){}
    

    void emplace_back(uint8_t byte)
	{
        vec_.emplace_back(byte);
    }

    size_t size() const
	{
		return vec_.size();
	}
	
	BYTE &operator[](QWORD index)
	{
		if (index >= vec_.size())
			throw std::out_of_range("FixedSink::operator[] index out of range");
		return vec_[index];
	}
	
	std::vector<BYTE> &get_buffer()
	{
		return vec_;
	}
	
	VectorSink &operator = (VectorSink &equ)
	{
		vec_ = equ.get_buffer();
		return *this;
	}
};

using ibstream_mem   = basic_ibstream<MemorySource>;
using ibstream_vec   = basic_ibstream<VectorSource>;
using obstream_vec   = basic_obstream<VectorSink>;
using obstream_fixed = basic_obstream<FixedSink>;


// 通过以下方式，实现以 bout << setbw(..)的方式操作bit流的输出长度
struct _SETW { int width; };

// set_bit_width
template<typename ty>
_SETW setbw(ty __n) { return {static_cast<int>(__n)}; }

template<typename vec>
basic_ibstream<vec> &operator >> (basic_ibstream<vec> &__ib, _SETW __f)
{
	__ib.set_width(__f.width);
	return __ib;
}

template<typename vec>
basic_obstream<vec> &operator << (basic_obstream<vec> &__ob, _SETW __f)
{
	__ob.set_width(__f.width);
	return __ob;
}
