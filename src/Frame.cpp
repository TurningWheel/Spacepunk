// Frame.cpp

#include "Main.hpp"
#include "LinkedList.hpp"
#include "Button.hpp"
#include "Frame.hpp"
#include "Renderer.hpp"
#include "Engine.hpp"
#include "ShaderProgram.hpp"
#include "Script.hpp"
#include "Image.hpp"
#include "Field.hpp"

bool Frame::tabbing = false;
const Sint32 Frame::sliderSize = 15;

Frame::entry_t::~entry_t() {
	if( listener ) {
		listener->entry = nullptr;
	}
}

Frame::Frame(const char* _name, const char* _script) {
	size.x = 0;
	size.y = 0;
	size.w = 0;
	size.h = 0;

	actualSize.x = 0;
	actualSize.y = 0;
	actualSize.w = 0;
	actualSize.h = 0;

	color = glm::vec4(0.f);

	name = _name;
	scriptStr = _script;
	if( scriptStr.empty() ) {
		scriptStr = _name;
	}
	if( !scriptStr.empty() ) {
		scriptPath.format("scripts/client/gui/%s.lua", scriptStr.get());

		String filename = mainEngine->buildPath(scriptPath);
		FILE* fp = nullptr;
		if( (fp=fopen(filename.get(),"r")) != nullptr ) {
			fclose(fp);
			script = new Script(*this);

			int result = script->load(scriptPath.get());
			if( result==1 ) {
				scriptPath = "";
			}
		}
	}
}

Frame::Frame(Frame& _parent, const char* _name, const char* _script) : Frame(_name, _script) {
	parent = &_parent;
	_parent.getFrames().addNodeLast(this);
}

Frame::~Frame() {
	clear();
	if( script ) {
		delete script;
		script = nullptr;
	}
}

void Frame::draw(Renderer& renderer) {
	Frame::draw(renderer, size, actualSize);
}

void Frame::draw(Renderer& renderer, Rect<int> _size, Rect<int> _actualSize) {
	if( disabled )
		return;

	_size.x += max(0,size.x-_actualSize.x);
	_size.y += max(0,size.y-_actualSize.y);
	if( size.h < actualSize.h ) {
		_size.w = min( size.w-sliderSize, _size.w-sliderSize-size.x+_actualSize.x ) + min(0,size.x-_actualSize.x);
	} else {
		_size.w = min( size.w, _size.w-size.x+_actualSize.x ) + min(0,size.x-_actualSize.x);
	}
	if( size.w < actualSize.w ) {
		_size.h = min( size.h-sliderSize, _size.h-sliderSize-size.y+_actualSize.y ) + min(0,size.y-_actualSize.y);
	} else {
		_size.h = min( size.h, _size.h-size.y+_actualSize.y ) + min(0,size.y-_actualSize.y);
	}
	if( _size.w <= 0 || _size.h <= 0 )
		return;

	if( border>0 ) {
		if( high ) {
			renderer.drawHighFrame( _size, border, color, hollow );
		} else {
			renderer.drawLowFrame( _size, border, color, hollow );
		}
	} else {
		if( !hollow ) {
			renderer.drawRect( &_size, color );
		}
	}

	Sint32 omousex = mainEngine->getOldMouseX();
	Sint32 omousey = mainEngine->getOldMouseY();

	// horizontal slider
	if( actualSize.w > size.w ) {

		// slider rail
		Rect<int> barRect;
		barRect.x = _size.x;
		barRect.y = _size.y+_size.h;
		barRect.w = _size.w;
		barRect.h = sliderSize;
		if( border>0 ) {
			renderer.drawLowFrame( barRect, border, color * glm::vec4(.75f,.75f,.75f,1.f) );
		} else {
			renderer.drawRect( &barRect, color * glm::vec4(.5f,.5f,.5f,1.f) );
		}

		// handle
		float winFactor = ((float)_size.w / (float)actualSize.w);
		int handleSize = max( (int)(size.w * winFactor), sliderSize );
		int sliderPos = winFactor * actualSize.x;

		Rect<int> handleRect;
		handleRect.x = _size.x + sliderPos;
		handleRect.y = _size.y + _size.h;
		handleRect.w = handleSize;
		handleRect.h = sliderSize;
		if( border>0 ) {
			if( barRect.containsPoint(omousex,omousey) ) {
				renderer.drawHighFrame( handleRect, border, color*1.5f );
			} else {
				renderer.drawHighFrame( handleRect, border, color );
			}
		} else {
			if( barRect.containsPoint(omousex,omousey) ) {
				renderer.drawRect( &handleRect, color*2.f );
			} else {
				renderer.drawRect( &handleRect, color*1.5f );
			}
		}
	}

	// vertical slider
	if( actualSize.h > size.h && _size.y ) {
		Rect<int> barRect;
		barRect.x = _size.x+_size.w;
		barRect.y = _size.y;
		barRect.w = sliderSize;
		barRect.h = _size.h;
		if( border>0 ) {
			renderer.drawLowFrame( barRect, border, color * glm::vec4(.75f,.75f,.75f,1.f) );
		} else {
			renderer.drawRect( &barRect, color * glm::vec4(.75f,.75f,.75f,1.f) );
		}

		// handle
		float winFactor = ((float)_size.h / (float)actualSize.h);
		int handleSize = max( (int)(size.h * winFactor), sliderSize );
		int sliderPos = winFactor * actualSize.y;

		Rect<int> handleRect;
		handleRect.x = _size.x + _size.w;
		handleRect.y = _size.y + sliderPos;
		handleRect.w = sliderSize;
		handleRect.h = handleSize;
		if( border>0 ) {
			if( barRect.containsPoint(omousex,omousey) ) {
				renderer.drawHighFrame( handleRect, border, color*1.5f );
			} else {
				renderer.drawHighFrame( handleRect, border, color );
			}
		} else {
			if( barRect.containsPoint(omousex,omousey) ) {
				renderer.drawRect( &handleRect, color*2.f );
			} else {
				renderer.drawRect( &handleRect, color*1.5f );
			}
		}
	}

	// slider filler (at the corner between sliders)
	if( actualSize.w > size.w && actualSize.h > size.h ) {
		Rect<int> barRect;
		barRect.x = _size.x+_size.w;
		barRect.y = _size.y+_size.h;
		barRect.w = sliderSize;
		barRect.h = sliderSize;
		if( border>0 ) {
			if( high ) {
				renderer.drawHighFrame( barRect, border, color );
			} else {
				renderer.drawLowFrame( barRect, border, color );
			}
		} else {
			renderer.drawRect( &barRect, color );
		}
	}

	// render fields
	for( Node<Field*>* node=fields.getFirst(); node!=nullptr; node=node->getNext() ) {
		Field& field = *node->getData();
		field.draw(renderer,_size,actualSize);
	}

	// render images
	for( Node<image_t*>* node=images.getFirst(); node!=nullptr; node=node->getNext() ) {
		image_t& image = *node->getData();
		const Image* actualImage = image.image;
		if( actualImage==nullptr ) {
			actualImage = renderer.getNullImage();
		} else {
			// this is done just in case the cache was dumped...
			actualImage = image.image = mainEngine->getImageResource().dataForString(image.path.get());
			if( actualImage==nullptr ) {
				actualImage = renderer.getNullImage();
			} 
		}

		Rect<int> pos;
		pos.x = _size.x + image.pos.x - actualSize.x;
		pos.y =  _size.y + image.pos.y - actualSize.y;
		pos.w = image.pos.w > 0 ? image.pos.w : actualImage->getWidth();
		pos.h = image.pos.h > 0 ? image.pos.h : actualImage->getHeight();

		Rect<int> dest;
		dest.x = max( _size.x, pos.x );
		dest.y = max( _size.y, pos.y );
		dest.w = pos.w - ( dest.x - pos.x ) - max( 0, ( pos.x + pos.w ) - ( _size.x + _size.w ) );
		dest.h = pos.h - ( dest.y - pos.y ) - max( 0, ( pos.y + pos.h ) - ( _size.y + _size.h ) );

		Rect<int> src;
		src.x = max( 0, _size.x - pos.x );
		src.y = max( 0, _size.y - pos.y );
		src.w = pos.w - ( dest.x - pos.x ) - max( 0, ( pos.x + pos.w ) - ( _size.x + _size.w ) );
		src.h = pos.h - ( dest.y - pos.y ) - max( 0, ( pos.y + pos.h ) - ( _size.y + _size.h ) );

		actualImage->drawColor(&src,dest,image.color);
	}

	// render list entries
	int listStart = std::min( std::max( 0, actualSize.y / entrySize ), (int)list.getSize() - 1 );
	int i = listStart;
	Node<entry_t*>* node = node=list.nodeForIndex(listStart);
	for( ; node!=nullptr; node=node->getNext(), ++i ) {
		entry_t& entry = *node->getData();
		if (entry.text.empty()) {
			continue;
		}

		// get rendered text
		Text* text = mainEngine->getTextResource().dataForString(entry.text.get());
		if (text == nullptr) {
			continue;
		}

		// get the size of the rendered text
		int textSizeW = text->getWidth();
		int textSizeH = entrySize;

		Rect<int> pos;
		pos.x = _size.x + border - actualSize.x;
		pos.y = _size.y + border + i*entrySize - actualSize.y;
		pos.w = textSizeW;
		pos.h = textSizeH;

		Rect<int> dest;
		dest.x = max( _size.x, pos.x );
		dest.y = max( _size.y, pos.y );
		dest.w = pos.w - ( dest.x - pos.x ) - max( 0, ( pos.x + pos.w ) - ( _size.x + _size.w ) );
		dest.h = pos.h - ( dest.y - pos.y ) - max( 0, ( pos.y + pos.h ) - ( _size.y + _size.h ) );

		Rect<int> src;
		src.x = max( 0, _size.x - pos.x );
		src.y = max( 0, _size.y - pos.y );
		src.w = pos.w - ( dest.x - pos.x ) - max( 0, ( pos.x + pos.w ) - ( _size.x + _size.w ) );
		src.h = pos.h - ( dest.y - pos.y ) - max( 0, ( pos.y + pos.h ) - ( _size.y + _size.h ) );

		if( src.w<=0 || src.h<=0 || dest.w<=0 || dest.h<=0 )
			break;

		if( entry.pressed ) {
			renderer.drawRect( &dest, color*2.f );
		} else if( entry.highlighted ) {
			renderer.drawRect( &dest, color*1.5f );
		}

		text->drawColor( src, dest, entry.color );
	}

	// draw buttons
	for( Node<Button*>* node=buttons.getFirst(); node!=nullptr; node=node->getNext() ) {
		Button& button = *node->getData();
		button.draw(renderer,_size,actualSize);
	}

	// draw subframes
	for( Node<Frame*>* node=frames.getFirst(); node!=nullptr; node=node->getNext() ) {
		Frame& frame = *node->getData();
		frame.draw(renderer,_size,actualSize);
	}

	// root frame draws tooltip
	if( !parent ) {
		if( tooltip ) {
			Text* text = mainEngine->getTextResource().dataForString(tooltip);
			Rect<int> src;
			src.x = mainEngine->getMouseX() + 10;
			src.y = mainEngine->getMouseY();
			TTF_SizeUTF8( renderer.getMonoFont(), tooltip, &src.w, nullptr );
			src.h = text->getHeight();
			renderer.drawRect(&src, glm::vec4(0.f,0.f,1.f,.9f));
			Rect<int> src2(src.x + 3, src.y + 3, src.w - 6, src.h - 6);
			renderer.drawRect(&src2, glm::vec4(0.f,0.f,0.f,.9f));
			text->draw(Rect<int>(), Rect<int>(src.x,src.y,0,0));
		}
	}
}

Frame::result_t Frame::process() {
	result_t result = process(size, actualSize, true);

	tooltip = nullptr;
	if( result.tooltip ) {
		if( result.tooltip[0] != '\0' ) {
			if( SDL_GetTicks()-result.highlightTime >= tooltipTime ) {
				tooltip = result.tooltip;
			}
		}
	}

	return result;
}

Frame::result_t Frame::process(Rect<int> _size, Rect<int> _actualSize, bool usable) {
	result_t result;
	result.removed = false;
	result.usable = usable;
	result.highlightTime = SDL_GetTicks();
	result.tooltip = nullptr;

	if( disabled ) {
		return result;
	}
	if( mainEngine->isMouseRelative() ) {
		return result;
	}

	_size.x += max(0,size.x-_actualSize.x);
	_size.y += max(0,size.y-_actualSize.y);
	if( size.h < actualSize.h ) {
		_size.w = min( size.w-sliderSize, _size.w-sliderSize-size.x+_actualSize.x ) + min(0,size.x-_actualSize.x);
	} else {
		_size.w = min( size.w, _size.w-size.x+_actualSize.x ) + min(0,size.x-_actualSize.x);
	}
	if( size.w < actualSize.w ) {
		_size.h = min( size.h-sliderSize, _size.h-sliderSize-size.y+_actualSize.y ) + min(0,size.y-_actualSize.y);
	} else {
		_size.h = min( size.h, _size.h-size.y+_actualSize.y ) + min(0,size.y-_actualSize.y);
	}
	if( _size.w <= 0 || _size.h <= 0 )
		return result;

	// process frames
	Node<Frame*>* prevNode = nullptr;
	for( Node<Frame*>* node=frames.getLast(); node!=nullptr; node=prevNode ) {
		Frame& frame = *node->getData();

		prevNode = node->getPrev();

		result_t frameResult = frame.process(_size, actualSize, usable);
		if( !frameResult.removed ) {
			usable = result.usable = frameResult.usable;
			if( frameResult.tooltip!=nullptr ) {
				result = frameResult;
			}
		} else {
			// may our sad little frame rest in peace. amen
			delete node->getData();
			frames.removeNode(node);
		}
	}

	// process buttons
	Node<Button*>* prevButton = nullptr;
	for( Node<Button*>* node=buttons.getLast(); node!=nullptr; node=prevButton ) {
		Button& button = *node->getData();

		prevButton = node->getPrev();

		Button::result_t buttonResult = button.process(_size,actualSize,usable);
		if( usable && buttonResult.highlighted ) {
			result.highlightTime = buttonResult.highlightTime;
			result.tooltip = buttonResult.tooltip;
			if( buttonResult.clicked ) {
				if( name.empty() ) {
					mainEngine->fmsg(Engine::MSG_WARN,"button clicked in unnamed gui frame");
				} else {
					if( !button.getName() || button.getName()[0] == '\0' ) {
						mainEngine->fmsg(Engine::MSG_WARN,"unnamed button clicked in '%s' gui",name.get());
					} else if( script ) {
						Script::Args args(button.getParams());
						if (button.getClickCallback()) {
							(*button.getClickCallback())(args);
						} else {
							script->dispatch(button.getName(), &args);
						}
					}
				}
			}
			result.usable = usable = false;
		}
	}

	if( script ) {
		script->dispatch("process");

		Sint32 omousex = mainEngine->getOldMouseX();
		Sint32 omousey = mainEngine->getOldMouseY();

		// process the frame's list entries
		if( usable && list.getSize()>0 ) {
			int i;
			Node<entry_t*>* node = nullptr;
			Node<entry_t*>* nextnode = nullptr;
			for( i=0, node=list.getFirst(); node!=nullptr; node=nextnode, ++i ) {
				entry_t& entry = *node->getData();

				nextnode = node->getNext();

				if( entry.suicide ) {
					delete node->getData();
					list.removeNode(node);
					--i;
					continue;
				}

				Rect<int> entryRect;
				entryRect.x = _size.x + border - actualSize.x; entryRect.w = _size.w - border*2;
				entryRect.y = _size.y + border + i*entrySize - actualSize.y; entryRect.h = entrySize;

				if( _size.containsPoint(omousex,omousey) && entryRect.containsPoint(omousex,omousey) ) {
					result.highlightTime = entry.highlightTime;
					result.tooltip = entry.text.get();
					if( mainEngine->getMouseStatus(SDL_BUTTON_LEFT) ) {
						if( !entry.pressed ) {
							entry.pressed = true;
							if( script ) {
								if( mainEngine->getKeyStatus(SDL_SCANCODE_LCTRL) || mainEngine->getKeyStatus(SDL_SCANCODE_RCTRL) ) {
									StringBuf<64> dispatch("%sCtrlClick", 1, entry.name.get());
									Script::Args args(entry.params);
									script->dispatch(dispatch.get(), &args);
								} else {
									StringBuf<64> dispatch("%sClick", 1, entry.name.get());
									Script::Args args(entry.params);
									script->dispatch(dispatch.get(), &args);
								}
							}
						}
					} else {
						entry.pressed = false;
						if( script ) {
							StringBuf<64> dispatch("%sHighlighting", 1, entry.name.get());
							Script::Args args(entry.params);
							script->dispatch(dispatch.get(), &args);
							if( !entry.highlighted ) {
								entry.highlighted = true;
								StringBuf<64> dispatch("%sHighlight", 1, entry.name.get());
								Script::Args args(entry.params);
								script->dispatch(dispatch.get(), &args);
							}
						}
					}
					result.usable = usable = false;
				} else {
					entry.highlightTime = SDL_GetTicks();
					entry.highlighted = false;
					entry.pressed = false;
				}
			}
		}
	} else if( !name.empty() && !script ) {
		String filename = mainEngine->buildPath(scriptPath);
		FILE* fp = NULL;
		if( (fp=fopen(filename.get(),"r")) != NULL ) {
			fclose(fp);
			script = new Script(*this);
		}
	}

	// clear tabbing
	if( !mainEngine->getKeyStatus(SDL_SCANCODE_TAB) ) {
		tabbing = false;
	}

	// process fields
	Node<Field*>* prevField = nullptr;
	for( Node<Field*>* node=fields.getLast(); node!=nullptr; node=prevField ) {
		Field& field = *node->getData();

		prevField = node->getPrev();

		// handle tabbing
		Field* destField = nullptr;
		if( field.isSelected() ) {
			if( mainEngine->getKeyStatus(SDL_SCANCODE_TAB) && !tabbing ) {
				tabbing = true;
				Frame* gui = findHead();
				Frame* destFrame = gui->findFrame(field.getTabDestFrame());
				if( destFrame ) {
					destField = destFrame->findField(field.getTabDestField());
				}
			}
		}

		Field::result_t fieldResult = field.process(_size,actualSize,usable);
		if( usable ) {
			if( field.isSelected() && fieldResult.highlighted ) {
				result.usable = usable = false;
			}
			if( fieldResult.entered || destField ) {
				if( name.empty() ) {
					mainEngine->fmsg(Engine::MSG_WARN,"returned field in unnamed gui frame");
				} else {
					if( !field.getName() || field.getName()[0] == '\0' ) {
						mainEngine->fmsg(Engine::MSG_WARN,"unnamed field returned in '%s' gui",name.get());
					} else if( script ) {
						Script::Args args(field.getParams());
						args.addString(field.getText());
						script->dispatch(field.getName(), &args);
					}
				}
			}
		}

		if( destField ) {
			field.deselect();
			destField->select();
		}
	}

	// all but the parent frame will "capture" the mouse
	if( parent != nullptr ) {
		Sint32 mousex = mainEngine->getMouseX();
		Sint32 mousey = mainEngine->getMouseY();
		Sint32 omousex = mainEngine->getOldMouseX();
		Sint32 omousey = mainEngine->getOldMouseY();

		// main window space
		if( _size.containsPoint(omousex,omousey) ) {
			result.usable = false;

			// x scroll with mouse wheel x
			if( actualSize.w > size.w ) {
				if( mainEngine->getMouseWheelX()<0 ) {
					actualSize.x += min(entrySize*4,size.w);
				} else if( mainEngine->getMouseWheelX()>0 ) {
					actualSize.x -= min(entrySize*4,size.w);
				}
			}

			if( actualSize.h > size.h ) {
				// y scroll with mouse wheel y
				if( mainEngine->getMouseWheelY()<0 ) {
					actualSize.y += min(entrySize*4,size.h);
				} else if( mainEngine->getMouseWheelY()>0 ) {
					actualSize.y -= min(entrySize*4,size.h);
				}
			} else if( actualSize.w > size.w ) {
				// x scroll with mouse wheel y
				if( mainEngine->getMouseWheelY()<0 ) {
					actualSize.x += min(entrySize*4,size.w);
				} else if( mainEngine->getMouseWheelY()>0 ) {
					actualSize.x -= min(entrySize*4,size.w);
				}
			}

			// bound
			actualSize.x = min( max( 0, actualSize.x ), max( 0, actualSize.w-size.w ) );
			actualSize.y = min( max( 0, actualSize.y ), max( 0, actualSize.h-size.h ) );
		}

		// filler in between sliders
		if( actualSize.w > size.w && actualSize.h > size.h ) {
			Rect<int> sliderRect;
			sliderRect.x = _size.x+_size.w; sliderRect.w = sliderSize;
			sliderRect.y = _size.y+_size.h; sliderRect.h = sliderSize;
			if( sliderRect.containsPoint(omousex,omousey) ) {
				result.usable = false;
			}
		}

		// horizontal slider
		if( actualSize.w > size.w ) {
			Rect<int> sliderRect;
			sliderRect.x = _size.x; sliderRect.w = _size.w;
			sliderRect.y = _size.y+_size.h; sliderRect.h = sliderSize;
			if( sliderRect.containsPoint(omousex,omousey) ) {
				if( mainEngine->getMouseStatus(SDL_BUTTON_LEFT) ) {
					float winFactor = ((float)_size.w / (float)actualSize.w);
					actualSize.x = (mousex-_size.x) / winFactor - _size.w/2;
					actualSize.x = min( max( 0, actualSize.x ), max( 0, actualSize.w-size.w ) );
				}
				result.usable = false;
			}
		}

		// vertical slider
		if( actualSize.h > size.h ) {
			Rect<int> sliderRect;
			sliderRect.x = _size.x+_size.w; sliderRect.w = sliderSize;
			sliderRect.y = _size.y; sliderRect.h = _size.h;
			if( sliderRect.containsPoint(omousex,omousey) ) {
				if( mainEngine->getMouseStatus(SDL_BUTTON_LEFT) ) {
					float winFactor = ((float)_size.h / (float)actualSize.h);
					actualSize.y = (mousey-_size.y) / winFactor - _size.h/2;
					actualSize.y = min( max( 0, actualSize.y ), max( 0, actualSize.h-size.h ) );
				}
				result.usable = false;
			}
		}
	}

	// frame suicide :(
	if( toBeDeleted ) {
		result.removed = true;
	}

	return result;
}

Frame* Frame::addFrame(const char* name, const char* script) {
	return new Frame(*this,name,script);
}

Button* Frame::addButton(const char* name) {
	Button* button = new Button(*this);
	button->setName(name);
	return button;
}

Field* Frame::addField(const char* name, const int len) {
	Field* field = new Field(*this,len);
	field->setName(name);
	return field;
}

Frame::image_t* Frame::addImage( const Rect<Sint32>& pos, const glm::vec4& color, Image* image, const char* name ) {
	if (!image || !name) {
		return nullptr;
	}
	image_t* imageObj = new image_t();
	imageObj->pos = pos;
	imageObj->color = color;
	imageObj->name = name;
	imageObj->image = image;
	imageObj->path = image->getName();
	images.addNodeLast(imageObj);
	return imageObj;
}

Frame::entry_t* Frame::addEntry( const char* name, bool resizeFrame ) {
	entry_t* entry = new entry_t();
	entry->name = name;
	entry->color = glm::vec4(1.f);
	entry->image = nullptr;
	list.addNodeLast(entry);

	if( resizeFrame ) {
		resizeForEntries();
	}

	return entry;
}

void Frame::removeSelf() {
	toBeDeleted = true;
}

void Frame::clear() {
	// delete frames
	while( frames.getFirst() ) {
		delete frames.getFirst()->getData();
		frames.removeNode(frames.getFirst());
	}

	// delete buttons
	while( buttons.getFirst() ) {
		delete buttons.getFirst()->getData();
		buttons.removeNode(buttons.getFirst());
	}

	// delete fields
	while( fields.getFirst() ) {
		delete fields.getFirst()->getData();
		fields.removeNode(fields.getFirst());
	}

	// delete images
	while( images.getFirst() ) {
		delete images.getFirst()->getData();
		images.removeNode(images.getFirst());
	}

	// delete list
	while( list.getFirst() ) {
		delete list.getFirst()->getData();
		list.removeNode(list.getFirst());
	}
}

bool Frame::remove(const char* name) {
	for( Node<Frame*>* node=frames.getFirst(); node!=nullptr; node=node->getNext() ) {
		Frame& frame = *node->getData();
		if( strcmp(frame.getName(),name)==0 ) {
			delete node->getData();
			frames.removeNode(node);
			return true;
		}
	}
	for( Node<Button*>* node=buttons.getFirst(); node!=nullptr; node=node->getNext() ) {
		Button& button = *node->getData();
		if( strcmp(button.getName(),name)==0 ) {
			delete node->getData();
			buttons.removeNode(node);
			return true;
		}
	}
	for( Node<Field*>* node=fields.getFirst(); node!=nullptr; node=node->getNext() ) {
		Field& field = *node->getData();
		if( strcmp(field.getName(),name)==0 ) {
			delete node->getData();
			fields.removeNode(node);
			return true;
		}
	}
	for( Node<image_t*>* node=images.getFirst(); node!=nullptr; node=node->getNext() ) {
		image_t& image = *node->getData();
		if( strcmp(image.name.get(),name)==0 ) {
			delete node->getData();
			images.removeNode(node);
			return true;
		}
	}
	return false;
}

bool Frame::removeEntry( const char* name, bool resizeFrame ) {
	for( Node<entry_t*>* node=list.getFirst(); node!=nullptr; node=node->getNext() ) {
		entry_t& entry = *node->getData();
		if( entry.name==name ) {
			delete node->getData();
			list.removeNode(node);
			if( resizeFrame ) {
				resizeForEntries();
			}
			return true;
		}
	}
	return false;
}

Frame* Frame::findFrame( const char* name ) {
	for( Node<Frame*>* node=frames.getFirst(); node!=nullptr; node=node->getNext() ) {
		Frame& frame = *node->getData();
		if( strcmp(frame.getName(),name)==0 ) {
			return &frame;
		} else {
			Frame* subFrame = frame.findFrame(name);
			if( subFrame ) {
				return subFrame;
			}
		}
	}
	return nullptr;
}

Button* Frame::findButton( const char* name ) {
	for( Node<Button*>* node=buttons.getFirst(); node!=nullptr; node=node->getNext() ) {
		Button& button = *node->getData();
		if( strcmp(button.getName(),name)==0 ) {
			return &button;
		}
	}
	return nullptr;
}

Field* Frame::findField( const char* name ) {
	for( Node<Field*>* node=fields.getFirst(); node!=nullptr; node=node->getNext() ) {
		Field& field = *node->getData();
		if( strcmp(field.getName(),name)==0 ) {
			return &field;
		}
	}
	return nullptr;
}

Frame::image_t* Frame::findImage( const char* name ) {
	for( Node<image_t*>* node=images.getFirst(); node!=nullptr; node=node->getNext() ) {
		image_t& image = *node->getData();
		if( image.name==name ) {
			return &image;
		}
	}
	return nullptr;
}

Frame::entry_t* Frame::findEntry( const char* name ) {
	for( Node<entry_t*>* node=list.getFirst(); node!=nullptr; node=node->getNext() ) {
		entry_t& entry = *node->getData();
		if( entry.name==name ) {
			return &entry;
		}
	}
	return nullptr;
}

void Frame::resizeForEntries() {
	actualSize.h = (Uint32)list.getSize() * entrySize;
	actualSize.y = min( max( 0, actualSize.y ), max( 0, actualSize.h-size.h ) );
}

bool Frame::capturesMouse(Rect<int>* curSize, Rect<int>* curActualSize) {
	int xres = mainEngine->getXres();
	int yres = mainEngine->getYres();
	Rect<int> newSize = Rect<int>(0, 0, xres, yres);
	Rect<int> newActualSize = Rect<int>(0, 0, xres, yres);
	Rect<int>& _size = curSize ? *curSize : newSize;
	Rect<int>& _actualSize = curActualSize ? *curActualSize : newActualSize;
	
	if( parent ) {
		if( parent->capturesMouse(&_size, &_actualSize) ) {
			_size.x += max(0,size.x-_actualSize.x);
			_size.y += max(0,size.y-_actualSize.y);
			if( size.h < actualSize.h ) {
				_size.w = min( size.w-sliderSize, _size.w-sliderSize-size.x+_actualSize.x ) + min(0,size.x-_actualSize.x);
			} else {
				_size.w = min( size.w, _size.w-size.x+_actualSize.x ) + min(0,size.x-_actualSize.x);
			}
			if( size.w < actualSize.w ) {
				_size.h = min( size.h-sliderSize, _size.h-sliderSize-size.y+_actualSize.y ) + min(0,size.y-_actualSize.y);
			} else {
				_size.h = min( size.h, _size.h-size.y+_actualSize.y ) + min(0,size.y-_actualSize.y);
			}
			if( _size.w <= 0 || _size.h <= 0 ) {
				return false;
			} else {
				int omousex = mainEngine->getOldMouseX();
				int omousey = mainEngine->getOldMouseY();
				if( _size.containsPoint(omousex,omousey) ) {
					return true;
				} else {
					return false;
				}
			}
		} else {
			return false;
		}
	} else {
		return true;
	}
}

Frame* Frame::findHead() {
	if( parent ) {
		return parent->findHead();
	} else {
		return this;
	}
}
