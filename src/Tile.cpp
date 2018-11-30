// Tile.cpp

using namespace std;

#include "Main.hpp"

#ifdef PLATFORM_LINUX
#include <btBulletDynamicsCommon.h>
#else
#include <bullet3/btBulletDynamicsCommon.h>
#endif

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "LinkedList.hpp"
#include "Node.hpp"
#include "Engine.hpp"
#include "Entity.hpp"
#include "Tile.hpp"
#include "Light.hpp"
#include "TileWorld.hpp"
#include "ShaderProgram.hpp"
#include "Client.hpp"
#include "Camera.hpp"
#include "Material.hpp"

const char* Tile::defaultTexture = "images/tile/null.json";

Tile::Tile() {
	// default texture for ceiling
	ceilingTexture = 0;

	// default texture for floor
	floorTexture = 0;

	// iterate through each cardinal direction
	for( int sideInt=SIDE_EAST; sideInt<SIDE_TYPE_LENGTH; ++sideInt ) {
		side_t side = static_cast<side_t>(sideInt);
		upperTextures[side] = 0;
		lowerTextures[side] = 0;
	}
}

Tile::~Tile() {
	if( triMesh!=nullptr ) {
		delete triMesh;
	}
	if( triMeshShape!=nullptr ) {
		delete triMeshShape;
	}
	if( motionState!=nullptr ) {
		delete motionState;
	}
	if( rigidBody!=nullptr ) {
		dynamicsWorld->removeRigidBody(rigidBody);
		delete rigidBody;
	}
}

ShaderProgram* Tile::loadShader(const TileWorld& world, const Camera& camera, const ArrayList<Light*>& lights) {
	Client* client = mainEngine->getLocalClient();
	if( !client )
		return nullptr;

	const char* materialName = nullptr;
	switch( camera.getDrawMode() ) {
		case Camera::DRAW_DEPTH:
			materialName = "shaders/tile/depth.json";
			break;
		case Camera::DRAW_SILHOUETTE:
			materialName = "shaders/tile/silhouette.json";
			break;
		case Camera::DRAW_STENCIL:
			materialName = "shaders/tile/stencil.json";
			break;
		case Camera::DRAW_TRIANGLES:
			materialName = "shaders/tile/triangles.json";
			break;
		default:
			materialName = "shaders/tile/std.json";
			break;
	}
	Material* mat = mainEngine->getMaterialResource().dataForString(materialName);
	if( !mat ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to load tile material");
		return nullptr;
	}
	ShaderProgram& shader = mat->getShader();

	glLineWidth(1);

	if( &shader != ShaderProgram::getCurrentShader() ) {
		shader.mount();
	}
	if( camera.getDrawMode()==Camera::DRAW_DEPTH ) {

		// load projection matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(camera.getProjViewMatrix()));
	} else if( camera.getDrawMode()==Camera::DRAW_SILHOUETTE ) {

		// load projection matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(camera.getProjViewMatrix()));

		// load camera position into shader
		glm::vec3 cameraPos( camera.getGlobalPos().x, -camera.getGlobalPos().z, camera.getGlobalPos().y );
		glUniform3fv(shader.getUniformLocation("gCameraPos"), 1, glm::value_ptr(cameraPos));
	} else if( camera.getDrawMode() == Camera::DRAW_TRIANGLES ) {

		// load projection matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(camera.getProjViewMatrix()));
	} else if( camera.getDrawMode()==Camera::DRAW_STENCIL ) {

		// load projection matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(camera.getProjViewMatrix()));

		// load light data into shader
		if( lights.getSize() ) {
			glm::vec3 lightPos( lights[0]->getGlobalPos().x, -lights[0]->getGlobalPos().z, lights[0]->getGlobalPos().y );
			glUniform3fv(shader.getUniformLocation("gLightPos"), 1, glm::value_ptr(lightPos));
		} else {
			glm::vec3 lightPos( camera.getGlobalPos().x, -camera.getGlobalPos().z, camera.getGlobalPos().y );
			glUniform3fv(shader.getUniformLocation("gLightPos"), 1, glm::value_ptr(lightPos));
		}
		glUniform1i(shader.getUniformLocation("gNumLights"), 1);
	} else {

		// load projection matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(camera.getProjViewMatrix()));

		glm::vec3 cameraPos( camera.getGlobalPos().x, -camera.getGlobalPos().z, camera.getGlobalPos().y );
		glUniform3fv(shader.getUniformLocation("gCameraPos"), 1, glm::value_ptr(cameraPos));

		if( camera.getDrawMode() == Camera::DRAW_STANDARD ) {
			glUniform1i(shader.getUniformLocation("gActiveLight"), GL_TRUE);
		} else {
			glUniform1i(shader.getUniformLocation("gActiveLight"), GL_FALSE);
		}

		// load light data into shader
		if( lights.getSize() ) {
			Uint32 index = 0;
			StringBuf<32> buf;
			for (auto light : lights) {
				Vector lightAng = light->getGlobalAng().toVector();
				glm::vec3 lightDir( lightAng.x, -lightAng.z, lightAng.y );
				glm::vec3 lightPos( light->getGlobalPos().x, -light->getGlobalPos().z, light->getGlobalPos().y );
				glm::vec3 lightScale( light->getGlobalScale().x, -light->getGlobalScale().z, light->getGlobalScale().y );

				glUniform3fv(shader.getUniformLocation(buf.format("gLightPos[%d]",index)), 1, glm::value_ptr(lightPos));
				glUniform4fv(shader.getUniformLocation(buf.format("gLightColor[%d]",index)), 1, glm::value_ptr(glm::vec3(light->getColor())));
				glUniform1f(shader.getUniformLocation(buf.format("gLightIntensity[%d]",index)), light->getIntensity());
				glUniform1f(shader.getUniformLocation(buf.format("gLightRadius[%d]",index)), light->getRadius());
				glUniform3fv(shader.getUniformLocation(buf.format("gLightScale[%d]",index)), 1, glm::value_ptr(lightScale));
				glUniform3fv(shader.getUniformLocation(buf.format("gLightDirection[%d]",index)), 1, glm::value_ptr(lightDir));
				glUniform1i(shader.getUniformLocation(buf.format("gLightShape[%d]",index)), static_cast<GLint>(light->getShape()));
				++index;
				if (index >= maxLights) {
					break;
				}
			}
			glUniform1i(shader.getUniformLocation("gNumLights"), (GLint)lights.getSize());
		} else if (mainEngine->isEditorRunning() && world.isShowTools()) {
			glUniform3fv(shader.getUniformLocation("gLightPos[0]"), 1, glm::value_ptr(cameraPos));
			glUniform4fv(shader.getUniformLocation("gLightColor[0]"), 1, glm::value_ptr(glm::vec4(1,1,1,1)));
			glUniform1f(shader.getUniformLocation("gLightIntensity[0]"), 1);
			glUniform1f(shader.getUniformLocation("gLightRadius[0]"), 16384.f);
			glUniform3fv(shader.getUniformLocation("gLightScale[0]"), 1, glm::value_ptr(glm::vec3(1.f,1.f,1.f)));
			glUniform1i(shader.getUniformLocation("gLightShape[0]"), 0);
			glUniform1i(shader.getUniformLocation("gNumLights"), 1);
		} else {
			glUniform1i(shader.getUniformLocation("gNumLights"), 0);
		}

		// get cubemap
		static const char* path = "images/cubemap/tile/cubemap.json";
		Cubemap* cubemap = mainEngine->getCubemapResource().dataForString(path);
		assert(cubemap);

		// bind texture arrays
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(shader.getUniformLocation("gDiffuseMap"), 0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, mainEngine->getTileDiffuseTextures().getTexID());
		glActiveTexture(GL_TEXTURE1);
		glUniform1i(shader.getUniformLocation("gNormalMap"), 1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, mainEngine->getTileNormalTextures().getTexID());
		glActiveTexture(GL_TEXTURE2);
		glUniform1i(shader.getUniformLocation("gEffectsMap"), 2);
		glBindTexture(GL_TEXTURE_2D_ARRAY, mainEngine->getTileEffectsTextures().getTexID());
		glActiveTexture(GL_TEXTURE3);
		glUniform1i(shader.getUniformLocation("gCubeMap"), 3);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->getTexID());
	}

	return &shader;
}

void Tile::setCeilingSlopeHeightForVec(glm::vec3& vec) {
	switch( ceilingSlopeSide ) {
		case SIDE_EAST:
			vec.z += ceilingSlopeSize * (vec.x-x)/size;
			break;
		case SIDE_SOUTH:
			vec.z += ceilingSlopeSize * (vec.y-y)/size;
			break;
		case SIDE_WEST:
			vec.z += ceilingSlopeSize * (size-(vec.x-x))/size;
			break;
		case SIDE_NORTH:
			vec.z += ceilingSlopeSize * (size-(vec.y-y))/size;
			break;
		default:
			break;
	}
}

void Tile::cleanVertexBuffers() {
	for( int i = 0; i < 10; ++i ) {
		LinkedList<vertex_t>* list = intForVertices(i);
		list->removeAll();
	}
}

void Tile::compileCeilingVertices() {
	LinkedList<vertex_t>& list = ceilingVertices;
	vertex_t vert;

	// remove existing vertices
	list.removeAll();

	if( !hasVolume() )
		return;

	// northeast triangle
	vert.pos.x = x;
	vert.pos.y = y;
	vert.pos.z = ceilingHeight;
	setCeilingSlopeHeightForVec(vert.pos);
	list.addNodeLast(vert);

	vert.pos.x = x+size;
	vert.pos.y = y;
	vert.pos.z = ceilingHeight;
	setCeilingSlopeHeightForVec(vert.pos);
	list.addNodeLast(vert);

	vert.pos.x = x+size;
	vert.pos.y = y+size;
	vert.pos.z = ceilingHeight;
	setCeilingSlopeHeightForVec(vert.pos);
	list.addNodeLast(vert);

	// southwest triangle
	vert.pos.x = x;
	vert.pos.y = y;
	vert.pos.z = ceilingHeight;
	setCeilingSlopeHeightForVec(vert.pos);
	list.addNodeLast(vert);

	vert.pos.x = x+size;
	vert.pos.y = y+size;
	vert.pos.z = ceilingHeight;
	setCeilingSlopeHeightForVec(vert.pos);
	list.addNodeLast(vert);

	vert.pos.x = x;
	vert.pos.y = y+size;
	vert.pos.z = ceilingHeight;
	setCeilingSlopeHeightForVec(vert.pos);
	list.addNodeLast(vert);
}

void Tile::setFloorSlopeHeightForVec(glm::vec3& vec) {
	switch( floorSlopeSide ) {
		case SIDE_EAST:
			vec.z += floorSlopeSize * (vec.x-x)/size;
			break;
		case SIDE_SOUTH:
			vec.z += floorSlopeSize * (vec.y-y)/size;
			break;
		case SIDE_WEST:
			vec.z += floorSlopeSize * (size-(vec.x-x))/size;
			break;
		case SIDE_NORTH:
			vec.z += floorSlopeSize * (size-(vec.y-y))/size;
			break;
		default:
			break;
	}
}

void Tile::compileFloorVertices() {
	LinkedList<vertex_t>& list = floorVertices;
	vertex_t vert;

	// remove existing vertices
	list.removeAll();

	// northeast triangle
	vert.pos.x = x;
	vert.pos.y = y;
	vert.pos.z = floorHeight;
	setFloorSlopeHeightForVec(vert.pos);
	list.addNodeLast(vert);

	vert.pos.x = x+size;
	vert.pos.y = y+size;
	vert.pos.z = floorHeight;
	setFloorSlopeHeightForVec(vert.pos);
	list.addNodeLast(vert);

	vert.pos.x = x+size;
	vert.pos.y = y;
	vert.pos.z = floorHeight;
	setFloorSlopeHeightForVec(vert.pos);
	list.addNodeLast(vert);

	// southwest triangle
	vert.pos.x = x;
	vert.pos.y = y;
	vert.pos.z = floorHeight;
	setFloorSlopeHeightForVec(vert.pos);
	list.addNodeLast(vert);

	vert.pos.x = x;
	vert.pos.y = y+size;
	vert.pos.z = floorHeight;
	setFloorSlopeHeightForVec(vert.pos);
	list.addNodeLast(vert);

	vert.pos.x = x+size;
	vert.pos.y = y+size;
	vert.pos.z = floorHeight;
	setFloorSlopeHeightForVec(vert.pos);
	list.addNodeLast(vert);
}

Sint32 Tile::upperWallHeight(const Tile& neighbor, const side_t side, const corner_t corner) {
	Sint32 height = neighbor.ceilingHeight-ceilingHeight;
	if( corner==CORNER_LEFT ) {
		switch( side ) {
			case SIDE_EAST:
				if( ceilingSlopeSide==SIDE_EAST || ceilingSlopeSide==SIDE_NORTH )
					height -= ceilingSlopeSize;
				if( neighbor.ceilingSlopeSide==SIDE_WEST || neighbor.ceilingSlopeSide==SIDE_NORTH )
					height += neighbor.ceilingSlopeSize;
				break;
			case SIDE_SOUTH:
				if( ceilingSlopeSide==SIDE_SOUTH || ceilingSlopeSide==SIDE_EAST )
					height -= ceilingSlopeSize;
				if( neighbor.ceilingSlopeSide==SIDE_NORTH || neighbor.ceilingSlopeSide==SIDE_EAST )
					height += neighbor.ceilingSlopeSize;
				break;
			case SIDE_WEST:
				if( ceilingSlopeSide==SIDE_WEST || ceilingSlopeSide==SIDE_SOUTH )
					height -= ceilingSlopeSize;
				if( neighbor.ceilingSlopeSide==SIDE_EAST || neighbor.ceilingSlopeSide==SIDE_SOUTH )
					height += neighbor.ceilingSlopeSize;
				break;
			case SIDE_NORTH:
				if( ceilingSlopeSide==SIDE_NORTH || ceilingSlopeSide==SIDE_WEST )
					height -= ceilingSlopeSize;
				if( neighbor.ceilingSlopeSide==SIDE_SOUTH || neighbor.ceilingSlopeSide==SIDE_WEST )
					height += neighbor.ceilingSlopeSize;
				break;
			default:
				break;
		}
	} else if( corner==CORNER_RIGHT ) {
		switch( side ) {
			case SIDE_EAST:
				if( ceilingSlopeSide==SIDE_EAST || ceilingSlopeSide==SIDE_SOUTH )
					height -= ceilingSlopeSize;
				if( neighbor.ceilingSlopeSide==SIDE_WEST || neighbor.ceilingSlopeSide==SIDE_SOUTH )
					height += neighbor.ceilingSlopeSize;
				break;
			case SIDE_SOUTH:
				if( ceilingSlopeSide==SIDE_SOUTH || ceilingSlopeSide==SIDE_WEST )
					height -= ceilingSlopeSize;
				if( neighbor.ceilingSlopeSide==SIDE_NORTH || neighbor.ceilingSlopeSide==SIDE_WEST )
					height += neighbor.ceilingSlopeSize;
				break;
			case SIDE_WEST:
				if( ceilingSlopeSide==SIDE_WEST || ceilingSlopeSide==SIDE_NORTH )
					height -= ceilingSlopeSize;
				if( neighbor.ceilingSlopeSide==SIDE_EAST || neighbor.ceilingSlopeSide==SIDE_NORTH )
					height += neighbor.ceilingSlopeSize;
				break;
			case SIDE_NORTH:
				if( ceilingSlopeSide==SIDE_NORTH || ceilingSlopeSide==SIDE_EAST )
					height -= ceilingSlopeSize;
				if( neighbor.ceilingSlopeSide==SIDE_SOUTH || neighbor.ceilingSlopeSide==SIDE_EAST )
					height += neighbor.ceilingSlopeSize;
				break;
			default:
				break;
		}
	}
	return height;
}

void Tile::compileUpperVertices(Tile& neighbor, const side_t side) {
	LinkedList<vertex_t>& list = upperVertices[side];
	vertex_t vert;

	// remove existing vertices
	if( list.getFirst()!=nullptr )
		list.removeAll();

	if( !hasVolume() )
		return;

	Sint32 rightCornerHeight = upperWallHeight(neighbor,side,CORNER_RIGHT);
	Sint32 leftCornerHeight = upperWallHeight(neighbor,side,CORNER_LEFT);

	if( rightCornerHeight<=0 && leftCornerHeight<=0 )
		return;

	switch( side ) {
		case SIDE_EAST:
			if( leftCornerHeight>0 ) {
				vert.pos.x = x+size;
				vert.pos.y = y+size;
				vert.pos.z = ceilingHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				double oz = vert.pos.z;
				vert.pos.z = neighbor.ceilingHeight;
				neighbor.setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(oz,vert.pos.z);
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y;
				vert.pos.z = ceilingHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y;
				vert.pos.z = ceilingHeight+leftCornerHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(vert.pos.z,(double)(floorHeight+floorSlopeSize));
				list.addNodeLast(vert);
			}

			if( rightCornerHeight>0 ) {
				vert.pos.x = x+size;
				vert.pos.y = y+size;
				vert.pos.z = ceilingHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y;
				vert.pos.z = ceilingHeight+leftCornerHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(vert.pos.z,(double)(floorHeight+floorSlopeSize));
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y+size;
				vert.pos.z = ceilingHeight+rightCornerHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(vert.pos.z,(double)(floorHeight+floorSlopeSize));
				list.addNodeLast(vert);
			}
			break;
		case SIDE_SOUTH:
			if( leftCornerHeight>0 ) {
				vert.pos.x = x;
				vert.pos.y = y+size;
				vert.pos.z = ceilingHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				double oz = vert.pos.z;
				vert.pos.z = neighbor.ceilingHeight;
				neighbor.setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(oz,vert.pos.z);
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y+size;
				vert.pos.z = ceilingHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y+size;
				vert.pos.z = ceilingHeight+leftCornerHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(vert.pos.z,(double)(floorHeight+floorSlopeSize));
				list.addNodeLast(vert);
			}

			if( rightCornerHeight>0 ) {
				vert.pos.x = x;
				vert.pos.y = y+size;
				vert.pos.z = ceilingHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y+size;
				vert.pos.z = ceilingHeight+leftCornerHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(vert.pos.z,(double)(floorHeight+floorSlopeSize));
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y+size;
				vert.pos.z = ceilingHeight+rightCornerHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(vert.pos.z,(double)(floorHeight+floorSlopeSize));
				list.addNodeLast(vert);
			}
			break;
		case SIDE_WEST:
			if( leftCornerHeight>0 ) {
				vert.pos.x = x;
				vert.pos.y = y;
				vert.pos.z = ceilingHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				double oz = vert.pos.z;
				vert.pos.z = neighbor.ceilingHeight;
				neighbor.setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(oz,vert.pos.z);
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y+size;
				vert.pos.z = ceilingHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y+size;
				vert.pos.z = ceilingHeight+leftCornerHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(vert.pos.z,(double)(floorHeight+floorSlopeSize));
				list.addNodeLast(vert);
			}

			if( rightCornerHeight>0 ) {
				vert.pos.x = x;
				vert.pos.y = y;
				vert.pos.z = ceilingHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y+size;
				vert.pos.z = ceilingHeight+leftCornerHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(vert.pos.z,(double)(floorHeight+floorSlopeSize));
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y;
				vert.pos.z = ceilingHeight+rightCornerHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(vert.pos.z,(double)(floorHeight+floorSlopeSize));
				list.addNodeLast(vert);
			}
			break;
		case SIDE_NORTH:
			if( leftCornerHeight>0 ) {
				vert.pos.x = x+size;
				vert.pos.y = y;
				vert.pos.z = ceilingHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				double oz = vert.pos.z;
				vert.pos.z = neighbor.ceilingHeight;
				neighbor.setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(oz,vert.pos.z);
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y;
				vert.pos.z = ceilingHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y;
				vert.pos.z = ceilingHeight+leftCornerHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(vert.pos.z,(double)(floorHeight+floorSlopeSize));
				list.addNodeLast(vert);
			}

			if( rightCornerHeight>0 ) {
				vert.pos.x = x+size;
				vert.pos.y = y;
				vert.pos.z = ceilingHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y;
				vert.pos.z = ceilingHeight+leftCornerHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(vert.pos.z,(double)(floorHeight+floorSlopeSize));
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y;
				vert.pos.z = ceilingHeight+rightCornerHeight;
				setCeilingSlopeHeightForVec(vert.pos);
				vert.pos.z = fmin(vert.pos.z,(double)(floorHeight+floorSlopeSize));
				list.addNodeLast(vert);
			}
			break;
		default:
			break;
	}
}

Sint32 Tile::lowerWallHeight(const Tile& neighbor, const side_t side, const corner_t corner) {
	Sint32 height = floorHeight-neighbor.floorHeight;
	if( corner==CORNER_LEFT ) {
		switch( side ) {
			case SIDE_EAST:
				if( floorSlopeSide==SIDE_EAST || floorSlopeSide==SIDE_NORTH )
					height += floorSlopeSize;
				if( neighbor.floorSlopeSide==SIDE_WEST || neighbor.floorSlopeSide==SIDE_NORTH )
					height -= neighbor.floorSlopeSize;
				break;
			case SIDE_SOUTH:
				if( floorSlopeSide==SIDE_SOUTH || floorSlopeSide==SIDE_EAST )
					height += floorSlopeSize;
				if( neighbor.floorSlopeSide==SIDE_NORTH || neighbor.floorSlopeSide==SIDE_EAST )
					height -= neighbor.floorSlopeSize;
				break;
			case SIDE_WEST:
				if( floorSlopeSide==SIDE_WEST || floorSlopeSide==SIDE_SOUTH )
					height += floorSlopeSize;
				if( neighbor.floorSlopeSide==SIDE_EAST || neighbor.floorSlopeSide==SIDE_SOUTH )
					height -= neighbor.floorSlopeSize;
				break;
			case SIDE_NORTH:
				if( floorSlopeSide==SIDE_NORTH || floorSlopeSide==SIDE_WEST )
					height += floorSlopeSize;
				if( neighbor.floorSlopeSide==SIDE_SOUTH || neighbor.floorSlopeSide==SIDE_WEST )
					height -= neighbor.floorSlopeSize;
				break;
			default:
				break;
		}
	} else if( corner==CORNER_RIGHT ) {
		switch( side ) {
			case SIDE_EAST:
				if( floorSlopeSide==SIDE_EAST || floorSlopeSide==SIDE_SOUTH )
					height += floorSlopeSize;
				if( neighbor.floorSlopeSide==SIDE_WEST || neighbor.floorSlopeSide==SIDE_SOUTH )
					height -= neighbor.floorSlopeSize;
				break;
			case SIDE_SOUTH:
				if( floorSlopeSide==SIDE_SOUTH || floorSlopeSide==SIDE_WEST )
					height += floorSlopeSize;
				if( neighbor.floorSlopeSide==SIDE_NORTH || neighbor.floorSlopeSide==SIDE_WEST )
					height -= neighbor.floorSlopeSize;
				break;
			case SIDE_WEST:
				if( floorSlopeSide==SIDE_WEST || floorSlopeSide==SIDE_NORTH )
					height += floorSlopeSize;
				if( neighbor.floorSlopeSide==SIDE_EAST || neighbor.floorSlopeSide==SIDE_NORTH )
					height -= neighbor.floorSlopeSize;
				break;
			case SIDE_NORTH:
				if( floorSlopeSide==SIDE_NORTH || floorSlopeSide==SIDE_EAST )
					height += floorSlopeSize;
				if( neighbor.floorSlopeSide==SIDE_SOUTH || neighbor.floorSlopeSide==SIDE_EAST )
					height -= neighbor.floorSlopeSize;
				break;
			default:
				break;
		}
	}
	return height;
}

void Tile::compileLowerVertices(Tile& neighbor, const side_t side) {
	LinkedList<vertex_t>& list = lowerVertices[side];
	vertex_t vert;

	// remove existing vertices
	if( list.getFirst()!=nullptr )
		list.removeAll();

	if( floorHeight<=ceilingHeight && floorSlopeSize==ceilingSlopeSize && (floorSlopeSide==ceilingSlopeSide || floorSlopeSize==0) )
		return;

	Sint32 rightCornerHeight = lowerWallHeight(neighbor,side,CORNER_RIGHT);
	Sint32 leftCornerHeight = lowerWallHeight(neighbor,side,CORNER_LEFT);

	if( rightCornerHeight<=0 && leftCornerHeight<=0 )
		return;

	switch( side ) {
		case SIDE_EAST:
			if( leftCornerHeight>0 ) {
				vert.pos.x = x+size;
				vert.pos.y = y+size;
				vert.pos.z = floorHeight;
				setFloorSlopeHeightForVec(vert.pos);
				double oz = vert.pos.z;
				vert.pos.z = neighbor.floorHeight;
				neighbor.setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(oz,vert.pos.z);
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y;
				vert.pos.z = floorHeight-leftCornerHeight;
				setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(vert.pos.z,(double)(ceilingHeight-ceilingSlopeSize));
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y;
				vert.pos.z = floorHeight;
				setFloorSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);
			}

			if( rightCornerHeight>0 ) {
				vert.pos.x = x+size;
				vert.pos.y = y+size;
				vert.pos.z = floorHeight;
				setFloorSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y+size;
				vert.pos.z = floorHeight-rightCornerHeight;
				setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(vert.pos.z,(double)(ceilingHeight-ceilingSlopeSize));
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y;
				vert.pos.z = floorHeight-leftCornerHeight;
				setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(vert.pos.z,(double)(ceilingHeight-ceilingSlopeSize));
				list.addNodeLast(vert);
			}
			break;
		case SIDE_SOUTH:
			if( leftCornerHeight>0 ) {
				vert.pos.x = x;
				vert.pos.y = y+size;
				vert.pos.z = floorHeight;
				setFloorSlopeHeightForVec(vert.pos);
				double oz = vert.pos.z;
				vert.pos.z = neighbor.floorHeight;
				neighbor.setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(oz,vert.pos.z);
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y+size;
				vert.pos.z = floorHeight-leftCornerHeight;
				setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(vert.pos.z,(double)(ceilingHeight-ceilingSlopeSize));
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y+size;
				vert.pos.z = floorHeight;
				setFloorSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);
			}

			if( rightCornerHeight>0 ) {
				vert.pos.x = x;
				vert.pos.y = y+size;
				vert.pos.z = floorHeight;
				setFloorSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y+size;
				vert.pos.z = floorHeight-rightCornerHeight;
				setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(vert.pos.z,(double)(ceilingHeight-ceilingSlopeSize));
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y+size;
				vert.pos.z = floorHeight-leftCornerHeight;
				setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(vert.pos.z,(double)(ceilingHeight-ceilingSlopeSize));
				list.addNodeLast(vert);
			}
			break;
		case SIDE_WEST:
			if( leftCornerHeight>0 ) {
				vert.pos.x = x;
				vert.pos.y = y;
				vert.pos.z = floorHeight;
				setFloorSlopeHeightForVec(vert.pos);
				double oz = vert.pos.z;
				vert.pos.z = neighbor.floorHeight;
				neighbor.setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(oz,vert.pos.z);
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y+size;
				vert.pos.z = floorHeight-leftCornerHeight;
				setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(vert.pos.z,(double)(ceilingHeight-ceilingSlopeSize));
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y+size;
				vert.pos.z = floorHeight;
				setFloorSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);
			}

			if( rightCornerHeight>0 ) {
				vert.pos.x = x;
				vert.pos.y = y;
				vert.pos.z = floorHeight;
				setFloorSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y;
				vert.pos.z = floorHeight-rightCornerHeight;
				setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(vert.pos.z,(double)(ceilingHeight-ceilingSlopeSize));
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y+size;
				vert.pos.z = floorHeight-leftCornerHeight;
				setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(vert.pos.z,(double)(ceilingHeight-ceilingSlopeSize));
				list.addNodeLast(vert);
			}
			break;
		case SIDE_NORTH:
			if( leftCornerHeight>0 ) {
				vert.pos.x = x+size;
				vert.pos.y = y;
				vert.pos.z = floorHeight;
				setFloorSlopeHeightForVec(vert.pos);
				double oz = vert.pos.z;
				vert.pos.z = neighbor.floorHeight;
				neighbor.setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(oz,vert.pos.z);
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y;
				vert.pos.z = floorHeight-leftCornerHeight;
				setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(vert.pos.z,(double)(ceilingHeight-ceilingSlopeSize));
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y;
				vert.pos.z = floorHeight;
				setFloorSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);
			}

			if( rightCornerHeight>0 ) {
				vert.pos.x = x+size;
				vert.pos.y = y;
				vert.pos.z = floorHeight;
				setFloorSlopeHeightForVec(vert.pos);
				list.addNodeLast(vert);

				vert.pos.x = x+size;
				vert.pos.y = y;
				vert.pos.z = floorHeight-rightCornerHeight;
				setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(vert.pos.z,(double)(ceilingHeight-ceilingSlopeSize));
				list.addNodeLast(vert);

				vert.pos.x = x;
				vert.pos.y = y;
				vert.pos.z = floorHeight-leftCornerHeight;
				setFloorSlopeHeightForVec(vert.pos);
				vert.pos.z = fmax(vert.pos.z,(double)(ceilingHeight-ceilingSlopeSize));
				list.addNodeLast(vert);
			}
			break;
		default:
			break;
	}
}

bool Tile::hasVolume() const {
	if( ceilingHeight>=floorHeight && ceilingSlopeSize==floorSlopeSize && (ceilingSlopeSide==floorSlopeSide || ceilingSlopeSize==0) )
		return false;
	return true;
}

size_t Tile::calculateVertices() const {
	if( !hasVolume() )
		return 0;
	size_t result = 0;
	for( int i=0; i<10; ++i ) {
		const LinkedList<Tile::vertex_t>* list = nullptr;
		switch( i ) {
			case 0:
				list = &ceilingVertices;
				break;
			case 1:
				list = &floorVertices;
				break;
			case 2:
			case 3:
			case 4:
			case 5:
				list = &upperVertices[static_cast<side_t>(i-2)];
				break;
			case 6:
			case 7:
			case 8:
			case 9:
				list = &lowerVertices[static_cast<side_t>(i-6)];
				break;
			default:
				break;
		}
		if( list==nullptr )
			continue;
		result += list->getSize();
	}
	return result;
}

void Tile::buildBuffers() {
	// calculate number of vertices for the whole tile
	numVertices = calculateVertices();
}

LinkedList<Tile::vertex_t>* Tile::intForVertices(const int i) {
	switch( i ) {
		case 0:
			return &ceilingVertices;
		case 1:
			return &floorVertices;
		case 2:
		case 3:
		case 4:
		case 5:
			return &upperVertices[static_cast<side_t>(i-2)];
		case 6:
		case 7:
		case 8:
		case 9:
			return &lowerVertices[static_cast<side_t>(i-6)];
		default:
			return nullptr;
	}
}

void Tile::compileBulletPhysicsMesh() {
	// delete old rigid mesh
	if( rigidBody!=nullptr ) {
		dynamicsWorld->removeRigidBody(rigidBody);
		delete rigidBody;
		rigidBody = nullptr;
	}

	// check that there are any vertices to even build for
	if( floorVertices.getFirst()==nullptr ) {
		return;
	}

	// delete the old triangle mesh and generate a new one
	if( triMesh!=nullptr )
		delete triMesh;
	triMesh = new btTriangleMesh();

	LinkedList<Vector> vlist;
	for( int tri=0; tri<10; ++tri ) {
		// iterate through all the vertex structures
		LinkedList<vertex_t>* list = intForVertices(tri);
		if( list==nullptr )
			continue;

		// collect the vertex data and store it in a list
		for( Node<vertex_t>* node=list->getFirst(); node!=nullptr; node=node->getNext() ) {
			vertex_t& vert = node->getData();

			// create vertex for physics mesh
			Vector v(vert.pos.x,vert.pos.y,vert.pos.z);
			vlist.addNodeLast(v);

			if( vlist.getSize()==3 ) {
				// add the three vertices to the triangle mesh
				Vector& v0 = vlist.nodeForIndex(0)->getData();
				Vector& v1 = vlist.nodeForIndex(1)->getData();
				Vector& v2 = vlist.nodeForIndex(2)->getData();
				triMesh->addTriangle(v0,v1,v2,true);

				// clear the triangle list
				vlist.removeAll();
			}
		}

		// clear the vertex list for next time (in case there are leftovers)
		vlist.removeAll();
	}

	// build a new triangle shape
	if( triMeshShape!=nullptr )
		delete triMeshShape;
	triMeshShape = new btBvhTriangleMeshShape(triMesh,true,true);

	// create motion state
	if( motionState!=nullptr )
		delete motionState;
	motionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));

	// create rigid body
	btRigidBody::btRigidBodyConstructionInfo
		tileRigidBodyCI(0, motionState, triMeshShape, btVector3(0, 0, 0));
	rigidBody = new btRigidBody(tileRigidBodyCI);
	rigidBody->setUserIndex(World::nuid);
	rigidBody->setUserIndex2(World::nuid);
	rigidBody->setUserPointer((void*)this);

	// add a new rigid body to the simulation
	dynamicsWorld->addRigidBody(rigidBody);
}

bool Tile::selected() const {
	if( !world )
		return false;

	const Rect<int>& rect = world->getSelectedRect();
	int myX = x / size;
	int myY = y / size;
	
	if( myX < rect.x + min(0,rect.w) )
		return false;
	if( myX > rect.x + max(0,rect.w) )
		return false;
	if( myY < rect.y + min(0,rect.h) )
		return false;
	if( myY > rect.y + max(0,rect.h) )
		return false;
	return true;
}

Tile* Tile::findNeighbor(const Tile::side_t side) {

	int _x = ((int)x / size) + (int)(side==SIDE_EAST) - (int)(side==SIDE_WEST);
	int _y = ((int)y / size) + (int)(side==SIDE_SOUTH) - (int)(side==SIDE_NORTH);

	if( _x < 0 || _x >= (int)world->getWidth() ) {
		return nullptr;
	}
	if( _y < 0 || _y >= (int)world->getHeight() ) {
		return nullptr;
	}

	return &world->getTiles()[_y+_x*world->getHeight()];
}

void Tile::serialize(FileInterface * file) {
	Uint32 version = 0;
	file->property("Tile::version", version);

	file->property("ceilingHeight", ceilingHeight);
	file->property("floorHeight", floorHeight);
	file->property("ceilingSlopeSide", ceilingSlopeSide);
	file->property("ceilingSlopeSize", ceilingSlopeSize);
	file->property("floorSlopeSide", floorSlopeSide);
	file->property("floorSlopeSize", floorSlopeSize);

	if (hasVolume()) {
		file->property("customColorR", shaderVars.customColorR);
		file->property("customColorG", shaderVars.customColorG);
		file->property("customColorB", shaderVars.customColorB);

		Dictionary* textureStrings = &mainEngine->getTextureDictionary();
		file->property("ceilingTexture", ceilingTexture, textureStrings);
		file->property("floorTexture", floorTexture, textureStrings);

		file->property("upperTextures", upperTextures, textureStrings);
		file->property("lowerTextures", lowerTextures, textureStrings);
	}
}