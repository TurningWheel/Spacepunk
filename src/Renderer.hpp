//! @file Renderer.hpp

#pragma once

#include "Rect.hpp"
#include "Engine.hpp"
#include "Framebuffer.hpp"
#include "Resource.hpp"

class Font;
class Image;

//! Contains basic video and rendering context data as well as 2D drawing functions
class Renderer {
public:
	Renderer();
	~Renderer();

	//! color constants
	static const unsigned int colorBlack = 0xFF000000;
	static const unsigned int colorRed = 0xFFFF0000;
	static const unsigned int colorGreen = 0xFF00FF00;
	static const unsigned int colorBlue = 0xFF0000FF;
	static const unsigned int colorYellow = 0xFFFFFF00;
	static const unsigned int colorMagenta = 0xFFFF00FF;
	static const unsigned int colorCyan = 0xFF00FFFF;
	static const unsigned int colorWhite = 0xFFFFFFFF;

	//! fbo blit types
	enum BlitType {
		BASIC,
		HDR,
		BLUR_HORIZONTAL,
		BLUR_VERTICAL,
		GUI,
		MAX
	};

	const bool 					isInitialized() const { return (const bool)initialized; }
	const bool 					isFullscreen() const { return (const bool)fullscreen; }
	const Image*				getNullImage() const { return nullImg; }
	Resource<Framebuffer>&		getFramebufferResource() { return framebufferResource; }
	const char*					getCurrentFramebuffer() const { return currentFramebuffer.get(); }

	void	setFullscreen(bool _fullscreen) { fullscreen = _fullscreen; }
	void	setCurrentFramebuffer(const char* str) { currentFramebuffer = str; }

	//! sets up the renderer
	void init();

	//! gets the width of the active framebuffer
	int getXres();
	
	//! gets the height of the active framebuffer
	int getYres();

	//! gets the pixel at the given coordinates in the given SDL_Surface
	//! @param surface the SDL_Surface to inspect
	//! @param x the x coordinate of the pixel to get
	//! @param y the y coordinate of the pixel to get
	//! @return the 32-bit color value of the pixel
	static const Uint32 getPixel(const SDL_Surface* surface, const Uint32 x, const Uint32 y);

	//! sets a pixel in the given SDL_Surface
	//! @param surface the SDL_Surface to modify
	//! @param x the x coordinate of the pixel to set
	//! @param y the y coordinate of the pixel to set
	//! @param pixel the 32-bit color value to set the pixel to
	static void setPixel(SDL_Surface* surface, const Uint32 x, const Uint32 y, const Uint32 pixel);

	//! flip a surface
	//! @param surface the SDL_Surface to be flipped
	//! @param flags the directions to flip the surface in
	//! @return a flipped copy of the given SDL_Surface
	static SDL_Surface* flipSurface(SDL_Surface* surface, int flags);
	static const int flipHorizontal = 1;
	static const int flipVertical = 2;

	//! takes a screenshot of the current window and saves it as a timestamped .png
	void takeScreenshot();

	//! draws the command console at the given screenheight (0=top)
	void drawConsole(const Sint32 height, const char* input, const LinkedList<Engine::logmsg_t>& log, const Node<Engine::logmsg_t>* logStart);

	//! draw a raised frame for windows or buttons, etc.
	//! @param src the size and coordinates of the frame
	//! @param frameSize the size of the frame border in pixels
	//! @param color the frame's color
	//! @param hollow if true, the center of the frame will not be drawn
	void drawHighFrame(const Rect<int>& src, const int frameSize, const glm::vec4& color, const bool hollow = false);

	//! draw a lowered frame for windows or buttons, etc.
	//! @param src the size and coordinates of the frame
	//! @param frameSize the size of the frame border in pixels
	//! @param color the frame's color
	//! @param hollow if true, the center of the frame will not be drawn
	void drawLowFrame(const Rect<int>& src, const int frameSize, const glm::vec4& color, const bool hollow = false);

	//! draw a frame with a flattened edge
	//! @param src the size and coordinates of the frame
	//! @param frameSize the size of the frame border in pixels
	//! @param color the frame's color
	//! @param hollow if true, the center of the frame will not be drawn
	void drawFrame(const Rect<int>& src, const int frameSize, const glm::vec4& color, const bool hollow = false);

	//! draw a filled rectangle in screen space
	//! @param src the size and coordinates of the rectangle
	//! @param color the 32-bit color of the rectangle
	void drawRect(const Rect<int>* src, const glm::vec4& color);

	//! writes utf-8 text using a ttf font at the given screen coordinates
	//! @param font the font to use (if it is null, then nothing happens - saves you a check)
	//! @param rect the position and size of the rendered text
	//! @param str the str to print
	void printText(const Font* font, const Rect<int>& rect, const char* str);

	//! writes utf-8 text using a ttf font at the given screen coordinates
	//! @param font the font to use (if it is null, then nothing happens - saves you a check)
	//! @param rect the position and size of the rendered text
	//! @param color the color to blend the text with
	//! @param str the str to print
	void printTextColor(const Font* font, const Rect<int>& rect, const glm::vec4& color, const char* str);

	//! clears the screen and the z-buffer
	void clearBuffers();

	//! blits a framebuffer onto the window
	//! @param fbo the framebuffer to put on screen
	//! @param type what kind of technique to apply
	void blitFramebuffer(Framebuffer& fbo, GLenum attachment, BlitType type);

	//! combine two fbos together
	//! @param fbo0 first fbo to blend
	//! @param attachment0 buffer from first fbo to blend
	//! @param fbo1 second fbo to blend
	//! @param attachment1 buffer from second fbo to blend
	void blendFramebuffer(Framebuffer& fbo0, GLenum attachment0, Framebuffer& fbo1, GLenum attachment1);

	//! get the currently active fbo
	Framebuffer* getFramebuffer();

	//! refreshes the game window with the latest drawings. Called at the end of the frame
	void swapWindow();

	//! update the engine window to use the new render settings (resolution, etc.)
	//! @return true if video mode was successfully changed, false otherwise
	bool changeVideoMode();

	//! bind the fbo with the given name, creating a new one if it doesn't exist
	//! @param name the name of the fbo to retrieve
	//! @param width the width of the fbo
	//! @param height the height of the fbo
	//! @return the fbo
	Framebuffer* bindFBO(const char* name, int width, int height);

private:
	const char* loadStr = "Loading...";
	const Uint32 maxTextures = 1024;
	bool initialized = false;
	bool fullscreen;

	Resource<Framebuffer> framebufferResource;
	String currentFramebuffer;

	//! main window data
	SDL_Surface *mainsurface = nullptr;
	SDL_Window *window = nullptr;
	SDL_GLContext context = nullptr;
	bool glewWasInit = false;

	//! system images
	Image* nullImg = nullptr;

	static const GLuint indices[6];
	static const GLfloat positions[8];
	static const GLfloat texcoords[8];
	enum buffer_t {
		VERTEX_BUFFER,
		TEXCOORD_BUFFER,
		INDEX_BUFFER,
		BUFFER_TYPE_LENGTH
	};
	GLuint vbo[BUFFER_TYPE_LENGTH];
	GLuint vao = 0;

	int initVideo();
	int initResources();
};