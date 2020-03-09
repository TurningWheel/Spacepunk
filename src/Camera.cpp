// Camera.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "Client.hpp"
#include "Camera.hpp"
#include "Rotation.hpp"
#include "Renderer.hpp"
#include "World.hpp"
#include "Tile.hpp"
#include "Script.hpp"
#include "BBox.hpp"
#include "Mesh.hpp"
#include "Mixer.hpp"
#include "Console.hpp"

const char* Camera::meshStr = "assets/editor/camera/camera.FBX";
const char* Camera::materialStr = "assets/editor/camera/material.json";

Camera::Camera(Entity& _entity, Component* _parent) :
	Component(_entity, _parent) {
	Client* client = mainEngine->getLocalClient();
	if (client) {
		renderer = client->getRenderer();
	}

	name = typeStr[COMPONENT_CAMERA];
	if (renderer) {
		win.x = 0;
		win.y = 0;
		win.w = renderer->getXres();
		win.h = renderer->getYres();
	}

	// add a bbox for editor usage
	if (mainEngine->isEditorRunning() && entity->isShouldSave()) {
		BBox* bbox = addComponent<BBox>();
		bbox->setShape(BBox::SHAPE_SPHERE);
		bbox->setLocalScale(Vector(16.f));
		bbox->setEditorOnly(true);
		bbox->update();
	}

	// exposed attributes
	attributes.push(new AttributeFloat("Clip Near", clipNear));
	attributes.push(new AttributeFloat("Clip Far", clipFar));
	attributes.push(new AttributeInt("Window X", win.x));
	attributes.push(new AttributeInt("Window Y", win.y));
	attributes.push(new AttributeInt("Window W", win.w));
	attributes.push(new AttributeInt("Window H", win.h));
	attributes.push(new AttributeInt("Vertical FOV in Degrees", fov));
	attributes.push(new AttributeBool("Orthogonal", ortho));
}

Camera::~Camera() {
	Client* client = mainEngine->getLocalClient();
	if (client) {
		Mixer* mixer = client->getMixer();
		if (mixer) {
			if (mixer->getListener() == this) {
				mixer->setListener(nullptr);
			}
		}
	}
}

glm::mat4 Camera::makeInfReversedZProj(float radians, float aspect, float zNear) {
	float f = 1.f / tanf(radians / 2.f);
	return glm::mat4(
		f / aspect, 0.0f, 0.0f, 0.0f,
		0.0f, f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, zNear, 0.0f);
}

glm::mat4 Camera::makeOrthoProj(float width, float height, float depth) {
	float left = -width;
	float right = width;
	float top = height;
	float bottom = -height;
	float zNear = -depth;
	float zFar = depth;
	glm::mat4 m(1.f);
	m[0][0] = 2.f / (right - left);
	m[1][1] = 2.f / (top - bottom);
	m[3][0] = -(right + left) / (right - left);
	m[3][1] = -(top + bottom) / (top - bottom);
	m[2][2] = 1.f / (zFar - zNear);
	m[3][2] = -zNear / (zFar - zNear);
	return m;
}

static Cvar cvar_cameraAllOrtho("camera.allortho", "make all cameras orthographic", "0");

void Camera::setupProjection(bool scissor) {
	World* world = entity->getWorld();
	if (!renderer) {
		return;
	}
	if (!win.w || !win.h) {
		return;
	}

	bool o = ortho || cvar_cameraAllOrtho.toInt();

	if (o) {
		float width = (float)(fov) * ((float)win.w / win.h);
		float height = (float)(fov);
		float depth = clipFar;
		projMatrix = makeOrthoProj(width, height, depth);
	} else {
		projMatrix = makeInfReversedZProj(glm::radians((float)fov), (float)win.w / win.h, clipNear);
	}

	if (o) {
		Vector pos = gPos - gAng.toVector() * clipFar * .5f;
		Quaternion q = Quaternion(Rotation(PI / 2.f, 0.f, 0.f)) * gAng;
		glm::mat4 cameraRotation = glm::mat4(glm::quat(-q.w, q.z, q.y, -q.x));
		glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), glm::vec3(-pos.x, pos.z, -pos.y));
		viewMatrix = cameraRotation * cameraTranslation;
	} else {
		Quaternion q = Quaternion(Rotation(PI / 2.f, 0.f, 0.f)) * gAng;
		glm::mat4 cameraRotation = glm::mat4(glm::quat(-q.w, q.z, q.y, -q.x));
		glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), glm::vec3(-gPos.x, gPos.z, -gPos.y));
		viewMatrix = cameraRotation * cameraTranslation;
	}

	if (renderer && scissor) {
		int yres = renderer->getYres();
		glViewport(win.x, yres - win.h - win.y, win.w, win.h);
		glScissor(win.x, yres - win.h - win.y, win.w, win.h);
		glEnable(GL_SCISSOR_TEST);
	} else {
		glViewport(win.x, -win.y, win.w, win.h);
		glDisable(GL_SCISSOR_TEST);
	}

	// get combination of the two
	projViewMatrix = projMatrix * viewMatrix;
}

Vector Camera::worldPosToScreenPos(const Vector& original) const {
	if (!renderer) {
		return Vector();
	}

	// get object position
	glm::vec3 position(original.x, -original.z, original.y);

	// project object
	glm::vec4 viewport = glm::vec4((float)win.x, (float)win.y, (float)win.w, (float)win.h);
	glm::vec3 projected = glm::project(position, glm::mat4(1.f), projMatrix * viewMatrix, viewport);
	return Vector(projected.x, win.h - (projected.y - win.y * 2), projected.z);
}

void Camera::screenPosToWorldRay(int x, int y, Vector& out_origin, Vector& out_direction) const {
	if (!renderer) {
		return;
	}

	x = x - win.x;
	y = win.h - (y - win.y);

	// create ray screen vectors
	glm::vec4 rayStart(
		((float)x / (float)win.w - 0.5f) * 2.0f,
		((float)y / (float)win.h - 0.5f) * 2.0f,
		-1.0,
		1.0f
	);
	glm::vec4 rayEnd(
		((float)x / (float)win.w - 0.5f) * 2.0f,
		((float)y / (float)win.h - 0.5f) * 2.0f,
		0.0,
		1.0f
	);

	glm::mat4 projMatrix = glm::perspective(glm::radians((float)fov), (float)win.w / win.h, clipNear, clipFar);
	glm::mat4 inversePMM = glm::inverse(projMatrix * viewMatrix);
	glm::vec4 rayStartWorld = inversePMM * rayStart; rayStartWorld /= rayStartWorld.w;
	glm::vec4 rayEndWorld = inversePMM * rayEnd; rayEndWorld /= rayEndWorld.w;

	glm::vec3 rayDirWorld(rayEndWorld - rayStartWorld);
	rayDirWorld = glm::normalize(rayDirWorld);

	out_origin = Vector(rayStartWorld.x, rayStartWorld.z, -rayStartWorld.y);
	out_direction = Vector(rayDirWorld.x, rayDirWorld.z, -rayDirWorld.y);
}

void Camera::resetMatrices() {
	projMatrix = glm::mat4(1.f);
	viewMatrix = glm::mat4(1.f);
	projViewMatrix = glm::mat4(1.f);
}

void Camera::drawCube(const glm::mat4& transform, const glm::vec4& color) {
	cube.draw(*this, transform, color);
}

void Camera::drawLine3D(const float width, const glm::vec3& src, const glm::vec3& dest, const glm::vec4& color) {
	line3D.drawLine(*this, width, src, dest, color);
}

void Camera::drawLaser(const float width, const glm::vec3& src, const glm::vec3& dest, const glm::vec4& color) {
	line3D.drawLaser(*this, width, src, dest, color);
}

void Camera::draw(Camera& camera, const ArrayList<Light*>& lights) {
	// only render in the editor!
	if (!mainEngine->isEditorRunning() || !entity->getWorld()->isShowTools() || camera.isOrtho()) {
		return;
	}

	// don't render ourselves
	if (&camera == this) {
		return;
	}

	// don't render the editor minimap
	if (ortho && !entity->isShouldSave()) {
		return;
	}

	// do not render for these fx passes
	if (camera.getDrawMode() >= Camera::DRAW_GLOW) {
		return;
	}

	glm::mat4 matrix = glm::translate(glm::mat4(1.f), glm::vec3(0, -8.f, 0.f));
	Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr);
	Material* material = mainEngine->getMaterialResource().dataForString(materialStr);
	if (mesh && material) {
		ShaderProgram* shader = mesh->loadShader(*this, camera, lights, material, shaderVars, gMat * matrix);
		if (shader) {
			mesh->draw(camera, this, shader);
		}
	}
}

void Camera::load(FILE* fp) {
	Component::load(fp);

	Engine::freadl(&clipNear, sizeof(float), 1, fp, nullptr, "Camera::load()");
	Engine::freadl(&clipFar, sizeof(float), 1, fp, nullptr, "Camera::load()");
	Engine::freadl(&win.x, sizeof(Sint32), 1, fp, nullptr, "Camera::load()");
	Engine::freadl(&win.y, sizeof(Sint32), 1, fp, nullptr, "Camera::load()");
	Engine::freadl(&win.w, sizeof(Sint32), 1, fp, nullptr, "Camera::load()");
	Engine::freadl(&win.h, sizeof(Sint32), 1, fp, nullptr, "Camera::load()");
	Engine::freadl(&fov, sizeof(Sint32), 1, fp, nullptr, "Camera::load()");
	Engine::freadl(&ortho, sizeof(bool), 1, fp, nullptr, "Camera::load()");

	Uint32 reserved = 0;
	Engine::freadl(&reserved, sizeof(Uint32), 1, fp, nullptr, "Camera::load()");

	loadSubComponents(fp);
}

void Camera::serialize(FileInterface* file) {
	Component::serialize(file);

	Uint32 version = 0;
	file->property("Camera::version", version);
	file->property("clipNear", clipNear);
	file->property("clipFar", clipFar);
	file->property("win", win);
	file->property("fov", fov);
	file->property("ortho", ortho);
}

void Camera::markPoint(unsigned int x, unsigned int y, const glm::vec4& color) {
	point_t point;
	point.x = x;
	point.y = y;
	point.color = color;
	points.addNodeLast(point);
}

void Camera::markPoint(const Vector& pos, const glm::vec4& color) {
	Vector diff = pos - gPos;
	float dot = diff.dot(gAng.toVector());
	if (dot > 0) {
		Vector proj = worldPosToScreenPos(pos);
		Rect<int> src;
		src.x = proj.x - 4; src.y = proj.y - 4;
		src.w = 8; src.h = 8;
		if (win.containsPoint(src.x, src.y)) {
			markPoint(src.x, src.y, color);
		}
	}
}

void Camera::markLine(const Vector& start, const Vector& end, const glm::vec4& color) {
	line_t line;
	line.start = start;
	line.end = end;
	line.color = color;
	lines.addNodeLast(line);
}

void Camera::drawDebug() {
	// draw points
	{
		Node<point_t>* node = points.getFirst();
		Node<point_t>* nextnode = nullptr;
		for (; node != nullptr; node = nextnode) {
			point_t& point = node->getData();
			nextnode = node->getNext();

			Rect<Sint32> src;
			src.x = point.x - 2;
			src.y = point.y - 2;
			src.w = 4;
			src.h = 4;

			renderer->drawRect(&src, point.color);
			points.removeNode(node);
		}
	}

	// draw lines
	{
		Node<line_t>* node = lines.getFirst();
		Node<line_t>* nextnode = nullptr;
		for (; node != nullptr; node = nextnode) {
			line_t& line = node->getData();
			nextnode = node->getNext();

			drawLine3D(2, line.start, line.end, line.color);
			lines.removeNode(node);
		}
	}
}

void Camera::onFrameDrawn() {
	++framesDrawn;
}