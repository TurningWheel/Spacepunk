// Console.hpp

#pragma once

#include "Main.hpp"
#include "String.hpp"
#include "Map.hpp"

// console variable
struct Cvar {
public:
	Cvar(const char* _name, const char* _desc, const char* _value);
	virtual ~Cvar() {}

	static Map<Cvar*>& getMap();

	// get the str representation of the cvar
	// @return The value as a str
	const char* toStr() {
		if (!(cached & 1<<0)) {
			str = value.get();
			cached |= 1<<0;
		}
		return str;
	}

	// get the int representation of the cvar
	// @return The value as an int
	int toInt() {
		if (!(cached & 1<<1)) {
			d = strtol(value.get(), nullptr, 10);
			cached |= 1<<1;
		}
		return d;
	}

	// get the float representation of the cvar
	// @return The value as a float
	float toFloat() {
		if (!(cached & 1<<2)) {
			f = strtof(value.get(), nullptr);
			cached |= 1<<2;
		}
		return f;
	}

	// set the value of the cvar
	// @param _value The new value
	void set(const char* _value) {
		value = _value;
		cached = 0U;
	}

	// getters & setters
	const char*		getName() const		{ return name; }
	const char*		getDesc() const		{ return desc; }

private:
	Uint8 cached = 0U;
	String name;
	String desc;
	String value;
	
	const char* str;
	Sint32 d;
	float f;
};

// console command
struct Ccmd {
public:
	Ccmd(const char* _name, const char* _desc, int (*_func)(int, const char**));
	virtual ~Ccmd() {}

	static Map<Ccmd*>& getMap();

	String name;
	String desc;
	int (*func)(int argc, const char** argv) = nullptr;
};