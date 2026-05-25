#pragma once

struct ActorUID
{
public:
	ActorUID() = default;
	ActorUID(unsigned int salt, unsigned int index);

	bool IsValid() const;
	unsigned int GetIndex() const;

	bool operator == (ActorUID const& other) const;
	bool operator != (ActorUID const& other) const;

	static const ActorUID INVALID;

private:
	// each unsigned int / int is 4 bytes
	// each byte has 8 bits, means unsigned int has 32 bits
	// 4 bits equal to 1 nibble, used for hexadecimal from as 0 - 15, as A - F
	// so unsigned int has 8 hexadecimal, 0xFFFFFFFFu means 1111 1111 1111 1111 1111 1111 1111 1111
	// int has 8 hexadecimal, 0xFFFFFFFF means 1111 1111 1111 1111 1111 1111 1111 1111
	// int has 8 hexadecimal, 0x00000000 means 1111 1111 1111 1111 1111 1111 1111 1111
	// first 16 bits are salt, last 16 bits are indexes
	unsigned int m_data; // ++0xFFFFFFFFu -> 0x00000000, wrap
	// for int: Ox00000000, 0x0 means positive, 0x1 means negative, this is unsure
};