#pragma once
#include "Engine/core/Rgba8.hpp"
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Math/AABB2.hpp"
#include <vector>
#include <string>

bool IfThisFileCouldBeRead(std::string const& filePath);
int FileReadToBuffer(std::vector<uint8_t>& outBuffer, std::string const& filePath);
int FileReadToString(std::string& outString, std::string const& filePath);
bool FileWriteFromBuffer(std::vector<uint8_t> inBuffer, std::string const& filePathName);
bool CreateFolder(std::string filePathName);

enum class eBufferEndian : unsigned char	// default is int, saving memories?
{
	NATIVE,
	LITTLE,
	BIG,
	NUM_ENDIAN
};

eBufferEndian GetPlatformNativeEndian();
// void ReverseByteInPlace(void* ptrTo8BitWord)


void Reverse2BytesInPlace(void* ptrTo16BitWord);
void Reverse4BytesInPlace(void* ptrTo32BitDword);
void Reverse8BytesInPlace(void* ptrTo64BitQword);

class BufferWriter
{
public:
	BufferWriter(std::vector<uint8_t>& bufferPtr);

	eBufferEndian	GetEndianMode();
	void			SetEndianMode(eBufferEndian endianMode);

	void AppendChar(char c);
	// void AppendByte(uint8_t byte);
	void AppendByte(unsigned char c);
	void AppendBool(bool valid);
	void AppendUint32(uint32_t value);
	void AppendInt32(int32_t value);
	void AppendFloat(float value);
	void AppendDouble(double value);
	void AppendStringZeroTerminated(std::string const& str);	// written with a trailing 0 ('\0') after string
	void AppendStringAfter32BitLength(std::string const& str); // Append the size of the str first, then the str

	void AppendRgba(Rgba8 const& color); // four bytes in RGBA order (endian-independent)
	void AppendRgb(Rgba8 const& color);  // written as 3 bytes (RGB) only; ignores Alpha
	void AppendIntVec2(IntVec2 const& value);
	void AppendVec2(Vec2 const& value);
	void AppendVec3(Vec3 const& value);
	void AppendVertexPCU(Vertex_PCU const& value);

	eBufferEndian m_mode = eBufferEndian::BIG;

	bool m_isOppositeEndianMode = false;

private:
	unsigned char* AppendUnintializedBytes(int numBytes); // reserve space
	void	AppendByteArray(unsigned char const* byteArray, size_t numsByteToWrite);
	std::vector<uint8_t>& m_buffer;
};

class BufferParser
{
public:

	BufferParser(std::vector<uint8_t>& bufferPtr, eBufferEndian endianMode	= eBufferEndian::NATIVE);
	BufferParser(unsigned char* dataPtr, int size, eBufferEndian endianMode	= eBufferEndian::NATIVE);

	bool GuaranteeBufferDataAvaible(size_t numByte);
	void SetEndianMode(eBufferEndian endianMode);

	unsigned char	ParseByte();
	bool			ParseBool();
	char			ParseChar();
	float			ParseFloat();
	double			ParseDouble();
	uint16_t		ParseUint16();
	uint32_t		ParseUint32();
	int				ParseInt32();

	void ParseStringZeroTerminated(std::string& str);
	void ParseStringAfter32BitLength(std::string& str);
	void ParseStringOfLength(std::string& str, unsigned int strLen);

	Rgba8		ParseRgba();
	Rgba8		ParseRgb();
	IntVec2		ParseIntVec2();
	Vec2		ParseVec2();
	Vec3		ParseVec3();
	Vertex_PCU	ParseVertexPCU();
	AABB2		ParseAABB2();

	void		SetReadOffset(int offset);

	eBufferEndian m_endianMode = eBufferEndian::NATIVE;
	bool m_isOppositeEndianMode = false;

private:
	unsigned char const*	ParseBytes(size_t numBytes); // instead of checking boundaries multiple times
	unsigned char*	m_buffer = nullptr;
	int	m_size = 0;
	int	m_currentReadOffset = 0;
};

class HashedCaseInsensitiveString
{
public:
	HashedCaseInsensitiveString(std::string const& str);
	HashedCaseInsensitiveString(const char* cText);
	HashedCaseInsensitiveString(HashedCaseInsensitiveString const& rhs);	// copy constructor
	HashedCaseInsensitiveString() = default;

	bool operator==(HashedCaseInsensitiveString const& rhs);
	bool operator!=(HashedCaseInsensitiveString const& rhs);
	bool operator<(HashedCaseInsensitiveString const& rhs);
	bool operator==( char const* text ) const;
	bool operator!=( char const* text ) const;

	bool operator==(std::string const& text) const;
	bool operator!=(std::string const& text) const;
	void operator=(HashedCaseInsensitiveString const& assignFrom);
	void operator=(char const* text);
	void operator=(std::string const& text);


	std::string GetOrignialString() const;

private:
	std::string m_originalString;

	static unsigned int GenerateCaseInsensitiveHash(const char* cText);
	static unsigned int GenerateCaseInsensitiveHash(std::string const& str);
	unsigned int m_caseInsensitiveHash = 0;
};