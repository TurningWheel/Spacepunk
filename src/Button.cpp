// Button.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Client.hpp"
#include "Renderer.hpp"
#include "Frame.hpp"
#include "Button.hpp"
#include "Image.hpp"
#include "Text.hpp"

Button::Button() {
	size.x = 0; size.w = 32;
	size.y = 0; size.h = 32;
	color = glm::vec4(.5f,.5f,.5f,1.f);
	textColor = glm::vec4(1.f);
}

Button::Button(Frame& _parent) : Button() {
	parent = &_parent;
	_parent.getButtons().addNodeLast(this);
}

Button::~Button() {
	if (callback) {
		delete callback;
		callback = nullptr;
	}
}

void Button::setIcon(const char* _icon) {
	icon = _icon;
}

void Button::draw(Renderer& renderer, Rect<float> _size, Rect<float> _actualSize) {
	if( disabled )
		return;

	_size.x += max(0.f,size.x-_actualSize.x);
	_size.y += max(0.f,size.y-_actualSize.y);
	_size.w = min( size.w, _size.w-size.x+_actualSize.x ) + min(0.f,size.x-_actualSize.x);
	_size.h = min( size.h, _size.h-size.y+_actualSize.y ) + min(0.f,size.y-_actualSize.y);
	if( _size.w <= 0 || _size.h <= 0 )
		return;

	Rect<int> _sizePx = Frame::convertToPx(renderer, _size);

	glm::vec4 _color = highlighted ? color*1.5f : color;
	if( pressed ) {
		renderer.drawLowFrame(_sizePx,border,_color);
	} else {
		renderer.drawHighFrame(_sizePx,border,_color);
	}

	if( !text.empty() && style!=STYLE_CHECKBOX ) {
		Text* _text = mainEngine->getTextResource().dataForString(text.get());
		if( _text ) {
			Rect<int> pos;
			int textX = _sizePx.w/2-_text->getWidth()/2;
			int textY = _sizePx.h/2-_text->getHeight()/2;
			pos.x = _sizePx.x+textX; pos.w = min((int)_text->getWidth(),_sizePx.w);
			pos.y = _sizePx.y+textY; pos.h = min((int)_text->getHeight(),_sizePx.h);
			if( pos.w <= 0 || pos.h <= 0 ) {
				return;
			}
			_text->drawColor(Rect<int>(), pos, textColor);
		}
	} else if( icon.get() ) {
		// we check a second time, just incase the cache was dumped and the original pointer invalidated.
		Image* iconImg = mainEngine->getImageResource().dataForString(icon.get());
		if( iconImg ) {
			if( style!=STYLE_CHECKBOX || pressed==true ) {
				Rect<int> pos;
				pos.x = _sizePx.x+border; pos.w = _sizePx.w-border*2;
				pos.y = _sizePx.y+border; pos.h = _sizePx.h-border*2;
				if( pos.w <= 0 || pos.h <= 0 ) {
					return;
				}

				float w = iconImg->getWidth();
				float h = iconImg->getHeight();

				Rect<int> _actualSizePx = Frame::convertToPx(renderer, _actualSize);
				Rect<int> sizePx = Frame::convertToPx(renderer, size);

				Rect<Sint32> section;
				section.x = sizePx.x - _actualSizePx.x < 0 ? -(sizePx.x - _actualSizePx.x) * (w / (sizePx.w-border*2)) : 0;
				section.y = sizePx.y - _actualSizePx.y < 0 ? -(sizePx.y - _actualSizePx.y) * (h / (sizePx.h-border*2)) : 0;
				section.w = ((float)pos.w / (sizePx.w-border*2)) * w;
				section.h = ((float)pos.h / (sizePx.h-border*2)) * h;

				iconImg->draw(&section,pos);
			}
		}
	}
}

Button::result_t Button::process(Rect<float> _size, Rect<float> _actualSize, const bool usable) {
	result_t result;
	if( style!=STYLE_NORMAL ) {
		result.tooltip = nullptr;
		result.highlightTime = SDL_GetTicks();
		result.highlighted = false;
		result.pressed = pressed;
		result.clicked = false;
	} else {
		result.tooltip = nullptr;
		result.highlightTime = SDL_GetTicks();
		result.highlighted = false;
		result.pressed = false;
		result.clicked = false;
	}
	if( disabled ) {
		highlightTime = result.highlightTime;
		highlighted = false;
		if( style==STYLE_NORMAL ) {
			pressed = false;
		}
		return result;
	}
	if( !usable ) {
		highlightTime = result.highlightTime;
		highlighted = false;
		if( style==STYLE_NORMAL ) {
			pressed = false;
		}
		return result;
	}

	_size.x += max(0.f,size.x-_actualSize.x);
	_size.y += max(0.f,size.y-_actualSize.y);
	_size.w = min( size.w, _size.w-size.x+_actualSize.x ) + min(0.f,size.x-_actualSize.x);
	_size.h = min( size.h, _size.h-size.y+_actualSize.y ) + min(0.f,size.y-_actualSize.y);
	if( _size.w <= 0 || _size.h <= 0 ) {
		highlightTime = result.highlightTime;
		return result;
	}

	Client* client = mainEngine->getLocalClient(); assert(client);
	Renderer* renderer = client->getRenderer(); assert(renderer);
	auto _sizePx = Frame::convertToPx(*renderer, _size);

	Sint32 mousex = mainEngine->getMouseX();
	Sint32 mousey = mainEngine->getMouseY();
	Sint32 omousex = mainEngine->getOldMouseX();
	Sint32 omousey = mainEngine->getOldMouseY();
	
	if( _sizePx.containsPoint(omousex,omousey) ) {
		result.highlighted = highlighted = true;
		result.highlightTime = highlightTime;
		result.tooltip = tooltip.get();
	} else {
		result.highlighted = highlighted = false;
		result.highlightTime = highlightTime = SDL_GetTicks();
		result.tooltip = nullptr;
	}

	result.clicked = false;
	if( highlighted ) {
		if( mainEngine->getMouseStatus(SDL_BUTTON_LEFT) ) {
			if( _sizePx.containsPoint(mousex,mousey) ) {
				result.pressed = pressed = (reallyPressed==false);
			} else {
				pressed = reallyPressed;
			}
		} else {
			if( pressed!=reallyPressed ) {
				result.clicked = true;
				if( style!=STYLE_NORMAL ) {
					reallyPressed = (reallyPressed==false);
				}
			}
			pressed = reallyPressed;
		}
	} else {
		pressed = reallyPressed;
	}

	return result;
}