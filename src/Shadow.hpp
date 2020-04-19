//! @file Shadow.hpp

#include "Main.hpp"
#include "Rotation.hpp"
#include "Rect.hpp"

#pragma once

//! Contains all the data associated with a shadow map created from a Light
class Shadow {
public:
	Shadow();
	~Shadow();

	//! camera info
	struct camerainfo_t {
		GLenum face;
		Rotation dir;
		static const float fov;
		static const float clipNear;
		static const float clipFar;
	};
	static const int directions = 6;
	static const camerainfo_t cameraInfo[directions];

	//! create gl data for shadowmap
	void init();

	//! delete gl data for shadowmap
	void term();

	//! binds the shadow map for writing
	//! @param face The face to bind for writing
	void bindForWriting(GLenum face);

	//! binds the shadow map for reading
	//! @param textureUnit The texture unit to bind for reading
	//! @param attachment The texture we wish to sample (GL_COLOR_ATTACHMENT0 or GL_DEPTH_ATTACHMENT or GL_STENCIL_ATTACHMENT)
	void bindForReading(GLenum textureUnit, GLenum attachment) const;

	const GLuint	getFBO() const { return fbo; }
	bool			isInitialized() const { return fbo != 0; }

private:
	GLuint fbo = 0;
	GLuint shadowMap = 0;
	GLuint uidMap = 0;
};

extern Cvar cvar_shadowResolution;