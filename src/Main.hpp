// Main.hpp
// Recommended tab size is 4

#pragma once

#define GL_GLEXT_PROTOTYPES

// CMake Config
#ifndef _MSC_VER
#include "CMakeConfig.h"
#else
#define PLATFORM_WINDOWS
#endif

// windows headers
#ifdef PLATFORM_WINDOWS
#define NOMINMAX
#include <windows.h>

// disable some common warnings
#ifdef _MSC_VER
#pragma warning(disable: 4150) // deletion of pointer to incomplete type
#pragma warning(disable: 4305) // conversion to smaller type, loss of data
#pragma warning(disable: 4244) // integer type converted to smaller integer type
#endif

// Visual Leak Debugger
#ifdef BUILD_DEBUG
//#define VLD_FORCE_ENABLE
//#include <vld.h>
#endif
#endif

// C++ headers
#include <string>
#include <algorithm>
#include <iostream>
#include <vector>

// C headers
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>

// OpenGL headers
#ifndef PLATFORM_LINUX
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#define AL_ALEXT_PROTOTYPES

// OpenAL headers
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>
#include <AL/efx-creative.h>
#include <AL/efx-presets.h>

// SDL2 headers
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_gamecontroller.h>
#ifdef PLATFORM_WINDOWS
#include <SDL2/SDL_syswm.h>
#endif

// GLM forward-decl header
#define GLM_FORCE_RADIANS
#include <glm/fwd.hpp>

// native file dialog
#include <nfd/nfd.h>

extern const float PI;
extern const float SQRT2;
extern const char* versionStr;

extern class Engine* mainEngine;

using std::min;
using std::max;

// sgn() function
template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

//Fixes for Windows/Linux incompatibilities
#ifdef PLATFORM_LINUX
#define strcmpi strcasecmp
#include <unistd.h>
#endif
