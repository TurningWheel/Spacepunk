An original 3D engine by Sheridan Rathbun. Intended to be used by Spacepunk. This is the open-source engine release.

Hit the blue "Play" button at the top of the editor to run the demo level.

![Man on the beach](https://raw.githubusercontent.com/TurningWheel/Spacepunk/master/Screenshot.png)

Features:

	* OpenGL 4.5 core profile renderer
	* support for several major polygonal formats via Assimp
	* hardware skinned characters (skeletal animation)
	* support for voxlap voxel format (.vox)
	* Lua for client/server/world/entity/gui programming
	* rigid body dynamics and kinematic (game style) physics via Bullet
	* multiple convex collision shapes (box, sphere, capsule, cylinder) and collision meshes
	* multiplayer support (listen servers, dedicated servers, and clients)
	* splitscreen multiplayer (can be combined with online multiplayer)
	* fully dynamic light and shadows (either w/ shadow maps or stencil shadows)
	* multiple light shapes (point, cone, box)
	* json support for level format, asset definitions, etc.
	* shaders and material-based rendering
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

	Windows:

		Visual Studio 2017 or later is required to compile.
		Visual Studio 2015 was previously supported, but is no longer up to date.

		The project is configured to look for dependencies in C:/GameLibs,
		which should have "include" and "lib" directories. In the future,
		this may use an environment variable instead. For our private use,
		we distribute a GameLibs.zip directory that includes built versions
		of every necessary lib and header, which you can request from me by
		using my contact info below.

	Linux:

		The engine has been tested to compile on Ubuntu 18.04.1 LTS.
		You will need cmake, make, and g++ to compile. Run these commands to
		produce a spacepunk.bin file in the "build" folder:

		cmake -DCMAKE_BUILD_TYPE=Release
		make

		There is a known issue regarding LuaBridge where addConstructor() won´t
		compile correctly with g++. You can fix this by injecting the following code in
		LuaBridge/detail/Namespace.h (around line 836):

		Class <T>& addConstructor ()
		{
			lua_pushcclosure (L,
				&ctorPlacementProxy <typename FuncTraits <void (*) (void)>::Params, T>, 0);
			rawsetfield(L, -2, "__call");

			return *this;
		}

Controls:

	These can be rebound via autoexec.cfg in the base folder. Also check help in the editor.

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

	Send all suggestions and comments to sheridan.rathbun@gmail.com
