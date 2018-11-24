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

	int toInt() {
		return strtol(value.get(), nullptr, 10);
	}
	float toFloat() {
		return strtof(value.get(), nullptr);
	}
	const char* toStr() {
		return value.get();
	}

	String name;
	String desc;
	String value;
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