//! @file Console.hpp

#pragma once

#include "Main.hpp"
#include "String.hpp"
#include "Map.hpp"

//! console variable
struct Cvar {
public:
	Cvar() = delete;
	Cvar(const char* _name, const char* _desc, const char* _value);
	Cvar(const Cvar&) = delete;
	Cvar(Cvar&&) = delete;
	virtual ~Cvar() {}

	Cvar& operator=(const Cvar&) = delete;
	Cvar& operator=(Cvar&&) = delete;

	static Map<String, Cvar*>& getMap();

	//! get the str representation of the cvar
	//! @return The value as a str
	const char* toStr() {
		if (!(cached & 1 << 0)) {
			str = value.get();
			cached |= 1 << 0;
		}
		return str;
	}

	//! get the int representation of the cvar
	//! @return The value as an int
	int toInt() {
		if (!(cached & 1 << 1)) {
			d = strtol(value.get(), nullptr, 10);
			cached |= 1 << 1;
		}
		return d;
	}

	//! get the float representation of the cvar
	//! @return The value as a float
	float toFloat() {
		if (!(cached & 1 << 2)) {
			f = strtof(value.get(), nullptr);
			cached |= 1 << 2;
		}
		return f;
	}

	//! set the value of the cvar
	//! @param _value The new value
	void set(const char* _value) {
		value = _value;
		cached = 0U;
	}

	const char*		getName() const { return name.get(); }
	const char*		getDesc() const { return desc.get(); }
	const char*		getDefault() const { return defaultValue.get(); }

private:
	Uint8 cached = 0U;
	String name;
	String desc;
	String value;
	String defaultValue;

	const char* str;
	Sint32 d;
	float f;
};

//! console command
struct Ccmd {
public:
	Ccmd() = delete;
	Ccmd(const char* _name, const char* _desc, int(*_func)(int, const char**));
	Ccmd(const Ccmd&) = delete;
	Ccmd(Ccmd&&) = delete;
	virtual ~Ccmd() {}

	Ccmd& operator=(const Ccmd&) = delete;
	Ccmd& operator=(Ccmd&&) = delete;

	static Map<String, Ccmd*>& getMap();

	String name;
	String desc;
	int(*func)(int argc, const char** argv) = nullptr;
};