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

Controls:

	These can be rebound via autoexec.cfg in the base folder. Also check help in the editor.

Console:

	Type "help" for a full list of commands. Other basic commands:

	windowed			- toggle window mode
	fullscreen			- toggle fullscreen mode
	size <x> <y>		- where <x> and <y> are the desired resolution (eg 1280 720)
	clear				- clear the console
	version				- print engine version info
	editor				- start game editor
	dump				- clear engine cache
	exit				- exit immediately

Contact:

	Send all suggestions and comments to sheridan.rathbun@gmail.com