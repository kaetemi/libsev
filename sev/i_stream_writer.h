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

#ifndef SEV_I_STREAM_WRITER_H
#define SEV_I_STREAM_WRITER_H

#include "self_config.h"
#ifdef SEV_MODULE_STREAM_READER_WRITER

#include <utility>
#include <type_traits>
#include <memory>
#include <string>
#include <vector>

namespace sev {
	class IStream;
	class EventFiber;

#define SEV_STREAM_WRITER_BUFFER_DEFAULT (16 * 1024)

//! Stream writer
class SEV_LIB IStreamWriter
{
public:
	IStreamWriter(EventFiber *ef, IStream *stream, size_t buffer = SEV_STREAM_WRITER_BUFFER_DEFAULT);
	IStreamWriter(char *buffer, size_t index, size_t length);
	IStreamWriter(std::shared_ptr<char> buffer, size_t index, size_t length);
	virtual ~IStreamWriter();
	
protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	EventFiber *m_EventFiber;
	IStream *m_Stream;
	std::unique_ptr<char> m_UniqueBuffer;
	std::shared_ptr<char> m_SharedBuffer;
	char *m_Buffer;
	size_t m_Index;
	size_t m_Length;
	bool m_WriteError;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
public:
	//! Must update the event fiber whenever ownership is transfered over to another event fiber. Event fiber is only required when writing from a stream
	inline void setEventFiber(EventFiber *ef) { m_EventFiber = ef; }
	
	//! Check after writing if write error occured
	inline bool hasWriteError() { return m_WriteError; }
	
public:
	// Primitives
	
	//! Writes a buffer directly. Returns different value from given length on EOF (end of file / out of space) only
	virtual size_t writeBuffer(const char *buffer, size_t index, size_t length) = 0;
	
	//! Writes a bool. Default to false on EOF
	virtual void writeBool(bool v) = 0;
	
	//! Writes an integer. Default to 0 on EOF
	virtual void writeInt8LE(int8_t v) = 0;
	virtual void writeInt16LE(int16_t v) = 0;
	virtual void writeInt32LE(int32_t v) = 0;
	virtual void writeInt64LE(int64_t v) = 0;
	
	virtual void writeUInt8LE(uint8_t v) = 0;
	virtual void writeUInt16LE(uint16_t v) = 0;
	virtual void writeUInt32LE(uint32_t v) = 0;
	virtual void writeUInt64LE(uint64_t v) = 0;
	
	virtual void writeInt8LE(int8_t v, int bits) = 0;
	virtual void writeInt16LE(int16_t v, int bits) = 0;
	virtual void writeInt32LE(int32_t v, int bits) = 0;
	virtual void writeInt64LE(int64_t v, int bits) = 0;
	
	virtual void writeUInt8LE(uint8_t v, int bits) = 0;
	virtual void writeUInt16LE(uint16_t v, int bits) = 0;
	virtual void writeUInt32LE(uint32_t v, int bits) = 0;
	virtual void writeUInt64LE(uint64_t v, int bits) = 0;
	
public:
	// Aliases
	inline void writeSize(const size_t v) { writeUInt64LE(v); }
	inline void writeChar(const char v) { writeUInt8LE(v); }
	
	inline void writeInt8(const int8_t v) { writeInt8LE(v); }
	inline void writeInt16(const int16_t v) { writeInt16LE(v); }
	inline void writeInt32(const int32_t v) { writeInt32LE(v); }
	inline void writeInt64(const int64_t v) { writeInt64LE(v); }
	
	inline void writeUInt8(const uint8_t v) { writeUInt8LE(v); }
	inline void writeUInt16(const uint16_t v) { writeUInt16LE(v); }
	inline void writeUInt32(const uint32_t v) { writeUInt32LE(v); }
	inline void writeUInt64(const uint64_t v) { writeUInt64LE(v); }
	
	inline void writeInt8(const int8_t v, int bits) { writeInt8LE(v); }
	inline void writeInt16(const int16_t v, int bits) { writeInt16LE(v); }
	inline void writeInt32(const int32_t v, int bits) { writeInt32LE(v); }
	inline void writeInt64(const int64_t v, int bits) { writeInt64LE(v); }
	
	inline void writeUInt8(const uint8_t v, int bits) { writeUInt8LE(v); }
	inline void writeUInt16(const uint16_t v, int bits) { writeUInt16LE(v); }
	inline void writeUInt32(const uint32_t v, int bits) { writeUInt32LE(v); }
	inline void writeUInt64(const uint64_t v, int bits) { writeUInt64LE(v); }
	
public:
	// Templates
	
	// Writes a string. Always assume UTF-8
	void writeString(const std::string &v);
	
	template<typename T, typename U>
	inline void writePair(const std::pair<T, U> &v);
	
	template<typename T>
	void writeContainer(const T &v);
	
public:
	// Auto
	template<typename T> inline void write(const T &v) { v.writeStream(*this); }
	template<> inline void write<bool>(const bool &v) { writeBool(v); }
	template<> inline void write<int8_t>(const int8_t &v) { writeInt8(v); }
	template<> inline void write<int16_t>(const int16_t &v) { writeInt16(v); }
	template<> inline void write<int32_t>(const int32_t &v) { writeInt32(v); }
	template<> inline void write<int64_t>(const int64_t &v) { writeInt64(v); }
	template<> inline void write<uint8_t>(const uint8_t &v) { writeUInt8(v); }
	template<> inline void write<uint16_t>(const uint16_t &v) { writeUInt16(v); }
	template<> inline void write<uint32_t>(const uint32_t &v) { writeUInt32(v); }
	template<> inline void write<uint64_t>(const uint64_t &v) { writeUInt64(v); }
	template<> inline void write<std::string>(const std::string &v) { writeString(v); }
	template<typename U, typename V> inline void write(const std::pair<U, V> &v) { writePair<U, V>(v); }
	
	template<typename T> inline void serial(const T &v) { write(v); }

	inline void serialSize(const size_t v) { writeSize(v); }
	template<typename T> inline void serialContainer(const T &v) { writeContainer<T>(v); }

	inline uint16_t serialVersion(const uint16_t v) { writeUInt16(v); return v; }

private:
	IStreamWriter(IStreamWriter const&) = delete;
	IStreamWriter& operator=(IStreamWriter const&) = delete;
	
}; /* class IStreamWriter */

} /* namespace sev */

#include "i_stream_writer.inl"

#endif /* #ifdef SEV_MODULE_STREAM_READER_WRITER */

#endif /* #ifndef SEV_I_STREAM_WRITER_H */

/* end of file */
