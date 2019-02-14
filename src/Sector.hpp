// Sector.hpp

#pragma once

#define GLM_FORCE_RADIANS
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Main.hpp"
#include "ArrayList.hpp"
#include "String.hpp"
#include "Vector.hpp"

class Camera;
class Light;
class Material;
class SectorWorld;
class SectorVertex;

class Sector {
public:
	Sector(SectorWorld& _world);
	~Sector();

	struct vertex_t;
	struct face_t;

	friend class SectorWorld;

	// vertex
	struct vertex_t {
		~vertex_t();

		glm::vec3 position;
		glm::vec2 texcoord;

		face_t* face = nullptr;
		SectorVertex* joined = nullptr;
	};

	// triangle
	struct face_t {
		vertex_t vertices[3];
		Sector* sector = nullptr;
		Sector* neighbor = nullptr;
		bool selected = false;

		glm::vec3 normal;
		glm::vec3 tangent;
	};

	// default sector size
	static const float def;

	// find the face with the given normal
	// @param normal the normal to look for
	// @return the index of the face with the normal or -1 if it doesn't exist
	int findFaceWithNormal(const Vector& normal);

	// subdivide a given face into three faces
	// @param face the face to split
	// @param splitPoint where to split the face
	void splitFace(int faceIndex, const Vector& splitPoint);

	// build a new sector from an existing face
	// @param face the face to split
	// @param splitPoint where to split the face
	// @return a pointer to the new sector
	Sector* addSector(int faceIndex, const Vector& splitPoint);

	// joins two sectors by the given faces
	// @param myFace the face from this sector to join
	// @param theirFace the face from the other sector to join
	void connectSectors(int myFace, int theirFace);

	// uploads vertex data to gpu
	void uploadBuffers();

	// one frame update
	void process();

	// draws the sector
	// @param camera:
	void draw(Camera& camera, Light* light);

	// getters & setters
	const face_t*		getFace(Uint32 index) const			{ return faces[index]; }
	const void*			getMaterial() const					{ return materialStr.get(); }
	const bool			isUpdateNeeded() const				{ return updateNeeded; }

	void				setUpdateNeeded(const bool b)					{ updateNeeded = b; }
	void				setMaterial(const char* _material)				{ materialStr = _material; }

private:
	SectorWorld* world = nullptr;
	ArrayList<face_t*> faces;
	String materialStr;

	// editing variables
	bool selected = false;
	bool updateNeeded = false;

	enum buffer_t {
		POSITION_BUFFER,
		TEXCOORD_BUFFER,
		NORMAL_BUFFER,
		TANGENT_BUFFER,
		INDEX_BUFFER,
		BUFFER_TYPE_LENGTH
	};

	unsigned long numVertices = 0;
	unsigned long numIndices = 0;
	GLuint vbo[BUFFER_TYPE_LENGTH];
	GLuint vao = 0;

	ArrayList<glm::vec3> positionBuffer;
	ArrayList<glm::vec2> texcoordBuffer;
	ArrayList<glm::vec3> normalBuffer;
	ArrayList<glm::vec3> tangentBuffer;
	ArrayList<GLuint> indexBuffer;

	// bullet physics objects
	btDiscreteDynamicsWorld* dynamicsWorld = nullptr;
	btTriangleMesh* triMesh = nullptr;
	btCollisionShape* triMeshShape = nullptr;
	btDefaultMotionState* motionState = nullptr;
	btRigidBody* rigidBody = nullptr;

	// loads a shader needed to render this sector
	// @param camera the camera to render with
	// @param light the light to render with, if any
	// @param material the material to render with
	void loadShader(Camera& camera, Light* light, Material* material);

	// update vertex count based on number of faces w/ neighbors
	void countVertices();

	// update physics mesh
	void buildPhysicsMesh();
};