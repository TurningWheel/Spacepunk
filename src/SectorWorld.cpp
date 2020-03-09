// SectorWorld.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "SectorVertex.hpp"
#include "Sector.hpp"
#include "SectorWorld.hpp"
#include "Renderer.hpp"
#include "Editor.hpp"
#include "Entity.hpp"
#include "Client.hpp"

#include "Component.hpp"
#include "Light.hpp"
#include "Camera.hpp"

// there are two strings so we can convert different formats if need be.
const char* SectorWorld::fileMagicNumber = "SPACEPUNK_SWD_01";
const char* SectorWorld::fileMagicNumberSaveOut = "SPACEPUNK_SWD_01";

SectorWorld::SectorWorld(Game* _game, bool _silent, Uint32 _id, const char* _name)
	: World(_game)
	//	: pathFinder(*(new PathFinder(*this)))
{
	nameStr = _name;
	silent = _silent;
	clientObj = game->isClient();
	id = _id;
}

SectorWorld::~SectorWorld() {
	while (sectors.getSize()) {
		delete sectors.pop();
	}

	// delete grid objects
	destroyGrid();
}

void SectorWorld::initialize(bool empty) {
	World::initialize(empty);

	// add one sector
	Sector* sector = new Sector(*this);
	sector->uploadBuffers();
	sectors.push(sector);

	// add four unique vertices for the sector
	{
		SectorVertex* vertex = new SectorVertex(*this);
		vertex->setPos(Vector(0.f, 0.f, 0.f));
		vertex->own(sector->faces[0]->vertices[0]);
		vertex->own(sector->faces[1]->vertices[0]);
		vertex->own(sector->faces[2]->vertices[0]);
		vertex->updateRigidBody();
		vertices.push(vertex);
	}
	{
		SectorVertex* vertex = new SectorVertex(*this);
		vertex->setPos(Vector(0.f, Sector::def, 0.f));
		vertex->own(sector->faces[0]->vertices[1]);
		vertex->own(sector->faces[2]->vertices[2]);
		vertex->own(sector->faces[3]->vertices[2]);
		vertex->updateRigidBody();
		vertices.push(vertex);
	}
	{
		SectorVertex* vertex = new SectorVertex(*this);
		vertex->setPos(Vector(Sector::def, 0.f, 0.f));
		vertex->own(sector->faces[0]->vertices[2]);
		vertex->own(sector->faces[1]->vertices[1]);
		vertex->own(sector->faces[3]->vertices[1]);
		vertex->updateRigidBody();
		vertices.push(vertex);
	}
	{
		SectorVertex* vertex = new SectorVertex(*this);
		vertex->setPos(Vector(0.f, 0.f, -Sector::def));
		vertex->own(sector->faces[1]->vertices[2]);
		vertex->own(sector->faces[2]->vertices[1]);
		vertex->own(sector->faces[3]->vertices[0]);
		vertex->updateRigidBody();
		vertices.push(vertex);
	}

	// initialize entities
	for (auto pair : entities) {
		Entity* entity = pair.b;
		entity->update();
	}

	// create grid object
	createGrid();
	updateGridRigidBody(0.f);
}

const bool SectorWorld::selectVertex(const Uint32 index, const bool b) {
	if (index < vertices.getSize()) {
		vertices[index]->selected = b;
		vertices[index]->highlighted = b;
		if (b) {
			mainEngine->playSound("editor/message.wav");
		}
		return true;
	} else {
		return false;
	}
}

void SectorWorld::selectVertices(const bool b) {
	for (Uint32 c = 0; c < vertices.getSize(); ++c) {
		vertices[c]->selected = b;
		vertices[c]->highlighted = b;
	}
}

void SectorWorld::removeSector(Uint32 index) {
	// todo
}

void SectorWorld::deselectGeometry() {
	// todo
}

void SectorWorld::findEntitiesInRadius(const Vector& origin, float radius, LinkedList<Entity*>& outList, bool flat) {
	// todo
}

void SectorWorld::process() {
	World::process();

	for (Uint32 c = 0; c < sectors.getSize(); ++c) {
		sectors[c]->process();
	}
}

void SectorWorld::drawSceneObjects(Camera& camera, Light* light) {
	Client* client = mainEngine->getLocalClient();
	if (!client)
		return;

	// editor variables
	bool editorActive = false;
	bool ceilingMode = false;
	Uint32 highlightedObj = nuid;
	if (client->isEditorActive()) {
		editorActive = true;
		Editor* editor = client->getEditor();
		ceilingMode = editor->isCeilingMode();
		highlightedObj = editor->getHighlightedObj();
	}

	// draw sectors
	if (camera.getDrawMode() <= Camera::DRAW_GLOW ||
		camera.getDrawMode() == Camera::DRAW_TRIANGLES ||
		(camera.getDrawMode() == Camera::DRAW_SILHOUETTE && cvar_showEdges.toInt())) {
		for (Uint32 c = 0; c < sectors.getSize(); ++c) {
			sectors[c]->draw(camera, light);
		}
		ShaderProgram::unmount();

		// sector vertices
		if (editorActive && showTools) {
			for (Uint32 c = 0; c < vertices.getSize(); ++c) {
				vertices[c]->draw(camera);
			}
		}
	}

	if (camera.getDrawMode() == Camera::DRAW_DEPTH) {
		glPolygonOffset(1.f, 1.f);
		glEnable(GL_POLYGON_OFFSET_FILL);
	}

	// draw entities
	if (camera.getDrawMode() != Camera::DRAW_GLOW || !editorActive || !showTools) {
		for (auto pair : entities) {
			Entity* entity = pair.b;

			// in silhouette mode, skip unhighlighted or unselected actors
			if (camera.getDrawMode() == Camera::DRAW_SILHOUETTE) {
				if (!entity->isHighlighted() && entity->getUID() != highlightedObj) {
					continue;
				}
			}

			// draw the entity
			entity->draw(camera, ArrayList<Light*>({ light }));
		}
	}

	if (camera.getDrawMode() == Camera::DRAW_DEPTH) {
		glPolygonOffset(1.f, 0.f);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	// reset some gl state
	glActiveTexture(GL_TEXTURE0);
	ShaderProgram::unmount();
}

void SectorWorld::draw() {
	Client* client = mainEngine->getLocalClient();
	if (!client)
		return;
	Renderer* renderer = client->getRenderer();
	if (!renderer || !renderer->isInitialized())
		return;
	Editor* editor = client->getEditor();

	// build camera list
	LinkedList<Camera*> cameras;
	for (auto pair : entities) {
		Entity* entity = pair.b;
		entity->findAllComponents<Camera>(Component::COMPONENT_CAMERA, cameras);
	}

	// build light list
	LinkedList<Light*> lights;
	for (auto pair : entities) {
		Entity* entity = pair.b;
		if (entity->isFlag(Entity::flag_t::FLAG_VISIBLE)) {
			entity->findAllComponents<Light>(Component::COMPONENT_LIGHT, lights);
		}
	}

	// cull unselected cameras (editor)
	if (editor && editor->isInitialized()) {
		Node<Camera*>* nextNode = nullptr;
		for (Node<Camera*>* node = cameras.getFirst(); node != nullptr; node = nextNode) {
			Camera* camera = node->getData();
			nextNode = node->getNext();

			// in editor, skip all but selected cams and the main cams
			if (camera != editor->getEditingCamera() &&
				camera != editor->getMinimapCamera() &&
				!camera->getEntity()->isSelected()) {
				cameras.removeNode(node);
			}
		}
	}

	// iterate cameras
	for (Node<Camera*>* node = cameras.getFirst(); node != nullptr; node = node->getNext()) {
		Camera* camera = node->getData();

		// in editor, skip minimap if any other cameras are selected
		// replace it with our selected camera(s)
		Rect<Sint32> oldWin;
		if (editor && editor->isInitialized()) {
			if (camera != editor->getEditingCamera() &&
				camera != editor->getMinimapCamera()) {
				oldWin = camera->getWin();
				camera->setWin(editor->getMinimapCamera()->getWin());
			} else if (camera == editor->getMinimapCamera() && cameras.getSize() > 2) {
				continue;
			}
		}

		// skip deactivated cameras
		if (!camera->getEntity()->isFlag(Entity::flag_t::FLAG_VISIBLE)) {
			continue;
		}

		// clear the window area
		glClear(GL_DEPTH_BUFFER_BIT);
		Rect<int> backgroundRect = camera->getWin();
		renderer->drawRect(&backgroundRect, glm::vec4(0.f, 0.f, 0.f, 1.f));

		// set proper light blending function
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		// setup projection
		camera->setupProjection(true);

		// render scene into depth buffer
		camera->setDrawMode(Camera::DRAW_DEPTH);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDrawBuffer(GL_NONE);
		drawSceneObjects(*camera, nullptr);

		if (client->isEditorActive() && showTools && !camera->isOrtho()) {
			// render fullbright scene
			camera->setDrawMode(Camera::DRAW_STANDARD);
			static const GLenum attachments[Framebuffer::ColorBuffer::MAX] = {
				GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			glDrawBuffers(Framebuffer::ColorBuffer::MAX, attachments);
			glDisable(GL_STENCIL_TEST);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			drawSceneObjects(*camera, nullptr);
		} else {
			for (Node<Light*>* node = lights.getFirst(); node != nullptr; node = node->getNext()) {
				Light* light = node->getData();

				// render stencil shadows
				glEnable(GL_STENCIL_TEST);
				glDepthMask(GL_FALSE);
				glEnable(GL_DEPTH_CLAMP);
				glDisable(GL_CULL_FACE);
				glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
				glClear(GL_STENCIL_BUFFER_BIT);
				glDepthFunc(GL_GREATER);
				glDrawBuffer(GL_NONE);
				if (light->getEntity()->isFlag(Entity::flag_t::FLAG_SHADOW)) {
					camera->setDrawMode(Camera::DRAW_STENCIL);
					drawSceneObjects(*camera, light);
				}
				glDisable(GL_DEPTH_CLAMP);
				glEnable(GL_CULL_FACE);

				// render shadowed scene
				camera->setDrawMode(Camera::DRAW_STANDARD);
				static const GLenum attachments[Framebuffer::ColorBuffer::MAX] = {
					GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
				glDrawBuffers(Framebuffer::ColorBuffer::MAX, attachments);
				glStencilFunc(GL_EQUAL, 0x00, 0xFF);
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
				glDepthFunc(GL_GEQUAL);
				drawSceneObjects(*camera, light);
				glDisable(GL_STENCIL_TEST);
			}
		}

		// render scene with glow textures
		camera->setDrawMode(Camera::DRAW_GLOW);
		glDepthMask(GL_FALSE);
		static const GLenum attachments[Framebuffer::ColorBuffer::MAX] = {
			GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(Framebuffer::ColorBuffer::MAX, attachments);
		glDepthFunc(GL_GEQUAL);
		drawSceneObjects(*camera, nullptr);

		// render triangle lines
		if (cvar_showVerts.toInt()) {
			camera->setDrawMode(Camera::DRAW_TRIANGLES);
			drawSceneObjects(*camera, nullptr);
		}

		// render depth fail scene
		camera->setDrawMode(Camera::DRAW_DEPTHFAIL);
		glDepthFunc(GL_LESS);
		drawSceneObjects(*camera, nullptr);
		glDepthFunc(GL_GEQUAL);

		if (camera->isOrtho()) {
			// draw level mask
			camera->setDrawMode(Camera::DRAW_DEPTH);
			glEnable(GL_STENCIL_TEST);
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glDrawBuffer(GL_NONE);
			glClear(GL_STENCIL_BUFFER_BIT);
			glStencilFunc(GL_ALWAYS, 0x00, 0xFF);
			glStencilOp(GL_INCR, GL_INCR, GL_INCR);
			drawSceneObjects(*camera, nullptr);
			glStencilFunc(GL_EQUAL, 0x00, 0xFF);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			Renderer* renderer = camera->getRenderer();
			if (renderer) {
				renderer->drawRect(&camera->getWin(), glm::vec4(.25f, .25f, .25f, 1.f));
			}
			glStencilFunc(GL_ALWAYS, 0x00, 0xFF);

			// render state gets messed up after this, so reinit
			camera->setupProjection(true);
		} else {
			// render silhouettes
			camera->setDrawMode(Camera::DRAW_SILHOUETTE);
			drawSceneObjects(*camera, nullptr);
		}

		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_STENCIL_TEST);

		// draw editing grid
		if (client->isEditorActive() && showTools && gridVisible) {
			if (!camera->isOrtho()) {
				if (client->getEditor()->getEditingMode() == Editor::SECTORS) {
					drawGrid(*camera, 0);
				}
			}
		}

		// draw debug stuff
		camera->drawDebug();

		// reset selected cam in editor
		if (editor && editor->isInitialized()) {
			if (camera != editor->getEditingCamera() &&
				camera != editor->getMinimapCamera()) {
				camera->setWin(oldWin);
			}
		}

		// reset GL state
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		int xres = renderer->getXres();
		int yres = renderer->getYres();
		glViewport(0, 0, xres, yres);
		glScissor(0, 0, xres, yres);
		glEnable(GL_SCISSOR_TEST);
		ShaderProgram::unmount();
	}
}


bool SectorWorld::saveFile(const char* _filename, bool updateFilename) {
	mainEngine->fmsg(Engine::MSG_INFO, "unimplemented");
	return true;
}

void SectorWorld::createGrid() {
	Client* client = mainEngine->getLocalClient();
	if (!client) {
		return;
	}
	if (!mainEngine->isEditorRunning()) {
		return;
	}
	tileSize = cvar_snapTranslate.toFloat();

	for (int i = 0; i < BUFFER_MAX; ++i) {
		vbo[static_cast<buffer_t>(i)] = 0;
	}

	// create vertex array
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// create vertex data
	GLfloat* vertices = new GLfloat[(largeGridSize + 1 + largeGridSize + 1) * 3 * 2];
	GLfloat* colors = new GLfloat[(largeGridSize + 1 + largeGridSize + 1) * 4 * 2];
	for (Uint32 x = 0; x <= largeGridSize; ++x) {
		vertices[x * 6] = x * tileSize;
		vertices[x * 6 + 1] = 0;
		vertices[x * 6 + 2] = 0;
		vertices[x * 6 + 3] = x * tileSize;
		vertices[x * 6 + 4] = 0;
		vertices[x * 6 + 5] = largeGridSize * tileSize;

		if (x%largeGridSize) {
			colors[x * 8] = 1.f;
			colors[x * 8 + 1] = 0.f;
			colors[x * 8 + 2] = 0.f;
			colors[x * 8 + 3] = 1.f;
			colors[x * 8 + 4] = 1.f;
			colors[x * 8 + 5] = 0.f;
			colors[x * 8 + 6] = 0.f;
			colors[x * 8 + 7] = 1.f;
		} else {
			colors[x * 8] = 0.f;
			colors[x * 8 + 1] = 1.f;
			colors[x * 8 + 2] = 1.f;
			colors[x * 8 + 3] = 1.f;
			colors[x * 8 + 4] = 0.f;
			colors[x * 8 + 5] = 1.f;
			colors[x * 8 + 6] = 1.f;
			colors[x * 8 + 7] = 1.f;
		}
	}
	for (Uint32 y = 0; y <= largeGridSize; ++y) {
		vertices[(largeGridSize + 1) * 6 + y * 6] = 0;
		vertices[(largeGridSize + 1) * 6 + y * 6 + 1] = 0;
		vertices[(largeGridSize + 1) * 6 + y * 6 + 2] = y * tileSize;
		vertices[(largeGridSize + 1) * 6 + y * 6 + 3] = largeGridSize * tileSize;
		vertices[(largeGridSize + 1) * 6 + y * 6 + 4] = 0;
		vertices[(largeGridSize + 1) * 6 + y * 6 + 5] = y * tileSize;

		if (y%largeGridSize) {
			colors[(largeGridSize + 1) * 8 + y * 8] = 1.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 1] = 0.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 2] = 0.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 3] = 1.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 4] = 1.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 5] = 0.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 6] = 0.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 7] = 1.f;
		} else {
			colors[(largeGridSize + 1) * 8 + y * 8] = 0.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 1] = 1.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 2] = 1.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 3] = 1.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 4] = 0.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 5] = 1.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 6] = 1.f;
			colors[(largeGridSize + 1) * 8 + y * 8 + 7] = 1.f;
		}
	}

	// upload vertex data
	glGenBuffers(1, &vbo[BUFFER_VERTEX]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[BUFFER_VERTEX]);
	glBufferData(GL_ARRAY_BUFFER, (largeGridSize + 1 + largeGridSize + 1) * 3 * 2 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);
	delete[] vertices;

	// upload color data
	glGenBuffers(1, &vbo[BUFFER_COLOR]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[BUFFER_COLOR]);
	glBufferData(GL_ARRAY_BUFFER, (largeGridSize + 1 + largeGridSize + 1) * 4 * 2 * sizeof(GLfloat), colors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);
	delete[] colors;

	// create index data
	GLuint* indices = new GLuint[(largeGridSize + 1 + largeGridSize + 1) * 2];
	for (Uint32 i = 0; i < (largeGridSize + 1 + largeGridSize + 1) * 2; ++i) {
		indices[i] = i;
	}

	// upload index data
	glGenBuffers(1, &vbo[BUFFER_INDEX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[BUFFER_INDEX]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (largeGridSize + 1 + largeGridSize + 1) * 2 * sizeof(GLuint), indices, GL_STATIC_DRAW);
	delete[] indices;

	// unbind vertex array
	glBindVertexArray(0);
}

void SectorWorld::destroyGrid() {
	for (int i = 0; i < BUFFER_MAX; ++i) {
		buffer_t buffer = static_cast<buffer_t>(i);
		if (vbo[buffer]) {
			glDeleteBuffers(1, &vbo[buffer]);
		}
	}
	if (vao) {
		glDeleteVertexArrays(1, &vao);
	}
}

void SectorWorld::drawGrid(Camera& camera, float z) {
	//glLineWidth(2.f);

	// setup model matrix
	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.f), glm::vec3(pointerPos.x - largeGridSize * tileSize / 2.f, -z, pointerPos.y - largeGridSize * tileSize / 2.f));

	// setup view matrix
	const glm::mat4 viewMatrix = camera.getProjViewMatrix() * modelMatrix;

	// load shader
	Material* mat = mainEngine->getMaterialResource().dataForString("shaders/basic/grid.json");
	if (mat) {
		ShaderProgram& shader = mat->getShader();
		if (&shader != ShaderProgram::getCurrentShader())
			shader.mount();

		// upload uniform variables
		glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));

		// draw elements
		glBindVertexArray(vao);
		glDrawElements(GL_LINES, (largeGridSize + 1 + largeGridSize + 1) * 2, GL_UNSIGNED_INT, NULL);
		glBindVertexArray(0);

		shader.unmount();
	}
}

void SectorWorld::deleteGridRigidBody() {
	// delete old rigid body
	if (rigidBody != nullptr) {
		if (bulletDynamicsWorld) {
			bulletDynamicsWorld->removeRigidBody(rigidBody);
		}
		delete rigidBody;
		rigidBody = nullptr;
	}

	// delete motion state
	if (motionState != nullptr) {
		delete motionState;
		motionState = nullptr;
	}

	// delete collision volume
	if (collisionShapePtr != nullptr) {
		delete collisionShapePtr;
		collisionShapePtr = nullptr;
	}
}

void SectorWorld::updateGridRigidBody(float z) {
	deleteGridRigidBody();
	if (!mainEngine->isEditorRunning()) {
		return;
	}

	// setup new collision volume
	collisionShapePtr = new btStaticPlaneShape(btVector3(0, 0, -1), z);

	// create motion state
	btVector3 pos = btVector3(pointerPos.x, pointerPos.y, z);
	motionState = new btDefaultMotionState();

	// create rigid body
	btRigidBody::btRigidBodyConstructionInfo
		rigidBodyCI(0, motionState, collisionShapePtr, btVector3(0.f, 0.f, 0.f));
	rigidBody = new btRigidBody(rigidBodyCI);

	// add a new rigid body to the simulation
	bulletDynamicsWorld->addRigidBody(rigidBody);
}

void SectorWorld::generateObstacleCache()
{
	//TODO:
}

std::future<PathFinder::Path*> SectorWorld::findAPath(int startX, int startY, int endX, int endY) {
	//TODO:
	//return pathFinder.generateAStarPath(startX, startY, endX, endY);
	return std::future<PathFinder::Path*>();
}
