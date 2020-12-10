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
class Field : public Widget {
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

	//! activates the field for text editing
	virtual void activate() override;

	//! deactivate text editing
	void deactivate();

	//! deselects the field
	virtual void deselect() override;

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

	virtual type_t              getType() const override { return WIDGET_FIELD; }
	const char*					getText() const { return text.get(); }
	const char*					getFont() const { return font.get(); }
	const WideVector&			getColor() const { return color; }
	const Rect<int>				getSize() const { return size; }
	const int					getHJustify() const { return static_cast<int>(hjustify); }
	const int					getVJustify() const { return static_cast<int>(vjustify); }
	const bool					isEditable() const { return editable; }
	const bool					isNumbersOnly() const { return numbersOnly; }
	Script::Args&				getParams() { return params; }
	const Script::Function*		getCallback() const { return callback; }

	void	setText(const char* _text) { text = _text; }
	void	setPos(const int x, const int y) { size.x = x; size.y = y; }
	void	setSize(const Rect<int>& _size) { size = _size; }
	void	setColor(const WideVector& _color) { color = _color; }
	void	setEditable(const bool _editable) { editable = _editable; }
	void	setNumbersOnly(const bool _numbersOnly) { numbersOnly = _numbersOnly; }
	void	setJustify(const int _justify) { hjustify = vjustify = static_cast<justify_t>(_justify); }
	void	setHJustify(const int _justify) { hjustify = static_cast<justify_t>(_justify); }
	void	setVJustify(const int _justify) { vjustify = static_cast<justify_t>(_justify); }
	void	setScroll(const bool _scroll) { scroll = _scroll; }
	void	setCallback(const Script::Function* fn) { callback = fn; }
	void	setFont(const char* _font) { font = _font; }

private:
	Script::Args params;								//!< script arguments to use when calling script
	String font = Font::defaultFont;					//!< font to use for rendering the field
	String text;										//!< internal text buffer
	WideVector color = WideVector(1.f);					//!< text color
	Rect<int> size;										//!< size of the field in pixels
	justify_t hjustify = LEFT;							//!< horizontal text justification
	justify_t vjustify = TOP;							//!< vertical text justification
	bool editable = false;								//!< whether the field is read-only
	bool numbersOnly = false;							//!< whether the field can only contain numeric chars
	bool scroll = true;									//!< whether the field should scroll if the text is longer than its container
	bool selectAll = false;								//!< whether all the text is selected for editing
	bool activated = false;								//!< whether field is active for text editing
	const Script::Function* callback = nullptr;			//!< the callback to use after text is entered
};