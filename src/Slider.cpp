// Slider.cpp

#include "Slider.hpp"
#include "Renderer.hpp"

void Slider::draw(Renderer& renderer, Rect<int> _size, Rect<int> _actualSize) {
	Rect<int> _handleSize, _railSize;

	handleSize.x = railSize.x - handleSize.w / 2 + ((float)(value - minValue) / (maxValue - minValue)) * railSize.w;
	handleSize.y = railSize.y + railSize.h / 2 - handleSize.h / 2;

	// draw rail
	_railSize.x = _size.x + max(0, railSize.x - _actualSize.x);
	_railSize.y = _size.y + max(0, railSize.y - _actualSize.y);
	_railSize.w = min(railSize.w, _size.w - railSize.x + _actualSize.x) + min(0, railSize.x - _actualSize.x);
	_railSize.h = min(railSize.h, _size.h - railSize.y + _actualSize.y) + min(0, railSize.y - _actualSize.y);
	if (_railSize.w > 0 && _railSize.h > 0) {
		glm::vec4 _color = color * .5f;
		if (border) {
			renderer.drawLowFrame(_railSize, border, _color);
		} else {
			renderer.drawFrame(_railSize, border, _color);
		}
	}
	
	// draw handle
	_handleSize.x = _size.x + max(0, handleSize.x - _actualSize.x);
	_handleSize.y = _size.y + max(0, handleSize.y - _actualSize.y);
	_handleSize.w = min(handleSize.w, _size.w - handleSize.x + _actualSize.x) + min(0, handleSize.x - _actualSize.x);
	_handleSize.h = min(handleSize.h, _size.h - handleSize.y + _actualSize.y) + min(0, handleSize.y - _actualSize.y);
	if (_handleSize.w > 0 && _handleSize.h > 0) {
		glm::vec4 _color = disabled ? color * .5f : (highlighted ? color * 1.5f : color);
		if (border) {
			renderer.drawHighFrame(_handleSize, border, _color);
		} else {
			renderer.drawFrame(_handleSize, border, color);
			renderer.drawFrame(_handleSize, 1, _color, true);
		}
	}
}

Slider::result_t Slider::process(Rect<int> _size, Rect<int> _actualSize, const bool usable) {
	result_t result;
	result.tooltip = nullptr;
	result.highlightTime = SDL_GetTicks();
	result.highlighted = false;
	result.clicked = false;
	if (disabled) {
		highlightTime = result.highlightTime;
		highlighted = false;
		pressed = false;
		return result;
	}
	if (!usable) {
		highlightTime = result.highlightTime;
		highlighted = false;
		pressed = false;
		return result;
	}

	Rect<int> _handleSize, _railSize;

	handleSize.x = railSize.x - handleSize.w / 2 + ((float)(value - minValue) / (maxValue - minValue)) * railSize.w;
	handleSize.y = railSize.y + railSize.h / 2 - handleSize.h / 2;

	_railSize.x = _size.x + max(0, railSize.x - _actualSize.x);
	_railSize.y = _size.y + max(0, railSize.y - _actualSize.y);
	_railSize.w = min(railSize.w, _size.w - railSize.x + _actualSize.x) + min(0, railSize.x - _actualSize.x);
	_railSize.h = min(railSize.h, _size.h - railSize.y + _actualSize.y) + min(0, railSize.y - _actualSize.y);

	_handleSize.x = _size.x + max(0, handleSize.x - _actualSize.x);
	_handleSize.y = _size.y + max(0, handleSize.y - _actualSize.y);
	_handleSize.w = min(handleSize.w, _size.w - handleSize.x + _actualSize.x) + min(0, handleSize.x - _actualSize.x);
	_handleSize.h = min(handleSize.h, _size.h - handleSize.y + _actualSize.y) + min(0, handleSize.y - _actualSize.y);

	int offX = _size.x + railSize.x - _actualSize.x;
	_size.x = max(_size.x, _railSize.x - _handleSize.w / 2);
	_size.y = max(_size.y, _railSize.y + _railSize.h / 2 - _handleSize.h / 2);
	_size.w = min(_size.w, _railSize.w + _handleSize.w);
	_size.h = _handleSize.h;

	if (_size.w <= 0 || _size.h <= 0) {
		highlightTime = result.highlightTime;
		return result;
	}

	Sint32 mousex = (mainEngine->getMouseX() / (float)mainEngine->getXres()) * (float)Frame::virtualScreenX;
	Sint32 mousey = (mainEngine->getMouseY() / (float)mainEngine->getYres()) * (float)Frame::virtualScreenY;
	Sint32 omousex = (mainEngine->getOldMouseX() / (float)mainEngine->getXres()) * (float)Frame::virtualScreenX;
	Sint32 omousey = (mainEngine->getOldMouseY() / (float)mainEngine->getYres()) * (float)Frame::virtualScreenY;

	if (_size.containsPoint(omousex, omousey)) {
		result.highlighted = highlighted = true;
		result.highlightTime = highlightTime;
		result.tooltip = tooltip.get();
	} else {
		result.highlighted = highlighted = false;
		result.highlightTime = highlightTime = SDL_GetTicks();
		result.tooltip = nullptr;
	}

	result.clicked = false;
	if (highlighted) {
		if (mainEngine->getMouseStatus(SDL_BUTTON_LEFT)) {
			pressed = true;
			float oldValue = value;
			value = ((float)(mousex - offX) / railSize.w) * (float)(maxValue - minValue) + minValue;
			value = min(max(minValue, value), maxValue);
			if (oldValue != value) {
				result.clicked = true;
			}
		} else {
			pressed = false;
		}
	} else {
		pressed = false;
	}

	return result;
}
