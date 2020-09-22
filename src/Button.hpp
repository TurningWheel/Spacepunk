//! @file Button.hpp

#pragma once

#include "Rect.hpp"
#include "Main.hpp"
#include "Script.hpp"
#include "Font.hpp"
#include "WideVector.hpp"
#include "Widget.hpp"

class Renderer;
class Frame;
class Image;

//! A Button lives in a Frame and can have scripted actions or a native callback.
class Button : public Widget {
public:
	Button();
	Button(Frame& _parent);
	Button(const Button&) = delete;
	Button(Button&&) = delete;
	virtual ~Button();

	Button& operator=(const Button&) = delete;
	Button& operator=(Button&&) = delete;

	//! the result of the button process
	struct result_t {
		bool highlighted;				//!< was highlighted this frame
		bool pressed;					//!< was pressed this frame
		bool clicked;					//!< was activated this frame
		Uint32 highlightTime;			//!< time since button was highlighted
		const char* tooltip = nullptr;	//!< button tooltip to be displayed
	};

	//! button style
	enum style_t {
		STYLE_NORMAL,
		STYLE_TOGGLE,
		STYLE_CHECKBOX,
		STYLE_DROPDOWN,
		STYLE_MAX
	};

	//! draws the button
	//! @param renderer the renderer object used to draw the button
	//! @param _size size and position of button's parent frame
	//! @param _actualSize offset into the parent frame space (scroll)
	virtual void draw(Renderer& renderer, Rect<int> _size, Rect<int> _actualSize);

	//! handles button clicks, etc.
	//! @param _size size and position of button's parent frame
	//! @param _actualSize offset into the parent frame space (scroll)
	//! @param usable true if another object doesn't have the mouse's attention, false otherwise
	//! @return resultant state of the button after processing
	result_t process(Rect<int> _size, Rect<int> _actualSize, const bool usable);

	//! activates the button
	virtual void activate() override;

	virtual type_t              getType() const override { return WIDGET_BUTTON; }
	const char*					getText() const { return text.get(); }
	const char*					getFont() const { return font.get(); }
	int							getBorder() const { return border; }
	const Rect<int>&			getSize() const { return size; }
	int							getStyle() const { return style; }
	Script::Args&				getParams() { return params; }
	const Script::Function*		getCallback() const { return callback; }

	void	setBorder(int _border) { border = _border; }
	void	setPos(int x, int y) { size.x = x; size.y = y; }
	void	setSize(Rect<int>& _size) { size = _size; }
	void	setColor(const WideVector& _color) { color = _color; }
	void	setTextColor(const WideVector& _color) { textColor = _color; }
	void	setBorderColor(const WideVector& _color) { borderColor = _color; }
	void	setText(const char* _text) { text = _text; }
	void	setFont(const char* _font) { font = _font; }
	void	setIcon(const char* _icon);
	void	setTooltip(const char* _tooltip) { tooltip = _tooltip; }
	void	setStyle(int _style) { style = static_cast<style_t>(_style); }
	void	setCallback(const Script::Function* fn) { callback = fn; }

private:
	const Script::Function* callback = nullptr;		//!< native callback for clicking
	String text;									//!< button text, if any
	String font = Font::defaultFont;				//!< button font
	String icon;									//!< icon, if any (supersedes text content)
	String tooltip;									//!< if empty, button has no tooltip; otherwise, it does
	Script::Args params;							//!< optional function parameters to use when the button function is called	
	int border = 3;									//!< size of the button border in pixels
	Rect<int> size;									//!< size and position of the button within its parent frame
	WideVector color;								//!< the button's color
	WideVector textColor;							//!< text color
	WideVector borderColor;							//!< (optional) border color
	style_t style = STYLE_NORMAL;					//!< button style
};