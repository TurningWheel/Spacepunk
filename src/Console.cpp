// Console.cpp

#include "Console.hpp"

#ifdef PLATFORM_WINDOWS
#ifdef _MSC_VER
#pragma warning(disable: 4073) // initializers put in library initialization area
#endif
#pragma init_seg(lib)
static LinkedList<Cvar*> cvars;
static LinkedList<Ccmd*> ccmds;
#else
static LinkedList<Cvar*> cvars __attribute__ ((init_priority (101)));
static LinkedList<Ccmd*> ccmds __attribute__ ((init_priority (102)));
#endif

Cvar::Cvar(const char* _name, const char* _desc, const char* _value) {
	name = _name;
	desc = _desc;
	value = _value;
	cvars.addNodeLast(this);
}

LinkedList<Cvar*>& Cvar::getList() {
	return cvars;
}

Ccmd::Ccmd(const char* _name, const char* _desc, int (*_func)(int, const char**)) {
	name = _name;
	desc = _desc;
	func = _func;
	ccmds.addNodeLast(this);
}

LinkedList<Ccmd*>& Ccmd::getList() {
	return ccmds;
}