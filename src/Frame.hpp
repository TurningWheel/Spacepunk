//! @file Frame.hpp

#pragma once

#include "LinkedList.hpp"
#include "Rect.hpp"
#include "Script.hpp"
#include "WideVector.hpp"
#include "Font.hpp"
#include "Widget.hpp"

#include <memory>

class Field;
class Button;
class Field;
class Slider;
class Renderer;

//! A container for a gui (ie, a window)
//! When a frame's size is smaller than its actual size, sliders will automatically be placed in the frame.
//! Frame objects can be populated with Field objects, Button objects, other Frame objects, and more.
class Frame : public Widget {
public:
	Frame() = delete;
	Frame(const char* _name = "", const char* _script = "");
	Frame(Frame& parent, const char* _name = "", const char* _script = "");
	Frame(const Frame&) = delete;
	Frame(Frame&&) = delete;
	virtual ~Frame();

	Frame& operator=(const Frame&) = delete;
	Frame& operator=(Frame&&) = delete;

	//! border style
	enum border_style_t {
		BORDER_FLAT,
		BORDER_BEVEL_HIGH,
		BORDER_BEVEL_LOW,
		BORDER_MAX
	};

	//! frame image
	struct image_t {
		String name;
		String path;
		WideVector color;
		Rect<Sint32> pos;
	};

	struct entry_t;

	//! list entry listener
	struct listener_t {
		listener_t(entry_t* _entry) :
			entry(_entry) {}

		void onDeleted();
		void onChangeColor(bool selected, bool highlighted);
		void onChangeName(const char* name);

		//! Frame::entry_t*
		entry_t* entry = nullptr;
	};

	//! frame list entry
	struct entry_t {
		~entry_t();

		String name;
		String text;
		String tooltip;
		Script::Args params;
		WideVector color;
		String image;

		//! exists for lua, really
		void setParams(const Script::Args& src) {
			params.copy(src);
		}

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

	//! virtual screen size (width)
	static const int virtualScreenX = 1920;

	//! virtual screen size (height)
	static const int virtualScreenY = 1080;

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

	//! to be performed recursively on every frame after process()
	void postprocess();

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

	//! adds a new slider object to the current frame
	//! @param name the name of the slider
	//! @return the newly created slider object
	Slider* addSlider(const char* name);

	//! removes all objects and list entries from the frame
	void clear();

	//! remove all list entries from the frame
	void clearEntries();

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

	//! find a slider in this frame
	//! @param name the name of the slider to find
	//! @return the slider, or nullptr if it could not be found
	Slider* findSlider(const char* name);

	//! get the frame's parent frame, if any
	//! @return the parent frame, or nullptr if there is none
	Frame* getParent();

	//! resizes the frame to accomodate all list entries
	void resizeForEntries();

	//! deselect all frame elements recursively
	virtual void deselect() override;

	//! determines if the mouse is currently within the frame or not
	//! @param curSize used by the recursion algorithm, ignore or always pass nullptr
	//! @param curActualSize used by the recursion algorithm, ignore or always pass nullptr
	//! @return true if it is, false otherwise
	bool capturesMouse(Rect<int>* curSize = nullptr, Rect<int>* curActualSize = nullptr);

	//! set the list selection to the given index
	//! @param index the index to set the list selection to
	void setSelection(int index);

	virtual type_t					getType() const override { return WIDGET_FRAME; }
	const char*						getFont() const { return font.get(); }
	int						        getBorder() const { return border; }
	const Rect<int>&				getSize() const { return size; }
	const Rect<int>&				getActualSize() const { return actualSize; }
	int								getBorderStyle() const { return borderStyle; }
	LinkedList<Frame*>&				getFrames() { return frames; }
	LinkedList<Field*>&				getFields() { return fields; }
	LinkedList<Button*>&			getButtons() { return buttons; }
	LinkedList<Slider*>&			getSliders() { return sliders; }
	LinkedList<entry_t*>&			getEntries() { return list; }
	bool				    		isDisabled() const { return disabled; }
	bool					    	isHollow() const { return hollow; }
	bool					    	isDropDown() const { return dropDown; }
	Script*							getScript() { return script; }

	void	setFont(const char* _font) { font = _font; }
	void	setBorder(const int _border) { border = _border; }
	void	setPos(const int x, const int y) { size.x = x; size.y = y; }
	void	setSize(Rect<int>& _size) { size = _size; }
	void	setActualSize(Rect<int>& _actualSize) { actualSize = _actualSize; }
	void	setBorderStyle(int _borderStyle) { borderStyle = static_cast<border_style_t>(_borderStyle); }
	void	setHigh(bool b) { borderStyle = b ? BORDER_BEVEL_HIGH : BORDER_BEVEL_LOW; }
	void	setColor(const WideVector& _color) { color = _color; }
	void	setBorderColor(const WideVector& _color) { borderColor = _color; }
	void	setDisabled(const bool _disabled) { disabled = _disabled; }
	void	setHollow(const bool _hollow) { hollow = _hollow; }
	void	setDropDown(const bool _dropDown) { dropDown = _dropDown; }

private:
	Script* script = nullptr;							//!< script engine
	Uint32 ticks = 0;									//!< number of engine ticks this frame has persisted
	String font = Font::defaultFont;					//!< name of the font to use for frame entries
	int border = 3;										//!< size of the frame's border
	Rect<int> size;										//!< size and position of the frame in its parent frame
	Rect<int> actualSize;								//!< size of the frame's whole contents. when larger than size, activates sliders
	border_style_t borderStyle = BORDER_BEVEL_HIGH;		//!< border style
	WideVector color;									//!< the frame's color
	WideVector borderColor;								//!< the frame's border color (only used for flat border)
	String scriptStr;									//!< name of the frame's script (sans path and extension)
	StringBuf<256> scriptPath;							//!< path to the frame's script file
	const char* tooltip = nullptr;						//!< points to the tooltip that should be displayed by the (master) frame, or nullptr if none should be displayed
	bool hollow = false;								//!< if true, the frame is hollow; otherwise it is not
	bool toBeDeleted = false;							//!< if true, the frame will be removed at the end of its process
	bool draggingHSlider = false;						//!< if true, we are dragging the horizontal slider
	bool draggingVSlider = false;						//!< if true, we are dragging the vertical slider
	int oldSliderX = 0;									//!< when you start dragging a slider, this is set
	int oldSliderY = 0;									//!< when you start dragging a slider, this is set
	bool dropDown = false;								//!< if true, the frame is destroyed when specific inputs register
	Uint32 dropDownClicked = 0;							//!< key states stored for removing drop downs
	Node<entry_t*>* selection = nullptr;				//!< entry selection

	LinkedList<Frame*> frames;
	LinkedList<Button*> buttons;
	LinkedList<Field*> fields;
	LinkedList<image_t*> images;
	LinkedList<Slider*> sliders;
	LinkedList<entry_t*> list;

	void scrollToSelection();
	void activateEntry(entry_t& entry);
};
