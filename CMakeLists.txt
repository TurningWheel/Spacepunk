cmake_minimum_required (VERSION 3.0)
project (spacepunk)

##############################################################
#  project settings
##############################################################

# output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

# default build type
if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# cmake modules path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# always build 64-bit
set(ARCH_X64 1)

# OSX arch type
set(CMAKE_OSX_ARCHITECTURES x86_64)

# setup configure file
if( WIN32 )
	set(WINDOWS 1)
else()
	set(WINDOWS 0)
endif()
if( APPLE )
	set(APPLE 1)
else()
	set(APPLE 0)
endif()
configure_file ( "${PROJECT_SOURCE_DIR}/src/CMakeConfig.h.in" "${PROJECT_SOURCE_DIR}/src/CMakeConfig.h")

# rando stuff
set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}" )
set( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}" )

##############################################################
#  compiler flags
##############################################################

# CXX flags and other basic compile options
if( WINDOWS )
	# windows settings
	set(NOT_DWORD_DEFINED 0)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pg -O3 -ffast-math -funroll-loops -malign-double -fstrict-aliasing -std=c++14 -mwindows")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops -malign-double -fstrict-aliasing -std=c++14 -mwindows")
else()
	set(NOT_DWORD_DEFINED 1)
	if( APPLE )
		# osx settings
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pg -ffast-math -funroll-loops -std=c++14 -fstrict-aliasing")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops -std=c++14 -fstrict-aliasing")

		# bundle settings
		set(MACOSX_BUNDLE_BUNDLE_NAME Spacepunk)
		set(MACOSX_BUNDLE true)
		set(MACOSX_BUNDLE_INFO_STRING Spacepunk 1.0.0.0)
		set(MACOSX_BUNDLE_LONG_VERSION 1.0.0.0)
		set(MACOSX_BUNDLE_SHORT_VERSION_STRING 1.0.0.0)
		set(MACOSX_BUNDLE_BUNDLE_VERSION 1.0.0.0)
		set(MACOSX_BUNDLE_ICON_FILE "spacepunk.icns")

		# set run directory
		set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -LD_RUNPATH_SEARCH_PATHS=@loader_path/../Frameworks -rpath @loader_path/../Frameworks")
	else()
		# linux settings
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pg -ffast-math -funroll-loops -std=c++14 -fstrict-aliasing")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops -std=c++14 -fstrict-aliasing")

		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wl,-rpath=$ORIGIN")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wl,-rpath=$ORIGIN")
	endif()
endif()

# set cpu arch
if (ARCHITECTURE)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -mcpu=${ARCHITECTURE}")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEBUG} -mcpu=${ARCHITECTURE}")
endif()

message("***************************")
message("Debug flags: ")
message("${CMAKE_CXX_FLAGS_DEBUG}")
message("Release flags: ")
message("${CMAKE_CXX_FLAGS_RELEASE}")
message("***************************")

##############################################################
#  find packages and include dirs
##############################################################

if( WINDOWS )
	# windows packages
	find_package(Threads REQUIRED)
	find_package(Bullet REQUIRED)
	find_package(OpenGL REQUIRED)
	find_package(GLM REQUIRED)
	find_package(GLEW REQUIRED)
	find_package(SDL2 REQUIRED)
	find_package(SDL2_image REQUIRED)
	find_package(SDL2_mixer REQUIRED)
	find_package(SDL2_net REQUIRED)
	find_package(SDL2_ttf REQUIRED)
	find_package(PNG REQUIRED)
	find_package(Libdl REQUIRED)
	find_package(ASSIMP REQUIRED)
	find_package(Chaiscript)
	find_package(OpenAL REQUIRED)
	find_package(PkgConfig REQUIRED)
	find_package(LuaJIT REQUIRED)
	find_package(RapidJSON REQUIRED)
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
	#find_package(wxWidgets)

	include_directories(${GTK3_INCLUDE_DIRS})
	include_directories(${OPENAL_INCLUDE_DIR})
	include_directories(${OPENGL_INCLUDE_DIR})
	include_directories(${GLM_INCLUDE_DIRS})
	include_directories(${GLEW_INCLUDE_DIRS})
	include_directories(${THREADS_INCLUDE_DIR})
	include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${SDL2IMAGE_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${SDL2NET_INCLUDE_DIR} ${SDL2_NET_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIR} ${SDL2_TTF_INCLUDE_DIRS})
	include_directories(${BULLET_INCLUDE_DIRS})
	include_directories(${PNG_INCLUDE_DIR})
	include_directories(${LIBDL_INCLUDE_DIR})
	include_directories(${LUAJIT_INCLUDE_DIR})
	include_directories(${RAPIDJSON_INCLUDE_DIRS})
	#include_directories(${CHAISCRIPT_INCLUDE_DIR})
	#include_directories(${wxWidgets_INCLUDE_DIRS})

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${RAPIDJSON_CXX_FLAGS}")
else()
	if( APPLE )
		# osx packages
		find_library(OpenGL_LIBRARY OpenGL )
		set(EXTRA_LIBS ${OpenGL_LIBRARY})

		find_package(Threads REQUIRED)
		find_package(Bullet REQUIRED)
		find_package(GLM REQUIRED)
		find_package(GLEW REQUIRED)
		find_package(SDL2 REQUIRED)
		find_package(SDL2_image REQUIRED)
		find_package(SDL2_mixer REQUIRED)
		find_package(SDL2_net REQUIRED)
		find_package(SDL2_ttf REQUIRED)
		find_package(GLUT REQUIRED)
		find_package(Libdl REQUIRED)
		find_package(ASSIMP REQUIRED)
		#find_package(Chaiscript)
		find_package(OpenAL REQUIRED)
		find_package(PkgConfig REQUIRED)
		pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
		#find_package(wxWidgets)

		# png gets included in the section section using special code

		include_directories(${GTK3_INCLUDE_DIRS})
		include_directories(${OPENAL_INCLUDE_DIR})
		include_directories(${GLM_INCLUDE_DIRS})
		include_directories(${GLEW_INCLUDE_DIRS})
		include_directories(${THREADS_INCLUDE_DIR})
		include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${SDL2IMAGE_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${SDL2NET_INCLUDE_DIR} ${SDL2_NET_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIR} ${SDL2_TTF_INCLUDE_DIRS})
		include_directories(${BULLET_INCLUDE_DIRS})
		include_directories(${PNG_INCLUDE_DIR})
		include_directories(${GLUT_INCLUDE_DIR})
		include_directories(${LIBDL_INCLUDE_DIR})
		#include_directories(${CHAISCRIPT_INCLUDE_DIR})
		#include_directories(${wxWidgets_INCLUDE_DIRS})
	else()
		# linux packages
		include(FindPkgConfig)

		find_package(Threads REQUIRED)
		find_package(Bullet REQUIRED)
		find_package(OpenGL REQUIRED)
		find_package(GLM REQUIRED)
		find_package(GLEW REQUIRED)
		find_package(SDL2 REQUIRED)
		find_package(SDL2_image REQUIRED)
		find_package(SDL2_mixer REQUIRED)
		find_package(SDL2_net REQUIRED)
		find_package(SDL2_ttf REQUIRED)
		find_package(PNG REQUIRED)
		find_package(Libdl REQUIRED)
		find_package(ASSIMP REQUIRED)
		#find_package(Chaiscript)
		find_package(OpenAL REQUIRED)
		find_package(PkgConfig REQUIRED)
		pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
		#find_package(wxWidgets)
		find_package(NFD REQUIRED)
		find_package(LuaJIT REQUIRED)
		find_package(RapidJSON REQUIRED)

		include_directories(${GTK3_INCLUDE_DIRS})
		include_directories(${OPENAL_INCLUDE_DIR})
		include_directories(${OPENGL_INCLUDE_DIR})
		include_directories(${GLM_INCLUDE_DIRS})
		include_directories(${GLEW_INCLUDE_DIRS})
		include_directories(${THREADS_INCLUDE_DIR})
		include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${SDL2IMAGE_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${SDL2NET_INCLUDE_DIR} ${SDL2_NET_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIR} ${SDL2_TTF_INCLUDE_DIRS})
		include_directories(${BULLET_INCLUDE_DIRS})
		include_directories(${PNG_INCLUDE_DIR})
		include_directories(${LIBDL_INCLUDE_DIR})
		#include_directories(${CHAISCRIPT_INCLUDE_DIR})
		include_directories(${LUAJIT_INCLUDE_DIR})
		#include_directories(${wxWidgets_INCLUDE_DIRS})
		include_directories(${NFD_INCLUDE_DIRS})
		include_directories(${RAPIDJSON_INCLUDE_DIRS})
		message("NFD Include Dirs: ")
		message(${NFD_INCLUDE_DIRS})

		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${RAPIDJSON_CXX_FLAGS}")
	endif()
endif()

##############################################################
#  source files
##############################################################

add_subdirectory(src)

if( WINDOWS )
	# windows sources
	add_executable(spacepunk ${GAME_SOURCES})

	target_link_libraries(spacepunk -lmingw32)
	target_link_libraries(spacepunk ${SDL2_LIBRARIES} ${SDL2_LIBRARY} ${SDL2IMAGE_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDLMIXER_LIBRARY} ${SDL2_NET_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2TTF_LIBRARY})

	target_link_libraries(spacepunk ${ASSIMP_LIBRARIES})
	target_link_libraries(spacepunk ${LIBDL_LIBRARIES})
	target_link_libraries(spacepunk ${OPENAL_LIBRARY})
	target_link_libraries(spacepunk ${OPENGL_LIBRARIES})
	target_link_libraries(spacepunk ${GLEW_LIBRARIES})
	target_link_libraries(spacepunk ${BULLET_LIBRARIES})
	target_link_libraries(spacepunk ${THREADS_LIBRARIES})
	target_link_libraries(spacepunk ${PNG_LIBRARY})
	#target_link_libraries(spacepunk ${wxWidgets_LIBRARIES})
	target_link_libraries(spacepunk -lm)
	target_link_libraries(spacepunk -lnfd)
	link_directories(${GTK3_LIBRARY_DIRS})
	target_link_libraries(spacepunk ${GTK3_LIBRARIES})

	# it's necessary to link winsock on windows
	target_link_libraries(spacepunk wsock32 ws2_32)
	target_link_libraries(spacepunk LINK_PUBLIC ${LUAJIT_LIBRARIES})
else()
	if( APPLE )
		# osx sources
		set(COPY_FRAMEWORKS ${PROJECT_SOURCE_DIR}/../Frameworks/SDL.framework ${PROJECT_SOURCE_DIR}/../Frameworks/SDL_image.framework ${PROJECT_SOURCE_DIR}/../Frameworks/SDL_mixer.framework ${PROJECT_SOURCE_DIR}/../Frameworks/SDL_net.framework)
		message("{COPY_FRAMEWORKS}: ${COPY_FRAMEWORKS}")
		add_executable(spacepunk MACOSX_BUNDLE ${GAME_SOURCES} ${MACOSX_BUNDLE_ICON_FILE} libfmodex.dylib /opt/local/lib/libpng16.16.dylib)
		SET_SOURCE_FILES_PROPERTIES(/opt/local/lib/libpng16.16.dylib PROPERTIES MACOSX_PACKAGE_LOCATION MacOS)
		SET_SOURCE_FILES_PROPERTIES(${COPY_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
		SET_SOURCE_FILES_PROPERTIES(${MACOSX_BUNDLE_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
		set_source_files_properties(${GAME_SOURCES} PROPERTIES COMPILE_FLAGS "-x objective-c")

		target_link_libraries(spacepunk ${SDL2_LIBRARIES} ${SDL2_LIBRARY} ${SDL2IMAGE_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDLMIXER_LIBRARY} ${SDL2_NET_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2TTF_LIBRARY})

		target_link_libraries(spacepunk ${ASSIMP_LIBRARIES})
		target_link_libraries(spacepunk ${LIBDL_LIBRARIES})
		target_link_libraries(spacepunk ${EXTRA_LIBS}) # apple needs this for OpenGL to work
		target_link_libraries(spacepunk ${OPENAL_LIBRARY})
		target_link_libraries(spacepunk ${OPENGL_LIBRARIES})
		target_link_libraries(spacepunk ${GLEW_LIBRARIES})
		target_link_libraries(spacepunk ${BULLET_LIBRARIES})
		target_link_libraries(spacepunk ${THREADS_LIBRARIES})
		#target_link_libraries(spacepunk ${wxWidgets_LIBRARIES})
		target_link_libraries(spacepunk -lm)
		target_link_libraries(spacepunk -lnfd)
		link_directories(${GTK3_LIBRARY_DIRS})
		target_link_libraries(spacepunk ${GTK3_LIBRARIES})
		target_link_libraries(spacepunk ${LUAJIT_LIBRARIES})

		if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
			# 64 bit
			target_link_libraries(spacepunk -L/usr/lib64 -lstdc++)
		endif()

		install(TARGETS spacepunk RUNTIME DESTINATION bin COMPONENT Runtime BUNDLE DESTINATION Resources )
	else()
		# linux sources
		add_executable(spacepunk ${GAME_SOURCES})
		set_target_properties(spacepunk PROPERTIES SUFFIX ".bin")

		target_link_libraries(spacepunk ${SDL2_LIBRARIES} ${SDL2_LIBRARY} ${SDL2IMAGE_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDLMIXER_LIBRARY} ${SDL2_NET_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2TTF_LIBRARY})

		target_link_libraries(spacepunk ${ASSIMP_LIBRARIES})
		target_link_libraries(spacepunk ${LIBDL_LIBRARIES})
		target_link_libraries(spacepunk ${OPENAL_LIBRARY})
		target_link_libraries(spacepunk ${OPENGL_LIBRARIES})
		target_link_libraries(spacepunk ${GLEW_LIBRARIES})
		target_link_libraries(spacepunk ${BULLET_LIBRARIES})
		target_link_libraries(spacepunk ${THREADS_LIBRARIES})
		target_link_libraries(spacepunk ${PNG_LIBRARY})
		#target_link_libraries(spacepunk ${wxWidgets_LIBRARIES})
		target_link_libraries(spacepunk -lm)
		#target_link_libraries(spacepunk -lnfd)
		target_link_libraries(spacepunk ${NFD_LIBRARIES})
		link_directories(${GTK3_LIBRARY_DIRS})
		target_link_libraries(spacepunk ${GTK3_LIBRARIES})
		target_link_libraries(spacepunk ${LUAJIT_LIBRARIES})

		if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
			# 64 bit
			target_link_libraries(spacepunk -L/usr/lib64 -lstdc++)
		endif()
	endif()
endif()
