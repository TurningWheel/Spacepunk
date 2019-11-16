// Field.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "Frame.hpp"
#include "Field.hpp"

// no fields or frames should ever have this name!
const char* Field::invalidName = "WACKAWACKA";

Field::Field() : Field("") {
}

Field::Field(const int _textLen) {
	textLen = _textLen+1;
	text = (char*) calloc(textLen,sizeof(char));
}

Field::Field(const char* _text) {
	textLen = static_cast<Uint32>(strlen(_text))+1;
	text = (char*) calloc(textLen,sizeof(char));
	if( text ) {
		text[textLen-1] = '\0';
		strncpy(text,_text,textLen-1);
	}
}

Field::Field(Frame& _parent) {
	parent = &_parent;
	_parent.getFields().addNodeLast(this);
}

Field::Field(Frame& _parent, const int _textLen) : Field(_textLen) {
	parent = &_parent;
	_parent.getFields().addNodeLast(this);
}

Field::Field(Frame& _parent, const char* _text) : Field(_text) {
	parent = &_parent;
	_parent.getFields().addNodeLast(this);
}

Field::~Field() {
	deselect();
	if (text) {
		free(text);
		text = nullptr;
	}
	if (callback) {
		delete callback;
		callback = nullptr;
	}
}

void Field::select() {
	selected = true;
	mainEngine->setInputStr(text);
	mainEngine->setInputLen((int)textLen-1);
	mainEngine->setInputNumbersOnly(numbersOnly);
	SDL_StartTextInput();
}

void Field::deselect() {
	selectAll = false;
	selected = false;
	if( mainEngine->getInputStr() == text ) {
		mainEngine->setInputStr(nullptr);
		mainEngine->setInputLen(0);
		SDL_StopTextInput();
	}
}

void Field::draw(Renderer& renderer, Rect<int> _size, Rect<int> _actualSize) {
	Rect<int> rect;
	rect.x = _size.x + max(0,size.x-_actualSize.x);
	rect.y = _size.y + max(0,size.y-_actualSize.y);
	rect.w = min( size.w, _size.w-size.x+_actualSize.x ) + min(0,size.x-_actualSize.x);
	rect.h = min( size.h, _size.h-size.y+_actualSize.y ) + min(0,size.y-_actualSize.y);
	if( rect.w <= 0 || rect.h <= 0 )
		return;

	if( selected ) {
		renderer.drawRect(&rect,glm::vec4(0.f,0.f,.5f,1.f));
	}

	String str;
	if( selected && mainEngine->isCursorVisible() ) {
		str.alloc((Uint32)strlen(text)+2);
		str.assign(text);
		str.append("_");
	} else if( selected ) {
		str.alloc((Uint32)strlen(text)+2);
		str.assign(text);
		str.append(" ");
	} else {
		str.assign(text);
	}

	Text* text = nullptr;
	if( !str.empty() ) {
		text = mainEngine->getTextResource().dataForString(str.get());
		if( !text ) {
			return;
		}
	} else {
		return;
	}

	// get the size of the rendered text
	int textSizeW = text->getWidth();
	int textSizeH = text->getHeight();

	if( selected ) {
		textSizeH += 2;
		if( justify==RIGHT ) {
			textSizeH -= 4;
		} else if( justify==CENTER ) {
			textSizeH -= 2;
		}
		if( !mainEngine->isCursorVisible() ) {
			int w;
			TTF_SizeUTF8(renderer.getMonoFont(),"_",&w,nullptr);
			textSizeW += w;
			textSizeH += 2;
		}
	}

	Rect<int> pos;
	if( justify==LEFT ) {
		pos.x = _size.x + size.x - _actualSize.x;
		pos.y = _size.y + size.y - _actualSize.y;
	} else if( justify==CENTER ) {
		pos.x = _size.x + size.x + size.w/2 - textSizeW/2 - _actualSize.x;
		pos.y = _size.y + size.y + size.h/2 - textSizeH/2 - _actualSize.y;
	} else if( justify==RIGHT ) {
		pos.x = _size.x + size.x + size.w - textSizeW - _actualSize.x;
		pos.y = _size.y + size.y + size.h - textSizeH - _actualSize.y;
	}
	pos.w = textSizeW;
	pos.h = textSizeH;

	Rect<int> dest;
	dest.x = max( rect.x, pos.x );
	dest.y = max( rect.y, pos.y );
	dest.w = pos.w - ( dest.x - pos.x ) - max( 0, ( pos.x + pos.w ) - ( rect.x + rect.w ) );
	dest.h = pos.h - ( dest.y - pos.y ) - max( 0, ( pos.y + pos.h ) - ( rect.y + rect.h ) );

	Rect<int> src;
	src.x = max( 0, rect.x - pos.x );
	src.y = max( 0, rect.y - pos.y );
	src.w = pos.w - ( dest.x - pos.x ) - max( 0, ( pos.x + pos.w ) - ( rect.x + rect.w ) );
	src.h = pos.h - ( dest.y - pos.y ) - max( 0, ( pos.y + pos.h ) - ( rect.y + rect.h ) );

	// fit text to window
	if( justify==LEFT && scroll ) {
		src.x = max( src.x, textSizeW - rect.w );
	}

	if( src.w<=0 || src.h<=0 || dest.w<=0 || dest.h<=0 )
		return;

	if( selectAll && selected ) {
		renderer.drawRect(&rect,glm::vec4(.5f,.5f,0.f,1.f));
	}
	text->drawColor( src, dest, color );
}

Field::result_t Field::process(Rect<int> _size, Rect<int> _actualSize, const bool usable) {
	result_t result;
	result.highlighted = false;
	result.entered = false;
	if( !editable ) {
		if( selected ) {
			result.entered = true;
			deselect();
		}
		return result;
	}

	_size.x += max(0,size.x-_actualSize.x);
	_size.y += max(0,size.y-_actualSize.y);
	_size.w = min( size.w, _size.w-size.x+_actualSize.x ) + min(0,size.x-_actualSize.x);
	_size.h = min( size.h, _size.h-size.y+_actualSize.y ) + min(0,size.y-_actualSize.y);
	if( _size.w <= 0 || _size.h <= 0 ) {
		return result;
	}

	Sint32 omousex = mainEngine->getOldMouseX();
	Sint32 omousey = mainEngine->getOldMouseY();

	if( selected ) {
		if( mainEngine->getInputStr() != text ) {
			result.entered = true;
			deselect();
			if( mainEngine->getInputStr()==nullptr ) {
				SDL_StopTextInput();
			}
		}
		if( mainEngine->getKeyStatus(SDL_SCANCODE_RETURN) || mainEngine->getKeyStatus(SDL_SCANCODE_KP_ENTER) ) {
			result.entered = true;
			deselect();
		}
		if( mainEngine->getKeyStatus(SDL_SCANCODE_ESCAPE) || mainEngine->getMouseStatus(SDL_BUTTON_RIGHT) ) {
			result.entered = true;
			deselect();
		}

		if( selectAll ) {
			if( mainEngine->getAnyKeyStatus() ) {
				const char* keys = mainEngine->getLastInput();
				if( keys ) {
					if( !Engine::charsHaveLetters(keys, (Uint32)strlen(keys)) || !numbersOnly ) {
						strncpy(text, keys, textLen-1);
						selectAll = false;
					}
				}
			}
		}
	}

	if( omousex >= _size.x && omousex < _size.x+_size.w &&
		omousey >= _size.y && omousey < _size.y+_size.h ) {
		result.highlighted = true;
	}

	if( !result.highlighted && mainEngine->getMouseStatus(SDL_BUTTON_LEFT) ) {
		if( selected ) {
			result.entered = true;
			deselect();
		}
	} else if( result.highlighted && mainEngine->getMouseStatus(SDL_BUTTON_LEFT) ) {
		select();
		if( mainEngine->getDBCMouseStatus(SDL_BUTTON_LEFT) ) {
			selectAll = true;
		}
	}

	return result;
}