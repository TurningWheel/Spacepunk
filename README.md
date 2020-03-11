An original 3D engine by Sheridan Rathbun. Intended to be used by Spacepunk. This is the open-source engine release.

See the docs directory for an in-depth manual of the engine. Most of the functions described in there are directly accessible to Lua scripts.

Hit the blue "Play" button at the top of the editor to run the demo level.

![Man on the beach](https://raw.githubusercontent.com/TurningWheel/Spacepunk/master/Screenshot.png)

Features:

	* OpenGL 4.5 core profile renderer
	* support for several major model formats via Assimp
	* hardware skinned characters (skeletal animation)
	* blend multiple skeletal animations per bone per character
	* support for voxlap voxel format (.vox)
	* Lua for game scripting
	* JSON support for level format, asset definitions, etc.
	* rigid body dynamics and kinematic (game style) physics via Bullet
	* multiple convex collision shapes (box, sphere, capsule, cylinder), plus collision meshes
	* multiplayer support (listen servers, dedicated servers, and clients)
	* splitscreen multiplayer (can be combined with online multiplayer)
	* fully dynamic light and shadows (either w/ shadow maps or stencil shadows)
	* multiple light shapes (point, cone, box)
	* GLSL shaders and material-based rendering
	* in-game command line (console), cvars and ccmds can be declared anywhere
	* 3D audio and filters for special effects (OpenAL)
	* support for numerous input devices and remapping (game controllers)
	* entity component system (tree hierarchy)
	* a few different random level generators
	* UI engine (rendering + event callbacks defined in script)
	* asynchronous pathfinding
	* multi-world servers

Build/Install:

	The following libs must be installed to build Spacepunk:

		glew
		openAL
		SDL2
		SDL_net
		SDL_ttf
		SDL_image
		SDL_mixer
		bullet
		glm
		nfd (https://github.com/mlabbe/nativefiledialog)
		luajit
		luabridge (https://github.com/vinniefalco/LuaBridge)

		On Windows these are available through the included SpacepunkLibs.zip

	Windows:

		Visual Studio 2017 or later is required to compile.

		Create a SPACEPUNK_LIBS environment variable, and point it to the
		directory created when you extract SpacepunkLibs.zip

	Linux:

		The engine has been tested to compile on Ubuntu 18.04.1 LTS.
		You will need cmake, make, and g++ to compile. Run these commands to
		produce a spacepunk.bin file in the "build" folder:

		cmake -DCMAKE_BUILD_TYPE=Release
		make

		There is a known issue regarding LuaBridge where addConstructor() won't
		compile correctly with g++. You can fix this by injecting the following code in
		LuaBridge/detail/Namespace.h (around line 836):

		Class <T>& addConstructor ()
		{
			lua_pushcclosure (L,
				&ctorPlacementProxy <typename FuncTraits <void (*) (void)>::Params, T>, 0);
			rawsetfield(L, -2, "__call");

			return *this;
		}

	Macintosh:

		The engine has not been tested to compile on Macintosh, but should not
		take much effort to get it there. Please reach out to me using my
		contact info below if you are interested.

Controls:

	These can be inspected and rebound via autoexec.cfg in the base directory.
	Also check the help screen in the editor.

Console:

	Type "help" for a full list of commands. Other basic commands:

	windowed			- toggle window mode
	fullscreen			- toggle fullscreen mode
	size <x> <y>			- where <x> and <y> are the desired resolution (eg 1280 720)
	clear				- clear the console
	version				- print engine version info
	editor				- start game editor
	dump				- clear engine cache
	exit				- exit immediately

Contact:

	Send all suggestions, comments, and questions to sheridan.rathbun@gmail.com
