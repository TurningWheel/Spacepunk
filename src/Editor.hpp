// Editor.hpp

#pragma once

#include "Rotation.hpp"
#include "Vector.hpp"
#include "Tile.hpp"

class Frame;
class Client;
class Renderer;
class Sector;
class World;
class TileWorld;

class Editor {
public:
	Editor();
	~Editor();

	// editing mode
	enum editingmode_t {
		TILES = 0,
		TEXTURES = 1,
		ENTITIES = 2,
		SECTORS = 3,
		EDITINGMODE_TYPE_LENGTH
	};

	// string labels for the editing modes
	static const char* editingModeStr[EDITINGMODE_TYPE_LENGTH];

	// widget mode
	enum widgetmode_t {
		NONE = 0,
		TRANSLATE = 1,
		ROTATE = 2,
		SCALE = 3,
		WIDGETMODE_TYPE_LENGTH
	};

	// sets up the editor
	// @param _client the client that the editor is attached to
	void init(Client& _client);

	// sets up the editor
	// @param _client the client that the editor is attached to
	// @param name the name of the new world to create and edit
	// @param tiles if true, the world is tile-based
	// @param w the width of the new world, in tiles (unused for sector worlds)
	// @param h the height of the new world, in  (unused for sector worlds)
	void init(Client& _client, const char* name, bool tiles, int w, int h);

	// sets up the editor
	// @param _client the client that the editor is attached to
	// @param path file path to the world to load
	void init(Client& _client, const char* path);

	// toggles the entity properties window
	void buttonEntityProperties();

	// opens the entity add component window
	// @param uid the uid of the component which will get the new component
	void buttonEntityAddComponent(unsigned int uid);

	// confirms creation of a new world
	void buttonNewConfirm();

	// opens a new world dialog
	void buttonNew();

	// opens a save dialog
	void buttonSave();

	// opens a load dialog
	void buttonLoad();

	// open the editor settings window
	void buttonEditorSettings();

	// applies properties written in the editor settings window
	void buttonEditorSettingsApply();

	// open the map settings window
	void buttonMapSettings();

	// applies properties written in the map settings window
	void buttonMapSettingsApply();

	// opens the help window
	void buttonHelp();

	// rotate the world by the specified amount
	void buttonWorldRotate(int rotate);

	// play a sound effect, if they are enabled
	// @param path path to the sound to play
	void playSound(const char* path);

	// selects an entity in the editor
	// @param uid the uid of the entity to select
	// @param selected if true, the entity will be selected, otherwise it will be deselected
	void selectEntity(const Uint32 uid, const bool selected);

	// toggles the selection of an entity in the editor
	// @param uid the uid of the entity to select
	void toggleSelectEntity(const Uint32 uid);

	// selects all entities in the editor
	// @param selected if true, the entities will be selected, otherwise they will be deselected
	void selectAllEntities(const bool selected);

	// selects an entity def to spawn (assigns pointer)
	// @param name the name of the entity to spawn
	void selectEntityForSpawn(const char* name);

	// rename all selected entities
	// @param name the name to give to the entities
	void entitiesName(const char* name);

	// set the script for all selected entities
	// @param script the script to give to the entities
	void entitiesScript(const char* script);

	// toggles the given flag on all selected entities
	// @param flag the flag to toggle
	void entitiesFlag(const Uint32 flag);

	// exports each selected entity to a separate json file
	void entitiesSave();

	// set a key value pair on an entity
	// @param pair the encoded pair
	void entityKeyValueEnter(const char* pair);

	// remove a key value pair on an entity
	// @param key the key to search for and remove
	void entityKeyValueRemove(const char* pair);

	// update the editor field with the given pair
	// @param pair the encoded pair
	void entityKeyValueSelect(const char* pair);

	// expands a given component on selected entities
	// @param uid the uid of the component to expand
	void entityComponentExpand(unsigned int uid);

	// collapses a given component on selected entities
	// @param uid the uid of the component to expand
	void entityComponentCollapse(unsigned int uid);

	// add a new component to an entity
	// @param uid uid of the new component's parent (0 = entity root)
	// @param type the type of component to add
	void entityAddComponent(unsigned int uid, Uint32 type);

	// duplicate a component in the entity
	// @param uid the uid of the component to duplicate
	void entityCopyComponent(unsigned int uid);

	// removes a component from an entity
	// @param uid the uid of the component to remove
	void entityRemoveComponent(unsigned int uid);

	// renames a given component on selected entities
	// @param uid the uid of the component
	// @param name the new name
	void entityComponentName(unsigned int uid, const char* name);

	// translates a given component on selected entities
	// @param uid the uid of the component
	// @param dimension the dimension to translate
	// @param value the value to use
	void entityComponentTranslate(unsigned int uid, int dimension, float value);

	// rotates a given component on selected entities
	// @param uid the uid of the component
	// @param dimension the dimension to rotate
	// @param value the value to use
	void entityComponentRotate(unsigned int uid, int dimension, float value);

	// scales a given component on selected entities
	// @param uid the uid of the component
	// @param dimension the dimension to scale
	// @param value the value to use
	void entityComponentScale(unsigned int uid, int dimension, float value);

	// sets the x position of all selected entities
	// @param x the position value to use
	void widgetTranslateX(float x);

	// sets the y position of all selected entities
	// @param y the position value to use
	void widgetTranslateY(float y);

	// sets the z position of all selected entities
	// @param z the position value to use
	void widgetTranslateZ(float z);

	// sets the yaw of all selected entities
	// @param yaw the rotation value to use
	void widgetRotateYaw(float yaw);

	// sets the yaw of all selected entities
	// @param pitch the rotation value to use
	void widgetRotatePitch(float pitch);

	// sets the yaw of all selected entities
	// @param roll the rotation value to use
	void widgetRotateRoll(float roll);

	// sets the x scale of all selected entities
	// @param x the scale value to use
	void widgetScaleX(float x);

	// sets the y scale of all selected entities
	// @param y the scale value to use
	void widgetScaleY(float y);

	// sets the z scale of all selected entities
	// @param z the scale value to use
	void widgetScaleZ(float z);

	// optimizes the chunks of a tile world
	void optimizeChunks();

	// setup a frame in the editor
	void preProcess();

	// run a frame in the editor
	// @param usable if true, pointer is usable; if false, pointer is not usable
	void process(const bool usable);

	// end-frame actions
	void postProcess();

	// draw hud elements in the 3D view
	// @param renderer the renderer to draw with
	void draw(Renderer& renderer);

	// getters & setters
	const bool				isInitialized() const { return initialized; }
	const bool				isCeilingMode() const { return ceilingMode; }
	Sint32					getEditingMode() const { return static_cast<Sint32>(editingMode); }
	const Uint32			getHighlightedObj() const { return highlightedObj; }
	Sint32					getWidgetMode() const { return static_cast<Sint32>(widgetMode); }
	const int				getTextureSide() const { return textureSide; }
	const Camera*			getEditingCamera() const { return editingCamera; }
	const Camera*			getMinimapCamera() const { return minimap; }
	const char*				getTextureUnderMouse() const { return textureUnderMouse; }
	const bool				isTextureSelectorActive() const { return textureSelectorActive; }
	const bool				isFullscreen() const { return fullscreen; }

	void	setHighlightedObj(const Uint32 obj) { highlightedObj = obj; highlightedObjManuallySet = true; }
	void	setWidgetMode(const Uint32 _widgetMode) { widgetMode = static_cast<widgetmode_t>(_widgetMode); }
	void	setCeilingMode(const bool _ceilingMode) { ceilingMode = _ceilingMode; }
	void	setEditingMode(const Uint32 _editingMode) { editingMode = static_cast<editingmode_t>(_editingMode); }
	void	setTextureSide(const int _textureSide) { textureSide = _textureSide; }
	void	setTextureSelectorActive(const bool _b) { textureSelectorActive = _b; }
	void	setFullscreen(const bool _b) { fullscreen = _b; }

private:
	Client* client = nullptr;
	World* world = nullptr;

	bool initialized = false;

	// editing variables
	Camera* editingCamera = nullptr;
	Camera* minimap = nullptr;
	Uint32 editTick = 0;
	const char* textureUnderMouse = ""; // the name of the texture under the mouse pointer
	int textureSide = 0; // the side of the tile to texture
	bool ceilingMode = false; // when false, you're in floor editing mode, otherwise, ceiling mode
	editingmode_t editingMode = TILES;
	bool editingText = false;
	bool fullscreen = false; // when true, viewport takes up whole window
	bool guiNeedsUpdate = false; // when true, redo entity properties window
	Tile::shadervars_t tileShaderVars;

	// when this changes, we need to update the entity properties panel
	LinkedList<Uint32> selectedEntityManifest;

	// selecting objects
	LinkedList<Uint32> selectedObjects;
	Uint32 highlightedObj = UINT32_MAX; // entity that we're hovering the mouse over
	Uint32 highlightedVertex = UINT32_MAX; // vertex that we're hovering the mouse over
	Sector* highlightedSector = nullptr; // sector that we're hovering the mouse over
	int highlightedFace = -1; // sector face we're pointing at
	bool highlightedObjManuallySet = false;
	bool leftClick = false, leftClicking = false, leftClickLock = false;
	Entity* entityToSpawn = nullptr;

	// texture browser
	bool textureSelectorActive = false;
	Sint32 textureScroll = 0;

	// editing widget
	bool draggingWidget = false;
	Vector oldIntersection;
	widgetmode_t widgetMode = TRANSLATE;
	Vector oldWidgetPos = Vector(0.f);
	Vector widgetPos = Vector(0.f);
	Rotation widgetAng = Rotation(0.f, 0.f, 0.f);
	Vector widgetScale = Vector(1.f);
	bool widgetVisible = false;
	Entity* widget = nullptr;
	Entity* widgetX = nullptr;
	Entity* widgetXY = nullptr;
	Entity* widgetY = nullptr;
	Entity* widgetYZ = nullptr;
	Entity* widgetZ = nullptr;
	Entity* widgetZX = nullptr;
	LinkedList<Entity*> widgetActors;
	Map<void*, Vector> oldVecs;
	Map<void*, Quaternion> oldAngs;

	// clipboard
	LinkedList<Entity*> copiedEntities;
	TileWorld* copiedTiles = nullptr;

	// 3 orientations for the minimap
	static const Rotation minimapRot[3];

	// sets up widgets and pointers for the given world
	// @param world the world to put the new widgets in
	void initWidgets();

	// sets up the editor GUI
	// @param camRect the main viewport to build the editor gui around
	void initGUI(const Rect<int>& camRect);

	// updates the images in the given widget frame
	// @param parent the frame containing the widget icon
	// @param translateImg the path to the translate icon
	// @param rotateImg the path to the rotate icon
	// @param scaleImg the path to the scale icon
	void updateWidgetImages(Frame* parent, const char* translateImg, const char* rotateImg, const char* scaleImg);

	// update tile pane
	// @param world world we are editing
	// @param pointerX pointer X coord
	// @param pointerY pointer Y coord
	void updateTileFields(TileWorld& world, Sint32 pointerX, Sint32 pointerY);

	// update tiles in the given world
	// @param world world we are editing
	void updateTiles(TileWorld& world);

	// edit the tiles in the world
	void editTiles(bool usable);

	// edit the entities in the world
	void editEntities(bool usable);

	// edit the sectors in the world
	void editSectors(bool usable);

	// widget controls
	// @param world the world to edit
	void handleWidget(World& world);

	// update the editor gui (lists, etc)
	// @param gui the gui to update
	void updateGUI(Frame& gui);

	// add GUI for a component
	// @param properties the properties window
	// @param component the component to propertify
	// @param x x window offset
	// @param y y window offset
	void componentGUI(Frame& properties, Component* component, int& x, int& y);
};

extern Cvar cvar_snapEnabled;
extern Cvar cvar_snapTranslate;
extern Cvar cvar_snapRotate;
extern Cvar cvar_snapScale;