// Console.cpp

#include "Console.hpp"

Cvar::Cvar(const char* _name, const char* _desc, const char* _value) {
	name = _name;
	desc = _desc;
	value = _value;
	defaultValue = _value;
	getMap().insert(_name, this);
}

Map<String, Cvar*>& Cvar::getMap() {
	static Map<String, Cvar*> cvars;
	return cvars;
}

Ccmd::Ccmd(const char* _name, const char* _desc, int(*_func)(int, const char**)) {
	name = _name;
	desc = _desc;
	func = _func;
	getMap().insert(_name, this);
}

Map<String, Ccmd*>& Ccmd::getMap() {
	static Map<String, Ccmd*> ccmds;
	return ccmds;
}