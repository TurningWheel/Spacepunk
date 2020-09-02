//! @file Field.hpp

#pragma once

#include "Main.hpp"
#include "Rect.hpp"
#include "Script.hpp"
#include "Font.hpp"
#include "WideVector.hpp"

class Renderer;
class Frame;

//! A Field is a text field that lives in a Frame. It can be edited, or locked for editing to just have some static text in a window.
class Field {
public:
	Field();
	Field(const int _textLen);
	Field(const char* _text);
	Field(Frame& _parent);
	Field(Frame& _parent, const int _textLen);
	Field(Frame& _parent, const char* _text);
	Field(const Field&) = delete;
	Field(Field&&) = delete;
	~Field();

	Field& operator=(const Field&) = delete;
	Field& operator=(Field&&) = delete;

	//! no fields or frames should ever have this name!
	static const char* invalidName;

	//! text justification
	enum justify_t {
		TOP,
		BOTTOM,
		LEFT,
		RIGHT,
		CENTER,
		JUSTIFY_TYPE_LENGTH
	};

	//! the result of the field process
	struct result_t {
		bool highlighted;
		bool entered;
	};

	//! selects the field for text editing
	void select();

	//! deselects the field
	void deselect();

	//! draws the field
	//! @param renderer the renderer object used to draw the field
	//! @param _size size and position of field's parent frame
	//! @param _actualSize offset into the parent frame space (scroll)
	void draw(Renderer& renderer, Rect<int> _size, Rect<int> _actualSize);

	//! handles clicks, etc.
	//! @param _size size and position of field's parent frame
	//! @param _actualSize offset into the parent frame space (scroll)
	//! @param usable true if another object doesn't have the mouse's attention, false otherwise
	//! @return resultant state of the field after processing
	result_t process(Rect<int> _size, Rect<int> _actualSize, const bool usable);

	const char*					getName() const { return name.get(); }
	const char*					getText() const { return text; }
	const Uint32				getTextLen() const { return textLen; }
	const char*					getFont() const { return font.get(); }
	const WideVector&			getColor() const { return color; }
	const Rect<int>				getSize() const { return size; }
	const int					getHJustify() const { return static_cast<int>(hjustify); }
	const int					getVJustify() const { return static_cast<int>(vjustify); }
	const bool					isSelected() const { return selected; }
	const bool					isEditable() const { return editable; }
	const bool					isNumbersOnly() const { return numbersOnly; }
	const char*					getTabDestField() const { return tabDestField.get(); }
	const char*					getTabDestFrame() const { return tabDestFrame.get(); }
	Script::Args&				getParams() { return params; }
	const Script::Function*		getCallback() const { return callback; }

	void	setName(const char* _name) { name = _name; }
	void	setText(const char* _text) { memset(text, '\0', textLen); strncpy(text, _text, textLen); }
	void	setPos(const int x, const int y) { size.x = x; size.y = y; }
	void	setSize(const Rect<int>& _size) { size = _size; }
	void	setColor(const WideVector& _color) { color = _color; }
	void	setEditable(const bool _editable) { editable = _editable; }
	void	setNumbersOnly(const bool _numbersOnly) { numbersOnly = _numbersOnly; }
	void	setJustify(const int _justify) { hjustify = vjustify = static_cast<justify_t>(_justify); }
	void	setHJustify(const int _justify) { hjustify = static_cast<justify_t>(_justify); }
	void	setVJustify(const int _justify) { vjustify = static_cast<justify_t>(_justify); }
	void	setScroll(const bool _scroll) { scroll = _scroll; }
	void	setTabDestField(const char* _tabDest) { tabDestField = _tabDest; }
	void	setTabDestFrame(const char* _tabDest) { tabDestFrame = _tabDest; }
	void	setCallback(const Script::Function* fn) { callback = fn; }
	void	setFont(const char* _font) { font = _font; }

private:
	Frame* parent = nullptr;							//!< parent frame
	String name;										//!< internal name of the field
	Script::Args params;								//!< script arguments to use when calling script
	String font = Font::defaultFont;					//!< font to use for rendering the field
	char* text = nullptr;								//!< internal text buffer
	Uint32 textLen = 0;									//!< size of the text buffer in bytes
	WideVector color = WideVector(1.f);					//!< text color
	Rect<int> size;										//!< size of the field in pixels
	justify_t hjustify = LEFT;							//!< horizontal text justification
	justify_t vjustify = TOP;							//!< vertical text justification
	bool selected = false;								//!< whether the field is selected
	bool editable = false;								//!< whether the field is read-only
	bool numbersOnly = false;							//!< whether the field can only contain numeric chars
	bool scroll = true;									//!< whether the field should scroll if the text is longer than its container
	bool selectAll = false;								//!< whether all the text is selected for editing
	const Script::Function* callback = nullptr;			//!< the callback to use after text is entered
	String tabDestFrame = invalidName;					//!< the name of the frame to collect focus if the user presses the Tab key
	String tabDestField = invalidName;					//!< the name of the field to collect focus if the user presses the Tab key
};