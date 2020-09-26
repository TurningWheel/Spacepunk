// Editor.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Client.hpp"
#include "World.hpp"
#include "Resource.hpp"
#include "Material.hpp"
#include "Frame.hpp"
#include "Button.hpp"
#include "Field.hpp"
#include "Entity.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "Mixer.hpp"
#include "Renderer.hpp"
#include "Editor.hpp"
#include "Console.hpp"
#include "BasicWorld.hpp"

//Component Headers.
#include "Model.hpp"
#include "BBox.hpp"
#include "Speaker.hpp"
#include "Character.hpp"

// log message code strings
const char* Editor::editingModeStr[Editor::EDITINGMODE_TYPE_LENGTH] = {
	"Tile Mode",
	"Tile Texture Mode",
	"Entity Mode",
	"Sector Mode"
};

const Rotation Editor::minimapRot[3] = {
	Rotation(-PI / 2.f, PI / 2.f, 0.f),
	Rotation(-PI / 2.f, 0.f, 0.f),
	Rotation(0.f, 0.f, 0.f)
};

Cvar cvar_snapEnabled("editor.snap.enabled", "Enable snap-to-grid", "1");
Cvar cvar_snapTranslate("editor.snap.translate", "Translate snap", "32.0");
Cvar cvar_snapRotate("editor.snap.rotate", "Rotate snap", "15.0");
Cvar cvar_snapScale("editor.snap.scale", "Scale snap", "25.0");
Cvar cvar_editorSounds("editor.sounds", "Enable editor sound effects", "1");

Editor::~Editor() {
	if (copiedTiles) {
		delete copiedTiles;
		copiedTiles = 0;
	}
}

void Editor::init(Client& _client) {
	client = &_client;

	if (!initialized) {
		mainEngine->fmsg(Engine::MSG_INFO, "starting editor");
	} else {
		mainEngine->fmsg(Engine::MSG_INFO, "creating new map...");
	}

	// close all open worlds and create a blank one
	client->closeAllWorlds();
	//world = client->newTileWorld("Untitled World", 32, 32);
	//editingMode = TILES;
	world = client->newBasicWorld("Untitled World");
	editingMode = ENTITIES;

	// initialize components
	initWidgets();
	if (!initialized) {
		initGUI(editingCamera->getWin());
		mainEngine->fmsg(Engine::MSG_NOTE, "");
		mainEngine->fmsg(Engine::MSG_NOTE, "editor successfully started");
		mainEngine->fmsg(Engine::MSG_NOTE, "if this is your first time, click the button to the upper-right for a quick tour");
	}

	// enter edit mode
	initialized = true;
}

void Editor::init(Client& _client, const char* name, bool tiles, int w, int h) {
	client = &_client;

	if (!initialized) {
		mainEngine->fmsg(Engine::MSG_INFO, "starting editor");
	} else {
		mainEngine->fmsg(Engine::MSG_INFO, "creating new map...");
	}

	// close all open worlds and create a blank one
	client->closeAllWorlds();
	/*if( tiles ) {
		world = client->newTileWorld(name,w,h);
		editingMode = TILES;
	} else {
		world = client->newSectorWorld(name);
		editingMode = SECTORS;
	}*/
	world = client->newBasicWorld(name);
	editingMode = ENTITIES;

	// initialize components
	initWidgets();
	if (!initialized) {
		initGUI(editingCamera->getWin());
		mainEngine->fmsg(Engine::MSG_NOTE, "");
		mainEngine->fmsg(Engine::MSG_NOTE, "editor successfully started");
		mainEngine->fmsg(Engine::MSG_NOTE, "if this is your first time, click the button to the upper-right for a quick tour");
	}

	// enter edit mode
	initialized = true;
}

void Editor::init(Client& _client, const char* path) {
	if (!path || path[0] == '\0') {
		init(_client);
		return;
	}
	client = &_client;

	if (!initialized) {
		mainEngine->fmsg(Engine::MSG_INFO, "starting editor");
	} else {
		mainEngine->fmsg(Engine::MSG_INFO, "creating new map...");
	}

	// close all open worlds and create a blank one
	client->closeAllWorlds();
	world = client->loadWorld(path, true);
	if (world->getType() == World::WORLD_TILES) {
		editingMode = TILES;
	} else if (world->getType() == World::WORLD_SECTORS) {
		editingMode = SECTORS;
	} else {
		editingMode = ENTITIES;
	}

	// initialize components
	initWidgets();
	if (!initialized) {
		initGUI(editingCamera->getWin());
		mainEngine->fmsg(Engine::MSG_NOTE, "");
		mainEngine->fmsg(Engine::MSG_NOTE, "editor successfully started");
		mainEngine->fmsg(Engine::MSG_NOTE, "if this is your first time, click the button to the upper-right for a quick tour");
	}

	// add existing entities to level navigator
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;
		entity->addToEditorList();
	}

	// enter edit mode
	initialized = true;
}

void Editor::buttonEntityProperties() {
	Frame* frame = client->getGUI()->findFrame("editor_FrameEntityProperties");

	if (frame) {
		// the frame is open, we need to close it
		playSound("editor/menuclose.wav");
		frame->removeSelf();

		// close add component window...
		Frame* frame = client->getGUI()->findFrame("editor_FrameEntityAddComponent");
		if (frame) {
			frame->removeSelf();
		}
	} else {
		// open the frame
		playSound("editor/menuopen.wav");

		if (editingCamera) {
			frame = client->getGUI()->addFrame("editor_FrameEntityProperties");

			const Rect<int>& camRect = editingCamera->getWin();

			Rect<int> rect;
			rect.x = 0; rect.w = 400;
			rect.y = 0; rect.h = 400;
			frame->setActualSize(rect);
			rect.x = camRect.x + camRect.w - 400 - 8; rect.w = 400;
			rect.y = camRect.y + camRect.h - 400 - 48; rect.h = 400;
			frame->setSize(rect);
			frame->setColor(WideVector(.5f, .5f, .5f, 1.f));
		}
	}
}

void Editor::buttonEntityAddComponent(unsigned int uid) {
	Frame* frame = client->getGUI()->findFrame("editor_FrameEntityAddComponent");

	playSound("editor/open.wav");
	if (frame) {
		frame->removeSelf();
	}

	frame = client->getGUI()->addFrame("editor_FrameEntityAddComponent");

	int border = frame->getBorder();

	int mousex = mainEngine->getMouseX() * ((float)Frame::virtualScreenX / mainEngine->getXres());
	int mousey = mainEngine->getMouseY() * ((float)Frame::virtualScreenY / mainEngine->getYres());

	Rect<int> rect;
	rect.x = 0; rect.w = 200 + border * 2;
	rect.y = 0; rect.h = 200;
	frame->setActualSize(rect);
	rect.x = mousex + 10; rect.w = 200 + border * 2;
	rect.y = mousey + 10; rect.h = 200;
	frame->setSize(rect);
	frame->setColor(WideVector(.5f, .5f, .5f, 1.f));

	// component buttons
	for (Uint32 c = 0; c < Component::COMPONENT_MAX; ++c) {
		Button* button = frame->addButton("buttonAddComponent");

		Rect<int> rect;
		rect.x = border * 2 + (30 + border) * (c % 5); rect.w = 30;
		rect.y = border * 2 + (30 + border) * (c / 5); rect.h = 30;

		button->getParams().addInt(uid);
		button->getParams().addInt(c);
		button->setTooltip(Component::typeStr[c]);
		button->setIcon(Component::typeIcon[c]);
		button->setSize(rect);
	}

	// close button
	Button* button = frame->addButton("buttonClose");

	rect.x = border * 2 + (30 + border) * 5; rect.w = 30;
	rect.y = border * 2; rect.h = 30;

	button->setText("x");
	button->setSize(rect);
}

void Editor::buttonNewConfirm() {
	Frame* gui = client->getGUI();
	if (!gui)
		return;

	// collect tiles enabled
	bool tiles = false;
	{
		Frame* frame = gui->findFrame("editor_FrameNewConfirm");
		assert(frame);

		Button* button = frame->findButton("buttonTilesEnabled");
		assert(button);

		tiles = button->isPressed();
	}

	// collect name string
	const char* name = nullptr;
	{
		Frame* frame = gui->findFrame("editor_FrameNewConfirmName");
		assert(frame);

		Field* field = frame->findField("field");
		assert(field);

		name = field->getText();
	}

	// collect width
	int width = 32;
	{
		Frame* frame = gui->findFrame("editor_FrameNewConfirmWidth");
		assert(frame);

		Field* field = frame->findField("field");
		assert(field);

		width = strtol(field->getText(), nullptr, 10);
	}

	// collect height
	int height = 32;
	{
		Frame* frame = gui->findFrame("editor_FrameNewConfirmHeight");
		assert(frame);

		Field* field = frame->findField("field");
		assert(field);

		height = strtol(field->getText(), nullptr, 10);
	}

	// re-init editor with these properties
	init(*client, name, tiles, width, height);

	// close the new map window
	Frame* frame = gui->findFrame("editor_FrameNewConfirm");
	if (frame) {
		frame->removeSelf();
	}
}

void Editor::buttonNew() {
	Frame* gui = client->getGUI();
	if (!gui)
		return;
	if (gui->findFrame("editor_FrameNewConfirm"))
		return;

	Frame* topFrame = new Frame(*gui, "editor_FrameNewConfirm");

	int xres = Frame::virtualScreenX;
	int yres = Frame::virtualScreenY;

	Rect<int> rect;

	rect.x = 0; rect.w = 600;
	rect.y = 0; rect.h = 200;
	topFrame->setActualSize(rect);
	rect.x = xres / 2 - 300; rect.w = 600;
	rect.y = yres / 2 - 100; rect.h = 200;
	topFrame->setSize(rect);
	topFrame->setColor(WideVector(.5f, .5f, .5f, 1.f));

	// close button
	{
		Button* button = new Button(*topFrame);

		Rect<int> buttonRect;
		buttonRect.x = rect.w - 36; buttonRect.w = 30;
		buttonRect.y = 6; buttonRect.h = 30;
		button->setSize(buttonRect);
		button->setName("buttonClose");
		button->setText("x");
	}

	// okay button
	{
		Button* button = new Button(*topFrame);

		Rect<int> buttonRect;
		buttonRect.x = 6; buttonRect.w = 90;
		buttonRect.y = rect.h - 36; buttonRect.h = 30;
		button->setSize(buttonRect);
		button->setName("buttonOkay");
		button->setText("Okay");
	}

	// cancel button
	{
		Button* button = new Button(*topFrame);

		Rect<int> buttonRect;
		buttonRect.x = rect.w - 96; buttonRect.w = 90;
		buttonRect.y = rect.h - 36; buttonRect.h = 30;
		button->setSize(buttonRect);
		button->setName("buttonClose");
		button->setText("Cancel");
	}

	// window title
	{
		Field* field = topFrame->addField("editor_FrameNewConfirmTitle", 32);

		Rect<int> rect;
		rect.x = 12; rect.w = 300;
		rect.y = 12; rect.h = 30;
		field->setSize(rect);
		field->setText("New Map");
	}

	// name field
	{
		Frame* frame = topFrame->addFrame("editor_FrameNewConfirmName");

		Rect<int> rect;
		rect.x = 0; rect.w = 600 - 12;
		rect.y = 0; rect.h = 30;
		frame->setActualSize(rect);
		rect.x = 6; rect.w = 600 - 12;
		rect.y = 39; rect.h = 30;
		frame->setSize(rect);
		frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
		frame->setHigh(false);

		Field* field = frame->addField("field", 128);

		Rect<int> size;
		size.x = 3; size.w = rect.w - 6;
		size.y = 3; size.h = rect.h - 6;
		field->setSize(size);
		field->setText("Untitled Map");
		field->setEditable(true);
	}

	// width field
	{
		Frame* frame = topFrame->addFrame("editor_FrameNewConfirmWidth");

		Rect<int> rect;
		rect.x = 0; rect.w = 100;
		rect.y = 0; rect.h = 30;
		frame->setActualSize(rect);
		rect.x = 6; rect.w = 100;
		rect.y = 72; rect.h = 30;
		frame->setSize(rect);
		frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
		frame->setHigh(false);

		Field* field = frame->addField("field", 16);

		Rect<int> size;
		size.x = 3; size.w = rect.w - 6;
		size.y = 3; size.h = rect.h - 6;
		field->setSize(size);
		field->setText("32");
		field->setEditable(true);
		field->setNumbersOnly(true);
		field->setJustify(Field::RIGHT);

		// label
		{
			Field* field = topFrame->addField("field", 16);

			Rect<int> size;
			size.x = 112; size.w = 200;
			size.y = 78; size.h = 30;
			field->setSize(size);
			field->setText("Width");
		}
	}

	// height field
	{
		Frame* frame = topFrame->addFrame("editor_FrameNewConfirmHeight");

		Rect<int> rect;
		rect.x = 0; rect.w = 100;
		rect.y = 0; rect.h = 30;
		frame->setActualSize(rect);
		rect.x = 6; rect.w = 100;
		rect.y = 108; rect.h = 30;
		frame->setSize(rect);
		frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
		frame->setHigh(false);

		Field* field = frame->addField("field", 16);

		Rect<int> size;
		size.x = 3; size.w = rect.w - 6;
		size.y = 3; size.h = rect.h - 6;
		field->setSize(size);
		field->setText("32");
		field->setEditable(true);
		field->setNumbersOnly(true);
		field->setJustify(Field::RIGHT);

		// label
		{
			Field* field = topFrame->addField("field", 16);

			Rect<int> size;
			size.x = 112; size.w = 200;
			size.y = 114; size.h = 30;
			field->setSize(size);
			field->setText("Height");
		}
	}

	// tiles enabled flag
	{
		Button* button = topFrame->addButton("buttonTilesEnabled");
		button->setBorder(1);
		button->setIcon("images/gui/checkmark.png");
		button->setStyle(Button::STYLE_CHECKBOX);
		button->setPressed(true);
		button->setTooltip("If checked, the world will be tile-based.");

		Rect<int> size;
		size.x = 200; size.w = 30;
		size.y = 72; size.h = 30;
		button->setSize(size);

		// label
		{
			Field* label = topFrame->addField("labelTilesEnabled", 32);

			Rect<int> size;
			size.x = 200 + 30 + 3;
			size.w = 200;
			size.y = 72 + 3;
			size.h = 30;
			label->setSize(size);
			label->setText("Tile-based world");
		}
	}
}

void Editor::buttonEditorSettings() {
	Frame* gui = client->getGUI();
	if (!gui)
		return;
	if (gui->findFrame("editor_FrameEditorSettings"))
		return;

	Frame* topFrame = new Frame(*gui, "editor_FrameEditorSettings");

	int xres = Frame::virtualScreenX;
	int yres = Frame::virtualScreenY;

	Rect<int> winrect;

	int border = 3;
	winrect.x = 0; winrect.w = 500;
	winrect.y = 0; winrect.h = 300;
	topFrame->setActualSize(winrect);
	winrect.x = xres / 2 - 250; winrect.w = 500;
	winrect.y = yres / 2 - 200; winrect.h = 300;
	topFrame->setSize(winrect);
	topFrame->setColor(WideVector(.5f, .5f, .5f, 1.f));
	topFrame->setBorder(border);

	// close button
	{
		Button* button = new Button(*topFrame);

		Rect<int> buttonRect;
		buttonRect.x = winrect.w - 36; buttonRect.w = 30;
		buttonRect.y = 6; buttonRect.h = 30;
		button->setSize(buttonRect);
		button->setName("buttonClose");
		button->setText("x");
	}

	// okay button
	{
		Button* button = new Button(*topFrame);

		Rect<int> buttonRect;
		buttonRect.x = 6; buttonRect.w = 90;
		buttonRect.y = winrect.h - 36; buttonRect.h = 30;
		button->setSize(buttonRect);
		button->setName("buttonOkay");
		button->setText("Okay");
	}

	// cancel button
	{
		Button* button = new Button(*topFrame);

		Rect<int> buttonRect;
		buttonRect.x = winrect.w - 96; buttonRect.w = 90;
		buttonRect.y = winrect.h - 36; buttonRect.h = 30;
		button->setSize(buttonRect);
		button->setName("buttonClose");
		button->setText("Cancel");
	}

	// window title
	{
		Field* field = topFrame->addField("editor_FrameEditorSettingsTitle", 32);

		Rect<int> rect;
		rect.x = 12; rect.w = 300;
		rect.y = 12; rect.h = 30;
		field->setSize(rect);
		field->setText("Editor Settings");
	}

	int y = 36;

	// snap enabled flag
	{
		Button* button = topFrame->addButton("buttonSnapEnabled");
		button->setBorder(1);
		button->setIcon("images/gui/checkmark.png");
		button->setStyle(Button::STYLE_CHECKBOX);
		button->setPressed(cvar_snapEnabled.toInt() != 0 ? true : false);
		button->setTooltip("Toggles grid snapping on and off.");

		Rect<int> size;
		size.x = border * 2; size.w = 30;
		size.y = y; size.h = 30;
		button->setSize(size);

		// label
		{
			Field* label = topFrame->addField("labelSnapEnabled", 32);

			Rect<int> size;
			size.x = border * 2 + 30 + border;
			size.w = winrect.w - border * 4 - 30 - border;
			size.y = y + 5;
			size.h = 30;
			label->setSize(size);
			label->setText("Enable snap-to-grid");
		}

		y += size.h + border;
	}

	// translate snap field
	{
		Frame* frame = topFrame->addFrame("editor_FrameEditorSettingsSnapTranslate");

		Rect<int> rect;
		rect.x = 0; rect.w = 150;
		rect.y = 0; rect.h = 30;
		frame->setActualSize(rect);
		rect.x = border * 2; rect.w = 150;
		rect.y = y; rect.h = 30;
		frame->setSize(rect);
		frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
		frame->setHigh(false);

		Field* field = frame->addField("field", 128);

		StringBuf<32> text("%.2f", 1, cvar_snapTranslate.toFloat());
		field->setText(text.get());

		// label
		{
			Field* label = topFrame->addField("labelSnapTranslate", 32);

			Rect<int> size;
			size.x = border * 2 + 150 + border;
			size.w = winrect.w - border * 4 - 150 - border;
			size.y = y + 5;
			size.h = 30;
			label->setSize(size);
			label->setText("Translate snap");
		}

		Rect<int> size;
		size.x = 3; size.w = rect.w - 6;
		size.y = 3; size.h = rect.h - 6;
		field->setSize(size);
		field->setEditable(true);
		field->setWidgetSearchParent("editor_FrameEditorSettingsSnapRotate");
		field->setWidgetTab("field");

		y += rect.h + border;
	}

	// rotate snap field
	{
		Frame* frame = topFrame->addFrame("editor_FrameEditorSettingsSnapRotate");

		Rect<int> rect;
		rect.x = 0; rect.w = 150;
		rect.y = 0; rect.h = 30;
		frame->setActualSize(rect);
		rect.x = border * 2; rect.w = 150;
		rect.y = y; rect.h = 30;
		frame->setSize(rect);
		frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
		frame->setHigh(false);

		Field* field = frame->addField("field", 128);

		StringBuf<32> text("%.2f", 1, cvar_snapRotate.toFloat());
		field->setText(text.get());

		// label
		{
			Field* label = topFrame->addField("labelSnapRotate", 32);

			Rect<int> size;
			size.x = border * 2 + 150 + border;
			size.w = winrect.w - border * 4 - 150 - border;
			size.y = y + 5;
			size.h = 30;
			label->setSize(size);
			label->setText("Rotate snap");
		}

		Rect<int> size;
		size.x = 3; size.w = rect.w - 6;
		size.y = 3; size.h = rect.h - 6;
		field->setSize(size);
		field->setEditable(true);
		field->setWidgetSearchParent("editor_FrameEditorSettingsSnapScale");
		field->setWidgetTab("field");

		y += rect.h + border;
	}

	// scale snap field
	{
		Frame* frame = topFrame->addFrame("editor_FrameEditorSettingsSnapScale");

		Rect<int> rect;
		rect.x = 0; rect.w = 150;
		rect.y = 0; rect.h = 30;
		frame->setActualSize(rect);
		rect.x = border * 2; rect.w = 150;
		rect.y = y; rect.h = 30;
		frame->setSize(rect);
		frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
		frame->setHigh(false);

		Field* field = frame->addField("field", 128);

		StringBuf<32> text("%.2f", 1, cvar_snapScale.toFloat());
		field->setText(text.get());

		// label
		{
			Field* label = topFrame->addField("labelSnapScale", 32);

			Rect<int> size;
			size.x = border * 2 + 150 + border;
			size.w = winrect.w - border * 4 - 150 - border;
			size.y = y + 5;
			size.h = 30;
			label->setSize(size);
			label->setText("Scale snap");
		}

		Rect<int> size;
		size.x = 3; size.w = rect.w - 6;
		size.y = 3; size.h = rect.h - 6;
		field->setSize(size);
		field->setEditable(true);
		field->setWidgetSearchParent("editor_FrameEditorSettingsSnapTranslate");
		field->setWidgetTab("field");

		y += rect.h + border;
	}
}

void Editor::buttonEditorSettingsApply() {
	Frame* gui = client->getGUI();
	assert(gui);

	// collect snap enabled
	{
		Frame* frame = gui->findFrame("editor_FrameEditorSettings");
		assert(frame);

		Button* button = frame->findButton("buttonSnapEnabled");
		assert(button);

		cvar_snapEnabled.set(button->isPressed() ? "1" : "0");
	}

	// collect translate snap
	{
		Frame* frame = gui->findFrame("editor_FrameEditorSettingsSnapTranslate");
		assert(frame);

		Field* field = frame->findField("field");
		assert(field);

		cvar_snapTranslate.set(field->getText());
	}

	// collect rotate snap
	{
		Frame* frame = gui->findFrame("editor_FrameEditorSettingsSnapRotate");
		assert(frame);

		Field* field = frame->findField("field");
		assert(field);

		cvar_snapRotate.set(field->getText());
	}

	// collect scale snap
	{
		Frame* frame = gui->findFrame("editor_FrameEditorSettingsSnapScale");
		assert(frame);

		Field* field = frame->findField("field");
		assert(field);

		cvar_snapScale.set(field->getText());
	}
}

void Editor::buttonMapSettings() {
	Frame* gui = client->getGUI();
	if (!gui)
		return;
	if (gui->findFrame("editor_FrameMapSettings"))
		return;

	Frame* topFrame = new Frame(*gui, "editor_FrameMapSettings");

	int xres = Frame::virtualScreenX;
	int yres = Frame::virtualScreenY;

	Rect<int> rect;

	rect.x = 0; rect.w = 500;
	rect.y = 0; rect.h = 400;
	topFrame->setActualSize(rect);
	rect.x = xres / 2 - 250; rect.w = 500;
	rect.y = yres / 2 - 200; rect.h = 400;
	topFrame->setSize(rect);
	topFrame->setColor(WideVector(.5f, .5f, .5f, 1.f));

	// close button
	{
		Button* button = new Button(*topFrame);

		Rect<int> buttonRect;
		buttonRect.x = rect.w - 36; buttonRect.w = 30;
		buttonRect.y = 6; buttonRect.h = 30;
		button->setSize(buttonRect);
		button->setName("buttonClose");
		button->setText("x");
	}

	// okay button
	{
		Button* button = new Button(*topFrame);

		Rect<int> buttonRect;
		buttonRect.x = 6; buttonRect.w = 90;
		buttonRect.y = rect.h - 36; buttonRect.h = 30;
		button->setSize(buttonRect);
		button->setName("buttonOkay");
		button->setText("Okay");
	}

	// cancel button
	{
		Button* button = new Button(*topFrame);

		Rect<int> buttonRect;
		buttonRect.x = rect.w - 96; buttonRect.w = 90;
		buttonRect.y = rect.h - 36; buttonRect.h = 30;
		button->setSize(buttonRect);
		button->setName("buttonClose");
		button->setText("Cancel");
	}

	// window title
	{
		Field* field = topFrame->addField("editor_FrameMapSettingsTitle", 32);

		Rect<int> rect;
		rect.x = 12; rect.w = 300;
		rect.y = 12; rect.h = 30;
		field->setSize(rect);
		field->setText("Map Settings");
	}

	// name field
	{
		Frame* frame = topFrame->addFrame("editor_FrameMapSettingsName");

		Rect<int> rect;
		rect.x = 0; rect.w = 500 - 12;
		rect.y = 0; rect.h = 30;
		frame->setActualSize(rect);
		rect.x = 6; rect.w = 500 - 12;
		rect.y = 39; rect.h = 30;
		frame->setSize(rect);
		frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
		frame->setHigh(false);

		Field* field = frame->addField("field", 128);

		Rect<int> size;
		size.x = 3; size.w = rect.w - 6;
		size.y = 3; size.h = rect.h - 6;
		field->setSize(size);
		field->setText(world->getNameStr().get());
		field->setEditable(true);
		field->setWidgetSearchParent("editor_FrameMapSettingsUp");
		field->setWidgetTab("field");
	}

	// label
	{
		Field* field = topFrame->addField("field", 16);

		Rect<int> size;
		size.x = 6; size.w = 200;
		size.y = 78; size.h = 30;
		field->setSize(size);
		field->setText("Resize:");
	}

	// up field
	{
		Frame* frame = topFrame->addFrame("editor_FrameMapSettingsUp");

		Rect<int> rect;
		rect.x = 0; rect.w = 100;
		rect.y = 0; rect.h = 30;
		frame->setActualSize(rect);
		rect.x = 250 - rect.w / 2; rect.w = 100;
		rect.y = 125; rect.h = 30;
		frame->setSize(rect);
		frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
		frame->setHigh(false);

		Field* field = frame->addField("field", 16);

		Rect<int> size;
		size.x = 3; size.w = rect.w - 6;
		size.y = 3; size.h = rect.h - 6;
		field->setSize(size);
		field->setText("0");
		field->setEditable(true);
		field->setNumbersOnly(true);
		field->setJustify(Field::RIGHT);
		field->setWidgetSearchParent("editor_FrameMapSettingsRight");
		field->setWidgetTab("field");

		// label
		{
			Field* field = topFrame->addField("field", 16);

			Rect<int> size;
			size.x = 250 - 100; size.w = 200;
			size.y = 100; size.h = 30;
			field->setSize(size);
			field->setText("North");
			field->setJustify(Field::CENTER);
		}
	}

	// left field
	{
		Frame* frame = topFrame->addFrame("editor_FrameMapSettingsLeft");

		Rect<int> rect;
		rect.x = 0; rect.w = 100;
		rect.y = 0; rect.h = 30;
		frame->setActualSize(rect);
		rect.x = 250 - rect.w / 2 - 106; rect.w = 100;
		rect.y = 150; rect.h = 30;
		frame->setSize(rect);
		frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
		frame->setHigh(false);

		Field* field = frame->addField("field", 16);

		Rect<int> size;
		size.x = 3; size.w = rect.w - 6;
		size.y = 3; size.h = rect.h - 6;
		field->setSize(size);
		field->setText("0");
		field->setEditable(true);
		field->setNumbersOnly(true);
		field->setJustify(Field::RIGHT);
		field->setWidgetSearchParent("editor_FrameMapSettingsUp");
		field->setWidgetTab("field");

		// label
		{
			Field* field = topFrame->addField("field", 16);

			Rect<int> size;
			size.x = 6; size.w = rect.x - 12;
			size.y = 146; size.h = 30;
			field->setSize(size);
			field->setText("West");
			field->setJustify(Field::RIGHT);
		}
	}

	// right field
	{
		Frame* frame = topFrame->addFrame("editor_FrameMapSettingsRight");

		Rect<int> rect;
		rect.x = 0; rect.w = 100;
		rect.y = 0; rect.h = 30;
		frame->setActualSize(rect);
		rect.x = 250 - rect.w / 2 + 106; rect.w = 100;
		rect.y = 150; rect.h = 30;
		frame->setSize(rect);
		frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
		frame->setHigh(false);

		Field* field = frame->addField("field", 16);

		Rect<int> size;
		size.x = 3; size.w = rect.w - 6;
		size.y = 3; size.h = rect.h - 6;
		field->setSize(size);
		field->setText("0");
		field->setEditable(true);
		field->setNumbersOnly(true);
		field->setJustify(Field::RIGHT);
		field->setWidgetSearchParent("editor_FrameMapSettingsDown");
		field->setWidgetTab("field");

		// label
		{
			Field* field = topFrame->addField("field", 16);

			Rect<int> size;
			size.x = 250 - rect.w / 2 + 212; size.w = rect.x - 12;
			size.y = 156; size.h = 30;
			field->setSize(size);
			field->setText("East");
			field->setJustify(Field::LEFT);
		}
	}

	// down field
	{
		Frame* frame = topFrame->addFrame("editor_FrameMapSettingsDown");

		Rect<int> rect;
		rect.x = 0; rect.w = 100;
		rect.y = 0; rect.h = 30;
		frame->setActualSize(rect);
		rect.x = 250 - rect.w / 2; rect.w = 100;
		rect.y = 175; rect.h = 30;
		frame->setSize(rect);
		frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
		frame->setHigh(false);

		Field* field = frame->addField("field", 16);

		Rect<int> size;
		size.x = 3; size.w = rect.w - 6;
		size.y = 3; size.h = rect.h - 6;
		field->setSize(size);
		field->setText("0");
		field->setEditable(true);
		field->setNumbersOnly(true);
		field->setJustify(Field::RIGHT);
		field->setWidgetSearchParent("editor_FrameMapSettingsLeft");
		field->setWidgetTab("field");

		// label
		{
			Field* field = topFrame->addField("field", 16);

			Rect<int> size;
			size.x = 250 - 100; size.w = 200;
			size.y = 210; size.h = 30;
			field->setSize(size);
			field->setText("South");
			field->setJustify(Field::CENTER);
		}
	}

	// rotate buttons
	for (Sint32 rotate = 90; rotate < 360; rotate += 90) {
		Button* button = new Button(*topFrame);

		Rect<int> buttonRect;
		buttonRect.x = 6; buttonRect.w = 200;
		buttonRect.y = rect.h - 36 - (36 * (4 - rotate / 90)); buttonRect.h = 30;
		button->setSize(buttonRect);
		button->setName("buttonRotate");
		button->getParams().addInt(rotate);

		StringBuf<16> text("Rotate %d*", 1, rotate);
		button->setText(text.get());
	}
}

void Editor::buttonMapSettingsApply() {
	Frame* gui = client->getGUI();
	assert(gui);

	// collect name string
	const char* name = nullptr;
	{
		Frame* frame = gui->findFrame("editor_FrameMapSettingsName");
		assert(frame);

		Field* field = frame->findField("field");
		assert(field);

		name = field->getText();
	}

	// collect left
	int left = 0;
	{
		Frame* frame = gui->findFrame("editor_FrameMapSettingsLeft");
		assert(frame);

		Field* field = frame->findField("field");
		assert(field);

		left = strtol(field->getText(), nullptr, 10);
	}

	// collect right
	int right = 0;
	{
		Frame* frame = gui->findFrame("editor_FrameMapSettingsRight");
		assert(frame);

		Field* field = frame->findField("field");
		assert(field);

		right = strtol(field->getText(), nullptr, 10);
	}

	// collect up
	int up = 0;
	{
		Frame* frame = gui->findFrame("editor_FrameMapSettingsUp");
		assert(frame);

		Field* field = frame->findField("field");
		assert(field);

		up = strtol(field->getText(), nullptr, 10);
	}

	// collect down
	int down = 0;
	{
		Frame* frame = gui->findFrame("editor_FrameMapSettingsDown");
		assert(frame);

		Field* field = frame->findField("field");
		assert(field);

		down = strtol(field->getText(), nullptr, 10);
	}

	// apply settings to map
	world->setNameStr(name);
}

void Editor::buttonSave() {
	StringBuf<64> extensions;
	for (int c = 0; c < static_cast<int>(World::FILE_WLD); ++c) {
		if (c != 0) {
			extensions.append(",");
		}
		extensions.append(World::fileExtensions[c]);
	}
	String result = mainEngine->fileSaveDialog(extensions.get(), nullptr);
	if (!result.empty()) {
		playSound("editor/install.wav");
		world->changeFilename(result.get());
		world->saveFile();
	}
}

void Editor::buttonLoad() {
	StringBuf<64> extensions;
	for (int c = 0; c < static_cast<int>(World::FILE_MAX); ++c) {
		if (c != 0) {
			extensions.append(",");
		}
		extensions.append(World::fileExtensions[c]);
	}
	String result = mainEngine->fileOpenDialog(extensions.get(), nullptr);
	if (!result.empty()) {
		// create new empty world
		client->closeAllWorlds();
		world = client->loadWorld(result.get(), false);
		initWidgets();

		// add existing entities to level navigator
		for (auto pair : world->getEntities()) {
			Entity* entity = pair.b;
			entity->addToEditorList();
		}
	}
}

void Editor::buttonHelp() {
	Frame* gui = client->getGUI();
	if (!gui)
		return;
	if (gui->findFrame("editor_FrameHelp"))
		return;

	playSound("editor/logon.wav");
	Frame* topFrame = new Frame(*gui, "editor_FrameHelp");

	int xres = Frame::virtualScreenX;
	int yres = Frame::virtualScreenY;

	Rect<int> rect;

	rect.x = 0; rect.w = 800;
	rect.y = 0; rect.h = 600;
	topFrame->setActualSize(rect);
	rect.x = xres / 2 - 400; rect.w = 800;
	rect.y = yres / 2 - 300; rect.h = 600;
	topFrame->setSize(rect);
	topFrame->setColor(WideVector(.5f, .5f, .5f, 1.f));

	// close button
	{
		Button* button = new Button(*topFrame);

		Rect<int> buttonRect;
		buttonRect.x = rect.w - 36; buttonRect.w = 30;
		buttonRect.y = 6; buttonRect.h = 30;
		button->setSize(buttonRect);
		button->setName("buttonClose");
		button->setText("x");
	}

	// window title
	{
		Field* field = topFrame->addField("editor_FrameHelpTitle", 32);

		Rect<int> rect;
		rect.x = 12; rect.w = 300;
		rect.y = 12; rect.h = 30;
		field->setSize(rect);
		field->setText("Help");
	}

	// text area
	{
		Frame* frame = topFrame->addFrame("editor_FrameHelpText");

		rect.x = 6; rect.w = 800 - 6 - 6;
		rect.y = 39; rect.h = 600 - 39 - 6;
		frame->setSize(rect);
		rect.x = 0; rect.w = 900;
		rect.y = 0; rect.h = 900;
		frame->setActualSize(rect);
		frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
		frame->setHigh(false);

		struct EditorHelp {
			ArrayList<String> help;
			String compose() {
				Uint32 size = 1;
				for (auto& str : help) {
					size += str.getSize();
				}
				String result;
				result.alloc(size);
				for (auto& str : help) {
					result.append(str.get());
				}
				return result;
			}
			void serialize(FileInterface* file) {
				int version = 0;
				file->property("EditorHelp::version", version);
				file->property("help", help);
			}
		} editorHelp;

		String editorHelpFilename = mainEngine->buildPath("assets/editor/editorHelp.json");
		FileHelper::readObject(editorHelpFilename.get(), editorHelp);
		String help = editorHelp.compose();

		Field* field = frame->addField("field", help.getSize() + 1);
		assert(field);

		Rect<int> size;
		size.x = 3; size.w = rect.w - 6;
		size.y = 3; size.h = rect.h - 6;
		field->setSize(size);
		field->setScroll(false);
		field->setText(help.get());
	}
}

void Editor::buttonWorldRotate(int rotate) {
	// deprecated
}

void Editor::playSound(const char* path) {
	if (!path || path[0] == '\0')
		return;

	if (cvar_editorSounds.toInt()) {
		mainEngine->playSound(path);
	}
}

void Editor::toggleSelectEntity(const Uint32 uid) {
	Entity* entity = world->uidToEntity(uid);
	if (entity) {
		highlightedObjManuallySet = true;
		entity->setHighlighted(entity->isSelected() == false);
		entity->setSelected(entity->isSelected() == false);

		if (entity->isSelected()) {
			playSound("editor/rollover.wav");
		}
	}
}

void Editor::selectEntity(const Uint32 uid, const bool selected) {
	Entity* entity = world->uidToEntity(uid);
	if (entity) {
		highlightedObjManuallySet = true;
		entity->setSelected(selected);
		entity->setHighlighted(selected);
		playSound("editor/rollover.wav");
	}
}

void Editor::selectAllEntities(const bool selected) {
	highlightedObjManuallySet = true;
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;
		entity->setSelected(selected);
		entity->setHighlighted(selected);
	}

	if (selected) {
		playSound("editor/rollover.wav");
	}
}

void Editor::initWidgets() {
	const Sint32 xres = Frame::virtualScreenX;
	const Sint32 yres = Frame::virtualScreenY;

	Entity* entity = nullptr;

	// add camera
	entity = new Entity(world);
	entity->setShouldSave(false);
	entity->setFlags(static_cast<int>(Entity::flag_t::FLAG_PASSABLE) | static_cast<int>(Entity::flag_t::FLAG_VISIBLE));
	editingCamera = entity->addComponent<Camera>();
	editingCamera->setClipFar(99999.f);
	client->getMixer()->setListener(editingCamera);

	// set camera window
	Rect<int> camRect;
	camRect.x = xres / 5;
	camRect.w = xres - xres / 2.5f;
	camRect.y = 44;
	camRect.h = yres - 44 - 200;
	editingCamera->setWin(camRect);

	// set camera orientation
	Vector pos(0.f, 0.f, -64.f);
	entity->setPos(pos);
	Rotation ang;
	ang.yaw = PI / 4.f;
	ang.pitch = PI / 6.f;
	ang.roll = 0.f;
	entity->setLookDir(ang);

	// minimap camera
	entity = new Entity(world);
	entity->setShouldSave(false);
	entity->setFlags(static_cast<int>(Entity::flag_t::FLAG_PASSABLE) | static_cast<int>(Entity::flag_t::FLAG_VISIBLE));
	minimap = entity->addComponent<Camera>();
	Rect<int> mapRect;
	mapRect.x = 9; mapRect.w = camRect.x - 18;
	mapRect.y = camRect.y + 9; mapRect.h = mapRect.w;
	minimap->setWin(mapRect);
	minimap->setOrtho(true);
	minimap->setClipFar(999999.f);
	minimap->setFov(World::tileSize * 8);
	minimap->setLocalAng(minimapRot[0]);

	// clear widget variables
	widgetMode = TRANSLATE;
	widgetVisible = false;
	widget = nullptr;
	widgetX = nullptr;
	widgetXY = nullptr;
	widgetY = nullptr;
	widgetYZ = nullptr;
	widgetZ = nullptr;
	widgetZX = nullptr;
	widgetActors.removeAll();

	// main widget (used for scaling only)
	{
		widget = new Entity(world);
		widget->setName("widget");
		widget->setShouldSave(false);
		widget->setFlags(static_cast<int>(Entity::flag_t::FLAG_DEPTHFAIL) | static_cast<int>(Entity::flag_t::FLAG_PASSABLE) | static_cast<int>(Entity::flag_t::FLAG_FULLYLIT));
		widget->update();

		Mesh::shadervars_t shaderVars;
		shaderVars.customColorEnabled = true;
		shaderVars.customColorA = { 1.f,1.f,1.f,1.f };

		Model* model = widget->addComponent<Model>();
		model->setName("model");
		model->setMesh("assets/editor/gizmo/gizmo_scale_3-axis.FBX");
		model->setMaterial("assets/editor/gizmo/material.json");
		model->setDepthFailMat("assets/editor/gizmo/material_depth.json");
		model->setShaderVars(shaderVars);
		model->rotate(Rotation(PI / 2.f, 0.f, 0.f));

		BBox* bbox = model->addComponent<BBox>();
		bbox->setName("bbox");
		bbox->setShape(BBox::SHAPE_MESH);

		widgetActors.addNodeLast(widget);
	}

	// x widget
	{
		widgetX = new Entity(world);
		widgetX->setName("widgetX");
		widgetX->setShouldSave(false);
		widgetX->setFlags(static_cast<int>(Entity::flag_t::FLAG_DEPTHFAIL) | static_cast<int>(Entity::flag_t::FLAG_PASSABLE) | static_cast<int>(Entity::flag_t::FLAG_FULLYLIT));
		widgetX->update();

		Mesh::shadervars_t shaderVars;
		shaderVars.customColorEnabled = true;
		shaderVars.customColorA = { 1.f,0.f,0.f,1.f };

		Model* model = widgetX->addComponent<Model>();
		model->setName("model");
		model->setMesh("assets/editor/gizmo/gizmo_translate_1-axis.FBX");
		model->setMaterial("assets/editor/gizmo/material.json");
		model->setDepthFailMat("assets/editor/gizmo/material_depth.json");
		model->setShaderVars(shaderVars);
		model->rotate(Rotation(0.f, PI / 2.f, 0.f));

		BBox* bbox = model->addComponent<BBox>();
		bbox->setName("bbox");
		bbox->setShape(BBox::SHAPE_MESH);

		widgetActors.addNodeLast(widgetX);
	}

	// x/y widget
	{
		widgetXY = new Entity(world);
		widgetXY->setName("widgetXY");
		widgetXY->setShouldSave(false);
		widgetXY->setFlags(static_cast<int>(Entity::flag_t::FLAG_DEPTHFAIL) | static_cast<int>(Entity::flag_t::FLAG_PASSABLE) | static_cast<int>(Entity::flag_t::FLAG_FULLYLIT));
		widgetXY->update();

		Mesh::shadervars_t shaderVars;
		shaderVars.customColorEnabled = true;
		shaderVars.customColorA = { 1.f,1.f,0.f,1.f };

		Model* model = widgetXY->addComponent<Model>();
		model->setName("model");
		model->setMesh("assets/editor/gizmo/gizmo_translate_2-axis.FBX");
		model->setMaterial("assets/editor/gizmo/material.json");
		model->setDepthFailMat("assets/editor/gizmo/material_depth.json");
		model->setShaderVars(shaderVars);
		model->rotate(Rotation(0.f, 3 * PI / 2.f, 0.f));
		model->rotate(Rotation(0.f, 0.f, PI));

		BBox* bbox = model->addComponent<BBox>();
		bbox->setName("bbox");
		bbox->setShape(BBox::SHAPE_MESH);

		widgetActors.addNodeLast(widgetXY);
	}

	// y widget
	{
		widgetY = new Entity(world);
		widgetY->setName("widgetY");
		widgetY->setShouldSave(false);
		widgetY->setFlags(static_cast<int>(Entity::flag_t::FLAG_DEPTHFAIL) | static_cast<int>(Entity::flag_t::FLAG_PASSABLE) | static_cast<int>(Entity::flag_t::FLAG_FULLYLIT));
		widgetY->update();

		Mesh::shadervars_t shaderVars;
		shaderVars.customColorEnabled = true;
		shaderVars.customColorA = { 0.f,1.f,0.f,1.f };

		Model* model = widgetY->addComponent<Model>();
		model->setName("model");
		model->setMesh("assets/editor/gizmo/gizmo_translate_1-axis.FBX");
		model->setMaterial("assets/editor/gizmo/material.json");
		model->setDepthFailMat("assets/editor/gizmo/material_depth.json");
		model->setShaderVars(shaderVars);
		model->rotate(Rotation(PI / 2.f, PI / 2.f, 0.f));
		model->rotate(Rotation(PI / 2.f, 0.f, 0.f));
		//model->rotate(Rotation(0.f, 0.f, PI/2.f));

		auto q = model->getLocalAng();
		auto r = q.toRotation();

		BBox* bbox = model->addComponent<BBox>();
		bbox->setName("bbox");
		bbox->setShape(BBox::SHAPE_MESH);

		widgetActors.addNodeLast(widgetY);
	}

	// y/z widget
	{
		widgetYZ = new Entity(world);
		widgetYZ->setName("widgetYZ");
		widgetYZ->setShouldSave(false);
		widgetYZ->setFlags(static_cast<int>(Entity::flag_t::FLAG_DEPTHFAIL) | static_cast<int>(Entity::flag_t::FLAG_PASSABLE) | static_cast<int>(Entity::flag_t::FLAG_FULLYLIT));
		widgetYZ->update();

		Mesh::shadervars_t shaderVars;
		shaderVars.customColorEnabled = true;
		shaderVars.customColorA = { 0.f,1.f,1.f,1.f };

		Model* model = widgetYZ->addComponent<Model>();
		model->setName("model");
		model->setMesh("assets/editor/gizmo/gizmo_translate_2-axis.FBX");
		model->setMaterial("assets/editor/gizmo/material.json");
		model->setDepthFailMat("assets/editor/gizmo/material_depth.json");
		model->setShaderVars(shaderVars);
		model->rotate(Rotation(0.f, 0.f, 3 * PI / 2.f));

		BBox* bbox = model->addComponent<BBox>();
		bbox->setName("bbox");
		bbox->setShape(BBox::SHAPE_MESH);

		widgetActors.addNodeLast(widgetYZ);
	}

	// z widget
	{
		widgetZ = new Entity(world);
		widgetZ->setName("widgetZ");
		widgetZ->setShouldSave(false);
		widgetZ->setFlags(static_cast<int>(Entity::flag_t::FLAG_DEPTHFAIL) | static_cast<int>(Entity::flag_t::FLAG_PASSABLE) | static_cast<int>(Entity::flag_t::FLAG_FULLYLIT));
		widgetZ->update();

		Mesh::shadervars_t shaderVars;
		shaderVars.customColorEnabled = true;
		shaderVars.customColorA = { 0.f,0.f,1.f,1.f };

		Model* model = widgetZ->addComponent<Model>();
		model->setName("model");
		model->setMesh("assets/editor/gizmo/gizmo_translate_1-axis.FBX");
		model->setMaterial("assets/editor/gizmo/material.json");
		model->setDepthFailMat("assets/editor/gizmo/material_depth.json");
		model->setShaderVars(shaderVars);
		model->rotate(Rotation(3 * PI / 2.f, 0.f, 0.f));

		BBox* bbox = model->addComponent<BBox>();
		bbox->setName("bbox");
		bbox->setShape(BBox::SHAPE_MESH);

		widgetActors.addNodeLast(widgetZ);
	}

	// z/x widget
	{
		widgetZX = new Entity(world);
		widgetZX->setName("widgetZX");
		widgetZX->setShouldSave(false);
		widgetZX->setFlags(static_cast<int>(Entity::flag_t::FLAG_DEPTHFAIL) | static_cast<int>(Entity::flag_t::FLAG_PASSABLE) | static_cast<int>(Entity::flag_t::FLAG_FULLYLIT));
		widgetZX->update();

		Mesh::shadervars_t shaderVars;
		shaderVars.customColorEnabled = true;
		shaderVars.customColorA = { 1.f,0.f,1.f,1.f };

		Model* model = widgetZX->addComponent<Model>();
		model->setName("model");
		model->setMesh("assets/editor/gizmo/gizmo_translate_2-axis.FBX");
		model->setMaterial("assets/editor/gizmo/material.json");
		model->setDepthFailMat("assets/editor/gizmo/material_depth.json");
		model->setShaderVars(shaderVars);
		model->rotate(Rotation(PI / 2.f, 0.f, 0.f));

		BBox* bbox = model->addComponent<BBox>();
		bbox->setName("bbox");
		bbox->setShape(BBox::SHAPE_MESH);

		widgetActors.addNodeLast(widgetZX);
	}
}

void Editor::initGUI(const Rect<int>& camRect) {
	// setup gui
	Sint32 xres = Frame::virtualScreenX;
	Sint32 yres = Frame::virtualScreenY;

	Frame* gui = client->getGUI()->findFrame("editor_gui");
	Rect<int> guiRect;
	guiRect.x = 0;
	guiRect.y = 0;
	guiRect.w = xres;
	guiRect.h = yres;
	gui->setSize(guiRect);
	gui->setActualSize(guiRect);
	gui->setHollow(true);
	Rect<int> frameRect;

	// top-most frame (on screen)
	{
		Frame* topframe = new Frame(*gui, "editor_FrameTop");
		frameRect.x = 0; frameRect.w = xres;
		frameRect.y = 0; frameRect.h = camRect.y;
		topframe->setActualSize(frameRect);
		frameRect.x = 0; frameRect.w = xres;
		frameRect.y = 0; frameRect.h = camRect.y;
		topframe->setSize(frameRect);
		topframe->setColor(WideVector(.5f, .5f, .5f, 1.f));

		Button* button = nullptr;

		// new world
		button = topframe->addButton("buttonNew");
		button->setIcon("images/gui/new.png");
		button->setTooltip("New world");
		button->setPos(6, 6);

		// save world
		button = topframe->addButton("buttonSave");
		button->setIcon("images/gui/save.png");
		button->setTooltip("Save world");
		button->setPos(41, 6);

		// load world
		button = topframe->addButton("buttonLoad");
		button->setIcon("images/gui/open.png");
		button->setTooltip("Load world");
		button->setPos(76, 6);

		// ...

		// edit tiles
		/*button = topframe->addButton("buttonTiles");
		button->setIcon("images/gui/geometry.png");
		button->setTooltip("Edit tiles");
		button->setPos(146, 6);

		// edit textures
		/*button = topframe->addButton("buttonTextures");
		button->setIcon("images/gui/texture.png");
		button->setTooltip("Edit textures");
		button->setPos(181, 6);

		// edit entities
		button = topframe->addButton("buttonEntities");
		button->setIcon("images/gui/icon_characters.png");
		button->setTooltip("Edit entities");
		button->setPos(216, 6);

		// edit sectors
		button = topframe->addButton("buttonSectors");
		button->setIcon("images/gui/triangle.png");
		button->setTooltip("Edit sectors");
		button->setPos(251, 6);*/

		// ...

		// preview mode
		button = topframe->addButton("buttonPreview");
		button->setIcon("images/gui/preview.png");
		button->setTooltip("Preview mode");
		button->setPos(146, 6);

		// optimize geometry
		/*button = topframe->addButton("buttonOptimize");
		button->setIcon("images/gui/optimize.png");
		button->setTooltip("Optimize chunk geometry");
		button->setPos(356, 6);*/

		// editor settings
		button = topframe->addButton("buttonEditorSettings");
		button->setIcon("images/gui/wrench.png");
		button->setTooltip("Change editor settings");
		button->setPos(181, 6);

		// map settings
		button = topframe->addButton("buttonMapSettings");
		button->setIcon("images/gui/map.png");
		button->setTooltip("Change map settings");
		button->setPos(216, 6);

		// play test!
		button = topframe->addButton("buttonPlay");
		button->setIcon("images/gui/play.png");
		button->setTooltip("Playtest current level");
		button->setPos(251, 6);

		// ...

		// help window
		button = topframe->addButton("buttonHelp");
		button->setIcon("images/gui/help.png");
		button->setTooltip("Help");
		button->setPos(xres - 32 - 6, 6);
	}

	// left frame
	{
		Frame* topFrame = new Frame(*gui, "editor_FrameLeft");
		frameRect.x = 0; frameRect.w = camRect.x;
		frameRect.y = 0; frameRect.h = (yres - 44);
		topFrame->setActualSize(frameRect);
		frameRect.x = 0; frameRect.w = camRect.x;
		frameRect.y = camRect.y; frameRect.h = (yres - 44);
		topFrame->setSize(frameRect);
		topFrame->setColor(WideVector(.5f, .5f, .5f, 1.f));
		topFrame->setHollow(true);

		// tile controls
		static const bool tileControls = false;
		static const bool squashLeft = true;
		if (tileControls)
		{
			Frame* midFrame = new Frame(*topFrame, "editor_FrameTileControls");
			frameRect.x = 0; frameRect.w = camRect.x - 6;
			frameRect.y = 0; frameRect.h = camRect.x - 6 - 6;
			midFrame->setActualSize(frameRect);
			frameRect.x = 3; frameRect.w = camRect.x - 6;
			frameRect.y = 3; frameRect.h = camRect.x - 6 - 6;
			midFrame->setSize(frameRect);
			midFrame->setColor(WideVector(.5f, .5f, .5f, 1.f));
			midFrame->setBorder(0);

			// labels
			{
				int y = 33;

				// height
				{
					Field* field = midFrame->addField("", 16);
					field->setJustify(Field::CENTER);
					field->setText("Height");

					Rect<int> size;
					size.x = 0;
					size.w = frameRect.w;
					size.y = y;
					size.h = 30;
					field->setSize(size);

					y += 33;
				}

				// slope dir
				{
					Field* field = midFrame->addField("", 16);
					field->setJustify(Field::CENTER);
					field->setText("SlopeDir");

					Rect<int> size;
					size.x = 0;
					size.w = frameRect.w;
					size.y = y;
					size.h = 30;
					field->setSize(size);

					y += 33;
				}

				// slope size
				{
					Field* field = midFrame->addField("", 16);
					field->setJustify(Field::CENTER);
					field->setText("SlopeSize");

					Rect<int> size;
					size.x = 0;
					size.w = frameRect.w;
					size.y = y;
					size.h = 30;
					field->setSize(size);

					y += 33;
				}
			}

			int y = 33;

			// actual fields
			for (int c = 0; c < 2; ++c) {
				const char* label = (c == 0) ? "Floor" : "Ceiling";

				// main label
				{
					Field* field = midFrame->addField("", 16);
					if (c == 0) {
						field->setJustify(Field::LEFT);
					} else {
						field->setJustify(Field::RIGHT);
					}
					field->setText(label);

					Rect<int> size;
					size.x = (c == 0) ? 25 : frameRect.w / 2;
					size.w = frameRect.w / 2 - 25 * (c == 1);
					size.y = 6;
					size.h = 22;
					field->setSize(size);
				}

				y = 33;

				// height
				{
					StringBuf<64> name("editor_Tile%sHeight", 1, label);
					Frame* frame = new Frame(*midFrame, name.get());

					Rect<int> size;
					size.w = frameRect.w / 3;
					size.h = 30;
					size.x = 0;
					size.y = 0;
					frame->setActualSize(size);
					size.w = frameRect.w / 3;
					size.h = 30;
					size.x = (c == 0) ? 6 : frameRect.w - size.w - 6;
					size.y = y;
					frame->setSize(size);
					frame->setColor(WideVector(.25, .25, .25, 1.0));
					frame->setHigh(false);

					y += size.h + 3;

					Field* field = frame->addField("field", 9);
					size.x = 3; size.w = size.w - 6;
					size.y = 3; size.h = size.h - 6;
					field->setSize(size);
					field->setEditable(true);
					field->setNumbersOnly(true);
					field->setJustify(Field::RIGHT);
					if (c == 0) {
						field->setWidgetSearchParent("editor_TileCeilingHeight");
					} else {
						field->setWidgetSearchParent("editor_TileFloorSlopeDir");
					}
					field->setWidgetTab("field");
				}

				// slope dir
				{
					StringBuf<64> name("editor_Tile%sSlopeDir", 1, label);
					Frame* frame = new Frame(*midFrame, name.get());

					Rect<int> size;
					size.w = frameRect.w / 3;
					size.h = 30;
					size.x = 0;
					size.y = 0;
					frame->setActualSize(size);
					size.w = frameRect.w / 3;
					size.h = 30;
					size.x = (c == 0) ? 6 : frameRect.w - size.w - 6;
					size.y = y;
					frame->setSize(size);
					frame->setColor(WideVector(.25, .25, .25, 1.0));
					frame->setHigh(false);

					y += size.h + 3;

					Field* field = frame->addField("field", 9);
					size.x = 3; size.w = size.w - 6;
					size.y = 3; size.h = size.h - 6;
					field->setSize(size);
					field->setEditable(true);
					field->setNumbersOnly(true);
					field->setJustify(Field::RIGHT);
					if (c == 0) {
						field->setWidgetSearchParent("editor_TileCeilingSlopeDir");
					} else {
						field->setWidgetSearchParent("editor_TileFloorSlopeSize");
					}
					field->setWidgetTab("field");
				}

				// slope size
				{
					StringBuf<64> name("editor_Tile%sSlopeSize", 1, label);
					Frame* frame = new Frame(*midFrame, name.get());

					Rect<int> size;
					size.w = frameRect.w / 3;
					size.h = 30;
					size.x = 0;
					size.y = 0;
					frame->setActualSize(size);
					size.w = frameRect.w / 3;
					size.h = 30;
					size.x = (c == 0) ? 6 : frameRect.w - size.w - 6;
					size.y = y;
					frame->setSize(size);
					frame->setColor(WideVector(.25, .25, .25, 1.0));
					frame->setHigh(false);

					y += size.h + 3;

					Field* field = frame->addField("field", 9);
					size.x = 3; size.w = size.w - 6;
					size.y = 3; size.h = size.h - 6;
					field->setSize(size);
					field->setEditable(true);
					field->setNumbersOnly(true);
					field->setJustify(Field::RIGHT);
					if (c == 0) {
						field->setWidgetSearchParent("editor_TileCeilingSlopeSize");
					} else {
						field->setWidgetSearchParent("editor_TileFloorHeight");
					}
					field->setWidgetTab("field");
				}
			}

			for (int channel = 0; channel < 3; ++channel) {
				// label
				{
					StringBuf<32> name;
					WideVector color;
					switch (channel) {
					case 0:
						name = "Red";
						color = WideVector(1.0f, 0.1f, 0.1f, 1.0f);
						break;
					case 1:
						name = "Green";
						color = WideVector(0.1f, 1.0f, 0.1f, 1.0f);
						break;
					case 2:
						name = "Blue";
						color = WideVector(0.1f, 0.1f, 1.0f, 1.0f);
						break;
					}

					Field* label = midFrame->addField(StringBuf<32>("labelTileCustom%s", 1, name.get()).get(), 16);

					Rect<int> size;
					size.x = 6;
					size.w = frameRect.w - 12;
					size.y = y;
					size.h = 20;
					y += size.h + 3;
					label->setSize(size);

					label->setText(StringBuf<32>("Custom %s:", 1, name.get()).get());
					label->setColor(color);
				}

				// custom color
				for (int color = 0; color < 3; ++color) {
					StringBuf<64> name("editor_FrameTileCustomColor");
					switch (channel) {
					case 0:
						name.append("Red");
						break;
					case 1:
						name.append("Green");
						break;
					case 2:
						// pass-through
					default:
						name.append("Blue");
						break;
					}
					switch (color) {
					case 0:
						name.append("0");
						break;
					case 1:
						name.append("1");
						break;
					case 2:
						// pass-through
					default:
						name.append("2");
						break;
					}
					Frame* frame = midFrame->addFrame(name.get());

					Rect<int> size;
					size.x = 0;
					size.w = frameRect.w / 3 - 6;
					size.y = 0;
					size.h = 30;
					frame->setActualSize(size);
					size.x = 6 + (size.w + 3) * color;
					size.w = frameRect.w / 3 - 6;
					size.y = y;
					size.h = 30;
					frame->setSize(size);
					frame->setColor(WideVector(.25, .25, .25, 1.0));
					frame->setHigh(false);

					Field* field = frame->addField("field", 9);
					size.x = 3; size.w = frame->getSize().w - 6;
					size.y = 3; size.h = frame->getSize().h - 6;
					field->setSize(size);
					field->setEditable(true);
					field->setNumbersOnly(true);
					field->setJustify(Field::RIGHT);

					switch (color) {
					case 0:
						field->setColor(WideVector(1.f, .2f, .2f, 1.f));
						break;
					case 1:
						field->setColor(WideVector(.2f, 1.f, .2f, 1.f));
						break;
					default:
						field->setColor(WideVector(.2f, .2f, 1.f, 1.f));
						break;
					}
					//field->getParams().addInt(component->getUID());
					field->getParams().addInt(channel);
					field->getParams().addInt(color);

					char f[16];
					if (color == channel) {
						snprintf(f, 16, "1.0");
					} else {
						snprintf(f, 16, "0.0");
					}
					field->setText(f);
				}

				y += 30 + 3;
			}
		} else {
			// minimap window
			{
				Frame* frame = new Frame(*topFrame, "editor_Minimap");
				frameRect.x = 0; frameRect.w = camRect.x - 12;
				frameRect.y = 0; frameRect.h = camRect.x - 12;
				frame->setActualSize(frameRect);
				frameRect.x = 6; frameRect.w = camRect.x - 12;
				frameRect.y = 6; frameRect.h = camRect.x - 12;
				frame->setSize(frameRect);
				frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
				frame->setHigh(false);
				frame->setHollow(true);
			}

			// border "frame"
			{
				Frame* frame = new Frame(*topFrame);
				frameRect.x = 0; frameRect.w = camRect.x - 12;
				frameRect.y = 0; frameRect.h = 3;
				frame->setActualSize(frameRect);
				frameRect.x = 6; frameRect.w = camRect.x - 12;
				frameRect.y = 3; frameRect.h = 3;
				frame->setSize(frameRect);
				frame->setColor(WideVector(.5f, .5f, .5f, 1.f));
				frame->setBorder(0);
			}

			// border "frame"
			{
				Frame* frame = new Frame(*topFrame);
				frameRect.x = 0; frameRect.w = 3;
				frameRect.y = 0; frameRect.h = camRect.x - 9;
				frame->setActualSize(frameRect);
				frameRect.x = 3; frameRect.w = 3;
				frameRect.y = 3; frameRect.h = camRect.x - 9;
				frame->setSize(frameRect);
				frame->setColor(WideVector(.5f, .5f, .5f, 1.f));
				frame->setBorder(0);
			}

			// border "frame"
			{
				Frame* frame = new Frame(*topFrame);
				frameRect.x = 0; frameRect.w = 3;
				frameRect.y = 0; frameRect.h = camRect.x - 9;
				frame->setActualSize(frameRect);
				frameRect.x = camRect.x - 6; frameRect.w = 3;
				frameRect.y = 3; frameRect.h = camRect.x - 9;
				frame->setSize(frameRect);
				frame->setColor(WideVector(.5f, .5f, .5f, 1.f));
				frame->setBorder(0);
			}

			// border "frame"
			{
				Frame* frame = new Frame(*topFrame);
				frameRect.x = 0; frameRect.w = camRect.x - 6;
				frameRect.y = 0; frameRect.h = (yres - 44) - camRect.x + 3;
				frame->setActualSize(frameRect);
				frameRect.x = 3; frameRect.w = camRect.x - 6;
				frameRect.y = camRect.x - 6; frameRect.h = (yres - 44) - camRect.x + 3;
				frame->setSize(frameRect);
				frame->setColor(WideVector(.5f, .5f, .5f, 1.f));
				frame->setBorder(0);
			}
		}

		// content navigator
		{
			Frame* midFrame = new Frame(*topFrame, "editor_FrameLeftInternal");
			frameRect.x = 0; frameRect.w = camRect.x - 12;
			frameRect.y = 0; frameRect.h = (yres - 44) - 12 - (squashLeft ? (camRect.x - 12) : 0);
			midFrame->setActualSize(frameRect);
			frameRect.x = 6; frameRect.w = camRect.x - 12;
			frameRect.y = 6 + (squashLeft ? (camRect.x - 12) : 0); frameRect.h = (yres - 44) - 12 - (squashLeft ? (camRect.x - 12) : 0);
			midFrame->setSize(frameRect);
			midFrame->setColor(WideVector(.25f, .25f, .25f, 1.f));
			midFrame->setHigh(false);

			// top area of the content navigator window
			{
				Frame* frame = new Frame(*midFrame, "editor_FrameContentNavigatorTop");
				frameRect.x = 0; frameRect.w = std::max(camRect.x - 12 - 6, 3 + 36 * Component::COMPONENT_MAX + 3);
				frameRect.y = 0; frameRect.h = 90;
				frame->setActualSize(frameRect);
				frameRect.x = 3; frameRect.w = camRect.x - 12 - 6;
				frameRect.y = 3; frameRect.h = 90;
				frame->setSize(frameRect);
				frame->setBorder(0);
				frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
				frame->setHigh(false);

				// title text
				Field* field = frame->addField("text1", 17);
				field->setSize(Rect<int>(5, 5, 200, 30));
				field->setText("Content Navigator");

				// sorting buttons
				for (int c = 0; c < Component::type_t::COMPONENT_MAX; ++c) {
					Rect<int> pos;
					int size = 36;
					pos.x = 3 + (size*c); pos.w = size;
					pos.y = frameRect.h - size - 20; pos.h = size;
					Button* button = frame->addButton(Component::typeStr[c]);
					struct Callback : public Script::Function {
						Callback(int _c) : c(_c) {}
						virtual int operator()(Script::Args& args) const override {
							mainEngine->getLocalClient()->getEditor()->updateContentNavigatorFilters();
							return 0;
						}
						int c;
					};
					button->setCallback(new Callback(c));
					button->setStyle(Button::style_t::STYLE_TOGGLE);
					button->setIcon(Component::typeIcon[c]);
					button->setTooltip(Component::typeStr[c]);
					button->setSize(pos);
				}
			}

			// actual content navigator
			{
				Frame* frame = new Frame(*midFrame, "editor_FrameContentNavigatorList");
				frameRect.x = 0; frameRect.w = camRect.x - 12 - 6;
				frameRect.y = 0; frameRect.h = (yres - 44) - 90 - 18 - (squashLeft ? (camRect.x - 12) : 0);
				frame->setActualSize(frameRect);
				frameRect.x = 3; frameRect.w = camRect.x - 12 - 6;
				frameRect.y = 90 + 3; frameRect.h = (yres - 44) - 90 - 18 - (squashLeft ? (camRect.x - 12) : 0);
				frame->setSize(frameRect);
				frame->setBorder(0);
				frame->setColor(WideVector(.1f, .1f, .1f, 1.f));
				frame->setHigh(false);
				updateContentNavigatorFilters();
			}
		}
	}

	// right frame
	static const bool squashRight = false;
	{
		Frame* topFrame = new Frame(*gui, "editor_FrameRight");
		frameRect.x = 0; frameRect.w = camRect.x;
		frameRect.y = 0; frameRect.h = (yres - 44);
		topFrame->setActualSize(frameRect);
		frameRect.x = camRect.x + camRect.w; frameRect.w = camRect.x;
		frameRect.y = camRect.y; frameRect.h = (yres - 44);
		topFrame->setSize(frameRect);
		topFrame->setColor(WideVector(.5f, .5f, .5f, 1.f));

		// level navigator
		{
			Frame* midFrame = new Frame(*topFrame, "editor_FrameLevelNavigator");
			frameRect.x = 0; frameRect.w = camRect.x - 12;
			frameRect.y = 0; frameRect.h = (yres - 44) - (squashRight ? camRect.x : 0) - 12;
			midFrame->setActualSize(frameRect);
			frameRect.x = 6; frameRect.w = camRect.x - 12;
			frameRect.y = (squashRight ? camRect.x - 12 : 0) - 6 + 12; frameRect.h = (yres - 44) - (squashRight ? camRect.x : 0) - 12;
			midFrame->setSize(frameRect);
			midFrame->setColor(WideVector(.25f, .25f, .25f, 1.f));
			midFrame->setHigh(false);

			// top area of the level navigator window
			{
				Frame* frame = new Frame(*midFrame, "editor_FrameLevelNavigatorTop");
				frameRect.x = 0; frameRect.w = std::max(camRect.x - 12 - 6, 3 + 36 * Component::COMPONENT_MAX + 3);
				frameRect.y = 0; frameRect.h = 90;
				frame->setActualSize(frameRect);
				frameRect.x = 3; frameRect.w = camRect.x - 12 - 6;
				frameRect.y = 3; frameRect.h = 90;
				frame->setSize(frameRect);
				frame->setBorder(0);
				frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
				frame->setHigh(false);

				// title text
				Field* field = frame->addField("text1", 15);
				field->setSize(Rect<int>(5, 5, 200, 30));
				field->setText("Level Navigator");

				// sorting buttons
				for (int c = 0; c < Component::type_t::COMPONENT_MAX; ++c) {
					Rect<int> pos;
					int size = 36;
					pos.x = 3 + (size*c); pos.w = size;
					pos.y = frameRect.h - size - 20; pos.h = size;
					Button* button = frame->addButton(Component::typeStr[c]);
					struct Callback : public Script::Function {
						Callback(int _c) : c(_c) {}
						virtual int operator()(Script::Args& args) const override {
							mainEngine->getLocalClient()->getEditor()->updateLevelNavigatorFilters();
							return 0;
						}
						int c;
					};
					button->setCallback(new Callback(c));
					button->setStyle(Button::style_t::STYLE_TOGGLE);
					button->setIcon(Component::typeIcon[c]);
					button->setTooltip(Component::typeStr[c]);
					button->setSize(pos);
				}
			}

			// actual level navigator
			{
				Frame* frame = new Frame(*midFrame, "editor_FrameLevelNavigatorList");
				frameRect.x = 0; frameRect.w = camRect.x - 12 - 6;
				frameRect.y = 0; frameRect.h = (yres - 44) - 90 - 18 - (squashRight ? camRect.x - 12 : 0);
				frame->setActualSize(frameRect);
				frameRect.x = 3; frameRect.w = camRect.x - 12 - 6;
				frameRect.y = 90 + 3; frameRect.h = (yres - 44) - 90 - 18 - (squashRight ? camRect.x - 12: 0);
				frame->setSize(frameRect);
				frame->setBorder(0);
				frame->setColor(WideVector(.1f, .1f, .1f, 1.f));
				frame->setHigh(false);
			}
		}
	}

	// bottom frame
	{
		Frame* midFrame = new Frame(*gui, "editor_FrameBottom");
		frameRect.x = 0; frameRect.w = camRect.w;
		frameRect.y = 0; frameRect.h = camRect.y - 47;
		midFrame->setActualSize(frameRect);
		frameRect.x = camRect.x; frameRect.w = camRect.w;
		frameRect.y = yres - 200; frameRect.h = 200;
		midFrame->setSize(frameRect);
		midFrame->setColor(WideVector(.5f, .5f, .5f, 1.f));

		// mini-console
		{
			Frame* frame = new Frame(*midFrame, "");
			frameRect.x = 0; frameRect.w = camRect.w - 12;
			frameRect.y = 0; frameRect.h = 200 - 12;
			frame->setActualSize(frameRect);
			frameRect.x = 6; frameRect.w = camRect.w - 12;
			frameRect.y = 6; frameRect.h = 200 - 12;
			frame->setSize(frameRect);
			frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
			frame->setHigh(false);

			frame = new Frame(*frame, "editor_FrameBottomMiniConsole");
			frameRect.x = 0; frameRect.w = camRect.w - 12 - 6;
			frameRect.y = 0; frameRect.h = 200 - 12 - 6;
			frame->setActualSize(frameRect);
			frameRect.x = 3; frameRect.w = camRect.w - 12 - 6;
			frameRect.y = 3; frameRect.h = 200 - 12 - 6;
			frame->setSize(frameRect);
			frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
			frame->setBorder(0);
			frame->setHigh(false);
		}
	}

	// x panel
	{
		Frame* topFrame = new Frame(*gui);
		frameRect.x = 0; frameRect.w = 150;
		frameRect.y = 0; frameRect.h = 38;
		topFrame->setActualSize(frameRect);
		frameRect.x = camRect.x + 3; frameRect.w = 150;
		frameRect.y = camRect.y + camRect.h - 38 - 3; frameRect.h = 38;
		topFrame->setSize(frameRect);
		topFrame->setColor(WideVector(.5f, .5f, .5f, 1.f));
		topFrame->addImage(Rect<Sint32>(3, 3, 0, 0), WideVector(1.f), "images/gui/icon_translate-x.png");

		{
			Frame* frame = new Frame(*topFrame, "editor_FramePanelX");
			frameRect.x = 0; frameRect.w = 100;
			frameRect.y = 0; frameRect.h = 32;
			frame->setActualSize(frameRect);
			frameRect.x = 50 - 3; frameRect.w = 100;
			frameRect.y = 3; frameRect.h = 32;
			frame->setSize(frameRect);
			frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
			frame->setHigh(false);

			Field* field = frame->addField("propertyX", 9);
			frameRect.x = 3; frameRect.w = 100 - 6;
			frameRect.y = 3; frameRect.h = 32 - 6;
			field->setSize(frameRect);
			field->setEditable(true);
			field->setNumbersOnly(true);
			field->setJustify(Field::RIGHT);
			field->setWidgetTab("propertyY");
			field->setWidgetSearchParent("editor_FramePanelY");
		}
	}

	// y panel
	{
		Frame* topFrame = new Frame(*gui);
		frameRect.x = 0; frameRect.w = 150;
		frameRect.y = 0; frameRect.h = 38;
		topFrame->setActualSize(frameRect);
		frameRect.x = camRect.x + 3 + 150 + 3; frameRect.w = 150;
		frameRect.y = camRect.y + camRect.h - 38 - 3; frameRect.h = 38;
		topFrame->setSize(frameRect);
		topFrame->setColor(WideVector(.5f, .5f, .5f, 1.f));
		topFrame->addImage(Rect<Sint32>(3, 3, 0, 0), WideVector(1.f), "images/gui/icon_translate-y.png");

		{
			Frame* frame = new Frame(*topFrame, "editor_FramePanelY");
			frameRect.x = 0; frameRect.w = 100;
			frameRect.y = 0; frameRect.h = 32;
			frame->setActualSize(frameRect);
			frameRect.x = 50 - 3; frameRect.w = 100;
			frameRect.y = 3; frameRect.h = 32;
			frame->setSize(frameRect);
			frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
			frame->setHigh(false);

			Field* field = frame->addField("propertyY", 9);
			frameRect.x = 3; frameRect.w = 100 - 6;
			frameRect.y = 3; frameRect.h = 32 - 6;
			field->setSize(frameRect);
			field->setEditable(true);
			field->setNumbersOnly(true);
			field->setJustify(Field::RIGHT);
			field->setWidgetTab("propertyZ");
			field->setWidgetSearchParent("editor_FramePanelZ");
		}
	}

	// z panel
	{
		Frame* topFrame = new Frame(*gui);
		frameRect.x = 0; frameRect.w = 150;
		frameRect.y = 0; frameRect.h = 38;
		topFrame->setActualSize(frameRect);
		frameRect.x = camRect.x + 3 + 150 + 3 + 150 + 3; frameRect.w = 150;
		frameRect.y = camRect.y + camRect.h - 38 - 3; frameRect.h = 38;
		topFrame->setSize(frameRect);
		topFrame->setColor(WideVector(.5f, .5f, .5f, 1.f));
		topFrame->addImage(Rect<Sint32>(3, 3, 0, 0), WideVector(1.f), "images/gui/icon_translate-z.png");

		{
			Frame* frame = new Frame(*topFrame, "editor_FramePanelZ");
			frameRect.x = 0; frameRect.w = 100;
			frameRect.y = 0; frameRect.h = 32;
			frame->setActualSize(frameRect);
			frameRect.x = 50 - 3; frameRect.w = 100;
			frameRect.y = 3; frameRect.h = 32;
			frame->setSize(frameRect);
			frame->setColor(WideVector(.25f, .25f, .25f, 1.f));
			frame->setHigh(false);

			Field* field = frame->addField("propertyZ", 9);
			frameRect.x = 3; frameRect.w = 100 - 6;
			frameRect.y = 3; frameRect.h = 32 - 6;
			field->setSize(frameRect);
			field->setEditable(true);
			field->setNumbersOnly(true);
			field->setJustify(Field::RIGHT);
			field->setWidgetTab("propertyX");
			field->setWidgetSearchParent("editor_FramePanelX");
		}
	}

	// entity properties button
	{
		Button* button = gui->addButton("buttonEntityProperties");
		Rect<int> pos;
		pos.x = camRect.x + camRect.w - 40;
		pos.y = camRect.y + camRect.h - 40;
		pos.w = 36;
		pos.h = 36;
		button->setSize(pos);
		button->setIcon("images/gui/wrench.png");
		button->setTooltip("Entity properties");
		button->setStyle(Button::STYLE_TOGGLE);
	}
}

void Editor::updateContentNavigatorFilters() {
	Frame* gui = client->getGUI(); assert(gui);

	// clear existing entries
	{
		Frame* frame = gui->findFrame("editor_FrameContentNavigatorList"); assert(frame);
		auto& entries = frame->getEntries();
		while (entries.getFirst()) {
			delete entries.getFirst()->getData();
			entries.removeNode(entries.getFirst());
		}
	}

	// find all active filters
	ArrayList<Component::type_t> filters;
	{
		Frame* frame = gui->findFrame("editor_FrameContentNavigatorTop"); assert(frame);
		int c = 0;
		for (auto button : frame->getButtons()) {
			if (button->isPressed()) {
				filters.push(static_cast<Component::type_t>(c));
			}
			++c;
		}
	}

	// repopulate entries
	{
		Frame* frame = gui->findFrame("editor_FrameContentNavigatorList"); assert(frame);
		for (const Node<Entity::def_t*>* node = mainEngine->getEntityDefs().getFirst(); node != nullptr; node = node->getNext()) {
			const Entity::def_t* def = node->getData();
			bool matchesFilter = true;
			for (auto c : filters) {
				if (!def->entity.hasComponent(c)) {
					matchesFilter = false;
					break;
				}
			}
			if (def->exposedInEditor && matchesFilter) {
				Frame::entry_t* entry = frame->addEntry("spawn", true);
				entry->text = def->entity.getName();
				entry->params.addString(def->entity.getName());
				entry->color = WideVector(1.f);
			}
		}

		// sort entries by name
		class Sort : public LinkedList<Frame::entry_t*>::SortFunction {
		public:
			virtual ~Sort() {}
			virtual const bool operator()(Frame::entry_t* a, Frame::entry_t* b) const override {
				return strcmp(a->text.get(), b->text.get()) < 0;
			}
		} sortFn;
		frame->getEntries().sort(sortFn);
		frame->resizeForEntries();
	}
}

void Editor::updateLevelNavigatorFilters() {
	Frame* gui = client->getGUI(); assert(gui);

	// clear existing entries
	{
		Frame* frame = gui->findFrame("editor_FrameLevelNavigatorList"); assert(frame);
		auto& entries = frame->getEntries();
		while (entries.getFirst()) {
			delete entries.getFirst()->getData();
			entries.removeNode(entries.getFirst());
		}
		for (auto pair : world->getEntities()) {
			Entity* entity = pair.b;
			entity->removeListener();
		}
	}

	// find all active filters
	ArrayList<Component::type_t> filters;
	{
		Frame* frame = gui->findFrame("editor_FrameLevelNavigatorTop"); assert(frame);
		int c = 0;
		for (auto button : frame->getButtons()) {
			if (button->isPressed()) {
				filters.push(static_cast<Component::type_t>(c));
			}
			++c;
		}
	}

	// repopulate entries
	for (auto pair : world->getEntities()) {
		Frame* frame = gui->findFrame("editor_FrameLevelNavigatorList"); assert(frame);
		Entity* entity = pair.b;
		bool matchesFilter = true;
		for (auto c : filters) {
			if (!entity->hasComponent(c)) {
				matchesFilter = false;
				break;
			}
		}
		if (matchesFilter) {
			entity->addToEditorList();
		}

		// sort entries by name
		class Sort : public LinkedList<Frame::entry_t*>::SortFunction {
		public:
			virtual ~Sort() {}
			virtual const bool operator()(Frame::entry_t* a, Frame::entry_t* b) const override {
				return strcmp(a->text.get(), b->text.get()) < 0;
			}
		} sortFn;
		frame->getEntries().sort(sortFn);
		frame->resizeForEntries();
	}
}

void Editor::selectEntityForSpawn(const char* name) {
	entityToSpawn = nullptr;
	const Entity::def_t* def = Entity::findDef(name);
	if (def) {
		entityToSpawn = Entity::spawnFromDef(world, *def, world->getPointerPos(), Rotation());
		if (entityToSpawn) {
			entityToSpawn->setSelected(true);
			entityToSpawn->setHighlighted(true);
		}
	}
}

void Editor::entitiesName(const char* name) {
	LinkedList<Entity*> selectedEntities;
	world->findSelectedEntities(selectedEntities);
	for (auto entity : selectedEntities) {
		entity->setName(name);
	}
}

void Editor::entitiesScript(const char* script) {
	LinkedList<Entity*> selectedEntities;
	world->findSelectedEntities(selectedEntities);
	for (auto entity : selectedEntities) {
		entity->setScriptStr(script);
	}
}

void Editor::entitiesFlag(const Uint32 flag) {
	LinkedList<Entity*> selectedEntities;
	world->findSelectedEntities(selectedEntities);
	for (auto entity : selectedEntities) {
		entity->toggleFlag(flag);
	}
}

void Editor::entitiesSave() {
	LinkedList<Entity*> selectedEntities;
	world->findSelectedEntities(selectedEntities);
	for (auto entity : selectedEntities) {
		String result = mainEngine->fileSaveDialog("json", nullptr);
		if (!result.empty()) {
			entity->saveDef(result.get());
		}
	}
	mainEngine->loadAllDefs();
}

void Editor::entityKeyValueEnter(const char* pair) {
	LinkedList<Entity*> selectedEntities;
	world->findSelectedEntities(selectedEntities);
	for (Node<Entity*>* node = selectedEntities.getFirst(); node != nullptr; node = node->getNext()) {
		char* str = (char*)(pair);
		const char* key = strtok(str, ":");
		const char* value = strtok(nullptr, ":");
		if (key && value) {
			node->getData()->setKeyValue(key, value);
			guiNeedsUpdate = true;
		}
	}
}

void Editor::entityKeyValueRemove(const char* pair) {
	if (!pair || pair[0] == '\0') {
		return;
	}

	LinkedList<Entity*> selectedEntities;
	world->findSelectedEntities(selectedEntities);
	for (Node<Entity*>* node = selectedEntities.getFirst(); node != nullptr; node = node->getNext()) {
		char* str = (char*)(pair);
		const char* key = strtok(str, ":");
		if (key) {
			mainEngine->pressMouse(SDL_BUTTON_LEFT); // don't delete everything in the list at once
			node->getData()->deleteKeyValue(key);
			guiNeedsUpdate = true;
		}
	}
}

void Editor::entityKeyValueSelect(const char* pair) {
	Frame* gui = client->getGUI();
	assert(gui);
	Frame* frame = gui->findFrame("editor_FrameEntityPropertiesKeyValue");
	assert(frame);
	Field* field = frame->findField("field");
	assert(field);
	field->setText(pair);
}

void Editor::handleWidget(World& world) {
	bool highlightedWidget = false;

	Vector affect;
	Vector planeNormal;
	Vector planeOrigin = widgetPos;

	if (widget->isHighlighted()) {
		affect = Vector(1.f, 1.f, 1.f);
		planeNormal = (editingCamera->getGlobalPos() - planeOrigin).normal();
		highlightedWidget = true;
	} else if (widgetX->isHighlighted()) {
		affect = Vector(1.f, 0.f, 0.f);
		if (widgetMode == ROTATE) {
			planeNormal = Vector(1.f, 0.f, 0.f);
		} else {
			planeNormal = Vector(0.f, 1.f, 0.f);
		}
		highlightedWidget = true;
	} else if (widgetXY->isHighlighted()) {
		affect = Vector(1.f, 1.f, 0.f);
		planeNormal = Vector(0.f, 0.f, 1.f);
		highlightedWidget = true;
	} else if (widgetY->isHighlighted()) {
		affect = Vector(0.f, 1.f, 0.f);
		if (widgetMode == ROTATE) {
			planeNormal = Vector(0.f, 1.f, 0.f);
		} else {
			planeNormal = Vector(1.f, 0.f, 0.f);
		}
		highlightedWidget = true;
	} else if (widgetYZ->isHighlighted()) {
		affect = Vector(0.f, 1.f, 1.f);
		planeNormal = Vector(1.f, 0.f, 0.f);
		highlightedWidget = true;
	} else if (widgetZ->isHighlighted()) {
		affect = Vector(0.f, 0.f, 1.f);
		if (widgetMode == ROTATE) {
			planeNormal = Vector(0.f, 0.f, 1.f);
		} else {
			planeNormal = (editingCamera->getGlobalPos() - planeOrigin);
			planeNormal.z = 0.f;
			planeNormal.normalize();
		}
		highlightedWidget = true;
	} else if (widgetZX->isHighlighted()) {
		affect = Vector(1.f, 0.f, 1.f);
		planeNormal = Vector(0.f, 1.f, 0.f);
		highlightedWidget = true;
	}

	if (highlightedWidget) {
		int mouseX = mainEngine->getMouseX() * (mainEngine->getXres() / (float)Frame::virtualScreenX);
		int mouseY = mainEngine->getMouseY() * (mainEngine->getYres() / (float)Frame::virtualScreenY);

		Vector rayStart, rayEnd;
		editingCamera->screenPosToWorldRay(mouseX, mouseY, rayStart, rayEnd);
		rayEnd = rayStart + rayEnd * 4096.f;

		Vector intersection;
		if (Engine::lineIntersectPlane(rayStart, rayEnd, planeOrigin, planeNormal, intersection)) {
			if (!draggingWidget) {
				draggingWidget = true;
				oldIntersection = intersection;
			}

			// translation
			if (widgetMode == TRANSLATE) {
				if (editingMode == ENTITIES) {
					for (auto pair : world.getEntities()) {
						Entity* entity = pair.b;

						bool isWidget = !entity->isShouldSave() && entity->getName().find("widget") != UINT32_MAX;

						if (entity->isSelected() && !isWidget) {
							Vector* oldPos = oldVecs.find(entity);
							Vector diff = (intersection - oldIntersection);
							Vector newPos = (oldPos ? *oldPos : Vector()) + diff * affect;

							// grid snapping
							if (cvar_snapEnabled.toInt() && !isWidget) {
								newPos.x -= fmod(newPos.x, cvar_snapTranslate.toFloat());
								newPos.y -= fmod(newPos.y, cvar_snapTranslate.toFloat());
								newPos.z -= fmod(newPos.z, cvar_snapTranslate.toFloat());
							}

							entity->setPos(newPos);
						}
					}
				}
			}

			// rotation
			if (widgetMode == ROTATE) {
				for (auto pair : world.getEntities()) {
					Entity* entity = pair.b;

					bool isWidget = !entity->isShouldSave() && entity->getName().find("widget") != UINT32_MAX;

					if (entity->isSelected() || isWidget) {
						Vector* oldPos = oldVecs.find(entity);
						Vector diff = (intersection - oldIntersection);

						Quaternion* oldAng = oldAngs.find(entity);
						Rotation newAng = oldAng ? (*oldAng).toRotation() : Rotation();

						if (planeNormal.x == 1.f) {
							newAng.roll += (diff.y + diff.z) / 100.f;
						} else if (planeNormal.y == 1.f) {
							newAng.pitch += (diff.x + diff.z) / 100.f;
						} else if (planeNormal.z == 1.f) {
							newAng.yaw += (diff.x + diff.y) / 100.f;
						}

						// grid snapping
						if (cvar_snapEnabled.toInt()) {
							newAng.yaw = (static_cast<int>(floor(newAng.degreesYaw())) / cvar_snapRotate.toInt()) * cvar_snapRotate.toFloat();
							newAng.pitch = (static_cast<int>(floor(newAng.degreesPitch())) / cvar_snapRotate.toInt()) * cvar_snapRotate.toFloat();
							newAng.roll = (static_cast<int>(floor(newAng.degreesRoll())) / cvar_snapRotate.toInt()) * cvar_snapRotate.toFloat();

							newAng.yaw *= PI / 180.f;
							newAng.pitch *= PI / 180.f;
							newAng.roll *= PI / 180.f;
						}
						newAng.bindAngles();

						if (isWidget) {
							widgetAng = newAng;
						} else {
							entity->setAng(newAng);
						}
					}
				}
			}

			// scaling
			if (widgetMode == SCALE) {
				for (auto pair : world.getEntities()) {
					Entity* entity = pair.b;

					bool isWidget = !entity->isShouldSave() && entity->getName().find("widget") != UINT32_MAX;

					if (entity->isSelected() || isWidget) {
						Vector newScale = isWidget ? Vector(1.f) : entity->getScale();
						Vector size = (intersection - oldIntersection) / static_cast<float>(World::tileSize);
						size = size * affect;
						newScale += size;

						if (cvar_snapEnabled.toInt()) {
							float divisor = 100.f / cvar_snapScale.toFloat();
							Vector remainder;
							remainder.x = static_cast<float>(static_cast<int>(floor(newScale.x * divisor))) / divisor;
							remainder.y = static_cast<float>(static_cast<int>(floor(newScale.y * divisor))) / divisor;
							remainder.z = static_cast<float>(static_cast<int>(floor(newScale.z * divisor))) / divisor;
							newScale = remainder;
						}

						if (isWidget) {
							widgetScale = newScale;
						} else {
							entity->setScale(newScale);
						}
					}
				}
				oldIntersection = intersection;
			}
		}
	}
}

void Editor::editEntities(bool usable) {
	World& world = *this->world;

	// delete
	if (!mainEngine->getInputStr()) {
		if (mainEngine->pressKey(SDL_SCANCODE_DELETE)) {
			playSound("editor/close.wav");
			for (auto pair : world.getEntities()) {
				Entity* entity = pair.b;
				if (entity->isSelected()) {
					mainEngine->fmsg(Engine::MSG_INFO, "Deleting %s", entity->getName().get());
					entity->remove();
				}
			}
		}
	}

	// copy/paste
	if (!mainEngine->getInputStr()) {
		if (mainEngine->getKeyStatus(SDL_SCANCODE_LCTRL) || mainEngine->getKeyStatus(SDL_SCANCODE_RCTRL)) {
			bool paste = mainEngine->pressKey(SDL_SCANCODE_V);
			bool cut = mainEngine->pressKey(SDL_SCANCODE_X);
			bool copy = mainEngine->pressKey(SDL_SCANCODE_C);

			// whether we're cutting, pasting, or just copying, we need to copy some entities
			if (copy || cut || paste) {
				LinkedList<Entity*> selectedEntities;

				// if we're pasting, first paste all entities we've got in the clipboard
				if (paste) {
					world.findSelectedEntities(selectedEntities);

					// deselect all previously selected entities
					for (Node<Entity*>* node = selectedEntities.getFirst(); node != nullptr; node = node->getNext()) {
						Entity* entity = node->getData();
						entity->setSelected(false);
						entity->setHighlighted(false);
					}

					// paste entities...
					for (Node<Entity*>* node = copiedEntities.getFirst(); node != nullptr; node = node->getNext()) {
						Entity* entity = node->getData();
						entity->insertIntoWorld(&world);
						entity->finishInsertIntoWorld();
						entity->setSelected(true);
						entity->setHighlighted(true);
						entity->addToEditorList();
					}
				}

				// clear old contents of clipboard
				copiedEntities.removeAll();

				// copy all selected entities
				world.findSelectedEntities(selectedEntities);
				for (Node<Entity*>* node = selectedEntities.getFirst(); node != nullptr; node = node->getNext()) {
					Entity* entity = node->getData();

					// create a new copy of this entity
					Entity* newEntity = entity->copy(nullptr, nullptr);
					copiedEntities.addNodeLast(newEntity);
				}

				// delete selected entities after cutting
				if (cut) {
					for (Node<Entity*>* node = selectedEntities.getFirst(); node != nullptr; node = node->getNext()) {
						Entity* entity = node->getData();
						entity->remove();
					}
				}
			}
		}
	}

	// select none
	if (mainEngine->pressKey(SDL_SCANCODE_ESCAPE)) {
		if (!editingText) {
			playSound("editor/deselect.wav");
			world.selectEntities(false);
			world.deselectGeometry();
		}
	}

	// spawn a new entity
	if (entityToSpawn) {
		Vector pos = world.getPointerPos();

		// grid snapping
		if (cvar_snapEnabled.toInt()) {
			pos.x -= fmod(pos.x, cvar_snapTranslate.toFloat());
			pos.y -= fmod(pos.y, cvar_snapTranslate.toFloat());
			pos.z -= fmod(pos.z, cvar_snapTranslate.toFloat());
		}

		entityToSpawn->setPos(pos);
		if (!mainEngine->getMouseStatus(SDL_BUTTON_LEFT)) {
			entityToSpawn->addToEditorList();
			playSound("editor/place.wav");
			entityToSpawn = nullptr;
		}
	}

	// widget utilization
	if (editingCamera) {
		if (leftClicking) {
			handleWidget(world);
		} else {
			draggingWidget = false;

			oldVecs.clear();
			oldAngs.clear();
			for (auto pair : world.getEntities()) {
				Entity* entity = pair.b;
				oldVecs.insert(entity, entity->getPos());
				oldAngs.insert(entity, entity->getAng());
			}
		}
	}

	// select entity
	if (leftClick && !entityToSpawn && world.isPointerActive()) {
		Entity* entity;

		if ((entity = world.uidToEntity(highlightedObj)) != nullptr) {
			if (entity->isShouldSave()) {
				if (!mainEngine->getKeyStatus(SDL_SCANCODE_LCTRL)) {
					world.selectEntities(false);
				}

				entity->setSelected(true);
				entity->setHighlighted(true);

				playSound("editor/rollover.wav");
			} else {
				if (entity->getName().find("widget") != UINT32_MAX) {
					entity->setHighlighted(true);

					Model* model = entity->findComponentByName<Model>("model");
					Mesh::shadervars_t shaderVars = model->getShaderVars();
					shaderVars.highlightColor = glm::vec4(1.f, 1.f, 0.f, 1.f);
					model->setShaderVars(shaderVars);
				} else {
					if (!mainEngine->getKeyStatus(SDL_SCANCODE_LCTRL)) {
						world.selectEntities(false);
					}
				}
			}
		} else {
			if (world.isPointerActive()) {
				if (!mainEngine->getKeyStatus(SDL_SCANCODE_LCTRL)) {
					world.selectEntities(false);
				}
			}
		}
	}
}

void Editor::preProcess() {
	if (client->getGUI()) {
		updateGUI(*client->getGUI());
	}

	// autosaving
	bool autosave = (client->getTicks() && client->getTicks() % (mainEngine->getTicksPerSecond() * 60 * 5) == 0);
	if (autosave) {
		mainEngine->fmsg(Engine::MSG_INFO, "autosaving current map...");
		int count = (client->getTicks() / (mainEngine->getTicksPerSecond() * 60 * 5)) % 10;
		StringBuf<64> path;
		path.format("maps/autosave%d.%s", count, World::fileExtensions[static_cast<int>(world->getFiletype())]);
		path = mainEngine->buildPath(path.get()).get();
		playSound("editor/install.wav");
		world->saveFile(path.get(), true);
	}

	// allows the editor to clear the highlighted object when the mouse is out of the viewport
	if (!leftClicking) {
		highlightedObjManuallySet = false;
	}
}

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

void Editor::process(const bool usable) {
	World& world = *this->world;

	int xres = Frame::virtualScreenX;
	int yres = Frame::virtualScreenY;

	if (mainEngine->getKeyStatus(SDL_SCANCODE_LCTRL) || mainEngine->getKeyStatus(SDL_SCANCODE_RCTRL)) {
		if (mainEngine->pressKey(SDL_SCANCODE_F)) {
			fullscreen = (fullscreen == false);
			if (fullscreen) {
				playSound("editor/maximize.wav");
			} else {
				playSound("editor/cancel.wav");
			}
		}
	}

	if (mainEngine->getKeyStatus(SDL_SCANCODE_LCTRL) || mainEngine->getKeyStatus(SDL_SCANCODE_RCTRL)) {
		if (mainEngine->pressKey(SDL_SCANCODE_G)) {
			world.setGridVisible(world.isGridVisible() == false);
		}
	}

	// fullscreen stuff
	if (fullscreen) {
		if (minimap) {
			minimap->getEntity()->resetFlag(static_cast<int>(Entity::flag_t::FLAG_VISIBLE));
		}

		if (editingCamera) {
			editingCamera->setWin(Rect<int>(0, 0, Frame::virtualScreenX, Frame::virtualScreenY));
		}
	} else {
		if (minimap) {
			minimap->getEntity()->setFlag(static_cast<int>(Entity::flag_t::FLAG_VISIBLE));
		}

		if (editingCamera) {
			Rect<int> camRect;
			camRect.x = xres / 5;
			camRect.w = xres - xres / 2.5f;
			camRect.y = 44;
			camRect.h = yres - 44 - 200;
			editingCamera->setWin(camRect);
		}
	}

	// left clicking
	leftClick = false;
	if (mainEngine->getMouseStatus(SDL_BUTTON_LEFT) && !leftClicking && !leftClickLock) {
		leftClicking = true;
		leftClick = true;
	} else if (!mainEngine->getMouseStatus(SDL_BUTTON_LEFT)) {
		leftClicking = false;
		leftClickLock = false;
	}

	// minimap controls
	if (minimap && !mainEngine->isMouseRelative()) {
		Frame* gui = client->getGUI();
		if (gui) {
			Frame* frame = gui->findFrame("editor_Minimap");
			if (frame) {
				if (frame->capturesMouse()) {
					// zoom-in / zoom-out
					if (mainEngine->getMouseWheelY() > 0) {
						Sint32 newFov = max(minimap->getFov() - World::tileSize * 2, World::tileSize);
						minimap->setFov(newFov);
					} else if (mainEngine->getMouseWheelY() < 0) {
						Sint32 newFov = min(minimap->getFov() + World::tileSize * 2, World::tileSize * 128);
						minimap->setFov(newFov);
					}

					// change perspective
					if (mainEngine->pressKey(SDL_SCANCODE_1)) {
						minimap->setLocalAng(minimapRot[0]);
					}
					if (mainEngine->pressKey(SDL_SCANCODE_2)) {
						minimap->setLocalAng(minimapRot[1]);
					}
					if (mainEngine->pressKey(SDL_SCANCODE_3)) {
						minimap->setLocalAng(minimapRot[2]);
					}
				}
			}
		}
	}

	// viewport controls
	if (editingCamera && !mainEngine->getInputStr()) {
		Camera* camera = editingCamera;

		Rotation newAng(0.f, 0.f, 0.f);
		Vector newPos(0.f, 0.f, 0.f);

		// keyboard controls
		float buttonRight = (float)mainEngine->getKeyStatus(SDL_SCANCODE_D) * (1.f + (float)mainEngine->getKeyStatus(SDL_SCANCODE_LSHIFT));
		float buttonLeft = (float)mainEngine->getKeyStatus(SDL_SCANCODE_A) * (1.f + (float)mainEngine->getKeyStatus(SDL_SCANCODE_LSHIFT));
		float buttonForward = (float)mainEngine->getKeyStatus(SDL_SCANCODE_W) * (1.f + (float)mainEngine->getKeyStatus(SDL_SCANCODE_LSHIFT));
		float buttonBackward = (float)mainEngine->getKeyStatus(SDL_SCANCODE_S) * (1.f + (float)mainEngine->getKeyStatus(SDL_SCANCODE_LSHIFT));
		float buttonUp = (float)mainEngine->getKeyStatus(SDL_SCANCODE_E) * (1.f + (float)mainEngine->getKeyStatus(SDL_SCANCODE_LSHIFT));
		float buttonDown = (float)mainEngine->getKeyStatus(SDL_SCANCODE_Q) * (1.f + (float)mainEngine->getKeyStatus(SDL_SCANCODE_LSHIFT));

		// camera movement
		float timeFactor = 800.f / mainEngine->getTicksPerSecond();

		Vector forward = camera->getEntity()->getAng().toVector();
		newPos += forward * buttonForward * timeFactor;
		newPos -= forward * buttonBackward * timeFactor;

		Vector right = camera->getEntity()->getAng().rotate(Rotation(PI / 2.f, 0.f, 0.f)).toVector();
		newPos += right * buttonRight * timeFactor;
		newPos -= right * buttonLeft * timeFactor;

		Vector up = camera->getEntity()->getAng().rotate(Rotation(0.f, -PI / 2.f, 0.f)).toVector();
		newPos += up * buttonUp * timeFactor / 2.f;
		newPos -= up * buttonDown * timeFactor / 2.f;

		// mouse controls
		if (mainEngine->isMouseRelative()) {
			double mousex = mainEngine->getMouseMoveX() / 4.f;
			double mousey = mainEngine->getMouseMoveY() / 4.f;

			newAng.yaw += PI / (mainEngine->getTicksPerSecond() * 3) * (mousex);
			newAng.pitch += PI / (mainEngine->getTicksPerSecond() * 3) * (mousey);
		}

		float buttonLookRight = (float)mainEngine->getKeyStatus(SDL_SCANCODE_RIGHT) * (1.f + (float)mainEngine->getKeyStatus(SDL_SCANCODE_LSHIFT));
		float buttonLookLeft = (float)mainEngine->getKeyStatus(SDL_SCANCODE_LEFT) * (1.f + (float)mainEngine->getKeyStatus(SDL_SCANCODE_LSHIFT));
		float buttonLookUp = (float)mainEngine->getKeyStatus(SDL_SCANCODE_UP) * (1.f + (float)mainEngine->getKeyStatus(SDL_SCANCODE_LSHIFT));
		float buttonLookDown = (float)mainEngine->getKeyStatus(SDL_SCANCODE_DOWN) * (1.f + (float)mainEngine->getKeyStatus(SDL_SCANCODE_LSHIFT));

		newAng.roll += (buttonLookLeft - buttonLookRight) * .05f;
		newAng.pitch += (buttonLookDown - buttonLookUp) * .05f;

		// apply translation + rotation
		//camera->getEntity()->setRot(newAng);
		newAng = newAng + camera->getEntity()->getLookDir();
		newAng.wrapAngles();
		if (newAng.pitch > PI / 2.f) {
			newAng.pitch = PI / 2.f;
		} if (newAng.pitch < -PI / 2.f) {
			newAng.pitch = -PI / 2.f;
		}
		camera->getEntity()->setLookDir(newAng);
		camera->getEntity()->setAng(Quaternion(newAng));
		camera->getEntity()->setVel(newPos);
		camera->getEntity()->update();
		client->getMixer()->setListener(camera);

		if (minimap) {
			minimap->getEntity()->setPos(camera->getEntity()->getPos());
		}

		// move selector (mouse pointer)
		if (!usable) {
			world.setPointerActive(false);
		}
		Vector rayStart, rayEnd;

		// determine if the mouse is in the viewport
		previousInWindow = inWindow;
		inWindow = false;
		if (!mainEngine->isMouseRelative()) {
			Rect<int> rect = camera->getWin();
			Sint32 mouseX = (mainEngine->getMouseX() / (float)mainEngine->getXres()) * (float)Frame::virtualScreenX;
			Sint32 mouseY = (mainEngine->getMouseY() / (float)mainEngine->getYres()) * (float)Frame::virtualScreenY;
			if (rect.containsPoint(mouseX, mouseY)) {
				inWindow = true;
			}
		}

		LinkedList<World::hit_t> list;
		if (inWindow && (usable || entityToSpawn)) {
			Sint32 mouseX = mainEngine->getMouseX();
			Sint32 mouseY = mainEngine->getMouseY();
			camera->screenPosToWorldRay(mouseX, mouseY, rayStart, rayEnd);
			Vector rayStop = rayStart + rayEnd * camera->getClipFar();
			world.lineTraceList(rayStart, rayStop, list);
			if (list.getSize() > 0) {
				world.setPointerActive(false);
				highlightedVertex = World::nuid;
				if (!highlightedObjManuallySet) {
					highlightedObj = World::nuid;
				}
				highlightedSector = nullptr;
				highlightedFace = -1;

				// find the widget under the mouse, if it's there
				World::hit_t widgetUnderMouse;
				Node<World::hit_t>* nextNode;
				for (Node<World::hit_t>* node = list.getFirst(); node != nullptr; node = nextNode) {
					const World::hit_t& hit = node->getData();
					nextNode = node->getNext();

					if (hit.manifest) {
						if (hit.manifest->entity) {
							Entity* entity = hit.manifest->entity;
							if (entity == entityToSpawn) {
								list.removeNode(node);
								continue;
							} else if (!entity->isShouldSave()) {
								if (entity->getName().find("widget") != UINT32_MAX) {
									widgetUnderMouse = hit;
									break;
								}
							}
						}
					}
				}

				// pick the thing we're pointing at
				if (list.getSize() > 0) {
					const World::hit_t& firstObjectHit = list[0]->getData();
					const World::hit_t& hit = (widgetUnderMouse.manifest && widgetUnderMouse.manifest->entity) ? widgetUnderMouse : firstObjectHit;

					if (!hit.manifest || !hit.manifest->entity) {
						world.setPointerActive(true);

						if (editingMode == ENTITIES) {
							if (entityToSpawn != nullptr) {
								if (entityToSpawn->hasComponent(Component::COMPONENT_LIGHT)) {
									world.setPointerPos(hit.pos + hit.normal*5.f);
								} else {
									world.setPointerPos(hit.pos);
								}
							} else {
								world.setPointerPos(hit.pos);
							}
						} else {
							if (hit.normal.z == 0) {
								if (editingMode == TILES) {
									world.setPointerPos(hit.pos - hit.normal);
								} else if (editingMode == TEXTURES) {
									world.setPointerPos(hit.pos + hit.normal);
								}
							} else {
								world.setPointerPos(hit.pos);

								if (mainEngine->getMouseStatus(SDL_BUTTON_LEFT)) {
									if (hit.normal.z < 0) {
										ceilingMode = false;
									} else {
										ceilingMode = true;
									}
								}
							}
						}
					} else if (hit.manifest && hit.manifest->entity) {
						world.setPointerActive(true);
						world.setPointerPos(hit.pos);
						if (!leftClicking || leftClick) {
							highlightedObj = hit.manifest->entity->getUID();
						}
					}
				}
			}
		}
		if ((list.getSize() == 0 && usable && inWindow) ||
			(!inWindow && !entityToSpawn && !highlightedObjManuallySet && !leftClicking) ||
			(list.getSize() == 0 && entityToSpawn && inWindow) ||
			(leftClick && !inWindow)) {
			if (!highlightedObjManuallySet) {
				highlightedObj = World::nuid;
			}
			highlightedVertex = World::nuid;
			highlightedSector = nullptr;
			highlightedFace = -1;
			world.setPointerActive(true);
			world.setPointerPos(rayStart + rayEnd * 256.f);
		}
	}

	// closing entity add component window
	{
		Frame* gui = client->getGUI();
		Frame* frame = gui->findFrame("editor_FrameEntityAddComponent");
		if (frame) {
			if (mainEngine->pressKey(SDL_SCANCODE_ESCAPE)) {
				playSound("editor/cancel.wav");
				frame->removeSelf();
			}
		}
	}

	// closing entity properties window
	{
		Frame* gui = client->getGUI();
		Frame* frame = gui->findFrame("editor_FrameEntityProperties");
		if (frame) {
			if (mainEngine->pressKey(SDL_SCANCODE_ESCAPE)) {
				playSound("editor/menuclose.wav");
				frame->removeSelf();

				Button* button = client->getGUI()->findButton("buttonEntityProperties");
				if (button) {
					button->setPressed(false);
				}
			}
		}
	}

	// main editing functionality
	if (editingMode == TILES || editingMode == TEXTURES) {
		//editTiles(usable);
	} else if (editingMode == ENTITIES) {
		editEntities(usable);
	} else if (editingMode == SECTORS) {
		//editSectors(usable);
	}

	// toggle mouselook
	if (mainEngine->pressMouse(SDL_BUTTON_RIGHT)) {
		mainEngine->setMouseRelative(mainEngine->isMouseRelative() == false);
	}
}

void Editor::postProcess() {
	editingText = (mainEngine->getInputStr() != nullptr) ? true : false;
}

void Editor::updateWidgetImages(Frame* parent, const char* translateImg, const char* rotateImg, const char* scaleImg) {
	// translate mode
	if (widgetMode == TRANSLATE) {
		Frame::image_t* image = parent->findImage(rotateImg);
		if (image) {
			int x = image->pos.x;
			int y = image->pos.y;
			parent->remove(rotateImg);
			parent->addImage(Rect<Sint32>(x, y, 0, 0), WideVector(1.f, 1.f, 1.f, 1.f), translateImg);
		} else {
			Frame::image_t* image = parent->findImage(scaleImg);
			if (image) {
				int x = image->pos.x;
				int y = image->pos.y;
				parent->remove(scaleImg);
				parent->addImage(Rect<Sint32>(x, y, 0, 0), WideVector(1.f, 1.f, 1.f, 1.f), translateImg);
			}
		}
	}

	// rotate mode
	if (widgetMode == ROTATE) {
		Frame::image_t* image = parent->findImage(translateImg);
		if (image) {
			int x = image->pos.x;
			int y = image->pos.y;
			parent->remove(translateImg);
			parent->addImage(Rect<Sint32>(x, y, 0, 0), WideVector(1.f, 1.f, 1.f, 1.f), rotateImg);
		} else {
			Frame::image_t* image = parent->findImage(scaleImg);
			if (image) {
				int x = image->pos.x;
				int y = image->pos.y;
				parent->remove(scaleImg);
				parent->addImage(Rect<Sint32>(x, y, 0, 0), WideVector(1.f, 1.f, 1.f, 1.f), rotateImg);
			}
		}
	}

	// scale mode
	if (widgetMode == SCALE) {
		Frame::image_t* image = parent->findImage(translateImg);
		if (image) {
			int x = image->pos.x;
			int y = image->pos.y;
			parent->remove(translateImg);
			parent->addImage(Rect<Sint32>(x, y, 0, 0), WideVector(1.f, 1.f, 1.f, 1.f), scaleImg);
		} else {
			Frame::image_t* image = parent->findImage(rotateImg);
			if (image) {
				int x = image->pos.x;
				int y = image->pos.y;
				parent->remove(rotateImg);
				parent->addImage(Rect<Sint32>(x, y, 0, 0), WideVector(1.f, 1.f, 1.f, 1.f), scaleImg);
			}
		}
	}
}

void Editor::entityComponentExpand(unsigned int uid) {
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (!entity->isSelected()) {
			continue;
		}
		Component* component = entity->findComponentByUID<Component>(uid);

		component->setCollapsed(false);
		guiNeedsUpdate = true;
	}
}

void Editor::entityComponentCollapse(unsigned int uid) {
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (!entity->isSelected()) {
			continue;
		}
		Component* component = entity->findComponentByUID<Component>(uid);

		component->setCollapsed(true);
		guiNeedsUpdate = true;
	}
}

void Editor::entityRemoveComponent(unsigned int uid) {
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (!entity->isSelected()) {
			continue;
		}
		entity->removeComponentByUID(uid);
		entity->update();
		guiNeedsUpdate = true;

		// prevents user trying to add sub-components to removed components...
		Frame* frame = client->getGUI()->findFrame("editor_FrameEntityAddComponent");
		if (frame) {
			frame->removeSelf();
		}
	}
}

void Editor::entityCopyComponent(unsigned int uid) {
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (!entity->isSelected()) {
			continue;
		}
		Component* component = entity->findComponentByUID<Component>(uid);
		if (component)
		{
			if (component->getParent()) {
				component->copy(component->getParent());
			} else {
				component->copy(component->getEntity());
			}
			entity->update();
			guiNeedsUpdate = true;
		}
	}
}

void Editor::entityAddComponent(unsigned int uid, Uint32 type) {
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (!entity->isSelected()) {
			continue;
		}

		if (uid) {
			Component* component = entity->findComponentByUID<Component>(uid);
			component->addComponent(static_cast<Component::type_t>(type));
		} else {
			entity->addComponent(static_cast<Component::type_t>(type));
		}

		entity->update();
		guiNeedsUpdate = true;

		playSound("editor/mount.wav");
		Frame* frame = client->getGUI()->findFrame("editor_FrameEntityAddComponent");
		if (frame) {
			frame->removeSelf();
		}
	}
}

void Editor::entityComponentName(unsigned int uid, const char* name) {
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (!entity->isSelected()) {
			continue;
		}
		Component* component = entity->findComponentByUID<Component>(uid);

		component->setName(name);
	}
}

void Editor::entityComponentTranslate(unsigned int uid, int dimension, float value) {
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (!entity->isSelected()) {
			continue;
		}
		Component* component = entity->findComponentByUID<Component>(uid);

		Vector pos = component->getLocalPos();
		switch (dimension) {
		case 0:
			pos.x = value;
			break;
		case 1:
			pos.y = value;
			break;
		case 2:
			pos.z = value;
			break;
		default:
			break;
		}
		component->setLocalPos(pos);
		component->update();
	}
}

void Editor::entityComponentRotate(unsigned int uid, int dimension, float value) {
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (!entity->isSelected()) {
			continue;
		}
		Component* component = entity->findComponentByUID<Component>(uid);

		Rotation rot = component->getLocalAng().toRotation();
		switch (dimension) {
		case 0:
			rot.roll = value * PI / 180.f;
			break;
		case 1:
			rot.pitch = value * PI / 180.f;
			break;
		case 2:
			rot.yaw = value * PI / 180.f;
			break;
		default:
			break;
		}
		component->setLocalAng(rot);
		component->update();
	}
}

void Editor::entityComponentScale(unsigned int uid, int dimension, float value) {
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (!entity->isSelected()) {
			continue;
		}
		Component* component = entity->findComponentByUID<Component>(uid);

		Vector scale = component->getLocalScale();
		switch (dimension) {
		case 0:
			scale.x = value;
			break;
		case 1:
			scale.y = value;
			break;
		case 2:
			scale.z = value;
			break;
		default:
			break;
		}
		component->setLocalScale(scale);
		component->update();
	}
}

void Editor::widgetTranslateX(float x) {
	widgetPos.x = x;
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (entity->isSelected()) {
			Vector newPos = entity->getPos();
			newPos.x = x;
			entity->setPos(newPos);
		}
	}
}

void Editor::widgetTranslateY(float y) {
	widgetPos.y = y;
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (entity->isSelected()) {
			Vector newPos = entity->getPos();
			newPos.y = y;
			entity->setPos(newPos);
		}
	}
}

void Editor::widgetTranslateZ(float z) {
	widgetPos.z = z;
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (entity->isSelected()) {
			Vector newPos = entity->getPos();
			newPos.z = z;
			entity->setPos(newPos);
		}
	}
}

void Editor::widgetRotateYaw(float yaw) {
	widgetAng.yaw = yaw * PI / 180.f;
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (entity->isSelected()) {
			Rotation newAng = entity->getAng().toRotation();
			newAng.yaw = yaw * PI / 180.f;

			// get other angles as well
			Frame* gui = client->getGUI(); assert(gui);
			{
				float f = 0.f;
				const char* n = "editor_FrameEntityPropertiesPitch";
				Frame* frame = gui->findFrame(n);
				if (frame) {
					Field* field = frame->findField("field"); assert(field);
					Engine::readFloat(field->getText(), &f, 1);
				}
				newAng.pitch = f * PI / 180.f;
			}
			{
				float f = 0.f;
				const char* n = "editor_FrameEntityPropertiesRoll";
				Frame* frame = gui->findFrame(n);
				if (frame) {
					Field* field = frame->findField("field"); assert(field);
					Engine::readFloat(field->getText(), &f, 1);
				}
				newAng.roll = f * PI / 180.f;
			}

			entity->setAng(newAng);
		}
	}
}

void Editor::widgetRotatePitch(float pitch) {
	widgetAng.pitch = pitch * PI / 180.f;
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (entity->isSelected()) {
			Rotation newAng = entity->getAng().toRotation();
			newAng.pitch = pitch * PI / 180.f;

			// get other angles as well
			Frame* gui = client->getGUI(); assert(gui);
			{
				float f = 0.f;
				const char* n = "editor_FrameEntityPropertiesYaw";
				Frame* frame = gui->findFrame(n);
				if (frame) {
					Field* field = frame->findField("field"); assert(field);
					Engine::readFloat(field->getText(), &f, 1);
				}
				newAng.yaw = f * PI / 180.f;
			}
			{
				float f = 0.f;
				const char* n = "editor_FrameEntityPropertiesRoll";
				Frame* frame = gui->findFrame(n);
				if (frame) {
					Field* field = frame->findField("field"); assert(field);
					Engine::readFloat(field->getText(), &f, 1);
				}
				newAng.roll = f * PI / 180.f;
			}

			entity->setAng(newAng);
		}
	}
}

void Editor::widgetRotateRoll(float roll) {
	widgetAng.roll = roll * PI / 180.f;
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (entity->isSelected()) {
			Rotation newAng = entity->getAng().toRotation();
			newAng.roll = roll * PI / 180.f;

			// get other angles as well
			Frame* gui = client->getGUI(); assert(gui);
			{
				float f = 0.f;
				const char* n = "editor_FrameEntityPropertiesYaw";
				Frame* frame = gui->findFrame(n);
				if (frame) {
					Field* field = frame->findField("field"); assert(field);
					Engine::readFloat(field->getText(), &f, 1);
				}
				newAng.yaw = f * PI / 180.f;
			}
			{
				float f = 0.f;
				const char* n = "editor_FrameEntityPropertiesPitch";
				Frame* frame = gui->findFrame(n);
				if (frame) {
					Field* field = frame->findField("field"); assert(field);
					Engine::readFloat(field->getText(), &f, 1);
				}
				newAng.pitch = f * PI / 180.f;
			}

			entity->setAng(newAng);
		}
	}
}

void Editor::widgetScaleX(float x) {
	widgetScale.x = x;
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (entity->isSelected()) {
			Vector newScale = entity->getScale();
			newScale.x = x;
			entity->setScale(newScale);
		}
	}
}

void Editor::widgetScaleY(float y) {
	widgetScale.y = y;
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (entity->isSelected()) {
			Vector newScale = entity->getScale();
			newScale.y = y;
			entity->setScale(newScale);
		}
	}
}

void Editor::widgetScaleZ(float z) {
	widgetScale.z = z;
	for (auto pair : world->getEntities()) {
		Entity* entity = pair.b;

		if (entity->isSelected()) {
			Vector newScale = entity->getScale();
			newScale.z = z;
			entity->setScale(newScale);
		}
	}
}

void Editor::componentGUI(Frame& properties, Component* component, int& x, int& y) {
	if (!component || component->isEditorOnly()) {
		return;
	}

	int width = properties.getSize().w - Frame::sliderSize;
	int border = properties.getBorder();

	// name
	{
		Frame* frame = properties.addFrame("editor_FrameComponentName", "editor_FrameComponentName");

		Rect<int> size;
		size.x = 0; size.w = width - border * 4 - x - 30 - border;
		size.y = 0; size.h = 30;
		frame->setActualSize(size);
		size.x = 30 + border + x + border * 2; size.w = width - border * 4 - x - 30 - border;
		size.y = y; size.h = 30;
		frame->setSize(size);
		frame->setColor(WideVector(.25, .25, .25, 1.0));
		frame->setHigh(false);

		Field* field = frame->addField("field", 64);
		size.x = border; size.w = frame->getSize().w - border * 2;
		size.y = border; size.h = frame->getSize().h - border * 2;
		field->setSize(size);
		field->setEditable(true);

		field->getParams().addInt(component->getUID());

		field->setText(component->getName());
	}

	if (component->isCollapsed()) {
		// expand button
		{
			Button* button = properties.addButton("buttonExpand");
			button->setIcon("images/gui/arrow_down.png");
			button->setStyle(Button::STYLE_NORMAL);
			button->getParams().addInt(component->getUID());
			button->setBorder(2);

			Rect<int> size;
			size.x = border * 2 + x; size.w = 30;
			size.y = y; size.h = 30;
			button->setSize(size);

			y += size.h + border;
		}
	} else {
		// collapse button
		{
			Button* button = properties.addButton("buttonCollapse");
			button->setIcon("images/gui/arrow_up.png");
			button->setStyle(Button::STYLE_NORMAL);
			button->getParams().addInt(component->getUID());
			button->setBorder(2);

			Rect<int> size;
			size.x = border * 2 + x; size.w = 30;
			size.y = y; size.h = 30;
			button->setSize(size);

			y += size.h + border;
		}

		x += 30;

		// position label
		{
			Field* label = properties.addField("labelPosition", 16);

			Rect<int> size;
			size.x = x + border * 2;
			size.w = width - x - border * 4;
			size.y = y;
			size.h = 20;
			y += size.h + border;
			label->setSize(size);
			label->setText("Position:");
		}

		// translate x
		{
			StringBuf<32> name("editor_FrameComponentTranslateX%d", 1, component->getUID());
			Frame* frame = properties.addFrame(name.get(), "editor_FrameComponentTranslate");

			Rect<int> size;
			size.x = 0;
			size.w = (width - x) / 3 - border * 2;
			size.y = 0;
			size.h = 30;
			frame->setActualSize(size);
			size.x = border * 2 + x;
			size.w = (width - x) / 3 - border * 2;
			size.y = y;
			size.h = 30;
			frame->setSize(size);
			frame->setColor(WideVector(.25, .25, .25, 1.0));
			frame->setHigh(false);

			Field* field = frame->addField("field", 9);
			size.x = border; size.w = frame->getSize().w - border * 2;
			size.y = border; size.h = frame->getSize().h - border * 2;
			field->setSize(size);
			field->setEditable(true);
			field->setNumbersOnly(true);
			field->setJustify(Field::RIGHT);
			field->setColor(WideVector(1.f, .2f, .2f, 1.f));

			StringBuf<32> dest("editor_FrameComponentTranslateY%d", 1, component->getUID());
			field->setWidgetSearchParent(dest.get());
			field->setWidgetTab("field");

			field->getParams().addInt(0);
			field->getParams().addInt(component->getUID());

			char x[16];
			snprintf(x, 16, "%.2f", component->getLocalPos().x);
			field->setText(x);
		}

		// translate y
		{
			StringBuf<32> name("editor_FrameComponentTranslateY%d", 1, component->getUID());
			Frame* frame = properties.addFrame(name.get(), "editor_FrameComponentTranslate");

			Rect<int> size;
			size.x = 0;
			size.w = (width - x) / 3 - border * 2;
			size.y = 0;
			size.h = 30;
			frame->setActualSize(size);
			size.x = border * 2 + (width - x) / 3 - border + x;
			size.w = (width - x) / 3 - border * 2;
			size.y = y;
			size.h = 30;
			frame->setSize(size);
			frame->setColor(WideVector(.25, .25, .25, 1.0));
			frame->setHigh(false);

			Field* field = frame->addField("field", 9);
			size.x = border; size.w = frame->getSize().w - border * 2;
			size.y = border; size.h = frame->getSize().h - border * 2;
			field->setSize(size);
			field->setEditable(true);
			field->setNumbersOnly(true);
			field->setJustify(Field::RIGHT);
			field->setColor(WideVector(.2f, 1.f, .2f, 1.f));

			StringBuf<32> dest("editor_FrameComponentTranslateZ%d", 1, component->getUID());
			field->setWidgetSearchParent(dest.get());
			field->setWidgetTab("field");

			field->getParams().addInt(1);
			field->getParams().addInt(component->getUID());

			char y[16];
			snprintf(y, 16, "%.2f", component->getLocalPos().y);
			field->setText(y);
		}

		// translate z
		{
			StringBuf<32> name("editor_FrameComponentTranslateZ%d", 1, component->getUID());
			Frame* frame = properties.addFrame(name.get(), "editor_FrameComponentTranslate");

			Rect<int> size;
			size.x = 0;
			size.w = (width - x) / 3 - border * 2;
			size.y = 0;
			size.h = 30;
			frame->setActualSize(size);
			size.x = border * 2 + 2 * (width - x) / 3 - border * 2 + x;
			size.w = (width - x) / 3 - border * 2;
			size.y = y;
			size.h = 30;
			y += size.h + border;
			frame->setSize(size);
			frame->setColor(WideVector(.25, .25, .25, 1.0));
			frame->setHigh(false);

			Field* field = frame->addField("field", 9);
			size.x = border; size.w = frame->getSize().w - border * 2;
			size.y = border; size.h = frame->getSize().h - border * 2;
			field->setSize(size);
			field->setEditable(true);
			field->setNumbersOnly(true);
			field->setJustify(Field::RIGHT);
			field->setColor(WideVector(.2f, .2f, 1.f, 1.f));

			StringBuf<32> dest("editor_FrameComponentTranslateX%d", 1, component->getUID());
			field->setWidgetSearchParent(dest.get());
			field->setWidgetTab("field");

			field->getParams().addInt(2);
			field->getParams().addInt(component->getUID());

			char z[16];
			snprintf(z, 16, "%.2f", component->getLocalPos().z);
			field->setText(z);
		}

		// rotation label
		{
			Field* label = properties.addField("labelRotation", 16);

			Rect<int> size;
			size.x = border * 2 + x;
			size.w = width - border * 4 - x;
			size.y = y;
			size.h = 20;
			y += size.h + border;
			label->setSize(size);
			label->setText("Rotation:");
		}

		// roll
		{
			StringBuf<32> name("editor_FrameComponentRotateX%d", 1, component->getUID());
			Frame* frame = properties.addFrame(name.get(), "editor_FrameComponentRotate");

			Rect<int> size;
			size.x = 0;
			size.w = (width - x) / 3 - border * 2;
			size.y = 0;
			size.h = 30;
			frame->setActualSize(size);
			size.x = border * 2 + x;
			size.w = (width - x) / 3 - border * 2;
			size.y = y;
			size.h = 30;
			frame->setSize(size);
			frame->setColor(WideVector(.25, .25, .25, 1.0));
			frame->setHigh(false);

			Field* field = frame->addField("field", 9);
			size.x = border; size.w = frame->getSize().w - border * 2;
			size.y = border; size.h = frame->getSize().h - border * 2;
			field->setSize(size);
			field->setEditable(true);
			field->setNumbersOnly(true);
			field->setJustify(Field::RIGHT);
			field->setColor(WideVector(1.f, .2f, .2f, 1.f));

			StringBuf<32> dest("editor_FrameComponentRotateY%d", 1, component->getUID());
			field->setWidgetSearchParent(dest.get());
			field->setWidgetTab("field");

			field->getParams().addInt(0);
			field->getParams().addInt(component->getUID());

			char roll[16];
			snprintf(roll, 16, "%.2f", component->getLocalAng().toRotation().degreesRoll());
			field->setText(roll);
		}

		// pitch
		{
			StringBuf<32> name("editor_FrameComponentRotateY%d", 1, component->getUID());
			Frame* frame = properties.addFrame(name.get(), "editor_FrameComponentRotate");

			Rect<int> size;
			size.x = 0;
			size.w = (width - x) / 3 - border * 2;
			size.y = 0;
			size.h = 30;
			frame->setActualSize(size);
			size.x = border * 2 + (width - x) / 3 - border + x;
			size.w = (width - x) / 3 - border * 2;
			size.y = y;
			size.h = 30;
			frame->setSize(size);
			frame->setColor(WideVector(.25, .25, .25, 1.0));
			frame->setHigh(false);

			Field* field = frame->addField("field", 9);
			size.x = border; size.w = frame->getSize().w - border * 2;
			size.y = border; size.h = frame->getSize().h - border * 2;
			field->setSize(size);
			field->setEditable(true);
			field->setNumbersOnly(true);
			field->setJustify(Field::RIGHT);
			field->setColor(WideVector(.2f, 1.f, .2f, 1.f));

			StringBuf<32> dest("editor_FrameComponentRotateZ%d", 1, component->getUID());
			field->setWidgetSearchParent(dest.get());
			field->setWidgetTab("field");

			field->getParams().addInt(1);
			field->getParams().addInt(component->getUID());

			char pitch[16];
			snprintf(pitch, 16, "%.2f", component->getLocalAng().toRotation().degreesPitch());
			field->setText(pitch);
		}

		// yaw
		{
			StringBuf<32> name("editor_FrameComponentRotateZ%d", 1, component->getUID());
			Frame* frame = properties.addFrame(name.get(), "editor_FrameComponentRotate");

			Rect<int> size;
			size.x = 0;
			size.w = (width - x) / 3 - border * 2;
			size.y = 0;
			size.h = 30;
			frame->setActualSize(size);
			size.x = border * 2 + 2 * (width - x) / 3 - border * 2 + x;
			size.w = (width - x) / 3 - border * 2;
			size.y = y;
			size.h = 30;
			y += size.h + border;
			frame->setSize(size);
			frame->setColor(WideVector(.25, .25, .25, 1.0));
			frame->setHigh(false);

			Field* field = frame->addField("field", 9);
			size.x = border; size.w = frame->getSize().w - border * 2;
			size.y = border; size.h = frame->getSize().h - border * 2;
			field->setSize(size);
			field->setEditable(true);
			field->setNumbersOnly(true);
			field->setJustify(Field::RIGHT);
			field->setColor(WideVector(.2f, .2f, 1.f, 1.f));

			StringBuf<32> dest("editor_FrameComponentRotateX%d", 1, component->getUID());
			field->setWidgetSearchParent(dest.get());
			field->setWidgetTab("field");

			field->getParams().addInt(2);
			field->getParams().addInt(component->getUID());

			char yaw[16];
			snprintf(yaw, 16, "%.2f", component->getLocalAng().toRotation().degreesYaw());
			field->setText(yaw);
		}

		// scale label
		{
			Field* label = properties.addField("labelScale", 16);

			Rect<int> size;
			size.x = border * 2 + x;
			size.w = width - x - border * 4;
			size.y = y;
			size.h = 20;
			y += size.h + border;
			label->setSize(size);
			label->setText("Scale:");
		}

		// scale x
		{
			StringBuf<32> name("editor_FrameComponentScaleX%d", 1, component->getUID());
			Frame* frame = properties.addFrame(name.get(), "editor_FrameComponentScale");

			Rect<int> size;
			size.x = 0;
			size.w = (width - x) / 3 - border * 2;
			size.y = 0;
			size.h = 30;
			frame->setActualSize(size);
			size.x = border * 2 + x;
			size.w = (width - x) / 3 - border * 2;
			size.y = y;
			size.h = 30;
			frame->setSize(size);
			frame->setColor(WideVector(.25, .25, .25, 1.0));
			frame->setHigh(false);

			Field* field = frame->addField("field", 9);
			size.x = border; size.w = frame->getSize().w - border * 2;
			size.y = border; size.h = frame->getSize().h - border * 2;
			field->setSize(size);
			field->setEditable(true);
			field->setNumbersOnly(true);
			field->setJustify(Field::RIGHT);
			field->setColor(WideVector(1.f, .2f, .2f, 1.f));

			StringBuf<32> dest("editor_FrameComponentScaleY%d", 1, component->getUID());
			field->setWidgetSearchParent(dest.get());
			field->setWidgetTab("field");

			field->getParams().addInt(0);
			field->getParams().addInt(component->getUID());

			char x[16];
			snprintf(x, 16, "%.2f", component->getLocalScale().x);
			field->setText(x);
		}

		// scale y
		{
			StringBuf<32> name("editor_FrameComponentScaleY%d", 1, component->getUID());
			Frame* frame = properties.addFrame(name.get(), "editor_FrameComponentScale");

			Rect<int> size;
			size.x = 0;
			size.w = (width - x) / 3 - border * 2;
			size.y = 0;
			size.h = 30;
			frame->setActualSize(size);
			size.x = border * 2 + (width - x) / 3 - border + x;
			size.w = (width - x) / 3 - border * 2;
			size.y = y;
			size.h = 30;
			frame->setSize(size);
			frame->setColor(WideVector(.25, .25, .25, 1.0));
			frame->setHigh(false);

			Field* field = frame->addField("field", 9);
			size.x = border; size.w = frame->getSize().w - border * 2;
			size.y = border; size.h = frame->getSize().h - border * 2;
			field->setSize(size);
			field->setEditable(true);
			field->setNumbersOnly(true);
			field->setJustify(Field::RIGHT);
			field->setColor(WideVector(.2f, 1.f, .2f, 1.f));

			StringBuf<32> dest("editor_FrameComponentScaleZ%d", 1, component->getUID());
			field->setWidgetSearchParent(dest.get());
			field->setWidgetTab("field");

			field->getParams().addInt(1);
			field->getParams().addInt(component->getUID());

			char y[16];
			snprintf(y, 16, "%.2f", component->getLocalScale().y);
			field->setText(y);
		}

		// scale z
		{
			StringBuf<32> name("editor_FrameComponentScaleZ%d", 1, component->getUID());
			Frame* frame = properties.addFrame(name.get(), "editor_FrameComponentScale");

			Rect<int> size;
			size.x = 0;
			size.w = (width - x) / 3 - border * 2;
			size.y = 0;
			size.h = 30;
			frame->setActualSize(size);
			size.x = border * 2 + 2 * (width - x) / 3 - border * 2 + x;
			size.w = (width - x) / 3 - border * 2;
			size.y = y;
			size.h = 30;
			y += size.h + border;
			frame->setSize(size);
			frame->setColor(WideVector(.25, .25, .25, 1.0));
			frame->setHigh(false);

			Field* field = frame->addField("field", 9);
			size.x = border; size.w = frame->getSize().w - border * 2;
			size.y = border; size.h = frame->getSize().h - border * 2;
			field->setSize(size);
			field->setEditable(true);
			field->setNumbersOnly(true);
			field->setJustify(Field::RIGHT);
			field->setColor(WideVector(.2f, .2f, 1.f, 1.f));

			StringBuf<32> dest("editor_FrameComponentScaleX%d", 1, component->getUID());
			field->setWidgetSearchParent(dest.get());
			field->setWidgetTab("field");

			field->getParams().addInt(2);
			field->getParams().addInt(component->getUID());

			char z[16];
			snprintf(z, 16, "%.2f", component->getLocalScale().z);
			field->setText(z);
		}

		// do specialized component properties
		Component::type_t type = component->getType();
		for (auto attribute : component->getAttributes()) {
			attribute->createAttributeUI(properties, x, y, width);
		}

		// sub-components
		for (Uint32 c = 0; c < component->getComponents().getSize(); ++c) {
			Component* curr = component->getComponents()[c];
			componentGUI(properties, curr, x, y);
		}

		// add component
		{
			Button* button = properties.addButton("buttonAdd");
			button->setIcon("images/gui/add.png");
			button->setStyle(Button::STYLE_NORMAL);
			button->getParams().addInt(component->getUID());
			button->setBorder(2);
			button->setTooltip("Add a sub-component.");

			Rect<int> size;
			size.x = border * 2 + x; size.w = 30;
			size.y = y; size.h = 30;
			button->setSize(size);
		}

		// delete component
		{
			Button* button = properties.addButton("buttonDelete");
			button->setIcon("images/gui/delete.png");
			button->setStyle(Button::STYLE_NORMAL);
			button->getParams().addInt(component->getUID());
			button->setBorder(2);
			button->setTooltip("Delete this component.");

			Rect<int> size;
			size.x = border * 2 + x + 30 + border; size.w = 30;
			size.y = y; size.h = 30;
			button->setSize(size);
		}

		// copy component
		{
			Button* button = properties.addButton("buttonCopy");
			button->setIcon("images/gui/copy-icon.png");
			button->setStyle(Button::STYLE_NORMAL);
			button->getParams().addInt(component->getUID());
			button->setBorder(2);
			button->setTooltip("Duplicate this component.");

			Rect<int> size;
			size.x = border * 2 + x + 30 + border + 30 + border; size.w = 30;
			size.y = y; size.h = 30;
			button->setSize(size);

			y += size.h + border;
		}

		x -= 30;
	}
}

void Editor::updateGUI(Frame& gui) {
	// quit dialog
	if (mainEngine->isKillSignal()) {
		mainEngine->setKillSignal(false);
		fullscreen = false;

		Frame* topFrame = gui.findFrame("editor_FrameQuit");
		if (!topFrame) {
			topFrame = new Frame(gui, "editor_FrameQuit");

			playSound("editor/warning.wav");

			int xres = Frame::virtualScreenX;
			int yres = Frame::virtualScreenY;

			Rect<int> size;
			size.x = 0; size.w = 400;
			size.y = 0; size.h = 150;
			topFrame->setActualSize(size);
			size.x = xres / 2 - 200; size.w = 400;
			size.y = yres / 2 - 75; size.h = 150;
			topFrame->setSize(size);
			topFrame->setColor(WideVector(.5f, .5f, .5f, 1.f));

			// x button
			{
				Button* button = new Button(*topFrame);

				Rect<int> buttonRect;
				buttonRect.x = size.w - 36; buttonRect.w = 30;
				buttonRect.y = 6; buttonRect.h = 30;
				button->setSize(buttonRect);
				button->setName("buttonClose");
				button->setText("x");
			}

			// yes button
			{
				Button* button = new Button(*topFrame);

				Rect<int> buttonRect;
				buttonRect.x = 6; buttonRect.w = 90;
				buttonRect.y = size.h - 36; buttonRect.h = 30;
				button->setSize(buttonRect);
				button->setName("buttonYes");
				button->setText("Yes");
			}

			// no button
			{
				Button* button = new Button(*topFrame);

				Rect<int> buttonRect;
				buttonRect.x = size.w - size.w / 2 - 45; buttonRect.w = 90;
				buttonRect.y = size.h - 36; buttonRect.h = 30;
				button->setSize(buttonRect);
				button->setName("buttonNo");
				button->setText("No");
			}

			// cancel button
			{
				Button* button = new Button(*topFrame);

				Rect<int> buttonRect;
				buttonRect.x = size.w - 96; buttonRect.w = 90;
				buttonRect.y = size.h - 36; buttonRect.h = 30;
				button->setSize(buttonRect);
				button->setName("buttonClose");
				button->setText("Cancel");
			}

			// window title
			{
				Field* field = topFrame->addField("editor_FrameQuitTitle", 32);

				Rect<int> rect;
				rect.x = 12; rect.w = 150;
				rect.y = 12; rect.h = 30;
				field->setSize(rect);
				field->setText("Quit Editor");
			}

			// top warning text
			{
				Field* field = topFrame->addField("editor_FrameQuitWarningTop", 64);

				Rect<int> rect;
				rect.x = 3; rect.w = size.w - 6;
				rect.y = 3; rect.h = size.h - 6 - 15;
				field->setSize(rect);
				field->setJustify(Field::CENTER);
				field->setText("Any unsaved changes will be discarded!");
			}

			// bottom warning text
			{
				Field* field = topFrame->addField("editor_FrameQuitWarningBottom", 32);

				Rect<int> rect;
				rect.x = 3; rect.w = size.w - 6;
				rect.y = 3 + 15; rect.h = size.h - 6 - 15;
				field->setSize(rect);
				field->setJustify(Field::CENTER);
				field->setText("Would you like to save?");
			}
		}
	}

	// update the mini console
	{
		Frame* console = gui.findFrame("editor_FrameBottomMiniConsole");
		if (console) {
			int miniSize = console->getEntries().getSize();
			int mainSize = client->getConsole().getSize();
			if (miniSize != mainSize) {
				while (console->getEntries().getFirst()) {
					delete console->getEntries().getFirst()->getData();
					console->getEntries().removeNode(console->getEntries().getFirst());
				}
				const Node<Engine::logmsg_t>* node;
				for (node = client->getConsole().nodeForIndex(std::max(0, mainSize - 100)); node != nullptr; node = node->getNext()) {
					const Engine::logmsg_t& logMsg = node->getData();
					Frame::entry_t* entry = console->addEntry("", false);
					entry->text = logMsg.text.get();
					entry->color = WideVector(logMsg.color.x, logMsg.color.y, logMsg.color.z, 1.f);
				}
				console->resizeForEntries();
				Rect<int> actualSize = console->getActualSize();
				actualSize.y = actualSize.h - console->getSize().h;
				console->setActualSize(actualSize);
			}
		}
	}

	// update the entity properties panel
	{
		Frame* properties = gui.findFrame("editor_FrameEntityProperties");
		if (properties) {
			LinkedList<Entity*> selectedEntities;
			world->findSelectedEntities(selectedEntities);

			// determine if the frame needs to be updated
			if (selectedEntities.getSize() != selectedEntityManifest.getSize()) {
				guiNeedsUpdate = true;
			} else {
				Node<Entity*>* entityNode;
				Node<Uint32>* uidNode;
				for (entityNode = selectedEntities.getFirst(), uidNode = selectedEntityManifest.getFirst();
					entityNode != nullptr && uidNode != nullptr;
					entityNode = entityNode->getNext(), uidNode = uidNode->getNext()) {
					Entity* entity = entityNode->getData();
					Uint32 uid = uidNode->getData();

					if (entity->getUID() != uid) {
						guiNeedsUpdate = true;
						break;
					}
				}
			}

			// update the frame if necessary
			if (guiNeedsUpdate) {
				guiNeedsUpdate = false;

				// clear existing elements
				properties->clear();

				// update the manifest of selected entities
				selectedEntityManifest.removeAll();
				for (Node<Entity*>* node = selectedEntities.getFirst(); node != nullptr; node = node->getNext()) {
					Entity* entity = node->getData();

					Uint32 uid = entity->getUID();
					selectedEntityManifest.addNodeLast(uid);
				}

				// update the frame itself
				if (selectedEntities.getSize() == 0) {
					Rect<int> newActualSize = properties->getSize();
					newActualSize.x = 0;
					newActualSize.y = 0;
					properties->setActualSize(newActualSize);

					Field* field = properties->addField("text1", 26);
					field->setText("No entities selected!");
					field->setSize(properties->getActualSize());
					field->setJustify(Field::CENTER);
				} else {
					Entity* firstEntity = selectedEntities.getFirst()->getData();

					// first determine if we've got multiple entities selected...
					// if we do, only general entity properties are editable (no components)
					bool selectedMultiple = selectedEntities.getSize() > 1;

					// add the general properties!
					int border = properties->getBorder();
					int y = border * 2;
					int width = properties->getSize().w - Frame::sliderSize;

					// name
					{
						Frame* frame = properties->addFrame("editor_FrameEntityPropertiesName");

						Rect<int> size;
						size.x = 0; size.w = width - border * 4;
						size.y = 0; size.h = 30;
						frame->setActualSize(size);
						size.x = border * 2; size.w = width - border * 4;
						size.y = y; size.h = 30;
						y += size.h + border;
						frame->setSize(size);
						frame->setColor(WideVector(.25, .25, .25, 1.0));
						frame->setHigh(false);

						Field* field = frame->addField("field", 64);
						size.x = border; size.w = frame->getSize().w - border * 2;
						size.y = border; size.h = frame->getSize().h - border * 2;
						field->setSize(size);
						field->setEditable(true);

						if (selectedEntities.getSize() > 1) {
							field->setText("...");
						} else {
							field->setText(firstEntity->getName().get());
						}
					}

					// position label
					{
						Field* label = properties->addField("labelPosition", 16);

						Rect<int> size;
						size.x = border * 2;
						size.w = width - border * 4;
						size.y = y;
						size.h = 20;
						y += size.h + border;
						label->setSize(size);
						label->setText("Position:");
					}

					// translate x
					{
						Frame* frame = properties->addFrame("editor_FrameEntityPropertiesTranslateX");

						Rect<int> size;
						size.x = 0;
						size.w = width / 3 - border * 2;
						size.y = 0;
						size.h = 30;
						frame->setActualSize(size);
						size.x = border * 2;
						size.w = width / 3 - border * 2;
						size.y = y;
						size.h = 30;
						frame->setSize(size);
						frame->setColor(WideVector(.25, .25, .25, 1.0));
						frame->setHigh(false);

						Field* field = frame->addField("field", 9);
						size.x = border; size.w = frame->getSize().w - border * 2;
						size.y = border; size.h = frame->getSize().h - border * 2;
						field->setSize(size);
						field->setEditable(true);
						field->setNumbersOnly(true);
						field->setJustify(Field::RIGHT);
						field->setColor(WideVector(1.f, .2f, .2f, 1.f));
						field->setWidgetSearchParent("editor_FrameEntityPropertiesTranslateY");
						field->setWidgetTab("field");

						if (selectedEntities.getSize() > 1) {
							field->setText("...");
						} else {
							char x[16];
							snprintf(x, 16, "%.2f", firstEntity->getPos().x);
							field->setText(x);
						}
					}

					// translate y
					{
						Frame* frame = properties->addFrame("editor_FrameEntityPropertiesTranslateY");

						Rect<int> size;
						size.x = 0;
						size.w = width / 3 - border * 2;
						size.y = 0;
						size.h = 30;
						frame->setActualSize(size);
						size.x = border * 2 + width / 3 - border;
						size.w = width / 3 - border * 2;
						size.y = y;
						size.h = 30;
						frame->setSize(size);
						frame->setColor(WideVector(.25, .25, .25, 1.0));
						frame->setHigh(false);

						Field* field = frame->addField("field", 9);
						size.x = border; size.w = frame->getSize().w - border * 2;
						size.y = border; size.h = frame->getSize().h - border * 2;
						field->setSize(size);
						field->setEditable(true);
						field->setNumbersOnly(true);
						field->setJustify(Field::RIGHT);
						field->setColor(WideVector(.2f, 1.f, .2f, 1.f));
						field->setWidgetSearchParent("editor_FrameEntityPropertiesTranslateZ");
						field->setWidgetTab("field");

						if (selectedEntities.getSize() > 1) {
							field->setText("...");
						} else {
							char y[16];
							snprintf(y, 16, "%.2f", firstEntity->getPos().y);
							field->setText(y);
						}
					}

					// translate z
					{
						Frame* frame = properties->addFrame("editor_FrameEntityPropertiesTranslateZ");

						Rect<int> size;
						size.x = 0;
						size.w = width / 3 - border * 2;
						size.y = 0;
						size.h = 30;
						frame->setActualSize(size);
						size.x = border * 2 + 2 * width / 3 - border * 2;
						size.w = width / 3 - border * 2;
						size.y = y;
						size.h = 30;
						y += size.h + border;
						frame->setSize(size);
						frame->setColor(WideVector(.25, .25, .25, 1.0));
						frame->setHigh(false);

						Field* field = frame->addField("field", 9);
						size.x = border; size.w = frame->getSize().w - border * 2;
						size.y = border; size.h = frame->getSize().h - border * 2;
						field->setSize(size);
						field->setEditable(true);
						field->setNumbersOnly(true);
						field->setJustify(Field::RIGHT);
						field->setColor(WideVector(.2f, .2f, 1.f, 1.f));
						field->setWidgetSearchParent("editor_FrameEntityPropertiesTranslateX");
						field->setWidgetTab("field");

						if (selectedEntities.getSize() > 1) {
							field->setText("...");
						} else {
							char z[16];
							snprintf(z, 16, "%.2f", firstEntity->getPos().z);
							field->setText(z);
						}
					}

					// rotation label
					{
						Field* label = properties->addField("labelRotation", 16);

						Rect<int> size;
						size.x = border * 2;
						size.w = width - border * 4;
						size.y = y;
						size.h = 20;
						y += size.h + border;
						label->setSize(size);
						label->setText("Rotation:");
					}

					// roll
					{
						Frame* frame = properties->addFrame("editor_FrameEntityPropertiesRoll");

						Rect<int> size;
						size.x = 0;
						size.w = width / 3 - border * 2;
						size.y = 0;
						size.h = 30;
						frame->setActualSize(size);
						size.x = border * 2;
						size.w = width / 3 - border * 2;
						size.y = y;
						size.h = 30;
						frame->setSize(size);
						frame->setColor(WideVector(.25, .25, .25, 1.0));
						frame->setHigh(false);

						Field* field = frame->addField("field", 9);
						size.x = border; size.w = frame->getSize().w - border * 2;
						size.y = border; size.h = frame->getSize().h - border * 2;
						field->setSize(size);
						field->setEditable(true);
						field->setNumbersOnly(true);
						field->setJustify(Field::RIGHT);
						field->setColor(WideVector(1.f, .2f, .2f, 1.f));
						field->setWidgetSearchParent("editor_FrameEntityPropertiesPitch");
						field->setWidgetTab("field");

						if (selectedEntities.getSize() > 1) {
							field->setText("...");
						} else {
							char roll[16];
							snprintf(roll, 16, "%.2f", firstEntity->getAng().toRotation().degreesRoll());
							field->setText(roll);
						}
					}

					// pitch
					{
						Frame* frame = properties->addFrame("editor_FrameEntityPropertiesPitch");

						Rect<int> size;
						size.x = 0;
						size.w = width / 3 - border * 2;
						size.y = 0;
						size.h = 30;
						frame->setActualSize(size);
						size.x = border * 2 + width / 3 - border;
						size.w = width / 3 - border * 2;
						size.y = y;
						size.h = 30;
						frame->setSize(size);
						frame->setColor(WideVector(.25, .25, .25, 1.0));
						frame->setHigh(false);

						Field* field = frame->addField("field", 9);
						size.x = border; size.w = frame->getSize().w - border * 2;
						size.y = border; size.h = frame->getSize().h - border * 2;
						field->setSize(size);
						field->setEditable(true);
						field->setNumbersOnly(true);
						field->setJustify(Field::RIGHT);
						field->setColor(WideVector(.2f, 1.f, .2f, 1.f));
						field->setWidgetSearchParent("editor_FrameEntityPropertiesYaw");
						field->setWidgetTab("field");

						if (selectedEntities.getSize() > 1) {
							field->setText("...");
						} else {
							char pitch[16];
							snprintf(pitch, 16, "%.2f", firstEntity->getAng().toRotation().degreesPitch());
							field->setText(pitch);
						}
					}

					// yaw
					{
						Frame* frame = properties->addFrame("editor_FrameEntityPropertiesYaw");

						Rect<int> size;
						size.x = 0;
						size.w = width / 3 - border * 2;
						size.y = 0;
						size.h = 30;
						frame->setActualSize(size);
						size.x = border * 2 + 2 * width / 3 - border * 2;
						size.w = width / 3 - border * 2;
						size.y = y;
						size.h = 30;
						y += size.h + border;
						frame->setSize(size);
						frame->setColor(WideVector(.25, .25, .25, 1.0));
						frame->setHigh(false);

						Field* field = frame->addField("field", 9);
						size.x = border; size.w = frame->getSize().w - border * 2;
						size.y = border; size.h = frame->getSize().h - border * 2;
						field->setSize(size);
						field->setEditable(true);
						field->setNumbersOnly(true);
						field->setJustify(Field::RIGHT);
						field->setColor(WideVector(.2f, .2f, 1.f, 1.f));
						field->setWidgetSearchParent("editor_FrameEntityPropertiesRoll");
						field->setWidgetTab("field");

						if (selectedEntities.getSize() > 1) {
							field->setText("...");
						} else {
							char yaw[16];
							snprintf(yaw, 16, "%.2f", firstEntity->getAng().toRotation().degreesYaw());
							field->setText(yaw);
						}
					}

					// scale label
					{
						Field* label = properties->addField("labelScale", 16);

						Rect<int> size;
						size.x = border * 2;
						size.w = width - border * 4;
						size.y = y;
						size.h = 20;
						y += size.h + border;
						label->setSize(size);
						label->setText("Scale:");
					}

					// scale x
					{
						Frame* frame = properties->addFrame("editor_FrameEntityPropertiesScaleX");

						Rect<int> size;
						size.x = 0;
						size.w = width / 3 - border * 2;
						size.y = 0;
						size.h = 30;
						frame->setActualSize(size);
						size.x = border * 2;
						size.w = width / 3 - border * 2;
						size.y = y;
						size.h = 30;
						frame->setSize(size);
						frame->setColor(WideVector(.25, .25, .25, 1.0));
						frame->setHigh(false);

						Field* field = frame->addField("field", 9);
						size.x = border; size.w = frame->getSize().w - border * 2;
						size.y = border; size.h = frame->getSize().h - border * 2;
						field->setSize(size);
						field->setEditable(true);
						field->setNumbersOnly(true);
						field->setJustify(Field::RIGHT);
						field->setColor(WideVector(1.f, .2f, .2f, 1.f));
						field->setWidgetSearchParent("editor_FrameEntityPropertiesScaleY");
						field->setWidgetTab("field");

						if (selectedEntities.getSize() > 1) {
							field->setText("...");
						} else {
							char x[16];
							snprintf(x, 16, "%.2f", firstEntity->getScale().x);
							field->setText(x);
						}
					}

					// scale y
					{
						Frame* frame = properties->addFrame("editor_FrameEntityPropertiesScaleY");

						Rect<int> size;
						size.x = 0;
						size.w = width / 3 - border * 2;
						size.y = 0;
						size.h = 30;
						frame->setActualSize(size);
						size.x = border * 2 + width / 3 - border;
						size.w = width / 3 - border * 2;
						size.y = y;
						size.h = 30;
						frame->setSize(size);
						frame->setColor(WideVector(.25, .25, .25, 1.0));
						frame->setHigh(false);

						Field* field = frame->addField("field", 9);
						size.x = border; size.w = frame->getSize().w - border * 2;
						size.y = border; size.h = frame->getSize().h - border * 2;
						field->setSize(size);
						field->setEditable(true);
						field->setNumbersOnly(true);
						field->setJustify(Field::RIGHT);
						field->setColor(WideVector(.2f, 1.f, .2f, 1.f));
						field->setWidgetSearchParent("editor_FrameEntityPropertiesScaleZ");
						field->setWidgetTab("field");

						if (selectedEntities.getSize() > 1) {
							field->setText("...");
						} else {
							char y[16];
							snprintf(y, 16, "%.2f", firstEntity->getScale().y);
							field->setText(y);
						}
					}

					// scale z
					{
						Frame* frame = properties->addFrame("editor_FrameEntityPropertiesScaleZ");

						Rect<int> size;
						size.x = 0;
						size.w = width / 3 - border * 2;
						size.y = 0;
						size.h = 30;
						frame->setActualSize(size);
						size.x = border * 2 + 2 * width / 3 - border * 2;
						size.w = width / 3 - border * 2;
						size.y = y;
						size.h = 30;
						y += size.h + border;
						frame->setSize(size);
						frame->setColor(WideVector(.25, .25, .25, 1.0));
						frame->setHigh(false);

						Field* field = frame->addField("field", 9);
						size.x = border; size.w = frame->getSize().w - border * 2;
						size.y = border; size.h = frame->getSize().h - border * 2;
						field->setSize(size);
						field->setEditable(true);
						field->setNumbersOnly(true);
						field->setJustify(Field::RIGHT);
						field->setColor(WideVector(.2f, .2f, 1.f, 1.f));
						field->setWidgetSearchParent("editor_FrameEntityPropertiesScaleX");
						field->setWidgetTab("field");

						if (selectedEntities.getSize() > 1) {
							field->setText("...");
						} else {
							char z[16];
							snprintf(z, 16, "%.2f", firstEntity->getScale().z);
							field->setText(z);
						}
					}

					// script label
					{
						Field* label = properties->addField("labelScript", 16);

						Rect<int> size;
						size.x = border * 2;
						size.w = width - border * 4;
						size.y = y;
						size.h = 20;
						y += size.h + border;
						label->setSize(size);
						label->setText("Script:");
					}

					// script
					Field* scriptField = nullptr;
					{
						Frame* frame = properties->addFrame("editor_FrameEntityPropertiesScript");

						Rect<int> size;
						size.x = 0; size.w = width - border * 4 - 30 - border;
						size.y = 0; size.h = 30;
						frame->setActualSize(size);
						size.x = border * 2; size.w = width - border * 4 - 30 - border;
						size.y = y; size.h = 30;
						frame->setSize(size);
						frame->setColor(WideVector(.25, .25, .25, 1.0));
						frame->setHigh(false);

						Field* field = scriptField = frame->addField("field", 64);
						size.x = border; size.w = frame->getSize().w - border * 2;
						size.y = border; size.h = frame->getSize().h - border * 2;
						field->setSize(size);
						field->setEditable(true);

						if (selectedEntities.getSize() > 1) {
							field->setText("...");
						} else {
							field->setText(firstEntity->getScriptStr());
						}
					}

					// button
					{
						class ScriptButtonCallback : public Script::Function {
						public:
							ScriptButtonCallback(Entity* _entity, Field* _field) :
								entity(_entity),
								field(_field)
							{}
							virtual ~ScriptButtonCallback() {}
							virtual int operator()(Script::Args& args) const override {
								String result = mainEngine->fileOpenDialog("lua", nullptr);
								if (!result.empty()) {
									String value = result.get();

									// cut out slashes
									Uint32 i = 0;
									do {
										i = value.find('/', 0);
										if (i != String::npos) {
											value = value.substr(i + 1);
										}
									} while (i != String::npos);

#ifdef PLATFORM_WINDOWS
									// windows has to cut out backward slashes, too
									i = 0;
									do {
										i = value.find('\\', 0);
										if (i != String::npos) {
											value = value.substr(i + 1);
										}
									} while (i != String::npos);
#endif

									// remove suffix
									Uint32 offset = value.find(".lua");
									if (offset != String::npos) {
										value[offset] = '\0';
									}
									entity->setScriptStr(value.get());
									field->setText(value.get());
								}
								return result.empty() ? 1 : 0;
							}
						private:
							Entity* entity = nullptr;
							Field* field = nullptr;
						};

						Button* button = properties->addButton("");
						button->setBorder(1);
						button->setIcon("images/gui/open.png");
						button->setStyle(Button::STYLE_NORMAL);
						button->setCallback(new ScriptButtonCallback(firstEntity, scriptField));

						Rect<int> size;
						size.x = border * 2 + (width - border * 4 - 30); size.w = 30;
						size.y = y; size.h = 30;
						button->setSize(size);

						y += size.h + border;
					}

					// flags label
					{
						Field* label = properties->addField("labelFlags", 16);

						Rect<int> size;
						size.x = border * 2;
						size.w = width - border * 4;
						size.y = y;
						size.h = 20;
						y += size.h + border;
						label->setSize(size);
						label->setText("Flags:");
					}

					// do flags...
					{
						for (int c = 0; c < static_cast<int>(Entity::flag_t::FLAG_NUM); ++c) {
							Button* button = properties->addButton("buttonFlag");
							button->setBorder(1);
							button->setIcon("images/gui/checkmark.png");
							button->setStyle(Button::STYLE_CHECKBOX);
							button->setPressed(firstEntity->isFlag(static_cast<Entity::flag_t>((int)floor(pow(2, c)))));
							button->setTooltip(Entity::flagDesc[c]);
							button->getParams().addInt(1 << c);

							Rect<int> size;
							size.x = border * 2; size.w = 30;
							size.y = y; size.h = 30;
							button->setSize(size);

							// label
							{
								Field* label = properties->addField(StringBuf<64>("labelFlag%s", 1, Entity::flagStr[c]).get(), 16);

								Rect<int> size;
								size.x = border * 2 + 30 + border;
								size.w = width - border * 4 - 30 - border;
								size.y = y + 5;
								size.h = 30;
								label->setSize(size);
								label->setText(Entity::flagStr[c]);
							}

							y += size.h + border;
						}
					}

					// add unique properties for each kind of entity
					if (!selectedMultiple) {
						// key value label
						{
							Field* label = properties->addField("labelKeyValue", 24);

							Rect<int> size;
							size.x = border * 2;
							size.w = width - border * 4;
							size.y = y;
							size.h = 20;
							y += size.h + border;
							label->setSize(size);
							label->setText("Enter Key:Value pair:");
						}

						// key value
						{
							Frame* frame = properties->addFrame("editor_FrameEntityPropertiesKeyValue");

							Rect<int> size;
							size.x = 0; size.w = width - border * 4;
							size.y = 0; size.h = 30;
							frame->setActualSize(size);
							size.x = border * 2; size.w = width - border * 4;
							size.y = y; size.h = 30;
							y += size.h + border;
							frame->setSize(size);
							frame->setColor(WideVector(.25, .25, .25, 1.0));
							frame->setHigh(false);

							Field* field = frame->addField("field", 128);
							size.x = border; size.w = frame->getSize().w - border * 2;
							size.y = border; size.h = frame->getSize().h - border * 2;
							field->setSize(size);
							field->setEditable(true);
						}

						// key values list label
						{
							Field* label = properties->addField("labelKeyValuesList", 20);

							Rect<int> size;
							size.x = border * 2;
							size.w = width - border * 4;
							size.y = y;
							size.h = 20;
							y += size.h + border;
							label->setSize(size);
							label->setText("Key:Value pairs:");
						}

						// key values list
						{
							Frame* frame = properties->addFrame("editor_FrameEntityPropertiesKeyValuesList");

							Rect<int> size;
							size.x = 0; size.w = width - border * 4;
							size.y = 0; size.h = 150;
							frame->setActualSize(size);
							size.x = border * 2; size.w = width - border * 4;
							size.y = y; size.h = 150;
							y += size.h + border;
							frame->setSize(size);
							frame->setColor(WideVector(.25, .25, .25, 1.0));
							frame->setHigh(false);
							frame->setBorder(0);

							// list of key values
							StringBuf<64> text;
							for (auto& pair : firstEntity->getKeyValues()) {
								Frame::entry_t* entry = frame->addEntry("entry", true);
								text.format("%s:%s", pair.a.get(), pair.b.get());
								entry->text = text.get();
								entry->color = WideVector(1.f);
								entry->params.addString(text);
							}
						}

						// add sub-components
						int x = 0;
						for (Uint32 c = 0; c < firstEntity->getComponents().getSize(); ++c) {
							Component* component = firstEntity->getComponents()[c];
							componentGUI(*properties, component, x, y);
						}

						// add component button
						{
							Button* button = properties->addButton("buttonAdd");
							button->setIcon("images/gui/add.png");
							button->setStyle(Button::STYLE_NORMAL);
							button->getParams().addInt(0);
							button->setBorder(2);
							button->setTooltip("Add a sub-component.");

							Rect<int> size;
							size.x = border * 2 + x; size.w = 30;
							size.y = y; size.h = 30;
							button->setSize(size);
						}

						// save entity definition button
						{
							Button* button = properties->addButton("buttonSave");
							button->setIcon("images/gui/save.png");
							button->setStyle(Button::STYLE_NORMAL);
							button->setBorder(2);
							button->setTooltip("Export an entity definition.");

							Rect<int> size;
							size.x = border * 2 + x + 30 + 2; size.w = 30;
							size.y = y; size.h = 30;
							button->setSize(size);
						}

						y += 30 + border;
					}

					const Rect<int>& propSize = properties->getSize();
					Rect<int> newActualSize = properties->getActualSize();
					newActualSize.h = y + border;
					newActualSize.y = min(newActualSize.y, newActualSize.h - propSize.h);
					properties->setActualSize(newActualSize);
				}
			}
		} else { // if( properties )
			// clear selected entity manifest
			selectedEntityManifest.removeAll();
			selectedEntityManifest.addNodeLast(World::nuid);
		}
	}

	// change widget mode
	if (mainEngine->getInputStr() == nullptr) {
		if (mainEngine->pressKey(SDL_SCANCODE_SPACE)) {
			switch (widgetMode) {
			case TRANSLATE:
				widgetMode = ROTATE;
				break;
			case ROTATE:
				widgetMode = SCALE;
				break;
			case SCALE:
				widgetMode = TRANSLATE;
				break;
			default:
				widgetMode = TRANSLATE;
				break;
			}
		}
	}

	// update x widget property field
	{
		const char* translateImg = "images/gui/icon_translate-x.png";
		const char* rotateImg = "images/gui/icon_rotate-x.png";
		const char* scaleImg = "images/gui/icon_scale-x.png";

		Frame* frame = gui.findFrame("editor_FramePanelX");
		if (frame) {
			Field* field = frame->findField("propertyX");
			if (field && !field->isSelected()) {

				// translate mode
				if (widgetMode == TRANSLATE) {
					char buf[16];
					snprintf(buf, 16, "%.2f", widgetPos.x);
					field->setText(buf);
				}

				// rotate mode
				else if (widgetMode == ROTATE) {
					char buf[16];
					snprintf(buf, 16, "%.2f", widgetAng.degreesRoll());
					field->setText(buf);
				}

				// scale mode
				else if (widgetMode == SCALE) {
					char buf[16];
					snprintf(buf, 16, "%.2f", widgetScale.x);
					field->setText(buf);
				}
			}

			Frame* parent = frame->getParent();
			if (parent) {
				updateWidgetImages(parent, translateImg, rotateImg, scaleImg);
			}
		}
	}

	// update y widget property field
	{
		const char* translateImg = "images/gui/icon_translate-y.png";
		const char* rotateImg = "images/gui/icon_rotate-y.png";
		const char* scaleImg = "images/gui/icon_scale-y.png";

		Frame* frame = gui.findFrame("editor_FramePanelY");
		if (frame) {
			Field* field = frame->findField("propertyY");
			if (field && !field->isSelected()) {

				// translate mode
				if (widgetMode == TRANSLATE) {
					char buf[16];
					snprintf(buf, 16, "%.2f", widgetPos.y);
					field->setText(buf);
				}

				// rotate mode
				else if (widgetMode == ROTATE) {
					char buf[16];
					snprintf(buf, 16, "%.2f", widgetAng.degreesPitch());
					field->setText(buf);
				}

				// scale mode
				else if (widgetMode == SCALE) {
					char buf[16];
					snprintf(buf, 16, "%.2f", widgetScale.y);
					field->setText(buf);
				}
			}

			Frame* parent = frame->getParent();
			if (parent) {
				updateWidgetImages(parent, translateImg, rotateImg, scaleImg);
			}
		}
	}

	// update z widget property field
	{
		const char* translateImg = "images/gui/icon_translate-z.png";
		const char* rotateImg = "images/gui/icon_rotate-z.png";
		const char* scaleImg = "images/gui/icon_scale-z.png";

		Frame* frame = gui.findFrame("editor_FramePanelZ");
		if (frame) {
			Field* field = frame->findField("propertyZ");
			if (field && !field->isSelected()) {

				// translate mode
				if (widgetMode == TRANSLATE) {
					char buf[16];
					snprintf(buf, 16, "%.2f", widgetPos.z);
					field->setText(buf);
				}

				// rotate mode
				else if (widgetMode == ROTATE) {
					char buf[16];
					snprintf(buf, 16, "%.2f", widgetAng.degreesYaw());
					field->setText(buf);
				}

				// scale mode
				else if (widgetMode == SCALE) {
					char buf[16];
					snprintf(buf, 16, "%.2f", widgetScale.z);
					field->setText(buf);
				}
			}

			Frame* parent = frame->getParent();
			if (parent) {
				updateWidgetImages(parent, translateImg, rotateImg, scaleImg);
			}
		}
	}

	// update widgets
	if (editingCamera) {
		widgetVisible = false;

		// place the widget at the location average of all selected objects
		if (editingMode == ENTITIES) {
			LinkedList<Entity*> selectedEntities;
			world->getSelectedEntities(selectedEntities);
			if (selectedEntities.getSize() > 0) {
				Vector average;
				for (Node<Entity*>* node = selectedEntities.getFirst(); node != nullptr; node = node->getNext()) {
					Entity* entity = node->getData();
					average += entity->getPos();
				}
				average /= selectedEntities.getSize();
				widgetPos = average;
				if (!leftClicking) {
					oldWidgetPos = widgetPos;
				}
				widgetVisible = true;
			}
		}

		// set widget direction to face the camera
		const Vector& camPos = editingCamera->getGlobalPos();
		float fDir = atan2(camPos.y - widgetPos.y, camPos.x - widgetPos.x) * (180.f / PI);
		if (fDir < 0.f) {
			fDir += 360.f;
		}
		int iDir = floor(fDir / 90.f);

		// update all the widget entities
		int i;
		Node<Entity*>* node;
		for (i = 0, node = widgetActors.getFirst(); node != nullptr; node = node->getNext(), ++i) {
			Entity* entity = node->getData();

			// get components
			Model* model = entity->findComponentByName<Model>("model");

			// check visibility for particular entities and update meshes
			bool overrideVisible = false;
			if (widgetMode == TRANSLATE) {
				if (entity->getName().length() != 6) {
					overrideVisible = true;
				}

				// set primary axis mesh
				if (entity->getName().length() == 7) {
					const String str = model->getMesh();

					if (str != "assets/editor/gizmo/gizmo_translate_1-axis.FBX") {
						model->setMesh("assets/editor/gizmo/gizmo_translate_1-axis.FBX");
					}
				}

				// set dual-axis mesh
				if (entity->getName().length() == 8) {
					const String str = model->getMesh();

					if (str != "assets/editor/gizmo/gizmo_translate_2-axis.FBX") {
						model->setMesh("assets/editor/gizmo/gizmo_translate_2-axis.FBX");
					}
				}
			} else if (widgetMode == ROTATE) {
				if (entity->getName().length() == 7) {
					overrideVisible = true;
				}

				// set primary axis mesh
				if (entity->getName().length() == 7) {
					const String str = model->getMesh();

					if (str != "assets/editor/gizmo/gizmo_rotate_1-axis.FBX") {
						model->setMesh("assets/editor/gizmo/gizmo_rotate_1-axis.FBX");
					}
				}
			} else if (widgetMode == SCALE) {
				overrideVisible = true;

				// set all-axis mesh
				if (entity->getName().length() == 6) {
					const String str = model->getMesh();

					if (str != "assets/editor/gizmo/gizmo_scale_3-axis.FBX") {
						model->setMesh("assets/editor/gizmo/gizmo_scale_3-axis.FBX");
					}
				}

				// set primary axis mesh
				if (entity->getName().length() == 7) {
					const String str = model->getMesh();

					if (str != "assets/editor/gizmo/gizmo_scale_1-axis.FBX") {
						model->setMesh("assets/editor/gizmo/gizmo_scale_1-axis.FBX");
					}
				}

				// set dual-axis mesh
				if (entity->getName().length() == 8) {
					const String str = model->getMesh();

					if (str != "assets/editor/gizmo/gizmo_scale_2-axis.FBX") {
						model->setMesh("assets/editor/gizmo/gizmo_scale_2-axis.FBX");
					}
				}
			}
			bool visible = widgetVisible & overrideVisible;

			if (!mainEngine->getMouseStatus(SDL_BUTTON_LEFT)) {
				entity->setHighlighted(false);
			}

			entity->setPos(widgetPos);

			float dist = (widgetPos - editingCamera->getGlobalPos()).length();
			Vector widgetScale = Vector(dist / 200.f);

			entity->setScale(widgetScale);

			if (visible) {
				entity->setFlag(static_cast<int>(Entity::flag_t::FLAG_VISIBLE));
				if (entityToSpawn == nullptr) {
					entity->setFlag(static_cast<int>(Entity::flag_t::FLAG_ALLOWTRACE));
				} else {
					entity->resetFlag(static_cast<int>(Entity::flag_t::FLAG_ALLOWTRACE));
				}
				entity->update();
			} else {
				entity->resetFlag(static_cast<int>(Entity::flag_t::FLAG_VISIBLE));
				entity->resetFlag(static_cast<int>(Entity::flag_t::FLAG_ALLOWTRACE));
			}
		}
	}

	// update level navigator
	Frame* levelList = gui.findFrame("editor_FrameLevelNavigatorList");
	if (levelList) {
		levelList->resizeForEntries();
	}
}

static Cvar cvar_testPointer("editor.test.worldPosToScreenPos.enabled", "", "0");

void Editor::draw(Renderer& renderer) {
	Font* font = mainEngine->getFontResource().dataForString(Font::defaultFont);

	if (editingCamera) {
		const Camera* camera = editingCamera;
		const Rect<int>& rect = camera->getWin();

		// show edit mode
		Rect<int> pos;
		pos.x = rect.x + 18; pos.w = 0;
		pos.y = rect.y + 18; pos.h = 0;
		if (!fullscreen) {
			if (world->isShowTools()) {
				if (cvar_showMatrix.toInt()) {
					const glm::mat4& mat = editingCamera->getGlobalMat();
					char matrixChars[256];
					snprintf(matrixChars, 256,
						"%+07.1f %+07.1f %+07.1f %+07.1f\n"
						"%+07.1f %+07.1f %+07.1f %+07.1f\n"
						"%+07.1f %+07.1f %+07.1f %+07.1f\n"
						"%+07.1f %+07.1f %+07.1f %+07.1f\n",
						mat[0][0], mat[0][1], mat[0][2], mat[0][3],
						mat[1][0], mat[1][1], mat[1][2], mat[1][3],
						mat[2][0], mat[2][1], mat[2][2], mat[2][3],
						mat[3][0], mat[3][1], mat[3][2], mat[3][3]
					);
					/*char matrixChars[256];
					snprintf(matrixChars, 256, "%.2f %.2f %.2f"
						, editingCamera->getGlobalAng().yaw
						, editingCamera->getGlobalAng().pitch
						, editingCamera->getGlobalAng().roll);*/
					renderer.printText(font, pos, matrixChars);
				} else {
					//renderer.printText(pos, Editor::editingModeStr[editingMode]);
				}
			} else {
				renderer.printText(font, pos, "*** PREVIEW ***");
			}
		}

		// fps counter
		if (cvar_showFPS.toInt() && font) {
			char fps[16];
			snprintf(fps, 16, "%.2f", mainEngine->getFPS());
			Rect<int> pos;
			int width;
			font->sizeText(fps, &width, NULL);
			pos.x = rect.x + rect.w - 18 - width; pos.w = 0;
			pos.y = rect.y + 18; pos.h = 0;
			renderer.printText(font, pos, fps);
		}

		// mark pointer (test worldPosToScreenPos)
		if (cvar_testPointer.toInt()) {
			Vector diff = world->getPointerPos() - camera->getGlobalPos();
			float dot = diff.dot(camera->getGlobalAng().toVector());
			if (dot > 0) {
				Vector proj = camera->worldPosToScreenPos(world->getPointerPos());
				Rect<int> pos;
				pos.x = proj.x-4; pos.y = proj.y-4;
				pos.w = 8; pos.h = 8;
				if (camera->getWin().containsPoint(pos.x,pos.y)) {
					renderer.drawRect(&pos,glm::vec4(1.f,1.f,0.f,1.f));
				}
			}
		}
	}
}

static int console_editor(int argc, const char** argv) {
	String path;
	if (argc >= 1) {
		path = argv[0];
	}
	if (mainEngine->isPlayTest()) {
		mainEngine->setPlayTest(false);
		if (path.empty()) {
			path = ".playtest.wlb";
		}
	}
	mainEngine->loadAllDefs();
	mainEngine->startEditor(path.get());
	return 0;
}

static Ccmd ccmd_editor("editor", "closes any running games and starts up the level editor", &console_editor);
