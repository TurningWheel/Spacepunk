// Image.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "Image.hpp"

const GLfloat Image::positions[8] {
	0.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
	1.f, 0.f
};

const GLfloat Image::texcoords[8] {
	0.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
	1.f, 0.f
};

const GLuint Image::indices[6] {
	0, 1, 2,
	0, 2, 3
};

Image::Image(const char* _name) : Asset(_name) {
	bool clamp = false;
	if (_name && _name[0] == '#') {
		clamp = true;
		++_name;
	}
	path = mainEngine->buildPath(_name).get();
	
	mainEngine->fmsg(Engine::MSG_DEBUG,"loading image '%s'...",_name);
	if( (surf=IMG_Load(path.get()))==NULL ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to load image '%s'",_name);
		return;
	}

	// translate the original surface to an RGBA surface
	SDL_Surface* newSurf = SDL_CreateRGBSurface(0, surf->w, surf->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	SDL_BlitSurface(surf, nullptr, newSurf, nullptr); // blit onto a purely RGBA Surface
	SDL_FreeSurface(surf);
	surf = newSurf;

	// load the new surface as a GL texture
	SDL_LockSurface(surf);
	glGenTextures(1,&texid);
	glBindTexture(GL_TEXTURE_2D, texid);
	glTexStorage2D(GL_TEXTURE_2D, 4, GL_RGBA8, surf->w, surf->h);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->w, surf->h, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
	if (!clamp) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.f);
	glGenerateMipmap(GL_TEXTURE_2D);
	SDL_UnlockSurface(surf);

	// initialize buffer names
	for( int i=0; i<BUFFER_TYPE_LENGTH; ++i ) {
		vbo[static_cast<buffer_t>(i)] = 0;
	}

	// create vertex array
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// upload vertex data
	glGenBuffers(1, &vbo[VERTEX_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[VERTEX_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), positions, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	// upload texcoord data
	glGenBuffers(1, &vbo[TEXCOORD_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[TEXCOORD_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), texcoords, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);

	// upload index data
	glGenBuffers(1, &vbo[INDEX_BUFFER]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2 * 3 * sizeof(GLuint), indices, GL_STATIC_DRAW);

	// unbind vertex array
	glBindVertexArray(0);

	loaded = true;
}

Image::~Image() {
	if( surf ) {
		SDL_FreeSurface(surf);
		surf = nullptr;
	}
	if( texid ) {
		glDeleteTextures(1,&texid);
		texid = 0;
	}
	for( int i=0; i<BUFFER_TYPE_LENGTH; ++i ) {
		buffer_t buffer = static_cast<buffer_t>(i);
		if( vbo[buffer] ) {
			glDeleteBuffers(1,&vbo[buffer]);
		}
	}
	if( vao ) {
		glDeleteVertexArrays(1,&vao);
	}
}

void Image::draw( const Rect<int>* src, const Rect<int>& dest ) const {
	drawColor(src,dest,glm::vec4(1.f));
}

void Image::drawColor( const Rect<int>* src, const Rect<int>& dest, const glm::vec4& color ) const {
	int yres = mainEngine->getYres();
	int xres = mainEngine->getXres();

	// load shader
	Material* mat = mainEngine->getMaterialResource().dataForString("shaders/basic/2D.json");
	if( !mat ) {
		return;
	}
	ShaderProgram& shader = mat->getShader();
	if( &shader != ShaderProgram::getCurrentShader() ) {
		shader.mount();
	}

	glViewport(0, 0, xres, yres);
	glDisable(GL_DEPTH_TEST);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);

	// for the use of a whole image
	Rect<int> secondsrc;
	if( src==nullptr ) {
		secondsrc.x=0;
		secondsrc.y=0;
		secondsrc.w=surf->w;
		secondsrc.h=surf->h;
		src = &secondsrc;
	}

	// create view matrix
	glm::mat4 viewMatrix = glm::ortho( 0.f, (float)xres, 0.f, (float)yres, 1.f, -1.f );

	// bind texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texid);

	// upload uniform variables
	glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniform4fv(shader.getUniformLocation("gColor"), 1, glm::value_ptr(color));
	glUniform1i(shader.getUniformLocation("gTexture"), 0);

	// bind vertex array
	glBindVertexArray(vao);

	// upload positions
	GLfloat positions[8] = {
		(GLfloat)(dest.x), (GLfloat)(yres - dest.y),
		(GLfloat)(dest.x), (GLfloat)(yres - dest.y - dest.h),
		(GLfloat)(dest.x + dest.w), (GLfloat)(yres - dest.y - dest.h),
		(GLfloat)(dest.x + dest.w), (GLfloat)(yres - dest.y)
	};
	glBindBuffer(GL_ARRAY_BUFFER, vbo[VERTEX_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), positions, GL_DYNAMIC_DRAW);

	// upload texcoords
	GLfloat texcoords[8] = {
		(GLfloat)src->x / (GLfloat)surf->w, (GLfloat)src->y / (GLfloat)surf->h,
		(GLfloat)src->x / (GLfloat)surf->w, ((GLfloat)src->y + (GLfloat)src->h) / (GLfloat)surf->h,
		((GLfloat)src->x + (GLfloat)src->w) / (GLfloat)surf->w, ((GLfloat)src->y + (GLfloat)src->h) / (GLfloat)surf->h,
		((GLfloat)src->x + (GLfloat)src->w) / (GLfloat)surf->w, (GLfloat)src->y / (GLfloat)surf->h
	};
	glBindBuffer(GL_ARRAY_BUFFER, vbo[TEXCOORD_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), texcoords, GL_DYNAMIC_DRAW);

	// draw
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);
}