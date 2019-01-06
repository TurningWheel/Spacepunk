// Random.hpp

#pragma once

#include "Main.hpp"

class Random {
public:
	Random();
	~Random() {}

	// seed the rng based on the current time
	void seedTime();

	// seed the rng based on the given 32-bit seed
	// @param seed the seed to use
	void seedValue(Uint32 seed);

	// seed the rng based on the given value
	void seedBytes(const Uint8* seed, size_t size);

	// @return an unsigned int (8-bit)
	Uint8 getUint8();

	// @return a signed int (8-bit)
	Sint8 getSint8();

	// @return an unsigned int (16-bit)
	Uint16 getUint16();

	// @return a signed int (16-bit)
	Sint16 getSint16();

	// @return an unsigned int (32-bit)
	Uint32 getUint32();

	// @return a signed int (32-bit)
	Sint32 getSint32();

	// @return an unsigned long (64-bit)
	Uint64 getUint64();

	// @return a signed long (64-bit)
	Sint64 getSint64();

	// @return a double (64-bit) (range 0-1, both inclusive)
	double getDouble();

	// @return a float (32-bit) (range 0-1, both inclusive)
	float getFloat();

	// generate a random value of the given size
	// @param buffer the buffer to place the random value in
	// @param size the size of the buffer in bytes
	// @return a random number
	void getBytes(Uint8* buffer, size_t size);

private:
	unsigned char s[256];
	Sint32 s_i, s_j;

	inline void swapByte(Uint8* a, Uint8* b);
	inline Uint8 getByte(const Uint8* bytes, size_t num, size_t offset);
};