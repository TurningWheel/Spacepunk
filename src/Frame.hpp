//! @file Frame.hpp

#pragma once

#include "LinkedList.hpp"
#include "Rect.hpp"
#include "Button.hpp"
#include "Field.hpp"
#include "Script.hpp"
#include "WideVector.hpp"

#include <memory>

class Renderer;
class Image;
class Field;

//! A container for a gui (ie, a window)
//! When a frame's size is smaller than its actual size, sliders will automatically be placed in the frame.
//! Frame objects can be populated with Field objects, Button objects, other Frame objects, and more.
class Frame {
public:
	Frame(const char* _name = "", const char* _script = "");
	Frame(Frame& parent, const char* _name = "", const char* _script = "");
	~Frame();

	//! frame image
	struct image_t {
		String name;
		String path;
		WideVector color;
		Rect<Sint32> pos;
	};

	struct listener_t {
		listener_t(void* _entry) :
			entry(_entry) {}

		void onDeleted();
		void onChangeColor(bool selected, bool highlighted);
		void onChangeName(const char* name);

		//! Frame::entry_t*
		void* entry = nullptr;
	};

	//! frame list entry
	struct entry_t {
		~entry_t();

		StringBuf<32> name;
		String text;
		Script::Args params;
		WideVector color;
		String image;
		String path;
		bool pressed = false;
		bool highlighted = false;
		Uint32 highlightTime = 0;
		bool suicide = false;

		const Script::Function* click = nullptr;
		const Script::Function* ctrlClick = nullptr;
		const Script::Function* highlight = nullptr;
		const Script::Function* highlighting = nullptr;

		std::shared_ptr<listener_t> listener;
	};

	//! frame processing result structure
	struct result_t {
		bool usable;			//! true if the frame is still usable after processing elements, false otherwise
		Uint32 highlightTime;	//! current time since the last frame element was pressed
		const char* tooltip;	//! tooltip for the parent frame to display
		bool removed;			//! the frame was removed during the process
	};

	//! amount of time (ms) it takes for a highlighted object to display a tooltip
	static const Uint32 tooltipTime = 500;

	//! width/height of the slider(s) that appear when actualSize > size (in pixels)
	static const Sint32 sliderSize;

	//! vertical size of a list entry
	static const int entrySize = 20;

	//! draws the frame and all of its subelements
	//! @param renderer the renderer object used to draw the frame
	void draw(Renderer& renderer);

	//! draws the frame and all of its subelements
	//! @param renderer the renderer object used to draw the frame
	//! @param _size real position of the frame onscreen
	//! @param _actualSize offset into the frame space (scroll)
	void draw(Renderer& renderer, Rect<int> _size, Rect<int> _actualSize);

	//! handle clicks and other events
	//! @return compiled results of frame processing
	result_t process();

	//! handle clicks and other events
	//! @param _size real position of the frame onscreen
	//! @param _actualSize offset into the frame space (scroll)
	//! @param usable true if another object doesn't have the mouse's attention, false otherwise
	//! @return compiled results of frame processing
	result_t process(Rect<int> _size, Rect<int> actualSize, const bool usable);

	//! adds a new frame to the current frame
	//! @param name internal name of the new frame
	//! @param script script name of the new frame (sans extension + path)
	//! @return the newly created frame
	Frame* addFrame(const char* name = "", const char* script = "");

	//! adds a new button to the current frame
	//! @param name internal name of the new button
	//! @return the newly created button
	Button* addButton(const char* name);

	//! adds a new field to the current frame
	//! @param name internal name of the new field
	//! @param len the length of the field in characters
	//! @return the newly created field
	Field* addField(const char* name, const int len);

	//! adds a new image object to the current frame
	//! @param pos position of the image in the frame
	//! @param color the color of the image
	//! @param image the image to draw
	//! @param name the name of the image (unique id)
	//! @return the newly created image object
	image_t* addImage(const Rect<Sint32>& pos, const WideVector& color, String image, const char* name = "");

	//! adds a new entry to the frame's list
	//! @param name internal name of the new entry
	//! @param resizeFrame if true, the size of the frame will be reduced after removing the entry
	//! @return the newly created entry object
	entry_t* addEntry(const char* name, bool resizeFrame);

	//! removes all frames, buttons, fields, images, and list entries from the frame
	void clear();

	//! removes the frame itself, as well as all contained objects
	void removeSelf();

	//! remove an object from the frame
	//! @param name the name of the object to remove
	//! @return true if the object was successfully removed, false otherwise
	bool remove(const char* name);

	//! remove an entry from the frame list
	//! @param name the name of the object to remove
	//! @param resizeFrame if true, the size of the frame will be reduced after removing the entry
	//! @return true if the entry was successfully removed, false otherwise
	bool removeEntry(const char* name, bool resizeFrame);

	//! recursively searches all embedded frames for a specific frame
	//! @param name the name of the frame to find
	//! @return the frame with the given name, or nullptr if the frame could not be found
	Frame* findFrame(const char* name);

	//! find a button in this frame
	//! @param name the name of the button to find
	//! @return the button, or nullptr if it could not be found
	Button* findButton(const char* name);

	//! find a field in this frame
	//! @param name the name of the field to find
	//! @return the field, or nullptr if it could not be found
	Field* findField(const char* name);

	//! find an image in this frame
	//! @param name the name of the image to find
	//! @return the image, or nullptr if it could not be found
	image_t* findImage(const char* name);

	//! find an entry in this frame
	//! @param name the name of the entry to find
	//! @return the entry, or nullptr if it could not be found
	entry_t* findEntry(const char* name);

	//! resizes the frame to accomodate all list entries
	void resizeForEntries();

	//! determines if the mouse is currently within the frame or not
	//! @param curSize used by the recursion algorithm, ignore or always pass nullptr
	//! @param curActualSize used by the recursion algorithm, ignore or always pass nullptr
	//! @return true if it is, false otherwise
	bool capturesMouse(Rect<int>* curSize = nullptr, Rect<int>* curActualSize = nullptr);

	//! recursively locates the head frame (ie root gui) for this frame
	//! @return the head frame, which may be the current frame if we have no parent
	Frame* findHead();

	//! getters & setters
	Frame*						getParent() { return parent; }
	const char*					getName() const { return name.get(); }
	const int					getBorder() const { return border; }
	const Rect<int>&			getSize() const { return size; }
	const Rect<int>&			getActualSize() const { return actualSize; }
	const bool					isHigh() const { return high; }
	LinkedList<Frame*>&			getFrames() { return frames; }
	LinkedList<Field*>&			getFields() { return fields; }
	LinkedList<Button*>&		getButtons() { return buttons; }
	LinkedList<entry_t*>&		getEntries() { return list; }
	const bool					isDisabled() const { return disabled; }
	const bool					isHollow() const { return hollow; }

	void	setBorder(const int _border) { border = _border; }
	void	setPos(const int x, const int y) { size.x = x; size.y = y; }
	void	setSize(Rect<int>& _size) { size = _size; }
	void	setActualSize(Rect<int>& _actualSize) { actualSize = _actualSize; }
	void	setHigh(const bool _high) { high = _high; }
	void	setColor(const WideVector& _color) { color = _color; }
	void	setDisabled(const bool _disabled) { disabled = _disabled; }
	void	setHollow(const bool _hollow) { hollow = _hollow; }

private:
	Frame* parent = nullptr;		//! parent frame
	Script* script = nullptr;		//! script engine
	String name;					//! internal name of the frame
	int border = 3;					//! size of the frame's border
	Rect<int> size;					//! size and position of the frame in its parent frame
	Rect<int> actualSize;			//! size of the frame's whole contents. when larger than size, activates sliders
	bool high = true;				//! use Renderer::drawHighFrame(); else use Renderer::drawLowFrame()
	WideVector color;				//! the frame's color
	String scriptStr;				//! name of the frame's script (sans path and extension)
	StringBuf<256> scriptPath;		//! path to the frame's script file
	bool disabled = false;			//! if true, the frame is invisible and unusable
	bool focus = false;				//! if true, this frame has window focus
	const char* tooltip = nullptr;	//! points to the tooltip that should be displayed by the (master) frame, or nullptr if none should be displayed
	bool hollow = false;			//! if true, the frame is hollow; otherwise it is not
	bool toBeDeleted = false;		//! if true, the frame will be removed at the end of its process
	static bool tabbing;			//! used for tabbing between fields
	bool draggingHSlider = false;	//! if true, we are dragging the horizontal slider
	bool draggingVSlider = false;	//! if true, we are dragging the vertical slider
	int oldSliderX = 0;				//! when you start dragging a slider, this is set
	int oldSliderY = 0;				//! when you start dragging a slider, this is set

	LinkedList<Frame*> frames;
	LinkedList<Button*> buttons;
	LinkedList<Field*> fields;
	LinkedList<image_t*> images;
	LinkedList<entry_t*> list;
};