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
#include "Angle.hpp"
#include "Renderer.hpp"
#include "World.hpp"
#include "Tile.hpp"
#include "Script.hpp"
#include "BBox.hpp"
#include "Mesh.hpp"

const char* Camera::meshStr = "assets/editor/camera/camera.FBX";
const char* Camera::materialStr = "assets/editor/camera/material.json";

Camera::Camera(Entity& _entity, Component* _parent) :
	Component(_entity, _parent) {
	Client* client = mainEngine->getLocalClient();
	if( client ) {
		renderer = client->getRenderer();
	}

	name = typeStr[COMPONENT_CAMERA];
	if( renderer ) {
		win.x = 0;
		win.y = 0;
		win.w = renderer->getXres();
		win.h = renderer->getYres();
	}

	// add a bbox for editor usage
	if( mainEngine->isEditorRunning() && entity->isShouldSave() ) {
		BBox* bbox = addComponent<BBox>();
		bbox->setShape(BBox::SHAPE_SPHERE);
		bbox->setLocalPos(Vector(0.f, 0.f, -16.f));
		bbox->setLocalScale(Vector(16.f));
		bbox->setEditorOnly(true);
		bbox->update();
	}
}

Camera::~Camera() {
}

void Camera::setupProjection() {
	World* world = entity->getWorld();
	if( !renderer ) {
		return;
	}
	if( !win.w || !win.h ) {
		return;
	}
	if( ortho ) {
		if( !world )
			return;
		float width = (float)(fov) * ( (float)win.w / win.h );
		float height = (float)(fov);
		float depth = clipFar;

		// get camera transformation
		viewMatrix = glm::lookAt(
			glm::vec3( gPos.x, 0.f, gPos.y ), // origin
			glm::vec3( gPos.x, 1.f, gPos.y ), // target vector
			glm::vec3( 0.f,    0.f, 1.f    )  // up vector
		); 

		// get projection transformation
		projMatrix = glm::ortho( -width, width, height, -height, depth, -depth );
	} else {
		// get camera transformation
		glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f),glm::vec3( -gPos.x, gPos.z, -gPos.y ));
		glm::mat4 cameraRotation = glm::mat4( 1.f );
		cameraRotation = glm::rotate(cameraRotation, (float)(gAng.pitch), glm::vec3(1.f, 0.f, 0.f));
		cameraRotation = glm::rotate(cameraRotation, (float)(gAng.yaw + PI/2.f), glm::vec3(0.f, 1.f, 0.f));
		cameraRotation = glm::rotate(cameraRotation, (float)(gAng.roll), glm::vec3(cos(gAng.yaw), 0.f, sin(gAng.yaw)));
		viewMatrix = cameraRotation * cameraTranslation;

		// get projection transformation
		projMatrix = glm::perspective( glm::radians((float)fov), (float)win.w/win.h, clipNear, clipFar );
	}

	if( renderer ) {
		int yres = renderer->getYres();
		glViewport( win.x, yres-win.h-win.y, win.w, win.h );
		glScissor( win.x, yres-win.h-win.y, win.w, win.h );
		glEnable(GL_SCISSOR_TEST);
	}

	// get combination of the two
	projViewMatrix = projMatrix * viewMatrix;
}

Vector Camera::worldPosToScreenPos( const Vector& original ) const {
	if( !renderer ) {
		return Vector();
	}

	// get object position
	glm::vec3 position(original.x, -original.z, original.y);

	// project object
	glm::vec4 viewport = glm::vec4( (float)win.x, (float)win.y, (float)win.w, (float)win.h );
	glm::vec3 projected = glm::project( position, glm::mat4(1.f), projMatrix * viewMatrix, viewport);
	return Vector(projected.x, win.h - (projected.y - win.y*2), projected.z);
}

void Camera::screenPosToWorldRay( int x, int y, Vector& out_origin, Vector& out_direction ) const {
	if( !renderer ) {
		return;
	}

	x = x - win.x;
	y = win.h - (y - win.y);

	// create ray screen vectors
	glm::vec4 rayStart(
		((float)x/(float)win.w  - 0.5f) * 2.0f,
		((float)y/(float)win.h - 0.5f) * 2.0f,
		-1.0,
		1.0f
	);
	glm::vec4 rayEnd(
		((float)x/(float)win.w  - 0.5f) * 2.0f,
		((float)y/(float)win.h - 0.5f) * 2.0f,
		0.0,
		1.0f
	);

	glm::mat4 inversePMM    = glm::inverse(projMatrix * viewMatrix);
	glm::vec4 rayStartWorld = inversePMM * rayStart; rayStartWorld /= rayStartWorld.w;
	glm::vec4 rayEndWorld   = inversePMM * rayEnd  ; rayEndWorld   /= rayEndWorld.w;

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

void Camera::drawCube( const glm::mat4& transform, const glm::vec4& color ) {
	cube.draw(*this,transform,color);
}

void Camera::drawLine3D( const float width, const glm::vec3& src, const glm::vec3& dest, const glm::vec4& color ) {
	line3D.drawLine(*this,width,src,dest,color);
}

void Camera::drawLaser( const float width, const glm::vec3& src, const glm::vec3& dest, const glm::vec4& color ) {
	line3D.drawLaser(*this,width,src,dest,color);
}

void Camera::draw(Camera& camera, Light* light) {
	// only render in the editor!
	if( !mainEngine->isEditorRunning() || !entity->getWorld()->isShowTools() || camera.isOrtho() ) {
		return;
	}

	// don't render ourselves
	if( &camera == this ) {
		return;
	}

	// don't render ortho cameras
	if( ortho ) {
		return;
	}

	// do not render for these fx passes
	if( camera.getDrawMode() >= Camera::DRAW_GLOW ) {
		return;
	}

	Mesh* mesh = mainEngine->getMeshResource().dataForString(meshStr);
	Material* material = mainEngine->getMaterialResource().dataForString(materialStr);
	if( mesh && material ) {
		ShaderProgram* shader = mesh->loadShader(*this, camera, light, material, shaderVars, gMat);
		if( shader ) {
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
	if( dot > 0 ) {
		Vector proj = worldPosToScreenPos(pos);
		Rect<int> src;
		src.x = proj.x-4; src.y = proj.y-4;
		src.w = 8; src.h = 8;
		if( win.containsPoint(src.x,src.y) ) {
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
		for( ; node != nullptr; node = nextnode ) {
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
		for( ; node != nullptr; node = nextnode ) {
			line_t& line = node->getData();
			nextnode = node->getNext();

			drawLine3D(2, line.start, line.end, line.color);
			lines.removeNode(node);
		}
	}
}