// Packet.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Packet.hpp"

Packet::Packet(const Packet& src) {
	copy(src);
}

void Packet::clear() {
	for (Uint32 c = 0; c < maxLen; ++c) {
		data[c] = 0;
	}
	offset = 0;
}

void Packet::copy(const Packet& src) {
	offset = src.offset;
	memcpy(data, src.data, offset);
}

bool Packet::write8(Uint8 value) {
	return write((char*)(&value), 1);
}

bool Packet::write16(Uint16 value) {
	return write((char*)(&value), 2);
}

bool Packet::write32(Uint32 value) {
	return write((char*)(&value), 4);
}

bool Packet::write(const char* _data, unsigned int len) {
	if (_data == nullptr) {
		return false;
	}

	if (offset + len > maxLen) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to write %d bytes to packet; packet is full!");
		return false;
	}

	for (unsigned int c = 0; c < len; ++c) {
		data[offset + c] = _data[c];
	}
	offset += len;

	return true;
}

bool Packet::write(const char* str) {
	return write(str, (Uint32)strlen(str));
}

bool Packet::sign(Uint32 timestamp, Uint32 id) {
	if (!write32(timestamp))
		return false;
	if (!write32(id))
		return false;
	return true;
}

bool Packet::read8(Uint8& data) {
	return read((char*)(&data), 1);
}

bool Packet::read16(Uint16& data) {
	return read((char*)(&data), 2);
}

bool Packet::read32(Uint32& data) {
	return read((char*)(&data), 4);
}

bool Packet::read(char* _data, unsigned int len) {
	if (len <= 0) {
		return false;
	}
	if (_data == nullptr) {
		return false;
	}
	if (offset < len) {
		return false;
	}

	unsigned int _offset = (unsigned int)offset - len;
	for (unsigned int c = 0; c < len; ++c) {
		_data[c] = data[_offset + c];
	}
	offset -= len;

	return true;
}