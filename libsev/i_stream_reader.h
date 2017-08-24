/*

Copyright (C) 2017  by authors
Author: Jan Boon <support@no-break.space>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef SEV_I_STREAM_READER_H
#define SEV_I_STREAM_READER_H

#include <utility>
#include <string>
#include <vector>

#include "event_loop.h"

namespace sev {

#define SEV_STREAM_READER_BUFFER_DEFAULT (16 * 1024)

//! Stream reader
class IStreamReader
{
public:
	IStreamReader(EventFiber *ef, IStream *stream, size_t buffer = SEV_STREAM_READER_BUFFER_DEFAULT);
	IStreamReader(char *buffer, size_t index, size_t length);
	IStreamReader(std::shared_ptr<char> buffer, size_t index, size_t length);
	virtual ~IStreamReader();
	
private:
	EventFiber *m_EventFiber;
	IStream *m_Stream;
	std::unique_ptr<char> m_UniqueBuffer;
	std::shared_ptr<char> m_SharedBuffer;
	char *m_Buffer;
	size_t m_Index;
	size_t m_Length;
	bool m_ReadError;
	
public:
	//! Must update the event fiber whenever ownership is transfered over to another event fiber. Event fiber is only required when reading from a stream
	inline void setEventFiber(EventFiber *ef) { m_EventFiber = ef; }
	
	//! Check after reading if read error occured
	inline bool hasReadError() { return m_ReadError; }
	
public:
	// Primitives
	
	//! Reads a buffer directly. Returns different value from given length on EOF (end of file) only
	virtual size_t readBuffer(char *buffer, size_t index, size_t length) = 0;
	
	//! Reads a bool. Default to false on EOF
	inline bool readBool() = 0;
	
	//! Reads an integer. Default to 0 on EOF
	virtual int8_t readInt8LE() = 0;
	virtual int16_t readInt16LE() = 0;
	virtual int32_t readInt32LE() = 0;
	virtual int64_t readInt64LE() = 0;
	
	virtual uint8_t readUInt8LE() = 0;
	virtual uint16_t readUInt16LE() = 0;
	virtual uint32_t readUInt32LE() = 0;
	virtual uint64_t readUInt64LE() = 0;
	
	virtual int8_t readInt8LE(int bits) = 0;
	virtual int16_t readInt16LE(int bits) = 0;
	virtual int32_t readInt32LE(int bits) = 0;
	virtual int64_t readInt64LE(int bits) = 0;
	
	virtual uint8_t readUInt8LE(int bits) = 0;
	virtual uint16_t readUInt16LE(int bits) = 0;
	virtual uint32_t readUInt32LE(int bits) = 0;
	virtual uint64_t readUInt64LE(int bits) = 0;
	
public:
	// Aliases
	inline size_t readSize() { return (size_t)readUInt64LE(); }
	inline char readByte() { return (char)readUInt8LE(); }
	
	inline int8_t readInt8() { return readInt8LE(); }
	inline int16_t readInt16() { return readInt16LE(); }
	inline int32_t readInt32() { return readInt32LE(); }
	inline int64_t readInt64() { return readInt64LE(); }
	
	inline uint8_t readUInt8() { return readUInt8LE(); }
	inline uint16_t readUInt16() { return readUInt16LE(); }
	inline uint32_t readUInt32() { return readUInt32LE(); }
	inline uint64_t readUInt64() { return readUInt64LE(); }
	
	inline int8_t readInt8(int bits) { return readInt8LE(); }
	inline int16_t readInt16(int bits) { return readInt16LE(); }
	inline int32_t readInt32(int bits) { return readInt32LE(); }
	inline int64_t readInt64(int bits) { return readInt64LE(); }
	
	inline uint8_t readUInt8(int bits) { return readUInt8LE(); }
	inline uint16_t readUInt16(int bits) { return readUInt16LE(); }
	inline uint32_t readUInt32(int bits) { return readUInt32LE(); }
	inline uint64_t readUInt64(int bits) { return readUInt64LE(); }
	
public:
	// Templates
	
	// Reads a string. Always assume UTF-8
	std::string readString();
	
	template<T, U>
	inline std::pair<T, U> readPair();
	
	template<T>
	T readContainer();
	
public:
	// Auto
	template<T> inline void read(T &v) { v.readStream(*this); }
	template<> inline void read<bool>(bool &v) { v = readBool(); }
	template<> inline void read<size_t>(size_t &v) { v = readSize(); }
	template<> inline void read<int8_t>(int8_t &v) { v = readInt8(); }
	template<> inline void read<int16_t>(int16_t &v) { v = readInt16(); }
	template<> inline void read<int32_t>(int32_t &v) { v = readInt32(); }
	template<> inline void read<int64_t>(int64_t &v) { v = readInt64(); }
	template<> inline void read<uint8_t>(uint8_t &v) { v = readUInt8(); }
	template<> inline void read<uint16_t>(uint16_t &v) { v = readUInt16(); }
	template<> inline void read<uint32_t>(uint32_t &v) { v = readUInt32(); }
	template<> inline void read<uint64_t>(uint64_t &v) { v = readUInt64(); }
	template<> inline void read<std::string>(std::string &v) { v = readString(); }
	template<U, V> inline void read<std::pair<U, V>>(std::pair<U, V> &v) { v = readPair<U, V>(); }
	
	template<T> inline T read() { T v; read(v); return v; }
	template<T> inline void serial(T &v) { read(v); }
	
	template<T> inline void serialContainer(T &v) { v = readContainer<T>(); }
	
private:
    IStreamReader(IStreamReader const&) = delete;
    IStreamReader& operator=(IStreamReader const&) = delete;
	
}; /* class IStreamReader */

} /* namespace sev */

#include "i_stream_reader.inl"

#endif /* #ifndef SEV_I_STREAM_READER_H */

/* end of file */
