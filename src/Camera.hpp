//! @file Camera.hpp

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "Component.hpp"
#include "Rect.hpp"
#include "Cube.hpp"
#include "Line3D.hpp"
#include "Model.hpp"

class Renderer;
class World;

//! A Camera is an entity Component that provides a viewport into its World.
//! It also contains some 3D drawing functions and other data
class Camera : public Component {
public:
	//! drawing mode type
	enum drawmode_t {
		DRAW_BOUNDS,
		DRAW_DEPTH,

		//! these two for each light in a scene
		DRAW_STENCIL,
		DRAW_SHADOW,
		DRAW_STANDARD,

		//! additional fx passes
		DRAW_GLOW,
		DRAW_TRIANGLES,
		DRAW_DEPTHFAIL,
		DRAW_SILHOUETTE,

		DRAW_TYPE_LENGTH
	};

	//! point type
	struct point_t {
		unsigned int x;
		unsigned int y;
		glm::vec4 color;
	};

	//! line type
	struct line_t {
		Vector start;
		Vector end;
		glm::vec4 color;
	};

	//! occlusion query
	struct occlusion_query_t {
		bool result = false;
		GLuint id = 0;
	};

	Camera() = delete;
	Camera(Entity& _entity, Component* _parent);
	Camera(const Camera&) = delete;
	Camera(Camera&&) = delete;
	virtual ~Camera();

	//! camera model
	static const char* meshStr;
	static const char* materialStr;
	const Mesh::shadervars_t shaderVars;

	virtual type_t		getType() const override { return COMPONENT_CAMERA; }
	Renderer*&			getRenderer() { return renderer; }
	const float&		getClipNear() const { return clipNear; }
	const float&		getClipFar() const { return clipFar; }
	const Rect<Sint32>&	getWin() const { return win; }
	const Sint32&		getFov() const { return fov; }
	const glm::mat4&	getProjMatrix() const { return projMatrix; }
	const glm::mat4&	getViewMatrix() const { return viewMatrix; }
	const glm::mat4&	getProjViewMatrix() const { return projViewMatrix; }
	const drawmode_t	getDrawMode() const { return drawMode; }
	const bool&			isOrtho() const { return ortho; }
	unsigned int		getFramesDrawn() const { return (unsigned int)framesDrawn; }
	const bool&			isEnabled() const { return enabled; }
	Uint64				getOcclusionIndex() const { return occlusionIndex; }

	void	setClipNear(float _clipNear) { clipNear = _clipNear; }
	void	setClipFar(float _clipFar) { clipFar = _clipFar; }
	void	setWin(const Rect<Sint32>& rect) { win = rect; }
	void	setFov(Sint32 _fov) { fov = _fov; }
	void	setDrawMode(drawmode_t _drawMode) { drawMode = _drawMode; }
	void	setOrtho(const bool _ortho) { ortho = _ortho; }
	void	setEnabled(const bool _enabled) { enabled = _enabled; }
	void	setOcclusionIndex(Uint64 _occlusionIndex) { occlusionIndex = _occlusionIndex; }

	//! make a reversed-Z projection matrix with infinite range
	//! @param radians The vertical fov
	//! @param aspect The aspect ratio (w/h)
	//! @param zNear The near clip plane
	//! @return projection matrix
	static glm::mat4 makeInfReversedZProj(float radians, float aspect, float zNear);

	//! make an orthographic projection
	//! @param width the width of the view in pixels
	//! @param height the height of the view in pixels
	//! @param depth the depth of the view in pixels
	//! @return projection matrix
	static glm::mat4 makeOrthoProj(float width, float height, float depth);

	//! set this camera to be the current 3D listener
	void setListener();

	//! sets up the 3D projection for drawing
	//! @param scissor True if you want scissoring
	void setupProjection(bool scissor);

	//! resets the drawing matrices
	void resetMatrices();

	//! projects the given world position onto the screen
	//! @param original the world position to be projected
	//! return the screen position to be returned
	Vector worldPosToScreenPos(const Vector& original) const;

	//! determines origin and direction vectors for a ray that extends through a given point on the screen
	//! @param x the X position on the screen to peer through
	//! @param y the Y position on the screen to peer through
	//! @param out_origin the origin point of the ray to be returned
	//! @param out_direction the direction vector of the ray in world space to be returned
	void screenPosToWorldRay(int x, int y, Vector& out_origin, Vector& out_direction) const;

	//! draws a cube in the current camera view
	//! @param transform the transformation to apply to the cube
	//! @param color the color of the cube to draw
	void drawCube(const glm::mat4& transform, const glm::vec4& color);

	//! draws a 3d line in the current camera view
	//! @param width the width of the line in pixels
	//! @param src the starting point of the line in world space
	//! @param dest the ending point of the line in world space
	//! @param color the color of the line to draw
	void drawLine3D(const float width, const glm::vec3& src, const glm::vec3& dest, const glm::vec4& color);

	//! draws a laser in the current camera view
	//! @param width the width of the laser in pixels
	//! @param src the starting point of the laser in world space
	//! @param dest the ending point of the laser in world space
	//! @param color the color of the laser to draw
	void drawLaser(const float width, const glm::vec3& src, const glm::vec3& dest, const glm::vec4& color);

	//! marks a spot on the screen to draw a point
	//! @param x the x-coord of the point
	//! @param y the y-coord of the point
	//! @param color the color of the point
	void markPoint(unsigned int x, unsigned int y, const glm::vec4& color);

	//! marks a spot on the screen to draw a point
	//! @param pos the 3d coordinates of the point
	//! @param color the color of the point
	void markPoint(const Vector& pos, const glm::vec4& color);

	//! marks a spot on the screen to draw a line
	//! @param start the 3d coordinates of the line start
	//! @param end the 3d coordinates of the line end
	//! @param color the color of the point
	void markLine(const Vector& start, const Vector& end, const glm::vec4& color);

	//! draws the camera
	//! @param camera the camera through which to draw the camera
	//! @param light the light by which the camera should be illuminated (or nullptr for no illumination)
	virtual void draw(Camera& camera, const ArrayList<Light*>& lights) override;

	//! draws all marked points, lines, etc.
	void drawDebug();

	//! load the component from a file
	//! @param fp the file to read from
	virtual void load(FILE* fp) override;

	//! save/load this object to a file
	//! @param file interface to serialize with
	virtual void serialize(FileInterface* file) override;

	//! called when a frame is finished drawing from the camera
	void onFrameDrawn();

	//! clear all occlusion data
	void clearOcclusionData();

	//! get the occlusion query for a given entity
	//! @param entity the entity in question
	//! @return the occlusion query
	occlusion_query_t& getOcclusionQuery(Entity* entity);

	Camera& operator=(const Camera& src) {
		projMatrix = src.projMatrix;
		viewMatrix = src.viewMatrix;
		projViewMatrix = src.projViewMatrix;
		clipNear = src.clipNear;
		clipFar = src.clipFar;
		win = src.win;
		fov = src.fov;
		ortho = src.ortho;
		updateNeeded = true;
		return *this;
	}

	Camera& operator=(Camera&&) = delete;

protected:
	Renderer* renderer = nullptr;

	//! occlusion data
	Map<Uint64, Map<Entity*, occlusion_query_t>> occlusionData;
	Uint64 occlusionIndex = 0;

	//! drawing mode
	drawmode_t drawMode = DRAW_STANDARD;

	//! view and projection matrices
	glm::mat4 projMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projViewMatrix;

	//! viewport and projection variables
	float clipNear = 8.f;
	float clipFar = 1024.f;
	Rect<Sint32> win;
	Sint32 fov = 70;
	bool ortho = false;
	bool enabled = true;

	//! built-in basic objects
	Cube cube;
	Line3D line3D;
	LinkedList<point_t> points;
	LinkedList<line_t> lines;

	//! frame draw counter
	Uint32 framesDrawn = 0;
};
