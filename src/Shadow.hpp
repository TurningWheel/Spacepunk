// Shadow.hpp

#pragma once

class Shadow {
public:
	Shadow();
	~Shadow();

	// shadow map resolution
	static const int resolution = 512;

	// camera info
	struct camerainfo_t {
		GLenum face;
		Angle dir;
		static const float fov;
		static const Rect<Sint32> win;
		static const float clipNear;
		static const float clipFar;
	};
	static const int directions = 6;
	static const camerainfo_t cameraInfo[directions];

	// binds the shadow map for writing
	// @param face The face to bind for writing
	void bindForWriting(GLenum face);

	// binds the shadow map for reading
	// @param textureUnit The texture unit to bind for reading
	void bindForReading(GLenum textureUnit);

	// getters & setters
	const GLuint	getFBO() const			{ return fbo; }
	const GLuint	getShadowMap() const	{ return shadowMap; }

private:
	GLuint fbo = 0;
	GLuint shadowMap = 0;

	bool init();
};