// Sector.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "SectorVertex.hpp"
#include "Sector.hpp"
#include "SectorWorld.hpp"
#include "Camera.hpp"
#include "Material.hpp"

const float Sector::def = 512.f;

Sector::Sector(SectorWorld& _world) {
	world = &_world;

	dynamicsWorld = world->getBulletDynamicsWorld();

	for (int i = 0; i < BUFFER_TYPE_LENGTH; ++i) {
		vbo[static_cast<buffer_t>(i)] = 0;
	}

	faces.push(new face_t);
	faces.push(new face_t);
	faces.push(new face_t);
	faces.push(new face_t);

	faces[0]->sector = this;
	faces[0]->vertices[0].face = faces[0];
	faces[0]->vertices[0].position = glm::vec3(0.f, 0.f, 0.f);
	faces[0]->vertices[0].texcoord = glm::vec2(0.f, 0.f);
	faces[0]->vertices[1].face = faces[0];
	faces[0]->vertices[1].position = glm::vec3(0.f, def, 0.f);
	faces[0]->vertices[1].texcoord = glm::vec2(0.f, 1.f);
	faces[0]->vertices[2].face = faces[0];
	faces[0]->vertices[2].position = glm::vec3(def, 0.f, 0.f);
	faces[0]->vertices[2].texcoord = glm::vec2(1.f, 0.f);

	faces[1]->sector = this;
	faces[1]->vertices[0].face = faces[1];
	faces[1]->vertices[0].position = glm::vec3(0.f, 0.f, 0.f);
	faces[1]->vertices[0].texcoord = glm::vec2(0.f, 1.f);
	faces[1]->vertices[1].face = faces[1];
	faces[1]->vertices[1].position = glm::vec3(def, 0.f, 0.f);
	faces[1]->vertices[1].texcoord = glm::vec2(1.f, 1.f);
	faces[1]->vertices[2].face = faces[1];
	faces[1]->vertices[2].position = glm::vec3(0.f, 0.f, -def);
	faces[1]->vertices[2].texcoord = glm::vec2(0.f, 0.f);

	faces[2]->sector = this;
	faces[2]->vertices[0].face = faces[2];
	faces[2]->vertices[0].position = glm::vec3(0.f, 0.f, 0.f);
	faces[2]->vertices[0].texcoord = glm::vec2(1.f, 1.f);
	faces[2]->vertices[1].face = faces[2];
	faces[2]->vertices[1].position = glm::vec3(0.f, 0.f, -def);
	faces[2]->vertices[1].texcoord = glm::vec2(1.f, 0.f);
	faces[2]->vertices[2].face = faces[2];
	faces[2]->vertices[2].position = glm::vec3(0.f, def, 0.f);
	faces[2]->vertices[2].texcoord = glm::vec2(0.f, 1.f);

	faces[3]->sector = this;
	faces[3]->vertices[0].face = faces[3];
	faces[3]->vertices[0].position = glm::vec3(0.f, 0.f, -def);
	faces[3]->vertices[0].texcoord = glm::vec2(0.f, 0.f);
	faces[3]->vertices[1].face = faces[3];
	faces[3]->vertices[1].position = glm::vec3(def, 0.f, 0.f);
	faces[3]->vertices[1].texcoord = glm::vec2(0.f, 1.f);
	faces[3]->vertices[2].face = faces[3];
	faces[3]->vertices[2].position = glm::vec3(0.f, def, 0.f);
	faces[3]->vertices[2].texcoord = glm::vec2(1.f, 0.f);
}

Sector::~Sector() {
	for (int i = POSITION_BUFFER; i < BUFFER_TYPE_LENGTH; ++i) {
		if (vbo[static_cast<buffer_t>(i)]) {
			glDeleteBuffers(1, &vbo[static_cast<buffer_t>(i)]);
			vbo[static_cast<buffer_t>(i)] = 0;
		}
	}
	if (vao) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
	if (triMesh != nullptr) {
		delete triMesh;
	}
	if (triMeshShape != nullptr) {
		delete triMeshShape;
	}
	if (motionState != nullptr) {
		delete motionState;
	}
	if (rigidBody != nullptr) {
		dynamicsWorld->removeRigidBody(rigidBody);
		delete rigidBody;
	}
}

void Sector::splitFace(int faceIndex, const Vector& splitPoint) {
	faces.push(new face_t);
	faces.push(new face_t);
	faces.push(new face_t);

	// add new unique vertex
	SectorVertex* vertex = new SectorVertex(*world);
	vertex->setPos(splitPoint);

	// setup first new face
	faces[faces.getSize() - 3]->sector = this;
	faces[faces.getSize() - 3]->vertices[0].position = faces[faceIndex]->vertices[0].position;
	faces[faces.getSize() - 3]->vertices[1].position = faces[faceIndex]->vertices[1].position;
	faces[faces.getSize() - 3]->vertices[2].position = splitPoint;
	faces[faceIndex]->vertices[0].joined->own(faces[faces.getSize() - 3]->vertices[0]);
	faces[faceIndex]->vertices[1].joined->own(faces[faces.getSize() - 3]->vertices[1]);
	vertex->own(faces[faces.getSize() - 3]->vertices[2]);
	faces[faces.getSize() - 3]->vertices[0].face = faces[faces.getSize() - 3];
	faces[faces.getSize() - 3]->vertices[1].face = faces[faces.getSize() - 3];
	faces[faces.getSize() - 3]->vertices[2].face = faces[faces.getSize() - 3];
	faces[faces.getSize() - 3]->vertices[0].texcoord = faces[faceIndex]->vertices[0].texcoord;
	faces[faces.getSize() - 3]->vertices[1].texcoord = faces[faceIndex]->vertices[1].texcoord;
	faces[faces.getSize() - 3]->vertices[2].texcoord = faces[faceIndex]->vertices[2].texcoord;

	// setup second new face
	faces[faces.getSize() - 2]->sector = this;
	faces[faces.getSize() - 2]->vertices[0].position = faces[faceIndex]->vertices[1].position;
	faces[faces.getSize() - 2]->vertices[1].position = faces[faceIndex]->vertices[2].position;
	faces[faces.getSize() - 2]->vertices[2].position = splitPoint;
	faces[faceIndex]->vertices[1].joined->own(faces[faces.getSize() - 2]->vertices[0]);
	faces[faceIndex]->vertices[2].joined->own(faces[faces.getSize() - 2]->vertices[1]);
	vertex->own(faces[faces.getSize() - 2]->vertices[2]);
	faces[faces.getSize() - 2]->vertices[0].face = faces[faces.getSize() - 2];
	faces[faces.getSize() - 2]->vertices[1].face = faces[faces.getSize() - 2];
	faces[faces.getSize() - 2]->vertices[2].face = faces[faces.getSize() - 2];
	faces[faces.getSize() - 2]->vertices[0].texcoord = faces[faceIndex]->vertices[1].texcoord;
	faces[faces.getSize() - 2]->vertices[1].texcoord = faces[faceIndex]->vertices[2].texcoord;
	faces[faces.getSize() - 2]->vertices[2].texcoord = faces[faceIndex]->vertices[0].texcoord;

	// setup third new face
	faces[faces.getSize() - 1]->sector = this;
	faces[faces.getSize() - 1]->vertices[0].position = faces[faceIndex]->vertices[2].position;
	faces[faces.getSize() - 1]->vertices[1].position = faces[faceIndex]->vertices[0].position;
	faces[faces.getSize() - 1]->vertices[2].position = splitPoint;
	faces[faceIndex]->vertices[2].joined->own(faces[faces.getSize() - 1]->vertices[0]);
	faces[faceIndex]->vertices[0].joined->own(faces[faces.getSize() - 1]->vertices[1]);
	vertex->own(faces[faces.getSize() - 1]->vertices[2]);
	faces[faces.getSize() - 1]->vertices[0].face = faces[faces.getSize() - 1];
	faces[faces.getSize() - 1]->vertices[1].face = faces[faces.getSize() - 1];
	faces[faces.getSize() - 1]->vertices[2].face = faces[faces.getSize() - 1];
	faces[faces.getSize() - 1]->vertices[0].texcoord = faces[faceIndex]->vertices[2].texcoord;
	faces[faces.getSize() - 1]->vertices[1].texcoord = faces[faceIndex]->vertices[0].texcoord;
	faces[faces.getSize() - 1]->vertices[2].texcoord = faces[faceIndex]->vertices[1].texcoord;

	// remove original face
	Sector::face_t* oldFace = faces.remove(faceIndex);
	delete oldFace;

	vertex->updateRigidBody();
	world->getVertices().push(vertex);

	uploadBuffers();
}

Sector* Sector::addSector(int faceIndex, const Vector& splitPoint) {
	Sector* sector = new Sector(*world);
	sector->faces.push(new face_t);
	sector->faces.push(new face_t);
	sector->faces.push(new face_t);
	sector->faces.push(new face_t);

	// add new unique vertex
	SectorVertex* vertex = new SectorVertex(*world);
	vertex->setPos(splitPoint);

	sector->faces[0]->sector = sector;
	sector->faces[0]->vertices[0].position = faces[faceIndex]->vertices[0].position;
	sector->faces[0]->vertices[1].position = faces[faceIndex]->vertices[2].position;
	sector->faces[0]->vertices[2].position = faces[faceIndex]->vertices[1].position;
	faces[faceIndex]->vertices[0].joined->own(sector->faces[0]->vertices[0]);
	faces[faceIndex]->vertices[2].joined->own(sector->faces[0]->vertices[1]);
	faces[faceIndex]->vertices[1].joined->own(sector->faces[0]->vertices[2]);
	sector->faces[0]->vertices[0].face = sector->faces[0];
	sector->faces[0]->vertices[1].face = sector->faces[0];
	sector->faces[0]->vertices[2].face = sector->faces[0];

	sector->faces[1]->sector = sector;
	sector->faces[1]->vertices[0].position = faces[faceIndex]->vertices[0].position;
	sector->faces[1]->vertices[1].position = faces[faceIndex]->vertices[1].position;
	sector->faces[1]->vertices[2].position = splitPoint;
	faces[faceIndex]->vertices[0].joined->own(sector->faces[1]->vertices[0]);
	faces[faceIndex]->vertices[1].joined->own(sector->faces[1]->vertices[1]);
	vertex->own(sector->faces[1]->vertices[2]);
	sector->faces[1]->vertices[0].face = sector->faces[1];
	sector->faces[1]->vertices[1].face = sector->faces[1];
	sector->faces[1]->vertices[2].face = sector->faces[1];

	sector->faces[2]->sector = sector;
	sector->faces[2]->vertices[0].position = faces[faceIndex]->vertices[1].position;
	sector->faces[2]->vertices[1].position = faces[faceIndex]->vertices[2].position;
	sector->faces[2]->vertices[2].position = splitPoint;
	faces[faceIndex]->vertices[1].joined->own(sector->faces[2]->vertices[0]);
	faces[faceIndex]->vertices[2].joined->own(sector->faces[2]->vertices[1]);
	vertex->own(sector->faces[2]->vertices[2]);
	sector->faces[2]->vertices[0].face = sector->faces[2];
	sector->faces[2]->vertices[1].face = sector->faces[2];
	sector->faces[2]->vertices[2].face = sector->faces[2];

	sector->faces[3]->sector = sector;
	sector->faces[3]->vertices[0].position = faces[faceIndex]->vertices[2].position;
	sector->faces[3]->vertices[1].position = faces[faceIndex]->vertices[0].position;
	sector->faces[3]->vertices[2].position = splitPoint;
	faces[faceIndex]->vertices[2].joined->own(sector->faces[3]->vertices[0]);
	faces[faceIndex]->vertices[0].joined->own(sector->faces[3]->vertices[1]);
	vertex->own(sector->faces[3]->vertices[2]);
	sector->faces[3]->vertices[0].face = sector->faces[3];
	sector->faces[3]->vertices[1].face = sector->faces[3];
	sector->faces[3]->vertices[2].face = sector->faces[3];

	sector->faces[0]->neighbor = this;
	faces[faceIndex]->neighbor = sector;

	sector->uploadBuffers();
	vertex->updateRigidBody();
	world->getSectors().push(sector);
	world->getVertices().push(vertex);

	uploadBuffers();

	return sector;
}

void Sector::connectSectors(int myFace, int theirFace) {
	// todo
}

void Sector::countVertices() {
	numVertices = 0;
	numIndices = 0;
	for (Uint32 c = 0; c < faces.getSize(); ++c) {
		if (!faces[c]->neighbor) {
			numVertices += 3;
			numIndices += 3;
		}
	}
}

int Sector::findFaceWithNormal(const Vector& normal) {
	static const float epsilon = 0.0001f;
	glm::vec3 gNormal(normal);
	for (Uint32 c = 0; c < faces.getSize(); ++c) {
		if (faces[c]->normal.x >= gNormal.x - epsilon &&
			faces[c]->normal.x <= gNormal.x + epsilon) {
			if (faces[c]->normal.y >= gNormal.y - epsilon &&
				faces[c]->normal.y <= gNormal.y + epsilon) {
				if (faces[c]->normal.z >= gNormal.z - epsilon &&
					faces[c]->normal.z <= gNormal.z + epsilon) {
					return c;
				}
			}
		}
	}
	return -1;
}

void Sector::buildPhysicsMesh() {
	// delete old rigid mesh
	if (rigidBody != nullptr) {
		dynamicsWorld->removeRigidBody(rigidBody);
		delete rigidBody;
		rigidBody = nullptr;
	}

	// check that there are any vertices to even build for
	if (numVertices == 0) {
		return;
	}

	// delete the old triangle mesh and generate a new one
	if (triMesh != nullptr) {
		delete triMesh;
	}
	triMesh = new btTriangleMesh();
	ArrayList<Vector> vlist;
	vlist.alloc(3);

	// build mesh
	for (Uint32 face = 0; face < faces.getSize(); ++face) {
		if (!faces[face]->neighbor) {
			for (Uint32 vertex = 0; vertex < 3; ++vertex) {
				vertex_t& v = faces[face]->vertices[vertex];

				Vector pos(v.position.x, v.position.y, v.position.z);
				vlist.push(pos);

				if (vlist.getSize() == 3) {
					// add the three vertices to the triangle mesh
					Vector& v0 = vlist[0];
					Vector& v1 = vlist[1];
					Vector& v2 = vlist[2];
					triMesh->addTriangle(v0, v1, v2, true);

					// clear the triangle list
					vlist.clear();
				}
			}
		}
	}

	// build a new triangle shape
	if (triMeshShape != nullptr)
		delete triMeshShape;
	triMeshShape = new btBvhTriangleMeshShape(triMesh, true, true);

	// create motion state
	if (motionState != nullptr)
		delete motionState;
	motionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));

	// create rigid body
	btRigidBody::btRigidBodyConstructionInfo
		tileRigidBodyCI(0, motionState, triMeshShape, btVector3(0, 0, 0));
	rigidBody = new btRigidBody(tileRigidBodyCI);

	// add a new rigid body to the simulation
	dynamicsWorld->addRigidBody(rigidBody);
}

void Sector::uploadBuffers() {
	countVertices();

	buildPhysicsMesh();

	positionBuffer.clear();
	texcoordBuffer.clear();
	normalBuffer.clear();
	tangentBuffer.clear();
	indexBuffer.clear();
	positionBuffer.alloc(numVertices);
	texcoordBuffer.alloc(numVertices);
	normalBuffer.alloc(numVertices);
	tangentBuffer.alloc(numVertices);
	indexBuffer.alloc(numIndices);

	if (numVertices > 0) {
		GLuint index = 0;
		for (Uint32 face = 0; face < faces.getSize(); ++face) {
			if (!faces[face]->neighbor) {
				glm::vec3 e0 = faces[face]->vertices[1].position - faces[face]->vertices[0].position;
				glm::vec3 e1 = faces[face]->vertices[2].position - faces[face]->vertices[0].position;
				glm::vec2 uv0 = faces[face]->vertices[1].texcoord - faces[face]->vertices[0].texcoord;
				glm::vec2 uv1 = faces[face]->vertices[2].texcoord - faces[face]->vertices[0].texcoord;

				glm::vec3& normal = faces[face]->normal;
				glm::vec3& tangent = faces[face]->tangent;

				normal = glm::normalize(glm::cross(e0, e1));
				float r = 1.0f / (uv0.x * uv1.y - uv0.y * uv1.x);
				tangent = (e0 * uv1.y - e1 * uv0.y) * r;

				for (Uint32 vertex = 0; vertex < 3; ++vertex) {
					glm::vec3& position = faces[face]->vertices[vertex].position;
					positionBuffer.push(glm::vec3(position.x, -position.z, position.y));

					glm::vec2& texcoord = faces[face]->vertices[vertex].texcoord;
					texcoordBuffer.push(glm::vec2(texcoord.x, texcoord.y));

					normalBuffer.push(glm::vec3(normal.x, -normal.z, normal.y));
					tangentBuffer.push(tangent);

					indexBuffer.push(index);
					++index;
				}
			}
		}
	}

	if (vao) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
	if (!numVertices) {
		return;
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// upload position data
	if (vbo[POSITION_BUFFER]) {
		glDeleteBuffers(1, &vbo[POSITION_BUFFER]);
		vbo[POSITION_BUFFER] = 0;
	}
	glGenBuffers(1, &vbo[POSITION_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[POSITION_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec3), &positionBuffer[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	// upload texcoord data
	if (vbo[TEXCOORD_BUFFER]) {
		glDeleteBuffers(1, &vbo[TEXCOORD_BUFFER]);
		vbo[TEXCOORD_BUFFER] = 0;
	}
	glGenBuffers(1, &vbo[TEXCOORD_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[TEXCOORD_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec2), &texcoordBuffer[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);

	// upload normal data
	if (vbo[NORMAL_BUFFER]) {
		glDeleteBuffers(1, &vbo[NORMAL_BUFFER]);
		vbo[NORMAL_BUFFER] = 0;
	}
	glGenBuffers(1, &vbo[NORMAL_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[NORMAL_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec3), &normalBuffer[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(2);

	// upload tangent data
	if (vbo[TANGENT_BUFFER]) {
		glDeleteBuffers(1, &vbo[TANGENT_BUFFER]);
		vbo[TANGENT_BUFFER] = 0;
	}
	glGenBuffers(1, &vbo[TANGENT_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[TANGENT_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec3), &tangentBuffer[0], GL_STATIC_DRAW);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(3);

	// upload index data
	if (vbo[INDEX_BUFFER]) {
		glDeleteBuffers(1, &vbo[INDEX_BUFFER]);
		vbo[INDEX_BUFFER] = 0;
	}
	glGenBuffers(1, &vbo[INDEX_BUFFER]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(GLuint), &indexBuffer[0], GL_STATIC_DRAW);

	glBindVertexArray(0);
}

void Sector::process() {
	if (updateNeeded) {
		updateNeeded = false;
		uploadBuffers();
	}
}

void Sector::draw(Camera& camera, Light* light) {
	if (!numIndices) {
		return;
	}

	Material* material = mainEngine->getMaterialResource().dataForString(materialStr.get());

	// load shader
	loadShader(camera, light, material);

	// draw elements
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, (GLsizei)numIndices, GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);
}

void Sector::loadShader(Camera& camera, Light* light, Material* material) {
	Client* client = mainEngine->getLocalClient();
	if (!client)
		return;

	if (!material) {
		switch (camera.getDrawMode()) {
		case Camera::DRAW_DEPTH:
			material = mainEngine->getMaterialResource().dataForString("shaders/sector/depth.json");
			break;
		case Camera::DRAW_SILHOUETTE:
			material = mainEngine->getMaterialResource().dataForString("shaders/sector/silhouette.json");
			break;
		case Camera::DRAW_STENCIL:
			material = mainEngine->getMaterialResource().dataForString("shaders/sector/stencil.json");
			break;
		case Camera::DRAW_TRIANGLES:
			material = mainEngine->getMaterialResource().dataForString("shaders/sector/triangles.json");
			break;
		default:
			material = mainEngine->getMaterialResource().dataForString("shaders/sector/std.json");
			break;
		}
		if (!material) {
			mainEngine->fmsg(Engine::MSG_ERROR, "failed to load sector material");
			return;
		}
	}

	ShaderProgram& shader = material->getShader();
	//glLineWidth(1);

	if (&shader != ShaderProgram::getCurrentShader()) {
		shader.mount();
		if (camera.getDrawMode() == Camera::DRAW_DEPTH) {

			// load projection matrix into shader
			glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(camera.getProjViewMatrix()));
		} else if (camera.getDrawMode() == Camera::DRAW_SILHOUETTE) {

			// load projection matrix into shader
			glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(camera.getProjViewMatrix()));

			// load camera position into shader
			glm::vec3 cameraPos(camera.getGlobalPos().x, -camera.getGlobalPos().z, camera.getGlobalPos().y);
			glUniform3fv(shader.getUniformLocation("gCameraPos"), 1, glm::value_ptr(cameraPos));
		} else if (camera.getDrawMode() == Camera::DRAW_TRIANGLES) {

			// load projection matrix into shader
			glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(camera.getProjViewMatrix()));
		} else if (camera.getDrawMode() == Camera::DRAW_STENCIL) {

			// load projection matrix into shader
			glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(camera.getProjViewMatrix()));

			// load light data into shader
			if (light) {
				glm::vec3 lightPos(light->getGlobalPos().x, -light->getGlobalPos().z, light->getGlobalPos().y);
				glUniform3fv(shader.getUniformLocation("gLightPos"), 1, glm::value_ptr(lightPos));
			} else {
				glm::vec3 lightPos(camera.getGlobalPos().x, -camera.getGlobalPos().z, camera.getGlobalPos().y);
				glUniform3fv(shader.getUniformLocation("gLightPos"), 1, glm::value_ptr(lightPos));
			}
		} else {

			// load projection matrix into shader
			glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(camera.getProjViewMatrix()));

			glm::vec3 cameraPos(camera.getGlobalPos().x, -camera.getGlobalPos().z, camera.getGlobalPos().y);

			if (camera.getDrawMode() == Camera::DRAW_STANDARD) {
				glUniform1i(shader.getUniformLocation("gActiveLight"), GL_TRUE);
			} else {
				glUniform1i(shader.getUniformLocation("gActiveLight"), GL_FALSE);
			}

			// load light data into shader
			if (light) {
				Vector lightAng = light->getGlobalAng().toVector();
				glm::vec3 lightDir(lightAng.x, -lightAng.z, lightAng.y);
				glm::vec3 lightPos(light->getGlobalPos().x, -light->getGlobalPos().z, light->getGlobalPos().y);
				glm::vec3 lightScale(light->getGlobalScale().x, -light->getGlobalScale().z, light->getGlobalScale().y);

				glUniform3fv(shader.getUniformLocation("gCameraPos"), 1, glm::value_ptr(cameraPos));
				glUniform3fv(shader.getUniformLocation("gLightPos"), 1, glm::value_ptr(lightPos));
				glUniform4fv(shader.getUniformLocation("gLightColor"), 1, glm::value_ptr(glm::vec3(light->getColor())));
				glUniform1f(shader.getUniformLocation("gLightIntensity"), light->getIntensity());
				glUniform1f(shader.getUniformLocation("gLightRadius"), light->getRadius());
				glUniform3fv(shader.getUniformLocation("gLightScale"), 1, glm::value_ptr(lightScale));
				glUniform3fv(shader.getUniformLocation("gLightDirection"), 1, glm::value_ptr(lightDir));
				glUniform1i(shader.getUniformLocation("gLightShape"), static_cast<GLint>(light->getShape()));
			} else {
				glUniform3fv(shader.getUniformLocation("gCameraPos"), 1, glm::value_ptr(cameraPos));
				glUniform3fv(shader.getUniformLocation("gLightPos"), 1, glm::value_ptr(cameraPos));
				glUniform4fv(shader.getUniformLocation("gLightColor"), 1, glm::value_ptr(glm::vec4(1.f, 1.f, 1.f, 1.f)));
				glUniform1f(shader.getUniformLocation("gLightIntensity"), 1);
				glUniform1f(shader.getUniformLocation("gLightRadius"), 16384.f);
				glUniform3fv(shader.getUniformLocation("gLightScale"), 1, glm::value_ptr(glm::vec3(1.f, 1.f, 1.f)));
				glUniform1i(shader.getUniformLocation("gLightShape"), 0);
			}

			// load textures
			material->bindTextures(Material::STANDARD);
		}
	}
}

Sector::vertex_t::~vertex_t() {
	if (joined) {
		for (Uint32 c = 0; c < joined->vertices.getSize(); ++c) {
			if (joined->vertices[c] == this) {
				joined->vertices.remove(c);
				--c;
			}
		}
	}
}