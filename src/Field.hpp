// Field.hpp

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/vec4.hpp>

#include "Main.hpp"
#include "Rect.hpp"
#include "Script.hpp"

class Renderer;
class Frame;

class Field {
public:
	Field();
	Field(const int _textLen);
	Field(const char* _text);
	Field(Frame& _parent);
	Field(Frame& _parent, const int _textLen);
	Field(Frame& _parent, const char* _text);
	~Field();

	// no fields or frames should ever have this name!
	static const char* invalidName;

	// text justification
	enum justify_t {
		LEFT,
		CENTER,
		RIGHT,
		JUSTIFY_TYPE_LENGTH
	};

	// the result of the field process
	struct result_t {
		bool highlighted;
		bool entered;
	};

	// selects the field for text editing
	void select();

	// deselects the field
	void deselect();

	// draws the field
	// @param renderer the renderer object used to draw the field
	// @param _size size and position of field's parent frame
	// @param _actualSize offset into the parent frame space (scroll)
	void draw(Renderer& renderer, Rect<int> _size, Rect<int> _actualSize);

	// handles clicks, etc.
	// @param _size size and position of field's parent frame
	// @param _actualSize offset into the parent frame space (scroll)
	// @param usable true if another object doesn't have the mouse's attention, false otherwise
	// @return resultant state of the field after processing
	result_t process(Rect<int> _size, Rect<int> _actualSize, const bool usable);

	// getters & setters
	const char*					getName() const						{ return name.get(); }
	const char*					getText() const						{ return text; }
	const Uint32				getTextLen() const					{ return textLen; }
	const glm::vec4&			getColor() const					{ return color; }
	const Rect<int>				getSize() const						{ return size; }
	const justify_t				getJustify() const					{ return justify; }
	const bool					isSelected() const					{ return selected; }
	const bool					isEditable() const					{ return editable; }
	const bool					isNumbersOnly() const				{ return numbersOnly; }
	const char*					getTabDestField() const				{ return tabDestField.get(); }
	const char*					getTabDestFrame() const				{ return tabDestFrame.get(); }
	Script::Args&				getParams()							{ return params; }
	const Script::Function*		getCallback() const					{ return callback; }

	void	setName(const char* _name)							{ name = _name; }
	void	setText(const char* _text)							{ memset(text, '\0', textLen); strncpy(text,_text,textLen); }
	void	setPos(const int x, const int y)					{ size.x = x; size.y = y; }
	void	setSize(const Rect<int>& _size)						{ size = _size; }
	void	setColor(const glm::vec4& _color)					{ color = _color; }
	void	setEditable(const bool _editable)					{ editable = _editable; }
	void	setNumbersOnly(const bool _numbersOnly)				{ numbersOnly = _numbersOnly; }
	void	setJustify(const justify_t _justify)				{ justify = _justify; }
	void	setScroll(const bool _scroll)						{ scroll = _scroll; }
	void	setTabDestField(const char* _tabDest)				{ tabDestField = _tabDest; }
	void	setTabDestFrame(const char* _tabDest)				{ tabDestFrame = _tabDest; }
	void	setCallback(const Script::Function* fn)				{ callback = fn; }

private:
	Frame* parent = nullptr;	// parent frame

	String name;
	Script::Args params;
	char* text = nullptr;
	Uint32 textLen = 0;
	glm::vec4 color = glm::vec4(1.f,1.f,1.f,1.f);
	Rect<int> size;
	justify_t justify = LEFT;
	bool selected = false;
	bool editable = false;
	bool numbersOnly = false;
	bool scroll = true;
	bool selectAll = false;
	const Script::Function* callback = nullptr;

	String tabDestFrame = invalidName;
	String tabDestField = invalidName;
};