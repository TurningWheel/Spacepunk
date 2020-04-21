// BasicWorld.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "SectorVertex.hpp"
#include "Sector.hpp"
#include "BasicWorld.hpp"
#include "Renderer.hpp"
#include "Editor.hpp"
#include "Entity.hpp"
#include "Client.hpp"

#include "Component.hpp"
#include "Light.hpp"
#include "Camera.hpp"

// there are two strings so we can convert different formats if need be.
const char* BasicWorld::fileMagicNumber = "SPACEPUNK_BWD_01";
const char* BasicWorld::fileMagicNumberSaveOut = "SPACEPUNK_BWD_01";

BasicWorld::BasicWorld(Game* _game, bool _silent, Uint32 _id, const char* _name)
	: World(_game)
	//	: pathFinder(*(new PathFinder(*this)))
{
	nameStr = _name;
	silent = _silent;
	clientObj = game->isClient();
	id = _id;
	loaded = true;
}

BasicWorld::~BasicWorld() {
	destroyGrid();
}

void BasicWorld::initialize(bool empty) {
	World::initialize(empty);

	// initialize entities
	for (auto pair : entities) {
		Entity* entity = pair.b;
		entity->update();
	}

	// create grid object
	createGrid();
}

void BasicWorld::deselectGeometry() {
}

void BasicWorld::findEntitiesInRadius(const Vector& origin, float radius, LinkedList<Entity*>& outList, bool flat) {
	if (radius <= 0) {
		return;
	}
	for (auto pair : entities) {
		Entity* entity = pair.b;
		const Vector& pos = flat ? Vector(entity->getPos().x, entity->getPos().y, origin.z) : entity->getPos();
		if ((pos - origin).lengthSquared() <= radius * radius) {
			outList.addNodeFirst(entity);
		}
	}
}

void BasicWorld::process() {
	World::process();
}

void BasicWorld::drawSceneObjects(Camera& camera, const ArrayList<Light*>& lights, const ArrayList<Entity*>& entities) {
	Client* client = mainEngine->getLocalClient();
	if (!client)
		return;

	// editor variables
	bool editorActive = false;
	bool ceilingMode = false;
	Editor::editingmode_t editingMode = Editor::editingmode_t::TILES;
	Uint32 highlightedObj = nuid;
	if (client->isEditorActive()) {
		editorActive = true;
		Editor* editor = client->getEditor();
		ceilingMode = editor->isCeilingMode();
		highlightedObj = editor->getHighlightedObj();
		editingMode = static_cast<Editor::editingmode_t>(editor->getEditingMode());
	}

	float offset = cvar_depthOffset.toFloat();
	if (camera.getDrawMode() == Camera::DRAW_DEPTH && offset) {
		glPolygonOffset(offset, 0.f);
		glEnable(GL_POLYGON_OFFSET_FILL);
	}

	// draw entities
	if (camera.getDrawMode() != Camera::DRAW_STENCIL) {
		if (camera.getDrawMode() != Camera::DRAW_GLOW || !editorActive || !showTools) {
			for (auto entity : entities) {
				// in silhouette mode, skip unhighlighted or unselected actors
				if (camera.getDrawMode() == Camera::DRAW_SILHOUETTE) {
					if (!entity->isHighlighted() && entity->getUID() != highlightedObj) {
						continue;
					}
				}

				// draw the entity
				entity->draw(camera, lights);
			}
		}
	}

	if (camera.getDrawMode() == Camera::DRAW_DEPTH && offset) {
		glPolygonOffset(1.f, 0.f);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	// reset some gl state
	glActiveTexture(GL_TEXTURE0);
	ShaderProgram::unmount();
	camera.onFrameDrawn();
}

void BasicWorld::fillDrawList(const Camera& camera, ArrayList<Entity*>& entities) {
	for (auto pair : this->entities) {
		Entity* entity = pair.b;
		entities.push(entity);
	}
	class SortFn : public ArrayList<Entity*>::SortFunction {
	public:
		SortFn(const Camera& _camera) :
			camera(_camera) {}
		virtual ~SortFn() {}
		virtual const bool operator()(Entity* a, Entity* b) const override {
			float lengthA = (a->getPos() - camera.getGlobalPos()).lengthSquared();
			float lengthB = (b->getPos() - camera.getGlobalPos()).lengthSquared();
			return lengthA < lengthB;
		}
		const Camera& camera;
	};
	entities.sort(SortFn(camera));
}

void BasicWorld::draw() {
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
		entity->findAllComponents<Light>(Component::COMPONENT_LIGHT, lights);
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

		// skip deactivated cameras
		if (!camera->getEntity()->isFlag(Entity::flag_t::FLAG_VISIBLE) || !camera->isEnabled()) {
			continue;
		}

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

		// skip cameras whose window is too small
		if (camera->getWin().w <= 0 || camera->getWin().h <= 0) {
			continue;
		}

		// occlusion test
		if (!camera->getChunksVisible()) {
			camera->occlusionTest(camera->getClipFar(), cvar_renderCull.toInt());
		}

		ArrayList<Light*> cameraLightList;

		// build relevant light list
		// this could be done better
		const bool shadowsEnabled = (!client->isEditorActive() || !showTools) && !cvar_renderFullbright.toInt() && cvar_shadowsEnabled.toInt();
		for (Node<Light*>* node = lights.getFirst(); node != nullptr; node = node->getNext()) {
			Light* light = node->getData();

			// don't render invisible lights
			if (!light->getEntity()->isFlag(Entity::flag_t::FLAG_VISIBLE) || light->getIntensity() <= 0.f || light->getRadius() <= 0.f || light->getArc() <= 0.f) {
				continue;
			}

			/*if (light->getEntity()->isOccluded(*camera)) {
				continue;
			}*/

			if (cameraLightList.getSize() >= Mesh::maxLights) {
				break;
			}

			cameraLightList.push(light);
			if (shadowsEnabled) {
				if (light->getEntity()->isFlag(Entity::flag_t::FLAG_STATIC) || !cvar_shadowsStaticOnly.toInt()) {
					if (light->getEntity()->isFlag(Entity::flag_t::FLAG_SHADOW) && light->isShadow()) {
						light->createShadowMap();
					}
				}
			}
		}
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		// clear the window area
		glClear(GL_DEPTH_BUFFER_BIT);
		Rect<int> backgroundRect = camera->getWin();
		renderer->drawRect(&backgroundRect, glm::vec4(0.f, 0.f, 0.f, 1.f));

		// set proper light blending function
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		// setup projection
		camera->setupProjection(true);
		glEnable(GL_DEPTH_TEST);
		glDrawBuffer(GL_NONE);

		// build and sort entity list by distance to camera
		ArrayList<Entity*> drawList;
		fillDrawList(*camera, drawList);

		// render bounds for occlusion query
		camera->setDrawMode(Camera::DRAW_BOUNDS);
		drawSceneObjects(*camera, ArrayList<Light*>(), drawList);
		glClear(GL_DEPTH_BUFFER_BIT);

		// render scene into depth buffer
		camera->setDrawMode(Camera::DRAW_DEPTH);
		glDepthMask(GL_TRUE);
		drawSceneObjects(*camera, ArrayList<Light*>(), drawList);

		if ((client->isEditorActive() && showTools) || cvar_renderFullbright.toInt()) {
			// render fullbright scene
			camera->setDrawMode(Camera::DRAW_STANDARD);
			static const GLenum attachments[Framebuffer::ColorBuffer::MAX] = {
				GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			glDrawBuffers(Framebuffer::ColorBuffer::MAX, attachments);
			glDisable(GL_STENCIL_TEST);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glDepthFunc(GL_GEQUAL);
			drawSceneObjects(*camera, ArrayList<Light*>(), drawList);
		} else {
			// render shadowed scene
			camera->setDrawMode(Camera::DRAW_STANDARD);
			static const GLenum attachments[Framebuffer::ColorBuffer::MAX] = {
				GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			glDrawBuffers(Framebuffer::ColorBuffer::MAX, attachments);
			glDisable(GL_STENCIL_TEST);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glDepthFunc(GL_GEQUAL);
			drawSceneObjects(*camera, cameraLightList, drawList);
		}

		// render scene with glow textures
		camera->setDrawMode(Camera::DRAW_GLOW);
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_GEQUAL);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		drawSceneObjects(*camera, ArrayList<Light*>(), drawList);
		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		drawSceneObjects(*camera, ArrayList<Light*>(), drawList);

		static const GLenum attachments[Framebuffer::ColorBuffer::MAX] = {
			GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(Framebuffer::ColorBuffer::MAX, attachments);

		// render lasers
		for (auto& laser : lasers) {
			if (laser.maxLife > 0.f) {
				glm::vec4 color = laser.color;
				color.a *= laser.life / laser.maxLife;
				camera->drawLaser(laser.size, laser.start, laser.end, color);
			} else {
				camera->drawLaser(laser.size, laser.start, laser.end, laser.color);
			}
		}

		// render triangle lines
		if (cvar_showVerts.toInt()) {
			camera->setDrawMode(Camera::DRAW_TRIANGLES);
			drawSceneObjects(*camera, ArrayList<Light*>(), drawList);
		}

		// render depth fail scene
		camera->setDrawMode(Camera::DRAW_DEPTHFAIL);
		glDepthFunc(GL_LESS);
		drawSceneObjects(*camera, ArrayList<Light*>(), drawList);
		glDepthFunc(GL_GEQUAL);

		// render silhouettes
		camera->setDrawMode(Camera::DRAW_SILHOUETTE);
		drawSceneObjects(*camera, ArrayList<Light*>(), drawList);

		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_STENCIL_TEST);

		// draw editing grid
		if (client->isEditorActive() && showTools && gridVisible) {
			drawGrid(*camera);
		}

		// reset selected cam in editor
		if (editor && editor->isInitialized()) {
			if (camera != editor->getEditingCamera() &&
				camera != editor->getMinimapCamera()) {
				camera->setWin(oldWin);
			}
		}

		// draw debug stuff
		camera->drawDebug();

		// reset GL state
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		int xres = renderer->getXres();
		int yres = renderer->getYres();
		glViewport(0, 0, xres, yres);
		glScissor(0, 0, xres, yres);
		glEnable(GL_SCISSOR_TEST);
		ShaderProgram::unmount();

		cameraLightList.clear();
	}

	for (auto light : lights) {
		if (light->getShadowTicks() != light->getEntity()->getTicks()) {
			light->deleteShadowMap();
		}
	}
}

void BasicWorld::generateObstacleCache() {
	//TODO:
}

std::future<PathFinder::Path*> BasicWorld::findAPath(int startX, int startY, int endX, int endY) {
	//TODO:
	//return pathFinder.generateAStarPath(startX, startY, endX, endY);
	return std::future<PathFinder::Path*>();
}

void BasicWorld::createGrid() {
	Client* client = mainEngine->getLocalClient();
	if (!client) {
		return;
	}
	if (!mainEngine->isEditorRunning()) {
		return;
	}
	float tileSize = cvar_snapTranslate.toFloat();

	for (int i = 0; i < BUFFER_MAX; ++i) {
		vbo[static_cast<buffer_t>(i)] = 0;
	}

	// create vertex array
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// create vertex data
	glm::vec4 color(1.f, 0.4f, 0.1f, 1.f);
	glm::vec4 highlightColor(1.f, 0.2f, 0.2f, 1.f);
	GLfloat* vertices = new GLfloat[lines * 3 * 2];
	GLfloat* colors = new GLfloat[lines * 4 * 2];
	for (Uint32 x = 0; x <= gridWidth; ++x) {
		vertices[x * 6] = x * tileSize;
		vertices[x * 6 + 1] = 0;
		vertices[x * 6 + 2] = 0;
		vertices[x * 6 + 3] = x * tileSize;
		vertices[x * 6 + 4] = 0;
		vertices[x * 6 + 5] = gridHeight * tileSize;

		if (x%largeGridSize) {
			colors[x * 8] = color.r;
			colors[x * 8 + 1] = color.g;
			colors[x * 8 + 2] = color.b;
			colors[x * 8 + 3] = color.a;
			colors[x * 8 + 4] = color.r;
			colors[x * 8 + 5] = color.g;
			colors[x * 8 + 6] = color.b;
			colors[x * 8 + 7] = color.a;
		} else {
			colors[x * 8] = highlightColor.r;
			colors[x * 8 + 1] = highlightColor.g;
			colors[x * 8 + 2] = highlightColor.b;
			colors[x * 8 + 3] = highlightColor.a;
			colors[x * 8 + 4] = highlightColor.r;
			colors[x * 8 + 5] = highlightColor.g;
			colors[x * 8 + 6] = highlightColor.b;
			colors[x * 8 + 7] = highlightColor.a;
		}
	}
	for (Uint32 y = 0; y <= gridHeight; ++y) {
		vertices[(gridWidth + 1) * 6 + y * 6] = 0;
		vertices[(gridWidth + 1) * 6 + y * 6 + 1] = 0;
		vertices[(gridWidth + 1) * 6 + y * 6 + 2] = y * tileSize;
		vertices[(gridWidth + 1) * 6 + y * 6 + 3] = gridWidth * tileSize;
		vertices[(gridWidth + 1) * 6 + y * 6 + 4] = 0;
		vertices[(gridWidth + 1) * 6 + y * 6 + 5] = y * tileSize;

		if (y%largeGridSize) {
			colors[(gridWidth + 1) * 8 + y * 8] = color.r;
			colors[(gridWidth + 1) * 8 + y * 8 + 1] = color.g;
			colors[(gridWidth + 1) * 8 + y * 8 + 2] = color.b;
			colors[(gridWidth + 1) * 8 + y * 8 + 3] = color.a;
			colors[(gridWidth + 1) * 8 + y * 8 + 4] = color.r;
			colors[(gridWidth + 1) * 8 + y * 8 + 5] = color.g;
			colors[(gridWidth + 1) * 8 + y * 8 + 6] = color.b;
			colors[(gridWidth + 1) * 8 + y * 8 + 7] = color.a;
		} else {
			colors[(gridWidth + 1) * 8 + y * 8] = highlightColor.r;
			colors[(gridWidth + 1) * 8 + y * 8 + 1] = highlightColor.g;
			colors[(gridWidth + 1) * 8 + y * 8 + 2] = highlightColor.b;
			colors[(gridWidth + 1) * 8 + y * 8 + 3] = highlightColor.a;
			colors[(gridWidth + 1) * 8 + y * 8 + 4] = highlightColor.r;
			colors[(gridWidth + 1) * 8 + y * 8 + 5] = highlightColor.g;
			colors[(gridWidth + 1) * 8 + y * 8 + 6] = highlightColor.b;
			colors[(gridWidth + 1) * 8 + y * 8 + 7] = highlightColor.a;
		}
	}

	// upload vertex data
	glGenBuffers(1, &vbo[BUFFER_VERTEX]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[BUFFER_VERTEX]);
	glBufferData(GL_ARRAY_BUFFER, lines * 3 * 2 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);
	delete[] vertices;

	// upload color data
	glGenBuffers(1, &vbo[BUFFER_COLOR]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[BUFFER_COLOR]);
	glBufferData(GL_ARRAY_BUFFER, lines * 4 * 2 * sizeof(GLfloat), colors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);
	delete[] colors;

	// create index data
	GLuint* indices = new GLuint[lines * 2];
	for (Uint32 i = 0; i < lines * 2; ++i) {
		indices[i] = i;
	}

	// upload index data
	glGenBuffers(1, &vbo[BUFFER_INDEX]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[BUFFER_INDEX]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, lines * 2 * sizeof(GLuint), indices, GL_STATIC_DRAW);
	delete[] indices;

	// unbind vertex array
	glBindVertexArray(0);
}

void BasicWorld::destroyGrid() {
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

void BasicWorld::drawGrid(Camera& camera) {
	const glm::mat4 viewMatrix = camera.getProjViewMatrix();

	// load shader
	Material* mat = mainEngine->getMaterialResource().dataForString("shaders/basic/grid.json");
	if (mat) {
		ShaderProgram& shader = mat->getShader().mount();

		// upload uniform variables
		glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glm::vec3 cameraPos(camera.getGlobalPos().x, -camera.getGlobalPos().z, camera.getGlobalPos().y);
		glUniform3fv(shader.getUniformLocation("gCameraPos"), 1, glm::value_ptr(cameraPos));

		// draw elements
		glBindVertexArray(vao);
		glDrawElements(GL_LINES, lines * 2, GL_UNSIGNED_INT, NULL);
		glBindVertexArray(0);

		shader.unmount();
	}
}

bool BasicWorld::saveFile(const char* _filename, bool updateFilename) {
	const String& path = (_filename == nullptr || _filename[0] == '\0') ? filename : StringBuf<256>(_filename);
	if (updateFilename) {
		changeFilename(path.get());
	}

	// open file for saving
	if (path.empty()) {
		mainEngine->fmsg(Engine::MSG_WARN, "attempted to save world without filename");
		return false;
	}
	if (_filename == nullptr || _filename[0] == '\0') {
		mainEngine->fmsg(Engine::MSG_INFO, "saving world file '%s'...", shortname.get());
	} else {
		mainEngine->fmsg(Engine::MSG_INFO, "saving world file '%s'...", _filename);
	}

	if (filetype == FILE_BINARY) {
		return FileHelper::writeObject(path.get(), EFileFormat::Binary, *this);
	} else {
		return FileHelper::writeObject(path.get(), EFileFormat::Json, *this);
	}
}

void BasicWorld::serialize(FileInterface * file) {
	Uint32 version = 0;

	file->property("BasicWorld::version", version);
	file->property("nameStr", nameStr);

	file->propertyName("entities");
	if (file->isReading()) {
		Uint32 numEntities = 0;
		file->beginArray(numEntities);

		for (Uint32 index = 0; index < numEntities; ++index) {
			Entity * entity = new Entity(this);
			file->value(*entity);
		}

		file->endArray();
	} else {
		// write number of entities
		Uint32 numEntities = 0;
		for (auto pair : entities) {
			Entity* entity = pair.b;
			if (entity->isToBeDeleted() || !entity->isShouldSave()) {
				continue;
			}

			++numEntities;
		}

		file->beginArray(numEntities);

		for (auto pair : entities) {
			Entity* entity = pair.b;
			if (entity->isToBeDeleted() || !entity->isShouldSave()) {
				continue;
			}

			file->value(*entity);
		}

		file->endArray();
	}
}