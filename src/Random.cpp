// Random.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Random.hpp"

Random::Random() {
	seedTime();
}

void Random::seedBytes(const Uint8* seed, size_t size) {
	if( !seed || !size )
		return;

	Sint32 i, j;

	for( i=0; i<256; ++i )
		s[i] = i;

	for( i=j=0; i<256; ++i ) { 
		j = (j + s[i] + getByte(seed, size, i)) & 255;
		swapByte(s + i, s + j);
	}

	s_i = s_j = 0;
}

void Random::seedTime()  {
	static time_t t;
	if( t == 0 ) {
		t = time(nullptr);
	} else {
		++t;
	}
	seedBytes( (const Uint8*)&t, sizeof(time_t) );
}

void Random::seedValue(Uint32 seed) {
	seedBytes( (const Uint8*)&seed, sizeof(Uint32) );
}

Uint8 Random::getUint8() {
	s_i = (s_i + 1) & 255;
	s_j = (s_j + s[s_i]) & 255;
	swapByte(s + s_i, s + s_j);

	return s[(s[s_i] + s[s_j]) & 255];
}

Sint8 Random::getSint8() {
	return getUint8() & INT8_MAX;
}

Uint16 Random::getUint16() {
	Uint16 value;
	getBytes((Uint8*)&value,sizeof(Uint16));
	return value;
}

Sint16 Random::getSint16() {
	Sint16 value;
	getBytes((Uint8*)&value,sizeof(Sint16));
	return value & INT16_MAX;
}

Uint32 Random::getUint32() {
	Uint32 value;
	getBytes((Uint8*)&value,sizeof(Uint32));
	return value;
}

Sint32 Random::getSint32() {
	Sint32 value;
	getBytes((Uint8*)&value,sizeof(Sint32));
	return value & INT16_MAX;
}

Uint64 Random::getUint64() {
	Uint64 value;
	getBytes((Uint8*)&value,sizeof(Uint64));
	return value;
}

Sint64 Random::getSint64() {
	Sint64 value;
	getBytes((Uint8*)&value,sizeof(Sint64));
	return value & INT16_MAX;
}

void Random::getBytes(Uint8* buffer, size_t size) {
	while( size>0 ) {
		*buffer = getUint8();
		++buffer;
		--size;
	}
}

double Random::getDouble() {
	return getUint32() / (double)(UINT32_MAX);
}

float Random::getFloat() {
	return getUint32() / (float)(UINT32_MAX);
}

inline void Random::swapByte(Uint8* a, Uint8* b) {
	Uint8 temp = *a;
	*a = *b;
	*b = temp;
}

inline Uint8 Random::getByte(const Uint8* bytes, size_t num, size_t offset) {
	return bytes[offset % num];
}