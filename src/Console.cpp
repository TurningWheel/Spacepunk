// Console.cpp

#include "Console.hpp"

#ifdef PLATFORM_WINDOWS
#ifdef _MSC_VER
#pragma warning(disable: 4073) // initializers put in library initialization area
#endif
#pragma init_seg(lib)
static Map<Cvar*> cvars;
static Map<Ccmd*> ccmds;
#else
static Map<Cvar*> cvars __attribute__ ((init_priority (101)));
static Map<Ccmd*> ccmds __attribute__ ((init_priority (102)));
#endif

Cvar::Cvar(const char* _name, const char* _desc, const char* _value) {
	name = _name;
	desc = _desc;
	value = _value;
	cvars.insert(_name, this);
}

Map<Cvar*>& Cvar::getMap() {
	return cvars;
}

Ccmd::Ccmd(const char* _name, const char* _desc, int (*_func)(int, const char**)) {
	name = _name;
	desc = _desc;
	func = _func;
	ccmds.insert(_name, this);
}

Map<Ccmd*>& Ccmd::getMap() {
	return ccmds;
}