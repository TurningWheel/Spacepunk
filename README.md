The new 3D engine for our next game. This is the open-source engine release.

Existing features:

	* OpenGL 4.5 core-compliant renderer
	* tile-based world format
	* a few different random level generators
	* tiles can have different floor and ceiling heights and be sloped
	* support for several major polygonal formats through ASSIMP
	* hardware skinned characters (bones animation)
	* support for barony voxel format (.vox)
	* Lua for client/server/world/entity/gui programming
	* full multiplayer support (listen servers, dedicated servers, and clients)
	* splitscreen multiplayer (can be combined with online multiplayer)
	* fully dynamic lighting w/ stencil shadows
	* json support for level format, asset definitions, and other manifests
	* shader and material-based rendering
	* in-game command line (console)
	* multi-world servers
	* Asynchronous A* pathfinding
	* limited physics support through Bullet (incomplete)
	* 3D audio w/ filters for special effects (OpenAL)
	* Support for numerous input devices and remapping (game controllers)
	* UI engine

Controls:

	These can be rebound via autoexec.cfg in the base folder.

	`					- Open / close console
	Right click		 	- Toggle mouselook
	WASDEQ				- Move forward, left, backward, right, up, down
	Mouse               - Move selection cursor / do mouselook
	Backspace/Escape	- Deselect everything
	F6					- Take screenshot

Geometry mode:

	Left click          - Begin/end rectangle selection of tiles
	Mousewheel			- Raise/lower selected tiles
	Numpad 8, 4, 6, 2	- Slope selected tiles
	Numpad 5 			- Remove slope from selected tiles
	Numpad -, +			- Raise/lower selected tiles

Texture mode:

	Left click			- Begin/end rectangle selection of tiles
	Numpad 8, 4, 6, 2	- Choose texture for wall in given direction
	Numpad 5 			- Choose texture for floor/ceiling

Entity mode:

	Left click			- Select entity / manipulate widget

Console:

	windowed			- toggle window mode
	fullscreen			- toggle fullscreen mode
	size=<res>			- where <res> is the desired resolution (eg 1280x720)
	clear				- clear the console
	version				- print engine version info
	dedicated			- (init only) puts the engine in dedicated server (headless) mode
	game=<path>			- (init only) set game assets folder (used for mods)
	editor				- start game editor
	play <snd>			- where <snd> is the filename of a sound to play
	loadconfig <path>	- where <path> is the name of a config file to execute
	exit				- exit the game

Contact:

	Send all suggestions and comments to sheridan.rathbun@gmail.com