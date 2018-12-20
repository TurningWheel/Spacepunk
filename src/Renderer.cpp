// Renderer.cpp

#include "Main.hpp"
#include "LinkedList.hpp"
#include "Node.hpp"
#include "Renderer.hpp"
#include "Engine.hpp"
#include "savepng.hpp"
#include "Text.hpp"

Renderer::Renderer() {
	xres = mainEngine->getXres();
	yres = mainEngine->getYres();
	fullscreen = mainEngine->isFullscreen();
}

Renderer::~Renderer() {
	if( window ) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}
	if( context ) {
		SDL_GL_DeleteContext(context);
		context = nullptr;
	}
	if( mainsurface ) {
		SDL_FreeSurface(mainsurface);
		mainsurface = nullptr;
	}

	if( nullImg )
		delete nullImg;
	if( monoFont )
		TTF_CloseFont(monoFont);
}

void Renderer::init() {
	if( initVideo() )
		return;
	if( initResources() )
		return;
	initialized = true;
}

int Renderer::initVideo() {
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
#ifdef NDEBUG
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
#else
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
#endif

	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 4 );

	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );

	mainEngine->fmsg(Engine::MSG_INFO,"setting display mode to %dx%d...",xres,yres);

	Uint32 flags = 0;
	if( fullscreen )
		flags |= SDL_WINDOW_FULLSCREEN;
	flags |= SDL_WINDOW_OPENGL;

	if( !window ) {
		if((window=SDL_CreateWindow( mainEngine->getGameTitle(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, xres, yres, flags )) == nullptr) {
			mainEngine->fmsg(Engine::MSG_ERROR,"failed to set video mode: %s",SDL_GetError());
			return 1;
		}
	} else {
		SDL_SetWindowSize(window,xres,yres);
		if( fullscreen ) {
			SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN);
		} else {
			SDL_SetWindowFullscreen(window,0);
		}
		SDL_SetWindowPosition(window,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);
	}
	if( !context ) {
		context = SDL_GL_CreateContext(window);
		if(context == nullptr) {
			mainEngine->fmsg(Engine::MSG_ERROR,"failed to create GL context: %s",SDL_GetError());
			return 1;
		}
	}
	SDL_GL_MakeCurrent(window,context);

	int result=0;
	SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE,&result);


#ifndef PLATFORM_LINUX
	// get opengl extensions
	if( !glewWasInit ) {
		glewExperimental=GL_TRUE;
		GLenum err = glewInit();
		if( err != GLEW_OK ) {
			mainEngine->fmsg(Engine::MSG_ERROR,"failed to load OpenGL 4.3 extensions. You may have to update your drivers.");
			return 1;
		} else {
			glewWasInit = true;
		}
	}
#endif

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	Uint32 rmask = 0xff000000;
	Uint32 gmask = 0x00ff0000;
	Uint32 bmask = 0x0000ff00;
	Uint32 amask = 0x000000ff;
#else
	Uint32 rmask = 0x000000ff;
	Uint32 gmask = 0x0000ff00;
	Uint32 bmask = 0x00ff0000;
	Uint32 amask = 0xff000000;
#endif

	if( !mainsurface ) {
		if((mainsurface=SDL_CreateRGBSurface(0,xres,yres,32,rmask,gmask,bmask,amask)) == nullptr) {
			mainEngine->fmsg(Engine::MSG_ERROR,"failed to create main window surface: %s",SDL_GetError());
			return 1;
		}
	}

	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	glEnable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	glClearColor( 0.f, 0.f, 0.f, 0.f );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	swapWindow();

	mainEngine->fmsg(Engine::MSG_INFO,"display changed successfully.");
	return 0;
}

int Renderer::initResources() {
	// load null texture
	if( nullImg ) {
		delete nullImg;
	}
	if( (nullImg=new Image("images/system/null.png")) == nullptr ) {
		return 1;
	}

	// load font
	if( monoFont ) {
		TTF_CloseFont(monoFont);
	}

	String filename = mainEngine->buildPath("fonts/mono.ttf").get();
	//int pointSize = 16.f * (yres / 720.f); // font size
	int pointSize = 16;
	if( (monoFont=TTF_OpenFont(filename.get(),pointSize)) == NULL ) {
		mainEngine->fmsg(Engine::MSG_CRITICAL,"failed to load '%s': %s",filename.get(),TTF_GetError());
		return 1;
	}
	TTF_SetFontHinting(monoFont,TTF_HINTING_MONO);
	TTF_SetFontKerning(monoFont,0);

	return 0;
}

bool Renderer::changeVideoMode() {
	mainEngine->fmsg(Engine::MSG_INFO,"changing video mode.");

	// free text resource (it's all badly wrapped now)
	mainEngine->getTextResource().dumpCache();

	if( mainsurface ) {
		SDL_FreeSurface(mainsurface);
		mainsurface = nullptr;
	}

	// set video mode
	if( initVideo() ) {
		setXres(1280);
		setYres(720);
		setFullscreen(false);
		mainEngine->fmsg(Engine::MSG_WARN,"failed to set video mode to desired values, defaulting to safe video mode...");
		if( initVideo() ) {
			mainEngine->fmsg(Engine::MSG_ERROR,"failed to set video mode to safe video mode, aborting");
			return false;
		}
	}

	// reload default assets
	initResources();

	// success
	return true;
}

const Uint32 Renderer::getPixel(const SDL_Surface* surface, const Uint32 x, const Uint32 y) {
	int bpp = surface->format->BytesPerPixel;
	// Here p is the address to the pixel we want to retrieve
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
	case 1:
		return *p;
		break;

	case 2:
		return *(Uint16 *)p;
		break;

	case 3:
		if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
			return p[0] << 16 | p[1] << 8 | p[2];
		else
			return p[0] | p[1] << 8 | p[2] << 16;
		break;

	case 4:
		return *(Uint32 *)p;
		break;

	default:
		return 0;	   /* shouldn't happen, but avoids warnings */
	}
}

void Renderer::setPixel(SDL_Surface* surface, const Uint32 x, const Uint32 y, const Uint32 pixel) {
	int bpp = surface->format->BytesPerPixel;

	// Here p is the address to the pixel we want to set
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
	case 1:
		*p = pixel;
		break;

	case 2:
		*(Uint16 *)p = pixel;
		break;

	case 3:
		if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			p[0] = (pixel >> 16) & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = pixel & 0xff;
		} else {
			p[0] = pixel & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = (pixel >> 16) & 0xff;
		}
		break;

	case 4:
		*(Uint32 *)p = pixel;
		break;
	}
}

SDL_Surface* Renderer::flipSurface( SDL_Surface* surface, int flags ) {
	SDL_Surface *flipped = nullptr;
	Uint32 pixel;
	int x, rx;
	int y, ry;

	// prepare surface for flipping
	flipped = SDL_CreateRGBSurface( SDL_SWSURFACE, surface->w, surface->h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask );
	if( SDL_MUSTLOCK( surface ) ) {
		SDL_LockSurface( surface );
	}
	if( SDL_MUSTLOCK( flipped ) ) {
		SDL_LockSurface( flipped );
	}

	for( x=0, rx=flipped->w-1; x<flipped->w; ++x, --rx ) {
		for( y=0, ry=flipped->h-1; y<flipped->h; ++y, --ry ) {
			pixel = getPixel( surface, x, y );

			// copy pixel
			if( ( flags & flipVertical ) && ( flags & flipHorizontal ) ) {
				setPixel( flipped, rx, ry, pixel );
			} else if( flags & flipHorizontal ) {
				setPixel( flipped, rx, y, pixel );
			} else if( flags & flipVertical ) {
				setPixel( flipped, x, ry, pixel );
			}
		}
	}

	// restore image
	if( SDL_MUSTLOCK( surface ) ) {
		SDL_UnlockSurface( surface );
	}
	if( SDL_MUSTLOCK( flipped ) ) {
		SDL_UnlockSurface( flipped );
	}

	return flipped;
}

void Renderer::takeScreenshot() {
	// get timestamp
	time_t timer;
	struct tm* tm_info;
	time(&timer);
	tm_info = localtime(&timer);

	// build filename
	char buffer[32];
	char filename[256];
	strftime( buffer, 32, "%Y-%m-%d %H-%M-%S", tm_info );
	snprintf( filename, 256, "Screenshot %s.png", buffer );

	unsigned char* pixels = new unsigned char[xres*yres*3]; // 3 bytes for BGR
	glReadPixels(0,0,xres,yres,GL_BGR,GL_UNSIGNED_BYTE,pixels);
	SDL_Surface* temp = SDL_CreateRGBSurfaceFrom(pixels,xres,yres,24,xres*3,0,0,0,0);
	if( temp ) {
		SDL_Surface* temp2 = Renderer::flipSurface(temp,flipVertical);
		SDL_FreeSurface(temp);
		SDL_Surface* temp = SDL_CreateRGBSurface(0,xres,yres,24,0,0,0,0);
		SDL_FillRect(temp,NULL,colorBlack);
		SDL_Rect dest;
		dest.x = 0; dest.y = 0;
		dest.w = 0; dest.h = 0;
		SDL_BlitSurface(temp2,NULL,temp,&dest);
		SDL_FreeSurface(temp2);

		SDL_SavePNG(temp,filename);
		SDL_FreeSurface(temp);
		mainEngine->fmsg(Engine::MSG_INFO,"saved %s",filename);
	} else {
		mainEngine->fmsg(Engine::MSG_WARN,"failed to save %s",filename);
	}
	delete[] pixels;
}

void Renderer::drawHighFrame( const Rect<int>& src, const int frameSize, const glm::vec4& color, const bool hollow ) {
	Image* image = mainEngine->getImageResource().dataForString("images/system/white.png");
	if (!image) {
		return;
	}

	// draw top
	if (frameSize > 0) {
		glm::vec4 brightColor = color*1.5f; brightColor.a = color.a;
		Rect<int> size;
		size.x = src.x;
		size.y = src.y;
		size.w = src.w;
		size.h = frameSize;
		image->drawColor(nullptr, size, brightColor);
	}

	// draw left
	if (frameSize > 0) {
		glm::vec4 brightColor = color*1.5f; brightColor.a = color.a;
		Rect<int> size;
		size.x = src.x;
		size.y = src.y + frameSize;
		size.w = frameSize;
		size.h = src.h - frameSize;
		image->drawColor(nullptr, size, brightColor);
	}

	// draw bottom
	if (frameSize > 0) {
		glm::vec4 darkColor = color*.75f; darkColor.a = color.a;
		Rect<int> size;
		size.x = src.x + frameSize;
		size.y = src.y + src.h - frameSize;
		size.w = src.w - frameSize;
		size.h = frameSize;
		image->drawColor(nullptr, size, darkColor);
	}

	// draw right
	if (frameSize > 0) {
		glm::vec4 darkColor = color*.75f; darkColor.a = color.a;
		Rect<int> size;
		size.x = src.x + src.w - frameSize;
		size.y = src.y + frameSize;
		size.w = frameSize;
		size.h = src.h - frameSize * 2;
		image->drawColor(nullptr, size, darkColor);
	}

	// draw center rectangle
	if (!hollow) {
		Rect<int> size;
		size.x = src.x + frameSize;
		size.y = src.y + frameSize;
		size.w = src.w - frameSize * 2;
		size.h = src.h - frameSize * 2;
		image->drawColor(nullptr, size, color);
	}
}

void Renderer::drawLowFrame( const Rect<int>& src, const int frameSize, const glm::vec4& color, const bool hollow ) {
	Image* image = mainEngine->getImageResource().dataForString("images/system/white.png");
	if (!image) {
		return;
	}

	// draw top
	if (frameSize > 0) {
		glm::vec4 darkColor = color*.75f; darkColor.a = color.a;
		Rect<int> size;
		size.x = src.x;
		size.y = src.y;
		size.w = src.w;
		size.h = frameSize;
		image->drawColor(nullptr, size, darkColor);
	}

	// draw left
	if (frameSize > 0) {
		glm::vec4 darkColor = color*.75f; darkColor.a = color.a;
		Rect<int> size;
		size.x = src.x;
		size.y = src.y + frameSize;
		size.w = frameSize;
		size.h = src.h - frameSize;
		image->drawColor(nullptr, size, darkColor);
	}

	// draw bottom
	if (frameSize > 0) {
		glm::vec4 brightColor = color*1.5f; brightColor.a = color.a;
		Rect<int> size;
		size.x = src.x + frameSize;
		size.y = src.y + src.h - frameSize;
		size.w = src.w - frameSize;
		size.h = frameSize;
		image->drawColor(nullptr, size, brightColor);
	}

	// draw right
	if (frameSize > 0) {
		glm::vec4 brightColor = color*1.5f; brightColor.a = color.a;
		Rect<int> size;
		size.x = src.x + src.w - frameSize;
		size.y = src.y + frameSize;
		size.w = frameSize;
		size.h = src.h - frameSize * 2;
		image->drawColor(nullptr, size, brightColor);
	}

	// draw center rectangle
	if (!hollow) {
		Rect<int> size;
		size.x = src.x + frameSize;
		size.y = src.y + frameSize;
		size.w = src.w - frameSize * 2;
		size.h = src.h - frameSize * 2;
		image->drawColor(nullptr, size, color);
	}
}

void Renderer::drawConsole( const Sint32 height, const char* input, const LinkedList<Engine::logmsg_t>& log, const Node<Engine::logmsg_t>* logStart ) {
	Image* image = mainEngine->getImageResource().dataForString("images/system/white.png");
	if (!image) {
		return;
	}

	// draw main rectangle
	{
		Rect<int> size;
		size.x = 0;
		size.y = 0;
		size.w = xres;
		size.h = height - 3;
		glm::vec4 color(0.f, 0.f, 0.25f, 0.75f);
		image->drawColor(nullptr, size, color);
	}

	// draw low border
	{
		Rect<int> size;
		size.x = 0;
		size.y = height - 3;
		size.w = xres;
		size.h = 3;
		glm::vec4 color(0.f, 0.f, 0.5f, 0.75f);
		image->drawColor(nullptr, size, color);
	}

	// log contents
	int y = height-20;
	if( logStart==nullptr ) {
		logStart = log.getLast();
	} else {
		int w, h;
		TTF_SizeUTF8(monoFont,"^",&w,&h);
		y -= h;
		int c=0;
		for( int x=0; x+w<xres; x+=w, ++c );
		char* arrows = (char*) calloc(c+1,sizeof(char));
		if( arrows ) {
			for( int i=0; i<c; ++i ) {
				arrows[i] = '^';
			}
			Rect<int> pos;
			pos.x = 5; pos.w = 0;
			pos.y = y; pos.h = 0;
			printTextColor( pos, glm::vec4(1.f,0.f,0.f,1.f), arrows );
			free(arrows);
		}
	}
	for( const Node<Engine::logmsg_t>* node=logStart; node!=nullptr; node=node->getPrev() ) {
		const Engine::logmsg_t& logMsg = node->getData();
		const String* str = &logMsg.text;
		Text* text = mainEngine->getTextResource().dataForString((*str).get());
		if( text ) {
			y -= TTF_FontHeight(monoFont);
			Rect<int> pos;
			pos.x = 5; pos.w = 0;
			pos.y = y; pos.h = 0;
			text->drawColor(Rect<int>(),pos,glm::vec4(logMsg.color,1.f));
		}
		if( y < -TTF_FontHeight(monoFont) ) {
			break;
		}
	}
	Rect<int> pos;
	pos.x = 5; pos.w = 0;
	pos.y = height-20; pos.h = 0;
	if( mainEngine->isCursorVisible() ) {
		StringBuf<256> text(">%s_", input);
		printText( pos, text.get() );
	} else {
		StringBuf<256> text(">%s", input);
		printText( pos, text.get() );
	}
}

void Renderer::drawRect( const Rect<int>* src, const glm::vec4& color ) {
	Image* image = mainEngine->getImageResource().dataForString("images/system/white.png");
	if (!image) {
		return;
	}

	// for the use of the whole screen
	Rect<int> secondsrc;
	if( src==nullptr ) {
		secondsrc.x=0;
		secondsrc.y=0;
		secondsrc.w=xres;
		secondsrc.h=yres;
		src = &secondsrc;
	}

	// draw quad
	image->drawColor(nullptr, *src, color);
}

void Renderer::printText( const Rect<int>& rect, const char* str ) {
	printTextColor(rect,glm::vec4(1.f),str);
}

void Renderer::printTextColor( const Rect<int>& rect, const glm::vec4& color, const char* str ) {
	if( str == nullptr || str[0] == '\0' ) {
		return;
	}
	Text* text = mainEngine->getTextResource().dataForString(str);
	if( text ) {
		text->drawColor(Rect<int>(), rect, color);
	}
}

void Renderer::clearBuffers() {
	glEnable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	glClearColor( 0.f, 0.f, 0.f, 0.f );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	drawRect(nullptr,glm::vec4(0.f,0.f,0.f,1.f));
}

void Renderer::swapWindow() {
	SDL_GL_SwapWindow(window);
}
