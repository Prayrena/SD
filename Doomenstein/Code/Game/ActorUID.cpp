#include "Game/ActorUID.hpp"

ActorUID const ActorUID::INVALID(0x0000FFFFu, 0x0000FFFFu);

ActorUID::ActorUID(unsigned int salt, unsigned int index)
{
	// << will have bitwise left shift, move 16 bits to the left
	m_data = (((0x0000FFFFu & salt) << 16) | (0x0000FFFFu & index)); 
}

bool ActorUID::operator==(ActorUID const& other) const
{
	return m_data == other.m_data;
}

bool ActorUID::operator!=(ActorUID const& other) const
{
	return m_data != other.m_data;
}

bool ActorUID::IsValid() const
{
	return (*this != ActorUID::INVALID);
}

unsigned int ActorUID::GetIndex() const
{
	return (0x0000FFFFu & m_data);
}


