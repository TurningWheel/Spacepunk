//! @file Packet.hpp

#pragma once

#include "Main.hpp"

//! A Packet is essentially a data buffer designed to be sent over the net.
class Packet {
public:
	Packet() {}
	Packet(const Packet& src);
	~Packet() {}

	//! max size of the data buffer
	static const Uint32 maxLen = 1024;

	//! clears the contents of the packet buffer
	void clear();

	//! copies the contents of another packet into this packet
	void copy(const Packet& src);

	//! writes standard boilerplate stuff to the packet
	//! this function should be called LAST, as it will be read back FIRST!
	//! @return true if the write succeeded, false if it failed (eg the packet buffer is full)
	bool sign(Uint32 timestamp, Uint32 id);

	//! writes 8 bits to the packet buffer
	//! @param value the value to write to the buffer
	//! @return true if the write succeeded, false if it failed (eg the packet buffer is full)
	bool write8(Uint8 value);

	//! writes 16 bits to the packet buffer
	//! @param value the value to write to the buffer
	//! @return true if the write succeeded, false if it failed (eg the packet buffer is full)
	bool write16(Uint16 value);

	//! writes 32 bits to the packet buffer
	//! @param value the value to write to the buffer
	//! @return true if the write succeeded, false if it failed (eg the packet buffer is full)
	bool write32(Uint32 value);

	//! writes the specified number of bytes from the data buffer into the packet buffer
	//! @param data the data to write to the buffer
	//! @param len the length of the data in bytes
	//! @return true if the write succeeded, false if it failed (eg the packet buffer is full)
	bool write(const char* data, unsigned int len);

	//! writes the given string contents out to the packet buffer
	//! @param str the string to write
	//! @return true if the write succeeded, false if it failed (eg the packet buffer is full)
	bool write(const char* str);

	//! reads 8 bits from the packet buffer
	//! @param data the data buffer to fill with the read data
	//! @return true if the read succeeded, or false if it failed
	bool read8(Uint8& data);

	//! reads 16 bits from the packet buffer
	//! @param data the data buffer to fill with the read data
	//! @return true if the read succeeded, or false if it failed
	bool read16(Uint16& data);

	//! reads 32 bits from the packet buffer
	//! @param data the data buffer to fill with the read data
	//! @return true if the read succeeded, or false if it failed
	bool read32(Uint32& data);

	//! reads the specified number of bytes from the data buffer into the packet buffer
	//! @param data the data buffer to copy the data to
	//! @param len the length of the data in bytes
	//! @return true if the read succeeded, false if it failed
	bool read(char* data, unsigned int len);

	char data[maxLen] = { 0 };
	Uint32 offset = 0;
};