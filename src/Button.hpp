// Button.hpp

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/vec4.hpp>

#include "Rect.hpp"
#include "Main.hpp"
#include "Script.hpp"

class Renderer;
class Frame;
class Image;

class Button {
public:
	Button();
	Button(Frame& _parent);
	~Button();

	// the result of the button process
	struct result_t {
		bool highlighted;				// was highlighted this frame
		bool pressed;					// was pressed this frame
		bool clicked;					// was activated this frame
		Uint32 highlightTime;			// time since button was highlighted
		const char* tooltip = nullptr;	// button tooltip to be displayed
	};

	// button style
	enum style_t {
		STYLE_NORMAL,
		STYLE_TOGGLE,
		STYLE_CHECKBOX,
		STYLE_MAX
	};

	// draws the button
	// @param renderer the renderer object used to draw the button
	// @param _size size and position of button's parent frame
	// @param _actualSize offset into the parent frame space (scroll)
	void draw(Renderer& renderer, Rect<int> _size, Rect<int> _actualSize);

	// handles button clicks, etc.
	// @param _size size and position of button's parent frame
	// @param _actualSize offset into the parent frame space (scroll)
	// @param usable true if another object doesn't have the mouse's attention, false otherwise
	// @return resultant state of the button after processing
	result_t process(Rect<int> _size, Rect<int> _actualSize, const bool usable);

	// getters & setters
	const char*					getName() const				{ return name.get(); }
	const int					getBorder() const			{ return border; }
	const Rect<int>&			getSize() const				{ return size; }
	const bool					isPressed() const			{ return pressed; }
	const bool					isHighlighted() const		{ return highlighted; }
	const bool					isDisabled() const			{ return disabled; }
	const style_t				getStyle() const			{ return style; }
	Script::Args&				getParams()					{ return params; }
	const Script::Function*		getCallback() const			{ return callback; }

	void	setBorder(const int _border)				{ border = _border; }
	void	setPos(const int x, const int y)			{ size.x = x; size.y = y; }
	void	setSize(Rect<int>& _size)					{ size = _size; }
	void	setColor(const glm::vec4& _color)			{ color = _color; }
	void	setTextColor(const glm::vec4& _color)		{ textColor = _color; }
	void	setName(const char* _name)					{ name = _name; }
	void	setText(const char* _text)					{ text = _text; }
	void	setIcon(const char* _icon);
	void	setTooltip(const char* _tooltip)			{ tooltip = _tooltip; }
	void	setDisabled(const bool _disabled)			{ disabled = _disabled; }
	void	setStyle(const style_t _style)				{ style = _style; }
	void	setPressed(const bool _pressed)				{ reallyPressed = pressed = _pressed; }
	void	setCallback(const Script::Function* fn)		{ callback = fn; }

private:
	Frame* parent = nullptr;						// parent frame
	const Script::Function* callback = nullptr;		// native callback for clicking
	String name;									// internal button name
	String text;									// button text, if any
	String icon;									// icon, if any (supersedes text content)
	String tooltip;									// if empty, button has no tooltip; otherwise, it does
	Script::Args params;							// optional function parameters to use when the button function is called	
	int border = 3;									// size of the button border in pixels
	Rect<int> size;									// size and position of the button within its parent frame
	bool pressed = false;							// button pressed state
	bool reallyPressed = false;						// the "actual" button state, pre-mouse process
	bool highlighted = false;						// true if mouse is hovering over button; false otherwise
	glm::vec4 color;								// the button's color
	glm::vec4 textColor;							// text color
	style_t style = STYLE_NORMAL;					// button style
	bool disabled=false;							// if true, the button is invisible and unusable
	Uint32 highlightTime = 0;						// records the time since the button was highlighted
	Image* iconImg = nullptr;						// the icon image
};