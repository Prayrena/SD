#include "Engine/core/FileUtils.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/core/EngineCommon.hpp"
#include <stdio.h>
#include <Windows.h>
#include <fstream>

using namespace std;

// if this chunk have a different version save file, skip
bool IfThisFileCouldBeRead(std::string const& filePath)
{
	// Open for read
	errno_t err;
	FILE* pFile; // like a cursor in the word document
	char const* filePathName = filePath.c_str();

	// open the file
	err = fopen_s(&pFile, filePathName, "r");
	if (err != 0)
	{
		return false;
	}
	else
	{
		err = fclose(pFile);
		return true;
	}
}

// return the number of bytes read and resize the input buffer
int FileReadToBuffer(std::vector<uint8_t>& outBuffer, std::string const& filePath)
{
	// Open for read
	errno_t err;
	FILE* pFile = nullptr; // like a cursor in the word document
	char const* filePathName = filePath.c_str();

	// open the file
	err = fopen_s(&pFile, filePathName, "rb"); // b: _O_BINARY
	if (err != 0)
	{
		ERROR_AND_DIE(Stringf("The file %s could not be opened", filePath.c_str()));
	}

	// set the pointer to the end of the file
	int errorInfo;
	errorInfo = fseek(pFile, 0, SEEK_END);
	if (err != 0)
	{
		ERROR_AND_DIE("Could not read to the end of the file");
	}

	// get the file size
	long fileByteSize;
	fileByteSize = ftell(pFile);

	// resize, and copy over to the buffer
	outBuffer.resize(fileByteSize);
	fseek(pFile, 0, SEEK_SET); // put the pointer back to the beginning of the file
	fread(outBuffer.data(), sizeof(char), fileByteSize, pFile);

	// close the file
	err = fclose(pFile);
	if (err != 0)
	{
		ERROR_AND_DIE("The file just opened could not be closed");
	}

	return (int)fileByteSize;
}

// First read the file as a buffer of bytes, 
// then append a null terminator to make a c string, then make a std::string from that. 
int FileReadToString(std::string& outString, std::string const& filePath)
{
	std::vector<uint8_t> tempBuffer;	
	int fileByteSize = FileReadToBuffer(tempBuffer, filePath);
	outString.resize(fileByteSize);
	for (int i = 0; i < fileByteSize; ++i)
	{
		outString[i] = tempBuffer[i];
	}
	outString.push_back('\0'); // adding a null terminator
	return (int)outString.size();
}

bool CreateFolder(std::string filePathName)
{
	errno_t err;

	// todo:??? what is the difference between CreateDirectoryA and CreateDirectory
	err = CreateDirectoryA(LPCSTR(filePathName.c_str()), NULL);

	if (err == 0)
	{
		if (GetLastError() == ERROR_PATH_NOT_FOUND)
		{
			ERROR_AND_DIE(Stringf("Input file path %s does not exist", filePathName.c_str()));
		}	
		else if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			printf("The specified directory %s already exists", filePathName.c_str());
		}
		return false;
	}
	else
	{
		return true;
	}
}

eBufferEndian GetPlatformNativeEndian()
{
	unsigned int endianTest = 0x12345678;
	unsigned char* byteArray = reinterpret_cast<unsigned char*>(&endianTest);
	if (byteArray[0] == 0x12)
	{
		return eBufferEndian::BIG;
	}
	else if (byteArray[0] == 0x78)
	{
		return eBufferEndian::LITTLE;
	}
	else
	{
		ERROR_AND_DIE("Failed to get native Endian Mode");
	}
}

bool FileWriteFromBuffer(std::vector<uint8_t> inBuffer, std::string const& filePathName)
{
	// Open for read
	errno_t err;
	FILE* pFile; // like a cursor in the word document
	char const* filePath = filePathName.c_str();

	// todo: add this function is we do not want to overwrite the file in the future
	// if (std::filesystem::exists(filePathName)) {
	// 	// File exists, handle as needed (e.g., return false or prompt the user)
	// }

	// open the file
	// If the file doesn't exist, it will create a new file.
	// If the file already exists, it will overwrite the file's contents without prompting or preserving any old data
	err = fopen_s(&pFile, filePath, "wb");

	// close the file
	if (pFile == nullptr)
	{
		return false;
	}
	else
	{
		fwrite(inBuffer.data(), 1, inBuffer.size(), pFile); 
		err = fclose(pFile);
		return true;
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Reverse2BytesInPlace(void* ptrTo16BitWord)
{
	// in most modern systems, unsigned short is typically 2 bytes (16 bits)
	unsigned short us = *reinterpret_cast<unsigned short*>(ptrTo16BitWord);

	// we first change the void ptr to unsigned short ptr
	// then change the content the ptr is point at the memeory
	*(unsigned short*) ptrTo16BitWord = ((us && 0x00FF) << 8 |
										(us && 0xFF00) >> 8	);
}

void Reverse4BytesInPlace(void* ptrTo32BitDword)
{
	unsigned int* asUintPtr = reinterpret_cast<unsigned int*>(ptrTo32BitDword);
	unsigned int originalUint32 = *asUintPtr; // Get a copy of the original value

	// Reverse the bytes using bit shifting and masking
	unsigned int reversedUint =
		((originalUint32 & 0x000000FF) << 24) |  // Move byte 0 to byte 3
		((originalUint32 & 0x0000FF00) << 8) |  // Move byte 1 to byte 2
		((originalUint32 & 0x00FF0000) >> 8) |  // Move byte 2 to byte 1
		((originalUint32 & 0xFF000000) >> 24);   // Move byte 3 to byte 0

	// Update the original memory location with the reversed value
	*asUintPtr = reversedUint;
}

void Reverse8BytesInPlace(void* ptrTo64BitQword)
{
	// Interpret the input pointer as a 64-bit unsigned integer
	uint64_t* asUintPtr = reinterpret_cast<uint64_t*>(ptrTo64BitQword);
	uint64_t originalUint64 = *asUintPtr;  // Get a copy of the original value

	// Reverse the bytes using bit shifting and masking
	uint64_t reversedUint =
		((originalUint64 & 0x00000000000000FFULL) << 56) |  // Move byte 0 to byte 7
		((originalUint64 & 0x000000000000FF00ULL) << 40) |  // Move byte 1 to byte 6
		((originalUint64 & 0x0000000000FF0000ULL) << 24) |  // Move byte 2 to byte 5
		((originalUint64 & 0x00000000FF000000ULL) << 8) |  // Move byte 3 to byte 4
		((originalUint64 & 0x000000FF00000000ULL) >> 8) |  // Move byte 4 to byte 3
		((originalUint64 & 0x0000FF0000000000ULL) >> 24) |  // Move byte 5 to byte 2
		((originalUint64 & 0x00FF000000000000ULL) >> 40) |  // Move byte 6 to byte 1
		((originalUint64 & 0xFF00000000000000ULL) >> 56);   // Move byte 7 to byte 0

	// Update the original memory location with the reversed value
	*asUintPtr = reversedUint;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
BufferWriter::BufferWriter(std::vector<uint8_t>& bufferPtr)
	: m_buffer(bufferPtr)
{
	m_mode = GetPlatformNativeEndian();
	m_isOppositeEndianMode = false;
}

eBufferEndian BufferWriter::GetEndianMode()
{
	return m_mode;
}

void BufferWriter::SetEndianMode(eBufferEndian endianMode)				
{
	m_mode = endianMode;
	if (endianMode != GetPlatformNativeEndian())
	{
		m_isOppositeEndianMode = true;
	}
}

void BufferWriter::AppendChar(char c)
{
	m_buffer.push_back(static_cast<uint8_t>(c));
}

void BufferWriter::AppendByte(unsigned char c)
{
	m_buffer.push_back(static_cast<uint8_t>(c));
}

void BufferWriter::AppendBool(bool valid)
{
	m_buffer.push_back(static_cast<uint8_t>(valid));
}

void BufferWriter::AppendUint32(uint32_t value)
{
	if (m_isOppositeEndianMode)
	{
		Reverse4BytesInPlace(&value);
	}

	uint32_t* addressUint32 = reinterpret_cast<uint32_t*>(AppendUnintializedBytes(4));
	*addressUint32 = value;
}

void BufferWriter::AppendInt32(int32_t value)
{
	if (m_isOppositeEndianMode)
	{
		Reverse4BytesInPlace(&value);
	}

	int32_t* addressInt32 = reinterpret_cast<int32_t*>(AppendUnintializedBytes(4));
	*addressInt32 = value;
}

void BufferWriter::AppendFloat(float value)
{
	if (m_isOppositeEndianMode)
	{
		Reverse4BytesInPlace(&value);
	}

	float* addressOfFloat = reinterpret_cast<float*>(AppendUnintializedBytes(4));
	*addressOfFloat = value;	// float = float assignment
}

void BufferWriter::AppendDouble(double value)
{
	if (m_isOppositeEndianMode)
	{
		Reverse8BytesInPlace(&value);
	}

	double* addressOfDouble = reinterpret_cast<double*>(AppendUnintializedBytes(8));
	*addressOfDouble = value;
}

void BufferWriter::AppendStringZeroTerminated(string const& str)
{
	m_buffer.reserve(m_buffer.size() + str.size() + 1);	// +1 for the \0

	m_buffer.insert(m_buffer.end(), str.begin(), str.end());

	AppendChar('\0');
}

void BufferWriter::AppendStringAfter32BitLength(string const& str)
{
	m_buffer.reserve(m_buffer.size() + 4 + str.size());

	AppendUint32((uint32_t)str.size());

	// insert calculates the exact size needed before inserting, it avoids multiple reallocations
	m_buffer.insert(m_buffer.end(), str.begin(), str.end());
}

void BufferWriter::AppendRgba(Rgba8 const& color)
{
	m_buffer.reserve(m_buffer.size() + 4);
	AppendByte(color.r);
	AppendByte(color.g);
	AppendByte(color.b);
	AppendByte(color.a);
}

void BufferWriter::AppendRgb(Rgba8 const& color)
{
	m_buffer.reserve(m_buffer.size() + 3);
	AppendByte(color.r);
	AppendByte(color.g);
	AppendByte(color.b);
}

void BufferWriter::AppendIntVec2(IntVec2 const& value)
{
	m_buffer.reserve(m_buffer.size() + 8);
	AppendUint32((uint32_t)(value.x));
	AppendUint32((uint32_t)(value.y));
}

void BufferWriter::AppendVec2(Vec2 const& value)
{
	m_buffer.reserve(m_buffer.size() + 8);
	AppendFloat(value.x);
	AppendFloat(value.y);
}

void BufferWriter::AppendVec3(Vec3 const& value)
{
	m_buffer.reserve(m_buffer.size() + 12);
	AppendFloat(value.x);
	AppendFloat(value.y);
	AppendFloat(value.z);
}

void BufferWriter::AppendVertexPCU(Vertex_PCU const& value)
{
	m_buffer.reserve(m_buffer.size() + sizeof(Vertex_PCU));
	AppendVec3(value.m_position);
	AppendRgba(value.m_color);
	AppendVec2(value.m_uvTexCoords);
}

unsigned char* BufferWriter::AppendUnintializedBytes(int numBytes)
{
	int originalSize = (int)m_buffer.size();
	m_buffer.reserve(originalSize + numBytes);
	for (int i = 0; i < numBytes; ++i)
	{
		m_buffer.push_back('0');
	}
	return &m_buffer[originalSize];
}

void BufferWriter::AppendByteArray(unsigned char const* byteArray, size_t numsByteToWrite)
{
	size_t previousSize = m_buffer.size();
	m_buffer.resize(previousSize + numsByteToWrite);
	unsigned char* writePos = &m_buffer[previousSize];
	memcpy(writePos, byteArray, numsByteToWrite);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
BufferParser::BufferParser(std::vector<uint8_t>& bufferPtr, eBufferEndian endianMode /*= eBufferEndian::NATIVE*/)
		: BufferParser(bufferPtr.data(), (int)bufferPtr.size(), endianMode)	// constructor delegation
{

}

BufferParser::BufferParser(unsigned char* dataPtr, int size, eBufferEndian endianMode /*= eBufferEndian::NATIVE*/)
			: m_buffer(dataPtr)
			, m_size(size)
			, m_endianMode(endianMode)
{
	if (m_endianMode != GetPlatformNativeEndian())
	{
		m_isOppositeEndianMode = true;
	}
	else
	{
		m_isOppositeEndianMode = false;
	}
}

bool BufferParser::GuaranteeBufferDataAvaible(size_t numByte)
{
	// Ensure we have enough bytes to read
	// avoid buffer overrun
	if (m_currentReadOffset + numByte > m_size)
	{
		ERROR_RECOVERABLE("Not enough data to parse");
		return false;
	}
	else
	{
		return true;
	}
}

void BufferParser::SetEndianMode(eBufferEndian endianMode)
{
	m_endianMode = endianMode;
	if (m_endianMode != GetPlatformNativeEndian())
	{
		m_isOppositeEndianMode = true;
	}
	else
	{
		m_isOppositeEndianMode = false;
	}
}

unsigned char BufferParser::ParseByte()
{	
	return static_cast<unsigned char>(m_buffer[m_currentReadOffset++]);
}

bool BufferParser::ParseBool()
{
	return static_cast<bool>(m_buffer[m_currentReadOffset++]);
}

char BufferParser::ParseChar()
{
	return static_cast<char>(m_buffer[m_currentReadOffset++]);
}

float BufferParser::ParseFloat()
{
	float readBytesAsFloat = *reinterpret_cast<float const*>(ParseBytes(4));
	if (m_isOppositeEndianMode)
	{
		Reverse4BytesInPlace(&readBytesAsFloat);
	}
	return readBytesAsFloat;
}

double BufferParser::ParseDouble()
{
	double readBytesAsDouble = *reinterpret_cast<double const*>(ParseBytes(8));
	if (m_isOppositeEndianMode)
	{
		Reverse8BytesInPlace(&readBytesAsDouble);
	}
	return readBytesAsDouble;
}

uint16_t BufferParser::ParseUint16()
{
	uint16_t readBytesAsUint16 = *reinterpret_cast<uint16_t const*>(ParseBytes(2));
	if (m_isOppositeEndianMode)
	{
		Reverse2BytesInPlace(&readBytesAsUint16);
	}
	return readBytesAsUint16;
}

uint32_t BufferParser::ParseUint32()
{
	uint32_t readBytesAsUint32 = *reinterpret_cast<uint32_t const*>(ParseBytes(4));
	if (m_isOppositeEndianMode)
	{
		Reverse4BytesInPlace(&readBytesAsUint32);
	}
	return readBytesAsUint32;
}

int BufferParser::ParseInt32()
{
	int32_t readBytesAsInt32 = *reinterpret_cast<int const*>(ParseBytes(4));
	if (m_isOppositeEndianMode)
	{
		Reverse4BytesInPlace(&readBytesAsInt32);
	}
	return readBytesAsInt32;
}

void BufferParser::ParseStringZeroTerminated(std::string& str)
{
	char c = ParseByte();
	while (c != '\0')
	{
		str.push_back(c);
		c = ParseByte();
	}
}

void BufferParser::ParseStringAfter32BitLength(std::string& str)
{
	uint32_t size = ParseUint32();
	ParseStringOfLength(str, size);
}	

void BufferParser::ParseStringOfLength(std::string& out_str, unsigned int strLen)
{
	GuaranteeBufferDataAvaible(strLen);
	out_str.reserve(strLen);
	out_str.assign(reinterpret_cast<const char*>(m_buffer + m_currentReadOffset), strLen);
	// or use this:
	// out_str.resize(strLen);
	// memcpy(&out_str[0], m_buffer + m_currentReadOffset, strLen);
	// or use this:
	// Reserve space to avoid multiple reallocations
	// out_str.reserve(strLen);
	// 
	// // Insert characters directly from the buffer
	// out_str.insert(out_str.end(),
	// 		reinterpret_cast<const char*>(m_buffer + m_currentReadOffset),
	// 		reinterpret_cast<const char*>(m_buffer + m_currentReadOffset + strLen));
	m_currentReadOffset += strLen;
}

// each byte color is endian independent, you cannot parse each byte at the same time
// why not make them endian dependent? read and write would be much faster
Rgba8 BufferParser::ParseRgba()
{
	// Rgba8 color;
	// uint32_t colorPtr = *reinterpret_cast<uint32_t const*>(ParseBytes(4));
	// if (m_isOppositeEndianMode)
	// {
	//		Reverse4BytesInPlace(&colorPtr); // reverse the 4 bytes on stack memory
	// }
	// 
	// color.r = (unsigned char)((colorPtr & 0xFF'00'00'00) >> 24);
	// color.g = (unsigned char)((colorPtr & 0x00'FF'00'00) >> 16);
	// color.b = (unsigned char)((colorPtr & 0x00'00'FF'00) >> 8);
	// color.a = (unsigned char)((colorPtr & 0x00'00'00'FF));
	// 
	// return color;

	Rgba8 color;
	uint8_t colorBytes[4] = { ParseByte(), ParseByte(), ParseByte(), ParseByte() };

	color.r = (unsigned char)(colorBytes[0]);
	color.g = (unsigned char)(colorBytes[1]);
	color.b = (unsigned char)(colorBytes[2]);
	color.a = (unsigned char)(colorBytes[3]);

	return color;
}

Rgba8 BufferParser::ParseRgb()
{
	// Rgba8 color;
	// uint8_t colorBytes[3] = {colorBytesPtr[0], colorBytesPtr[1], colorBytesPtr[2]};
	// if (m_isOppositeEndianMode)
	// {
	//		Reverse4BytesInPlace(&colorBytes); // reverse the 4 bytes on stack memory
	// }
	// 
	// color.r = (unsigned char)(colorBytes[0]);
	// color.g = (unsigned char)(colorBytes[1]);
	// color.b = (unsigned char)(colorBytes[2]);
	// color.a = 255;
	// 
	// return color;

	Rgba8 color;
	uint8_t colorBytes[3] = { ParseByte(), ParseByte(), ParseByte() };

	color.r = (unsigned char)(colorBytes[0]);
	color.g = (unsigned char)(colorBytes[1]);
	color.b = (unsigned char)(colorBytes[2]);
	color.a = 255;

	return color;
}

IntVec2 BufferParser::ParseIntVec2()
{
	IntVec2 result;
	result.x = ParseInt32();
	result.y = ParseInt32();

	return result;
}

Vec2 BufferParser::ParseVec2()
{
	Vec2 result;
	result.x = ParseFloat();
	result.y = ParseFloat();

	return result;
}

Vec3 BufferParser::ParseVec3()
{
	Vec3 result;
	result.x = ParseFloat();
	result.y = ParseFloat();
	result.z = ParseFloat();

	return result;
}

Vertex_PCU BufferParser::ParseVertexPCU()
{
	Vertex_PCU result;
	result.m_position = ParseVec3();
	result.m_color = ParseRgba();
	result.m_uvTexCoords = ParseVec2();

	return result;
}

// the writing order is (minX, minY, maxX, maxY)
AABB2 BufferParser::ParseAABB2()
{
	AABB2 box2D;
	box2D.m_mins.x = ParseFloat();
	box2D.m_mins.y = ParseFloat();
	box2D.m_maxs.x = ParseFloat();
	box2D.m_maxs.y = ParseFloat();

	return box2D;
}

void BufferParser::SetReadOffset(int offset)
{
	m_currentReadOffset = offset;
}

unsigned char const* BufferParser::ParseBytes(size_t numBytes)
{
	GuaranteeBufferDataAvaible(numBytes);
	unsigned char const* positionBeforeAdvance = &m_buffer[m_currentReadOffset];
	m_currentReadOffset += (int)numBytes;
	return positionBeforeAdvance;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
HashedCaseInsensitiveString::HashedCaseInsensitiveString(const char* cText)
	: m_originalString(cText)
{
	m_caseInsensitiveHash = HashedCaseInsensitiveString::GenerateCaseInsensitiveHash(cText);
}

HashedCaseInsensitiveString::HashedCaseInsensitiveString(std::string const& str)
					: m_originalString(str)
{
	m_caseInsensitiveHash = HashedCaseInsensitiveString::GenerateCaseInsensitiveHash(str);
}

HashedCaseInsensitiveString::HashedCaseInsensitiveString(HashedCaseInsensitiveString const& rhs)
{
	// *this = HashedCaseInsensitiveString(assignFrom); // this will cause infinite recursion!
	m_caseInsensitiveHash = rhs.m_caseInsensitiveHash;
	m_originalString = rhs.m_originalString;
}

bool HashedCaseInsensitiveString::operator==(HashedCaseInsensitiveString const& rhs)
{
	if (m_caseInsensitiveHash != rhs.m_caseInsensitiveHash)
	{
		return false;
	}
	else
	{
		// stricmp: insensitive comparison
		return 0 == _stricmp(m_originalString.c_str(), rhs.m_originalString.c_str());
	}
}

bool HashedCaseInsensitiveString::operator!=(HashedCaseInsensitiveString const& rhs)
{
	return m_caseInsensitiveHash != rhs.m_caseInsensitiveHash;
}

bool HashedCaseInsensitiveString::operator<(HashedCaseInsensitiveString const& rhs)
{
	if (m_caseInsensitiveHash < rhs.m_caseInsensitiveHash)
	{
		return true;
	}
	else if (m_caseInsensitiveHash > rhs.m_caseInsensitiveHash)
	{
		return false;
	}

	// stricmp result < 0: The first string is less than the second string (ignoring case).
	return (_stricmp(m_originalString.c_str(), rhs.m_originalString.c_str()) < 0);
}

bool HashedCaseInsensitiveString::operator!=(std::string const& text) const
{
	return m_caseInsensitiveHash != GenerateCaseInsensitiveHash(text);
}

bool HashedCaseInsensitiveString::operator==(std::string const& text) const
{
	return m_caseInsensitiveHash == GenerateCaseInsensitiveHash(text);
}

bool HashedCaseInsensitiveString::operator==(char const* text) const
{
	if (text == nullptr) return false;

	if (m_caseInsensitiveHash != GenerateCaseInsensitiveHash(text))
	{
		return false;
	}

	// Optional fall back for safety in case of hash collision
	return _stricmp(m_originalString.c_str(), text) == 0;
}

bool HashedCaseInsensitiveString::operator!=(char const* text) const
{
	if (text == nullptr) return true;

	return m_caseInsensitiveHash != GenerateCaseInsensitiveHash(text);
}

void HashedCaseInsensitiveString::operator=(HashedCaseInsensitiveString const& assignFrom)
{
	if (this != &assignFrom) // prevent self-assignment
	{
		m_originalString = assignFrom.m_originalString;
		m_caseInsensitiveHash = assignFrom.m_caseInsensitiveHash;
	}
}

void HashedCaseInsensitiveString::operator=(char const* text)
{
	if (text == nullptr)
	{
		m_originalString.clear();
		m_caseInsensitiveHash = 0;
	}
	else
	{
		m_originalString = text;
		m_caseInsensitiveHash = GenerateCaseInsensitiveHash(text);
	}
}

void HashedCaseInsensitiveString::operator=(std::string const& text)
{
	m_originalString = text;
	m_caseInsensitiveHash = GenerateCaseInsensitiveHash(text);
}

std::string HashedCaseInsensitiveString::GetOrignialString() const
{
	return m_originalString;
}

unsigned int HashedCaseInsensitiveString::GenerateCaseInsensitiveHash(const char* cText)
{
	unsigned int hash = 0;	// 32 bits
	char const* scan = cText;
	while (*scan != '\0')
	{
		// loosing some of the info and overflow is the goal
		hash *= 31; // prime num, shift the num to the left by 31 bit
		hash += tolower(*scan);	// make it case insensitive
		++scan;		// work on the other
	}
	return hash;
}

unsigned int HashedCaseInsensitiveString::GenerateCaseInsensitiveHash(std::string const& str)
{
	return HashedCaseInsensitiveString::GenerateCaseInsensitiveHash(str.c_str());
}
