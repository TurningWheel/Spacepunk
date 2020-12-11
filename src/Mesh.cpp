// Mesh.cpp

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Main.hpp"
#include "LinkedList.hpp"
#include "Node.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include "Client.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Model.hpp"

Mesh::Mesh(const char* _name) : Asset(_name) {
	if (!_name || _name[0] == '\0') {
		return;
	}
	if (_name[0] == '#') {
		mainEngine->fmsg(Engine::MSG_DEBUG, "allocating composite mesh '%s'...", name.get());
		return;
	} else {
		mainEngine->fmsg(Engine::MSG_DEBUG, "loading mesh '%s'...", name.get());

		path = mainEngine->buildPath(_name).get();

		String extension = path.substr(path.length() - 4);
		if (extension == ".vox") {
			// load voxel mesh
			VoxelMeshData data = VoxelReader::readVoxel(path.get());
			Mesh::SubMesh* entry = new Mesh::SubMesh(data);
			mainEngine->fmsg(Engine::MSG_DEBUG, "loaded voxel mesh: %d verts", entry->getNumVertices());
			subMeshes.addNodeLast(entry);
			numVertices += entry->getNumVertices();
		} else {
			// load standard mesh
			importer = new Assimp::Importer();
			unsigned int flags = 0;
			flags |= aiProcess_SplitByBoneCount;
			flags |= aiProcess_SplitLargeMeshes;
			flags |= aiProcess_CalcTangentSpace;
			flags |= aiProcess_GenSmoothNormals;
			flags |= aiProcess_Triangulate;
			flags |= aiProcess_FlipUVs;
			flags |= aiProcess_JoinIdenticalVertices;
			//flags |= aiProcess_FixInfacingNormals;
			flags |= aiProcess_ValidateDataStructure;
			flags |= aiProcess_ImproveCacheLocality;
			flags |= aiProcess_RemoveRedundantMaterials;
			flags |= aiProcess_SortByPType;
			flags |= aiProcess_FindInvalidData;
			flags |= aiProcess_OptimizeMeshes;
#ifndef PLATFORM_LINUX
			flags |= aiProcess_OptimizeGraph; // ASSIMP crashes on linux when this is used
#endif
			flags |= aiProcess_LimitBoneWeights;
			scene = importer->ReadFile(path.get(), 0);
			if (!scene) {
				mainEngine->fmsg(Engine::MSG_ERROR, "failed to load mesh '%s': %s", name.get(), importer->GetErrorString());
				return;
			} else {
				if (!scene->HasAnimations()) {
					flags |= aiProcess_PreTransformVertices;
				}
				importer->ApplyPostProcessing(flags);
			}
			for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
				Mesh::SubMesh* entry = new Mesh::SubMesh(scene, scene->mMeshes[i]);
				if (i == 0) {
					minBox = entry->getMinBox();
					maxBox = entry->getMaxBox();
				} else {
					minBox.x = min(minBox.x, entry->getMinBox().x);
					minBox.y = min(minBox.y, entry->getMinBox().y);
					minBox.z = min(minBox.z, entry->getMinBox().z);
					maxBox.x = max(maxBox.x, entry->getMaxBox().x);
					maxBox.y = max(maxBox.y, entry->getMaxBox().y);
					maxBox.z = max(maxBox.z, entry->getMaxBox().z);
				}
				subMeshes.addNodeLast(entry);
				numBones += entry->getNumBones();
				numVertices += entry->getNumVertices();
				mainEngine->fmsg(Engine::MSG_DEBUG, "loaded submesh: %d verts, %d bones", entry->getNumVertices(), entry->getNumBones());
			}
		}
		mainEngine->fmsg(Engine::MSG_DEBUG, "loaded mesh '%s': %d entries, %d verts, %d bones", name.get(), subMeshes.getSize(), numVertices, numBones);
		return;
	}
}

Mesh::~Mesh(void) {
	clear();
}

bool Mesh::finalize() {
	if (scene) {
		for (auto& submesh : subMeshes) {
			submesh->finalize();
		}
		return loaded = subMeshes.getSize() != 0;
	} else {
		// if this is true, we are dealing with a multimesh, and not having a scene is normal.
		// otherwise, something has gone wrong and the mesh is invalid.
		return loaded = name[0] == '#';
	}
}

void Mesh::clear() {
	numBones = 0;
	numVertices = 0;
	minBox = Vector(0.f);
	maxBox = Vector(0.f);
	while (subMeshes.getFirst()) {
		delete subMeshes.getFirst()->getData();
		subMeshes.removeNode(subMeshes.getFirst());
	}
	if (importer) {
		if (scene) {
			importer->FreeScene();
			scene = nullptr;
		}
		delete importer;
		importer = nullptr;
	}
}

void Mesh::composeMesh(const LinkedList<Model*>& models, const glm::mat4& root) {
	unsigned int _numVertices = 0;
	unsigned int _numIndices = 0;
	for (auto model : models) {
		Mesh* mesh = mainEngine->getStaticMeshResource().dataForString(model->getMesh());
		if (!mesh) {
			mainEngine->fmsg(Engine::MSG_WARN, "tried to compose multimesh with mesh that couldn't be found");
			continue;
		}
		for (auto submesh : mesh->getSubMeshes()) {
			_numVertices += submesh->getNumVertices();
			_numIndices += submesh->getNumIndices();
		}
	}
	if (!_numVertices || !_numIndices) {
		return;
	}
	Mesh::SubMesh* entry = new Mesh::SubMesh(_numIndices, _numVertices);
	for (auto model : models) {
		Mesh* mesh = mainEngine->getStaticMeshResource().dataForString(model->getMesh());
		if (!mesh) {
			mainEngine->fmsg(Engine::MSG_WARN, "tried to compose multimesh with mesh that couldn't be found");
			continue;
		}
		for (auto submesh : mesh->getSubMeshes()) {
			entry->append(*submesh, glm::inverse(root) * model->getGlobalMat());
		}
	}
	entry->finalize();
	subMeshes.addNodeLast(entry);
	numVertices += entry->getNumVertices();
	mainEngine->fmsg(Engine::MSG_DEBUG, "composed mesh '%s': %d entries, %d verts", name.get(), subMeshes.getSize(), numVertices);
}

unsigned int Mesh::boneIndexForName(const char* name) const {
	int result = 0;
	for (const Node<SubMesh*>* node = subMeshes.getFirst(); node != nullptr; node = node->getNext()) {
		const SubMesh* entry = node->getData();

		int index = entry->boneIndexForName(name);
		if (index == UINT32_MAX) {
			result += entry->getNumBones();
		} else {
			return result + index;
		}
	}
	return UINT32_MAX;
}

const bool Mesh::hasAnimations() const {
	if (scene) {
		return scene->HasAnimations();
	} else {
		return false;
	}
}

float Mesh::getAnimLength() const {
	if (scene) {
		if (scene->mNumAnimations > 0 && scene->mAnimations[0]) {
			return scene->mAnimations[0]->mDuration;
		}
	}
	return 0.f;
}

ShaderProgram* Mesh::loadShader(const Component& component, Camera& camera, const ArrayList<Light*>& lights, Material* material, const Mesh::shadervars_t& shaderVars, const glm::mat4& matrix) {
	Client* client = mainEngine->getLocalClient();
	if (!client)
		return nullptr;
	Renderer* renderer = camera.getRenderer();
	if (!renderer)
		return nullptr;
	bool editor = false;
	if (mainEngine->isEditorRunning() && component.getEntity()->getWorld()) {
		editor = component.getEntity()->getWorld()->isShowTools();
	}

	// don't highlight if lineWidth == 0
	if (camera.getDrawMode() == Camera::DRAW_SILHOUETTE ||
		camera.getDrawMode() == Camera::DRAW_TRIANGLES) {
		if (shaderVars.lineWidth <= 0) {
			return nullptr;
		}
	}

	// get shader program
	Material* mat = nullptr;
	switch (camera.getDrawMode()) {
	case Camera::DRAW_BOUNDS:
	case Camera::DRAW_DEPTH:
		mat = mainEngine->getMaterialResource().dataForString("shaders/actor/depth.json");
		break;
	case Camera::DRAW_SILHOUETTE:
		mat = mainEngine->getMaterialResource().dataForString("shaders/actor/silhouette.json");
		break;
	case Camera::DRAW_STENCIL:
		mat = mainEngine->getMaterialResource().dataForString("shaders/actor/stencil.json");
		break;
	case Camera::DRAW_DEPTHFAIL:
		mat = material;
		break;
	case Camera::DRAW_TRIANGLES:
		mat = mainEngine->getMaterialResource().dataForString("shaders/actor/triangles.json");
		break;
	case Camera::DRAW_SHADOW:
		mat = mainEngine->getMaterialResource().dataForString("shaders/actor/shadow.json");
		break;
	default:
		if (material == nullptr) {
			mat = mainEngine->getMaterialResource().dataForString("shaders/actor/std.json");
		} else {
			mat = material;
		}
		break;
	}

	if (!mat) {
		mat = mainEngine->getMaterialResource().dataForString("shaders/actor/error.json");
	}

	ShaderProgram& shader = mat->getShader().mount();

	// set line width
	if (shaderVars.lineWidth > 0) {
		//glLineWidth(shaderVars.lineWidth);
	}

	// load highlight color into shader
	glUniform4fv(shader.getUniformLocation("gHighlightColor"), 1, glm::value_ptr(shaderVars.highlightColor));

	// load textures, if necessary
	unsigned int textureUnit = 0;
	if (camera.getDrawMode() == Camera::DRAW_STANDARD || camera.getDrawMode() == Camera::DRAW_DEPTHFAIL || camera.getDrawMode() == Camera::DRAW_GLOW) {
		// load per-mesh shader vars
		glUniform1i(shader.getUniformLocation("gCustomColorEnabled"), shaderVars.customColorEnabled ? GL_TRUE : GL_FALSE);
		if (shaderVars.customColorEnabled == true) {
			glUniform4fv(shader.getUniformLocation("gCustomColorR"), 1, &shaderVars.customColorR[0]);
			glUniform4fv(shader.getUniformLocation("gCustomColorG"), 1, &shaderVars.customColorG[0]);
			glUniform4fv(shader.getUniformLocation("gCustomColorB"), 1, &shaderVars.customColorB[0]);
			glUniform4fv(shader.getUniformLocation("gCustomColorA"), 1, &shaderVars.customColorA[0]);
		}

		// bind textures
		if (mat) {
			if (camera.getDrawMode() == Camera::DRAW_GLOW) {
				textureUnit = mat->bindTextures(Material::GLOW);
			} else {
				textureUnit = mat->bindTextures(Material::STANDARD);
			}
		}
	}

	// load common shader vars
	glUniform1i(shader.getUniformLocation("gTime"), (GLint)mainEngine->getTicks());
	glm::vec3 gMaxBox(maxBox.x, -maxBox.z, maxBox.y);
	glUniform3fv(shader.getUniformLocation("gBoundingBox"), 1, glm::value_ptr(gMaxBox));

	glm::mat4 viewMatrix = camera.getProjViewMatrix();

	if (camera.getDrawMode() <= Camera::DRAW_DEPTH) {

		// load model matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gModel"), 1, GL_FALSE, glm::value_ptr(matrix));

		// load projection matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
	} else if (camera.getDrawMode() == Camera::DRAW_SHADOW) {
		// load guid
		Uint32 hashed = component.getEntity()->getUID();
		hashed ^= component.getUID() + 0x9e3779b9 + (hashed << 6) + (hashed >> 2);
		glUniform1ui(shader.getUniformLocation("gUID"), hashed);

		// load model matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gModel"), 1, GL_FALSE, glm::value_ptr(matrix));

		// load projection matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));

		// load light data into shader
		if (lights.getSize()) {
			glm::vec3 lightPos(lights[0]->getGlobalPos().x, -lights[0]->getGlobalPos().z, lights[0]->getGlobalPos().y);
			glUniform3fv(shader.getUniformLocation("gLightPos"), 1, glm::value_ptr(lightPos));
		} else {
			glm::vec3 lightPos(camera.getGlobalPos().x, -camera.getGlobalPos().z, camera.getGlobalPos().y);
			glUniform3fv(shader.getUniformLocation("gLightPos"), 1, glm::value_ptr(lightPos));
		}
	} else if (camera.getDrawMode() == Camera::DRAW_SILHOUETTE || camera.getDrawMode() == Camera::DRAW_TRIANGLES) {

		// load model matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gModel"), 1, GL_FALSE, glm::value_ptr(matrix));

		// load projection matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));

		// load camera position into shader
		glm::vec3 cameraPos(camera.getGlobalPos().x, -camera.getGlobalPos().z, camera.getGlobalPos().y);
		glUniform3fv(shader.getUniformLocation("gCameraPos"), 1, glm::value_ptr(cameraPos));

	} else if (camera.getDrawMode() == Camera::DRAW_STENCIL) {

		// load model matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gModel"), 1, GL_FALSE, glm::value_ptr(matrix));

		// load projection matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));

		// load light data into shader
		if (lights.getSize()) {
			glm::vec3 lightPos(lights[0]->getGlobalPos().x, -lights[0]->getGlobalPos().z, lights[0]->getGlobalPos().y);
			glUniform3fv(shader.getUniformLocation("gLightPos"), 1, glm::value_ptr(lightPos));
		} else {
			glm::vec3 lightPos(camera.getGlobalPos().x, -camera.getGlobalPos().z, camera.getGlobalPos().y);
			glUniform3fv(shader.getUniformLocation("gLightPos"), 1, glm::value_ptr(lightPos));
		}
	} else {
		// load guid
		Uint32 hashed = component.getEntity()->getUID();
		hashed ^= component.getUID() + 0x9e3779b9 + (hashed << 6) + (hashed >> 2);
		glUniform1ui(shader.getUniformLocation("gUID"), hashed);

		// load model matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gModel"), 1, GL_FALSE, glm::value_ptr(matrix));

		// load model normal matrix
		glm::mat4 normalMat = component.getGlobalMat();
		normalMat[3] = glm::vec4(0.f, 0.f, 0.f, 1.f);
		glUniformMatrix4fv(shader.getUniformLocation("gNormalTransform"), 1, GL_FALSE, glm::value_ptr(normalMat));

		// load projection matrix into shader
		glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));

		// load camera position into shader
		glm::vec3 cameraPos = glm::vec3(camera.getGlobalPos().x, -camera.getGlobalPos().z, camera.getGlobalPos().y);
		glUniform3fv(shader.getUniformLocation("gCameraPos"), 1, glm::value_ptr(cameraPos));

		// load light data into shader
		if (component.getEntity()->isFlag(Entity::flag_t::FLAG_FULLYLIT) || camera.getDrawMode() == Camera::DRAW_GLOW || camera.getDrawMode() == Camera::DRAW_DEPTHFAIL) {
			glUniform1i(shader.getUniformLocation("gActiveLight"), GL_FALSE);
			glUniform1i(shader.getUniformLocation("gNumLights"), 0);

			char buf[32];
			for (int index = 0; index < maxLights; ++textureUnit, ++index) {
				glUniform1i(shader.getUniformLocation(ShaderProgram::uniformArray(buf, "gShadowmapEnabled", 17, index)), GL_FALSE);
				glUniform1i(shader.getUniformLocation(ShaderProgram::uniformArray(buf, "gShadowmap", 10, index)), textureUnit);
				camera.getEntity()->getWorld()->getDefaultShadow().bindForReading(GL_TEXTURE0 + textureUnit, GL_DEPTH_ATTACHMENT);
			}
			/*for (int index = 0; index < maxLights; ++textureUnit, ++index) {
				glUniform1i(shader.getUniformLocation(ShaderProgram::uniformArray(buf, "gUIDmap", 7, index)), textureUnit);
				camera.getEntity()->getWorld()->getDefaultShadow().bindForReading(GL_TEXTURE0 + textureUnit, GL_COLOR_ATTACHMENT0);
			}*/
		} else {
			glUniform1i(shader.getUniformLocation("gActiveLight"), GL_TRUE);
			int oldTextureUnit = textureUnit;
			if (lights.getSize()) {
				textureUnit = shader.uploadLights(camera, lights, maxLights, textureUnit);
			} else if (editor) {
				glUniform3fv(shader.getUniformLocation("gLightPos[0]"), 1, glm::value_ptr(cameraPos));
				glUniform3fv(shader.getUniformLocation("gLightColor[0]"), 1, glm::value_ptr(glm::vec3(1.f, 1.f, 1.f)));
				glUniform1f(shader.getUniformLocation("gLightIntensity[0]"), 1.f);
				glUniform1f(shader.getUniformLocation("gLightRadius[0]"), 16384.f);
				glUniform3fv(shader.getUniformLocation("gLightScale[0]"), 1, glm::value_ptr(glm::vec3(1.f, 1.f, 1.f)));
				glUniform1i(shader.getUniformLocation("gLightShape[0]"), 0);
				glUniform1i(shader.getUniformLocation("gShadowmapEnabled[0]"), GL_FALSE);
				glUniform1i(shader.getUniformLocation("gNumLights"), 1);
			} else {
				glUniform1i(shader.getUniformLocation("gNumLights"), 0);
			}

			char buf[32];
			unsigned int newTextureUnit = textureUnit;
			for (int index = newTextureUnit - oldTextureUnit; index < maxLights; ++textureUnit, ++index) {
				glUniform1i(shader.getUniformLocation(ShaderProgram::uniformArray(buf, "gShadowmapEnabled", 17, index)), GL_FALSE);
				glUniform1i(shader.getUniformLocation(ShaderProgram::uniformArray(buf, "gShadowmap", 10, index)), textureUnit);
				camera.getEntity()->getWorld()->getDefaultShadow().bindForReading(GL_TEXTURE0 + textureUnit, GL_DEPTH_ATTACHMENT);
			}
			/*for (int index = newTextureUnit - oldTextureUnit; index < maxLights; ++textureUnit, ++index) {
				glUniform1i(shader.getUniformLocation(ShaderProgram::uniformArray(buf, "gUIDmap", 7, index)), textureUnit);
				camera.getEntity()->getWorld()->getDefaultShadow().bindForReading(GL_TEXTURE0 + textureUnit, GL_COLOR_ATTACHMENT0);
			}*/
		}
	}

	return &shader;
}

void Mesh::skin(const AnimationMap& animations, SkinCache& skincache) const {
	if (!hasAnimations()) {
		return;
	}

	skincache.clear();
	skincache.resize(subMeshes.getSize());

	// remove unused animations from the map
	AnimationMap _anims;
	for (auto& anim : animations) {
		bool foundWeight = false;
		for (auto& weight : anim.b.getWeights()) {
			if (weight.b.value > 0.f) {
				foundWeight = true;
				break;
			}
		}
		if (foundWeight) {
			_anims.insert(anim.a, anim.b);
		}
	}

	Uint32 index = 0;
	for (auto& entry : subMeshes) {
		if (hasAnimations()) {
			if (skincache[index].anims.empty()) {
				entry->boneTransform(_anims, skincache[index]);
			}
		}

		++index;
	}
}

static Cvar cvar_showBones("showbones", "displays bones in animated models as dots for debug purposes", "0");
static Cvar cvar_findBone("findbone", "used with showbones, displays only the bone with the given name", "");

void Mesh::draw(Camera& camera, const Component* component, SkinCache& skincache, ShaderProgram* shader) {
	if (skincache.getSize() < subMeshes.getSize()) {
		skincache.resize(subMeshes.getSize());
	}

	Uint32 index = 0;
	for (auto& entry : subMeshes) {
		if (shader) {
			if (hasAnimations()) {
				glUniform1i(shader->getUniformLocation("gAnimated"), GL_TRUE);

				unsigned int numBones = std::min((unsigned int)skincache[index].anims.getSize(), (unsigned int)SubMesh::maxBones);

				char name[16];
				for (unsigned int i = 0; i < numBones; ++i) {
					if (entry->getBones()[i].real == false) {
						break;
					}
					glUniformMatrix4fv(shader->getUniformLocation(ShaderProgram::uniformArray(name, "gBones", 6, i)), 1, GL_FALSE, glm::value_ptr(skincache[index].anims[i]));
#ifndef NDEBUG
					// debug stuff
					if (cvar_showBones.toInt() && component) {
						String findBone = cvar_findBone.toStr();
						if (findBone == "" || findBone == entry->getBones()[i].name) {
							glm::mat4 mat = component->getGlobalMat() * skincache[index].offsets[i];
							Vector pos0 = Vector(mat[3][0], mat[3][2], -mat[3][1]);
							Vector pos1 = pos0 + Vector(mat[0][0], mat[0][2], -mat[0][1]).normal() * 10.f;
							Vector pos2 = pos0 + Vector(mat[1][0], mat[1][2], -mat[1][1]).normal() * 10.f;
							Vector pos3 = pos0 + Vector(mat[2][0], mat[2][2], -mat[2][1]).normal() * 10.f;
							camera.markPoint(pos0, glm::vec4(.5f, .5f, .5f, 1.f));
							camera.markLine(pos0, pos1, glm::vec4(.5f, 0.f, 0.f, 1.f));
							camera.markLine(pos0, pos2, glm::vec4(0.f, .5f, 0.f, 1.f));
							camera.markLine(pos0, pos3, glm::vec4(0.f, 0.f, .5f, 1.f));
						}
					}
#endif
				}
			} else {
				glUniform1i(shader->getUniformLocation("gAnimated"), GL_FALSE);
			}
		}

		entry->draw(camera);
		++index;
	}
}

void Mesh::draw(Camera& camera, const Component* component, ShaderProgram* shader) {
	SkinCache skincache;
	for (Uint32 c = 0; c < subMeshes.getSize(); ++c) {
		skincache.push(skincache_t());
	}
	draw(camera, component, skincache, shader);
}

Mesh::SubMesh::SubMesh(unsigned int _numIndices, unsigned int _numVertices) {
	for (int i = 0; i < BUFFER_TYPE_LENGTH; ++i) {
		vbo[static_cast<buffer_t>(i)] = 0;
	}

	elementCount = _numIndices;
	numVertices = _numVertices;

	vertices = new float[numVertices * 3];
	texCoords = new float[numVertices * 2];
	normals = new float[numVertices * 3];
	colors = new float[numVertices * 4];
	tangents = new float[numVertices * 3];
	indices = new GLuint[elementCount];
	for (unsigned int c = 0; c < elementCount; ++c) {
		indices[c] = 0U;
	}
	for (unsigned int c = 0; c < numVertices * 2; ++c) {
		texCoords[c] = 0.f;
	}
	for (unsigned int c = 0; c < numVertices * 3; ++c) {
		vertices[c] = 0.f;
		normals[c] = 0.f;
		tangents[c] = 0.f;
	}
	for (unsigned int c = 0; c < numVertices * 4; ++c) {
		colors[c] = 0.f;
	}
}

Mesh::SubMesh::SubMesh(const SubMesh& submesh, const glm::mat4& root) :
	SubMesh(submesh.getNumIndices(), submesh.getNumVertices()) {
	append(submesh, root);
}

void Mesh::SubMesh::finalize() {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	if (vertices) {
		glGenBuffers(1, &vbo[VERTEX_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[VERTEX_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, 3 * numVertices * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);
	}
	if (texCoords) {
		glGenBuffers(1, &vbo[TEXCOORD_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[TEXCOORD_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, 2 * numVertices * sizeof(GLfloat), texCoords, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(1);
	}
	if (normals) {
		glGenBuffers(1, &vbo[NORMAL_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[NORMAL_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, 3 * numVertices * sizeof(GLfloat), normals, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(2);
	}
	if (colors) {
		glGenBuffers(1, &vbo[COLOR_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[COLOR_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, 4 * numVertices * sizeof(GLfloat), colors, GL_STATIC_DRAW);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(3);
	}
	if (vertexbonedata) {
		glGenBuffers(1, &vbo[BONE_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[BONE_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexBoneData) * numVertices, vertexbonedata, GL_STATIC_DRAW);

		glVertexAttribIPointer(4, 4, GL_INT, sizeof(VertexBoneData), NULL);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)(sizeof(GLint) * 4));
		glEnableVertexAttribArray(5);
	}
	if (tangents) {
		glGenBuffers(1, &vbo[TANGENT_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[TANGENT_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, 3 * numVertices * sizeof(GLfloat), tangents, GL_STATIC_DRAW);
		glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(6);
	}
	if (indices) {
		glGenBuffers(1, &vbo[INDEX_BUFFER]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[INDEX_BUFFER]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementCount * sizeof(GLuint), indices, GL_STATIC_DRAW);
	}

	glBindVertexArray(0);
}

void Mesh::SubMesh::append(const SubMesh& submesh, const glm::mat4& root) {
	const glm::mat4& positionMat = root;
	glm::mat4 normalMat = root;
	normalMat[3] = glm::vec4(0.f, 0.f, 0.f, 1.f);

	// positions
	if (submesh.getVertices()) {
		memcpy(&vertices[lastVertex * 3], submesh.getVertices(), sizeof(float) * submesh.getNumVertices() * 3);
		for (unsigned int c = lastVertex; c < lastVertex + submesh.getNumVertices(); ++c) {
			glm::vec4 vertex(vertices[c * 3 + 0], vertices[c * 3 + 1], vertices[c * 3 + 2], 1.f);
			vertex = positionMat * vertex;
			vertices[c * 3 + 0] = vertex.x;
			vertices[c * 3 + 1] = vertex.y;
			vertices[c * 3 + 2] = vertex.z;
		}
	}

	// texcoords
	if (submesh.getTexCoords()) {
		memcpy(&texCoords[lastVertex * 2], submesh.getTexCoords(), sizeof(float) * submesh.getNumVertices() * 2);
	}

	// normals
	if (submesh.getNormals()) {
		memcpy(&normals[lastVertex * 3], submesh.getNormals(), sizeof(float) * submesh.getNumVertices() * 3);
		for (unsigned int c = lastVertex; c < lastVertex + submesh.getNumVertices(); ++c) {
			glm::vec4 normal(normals[c * 3], normals[c * 3 + 1], normals[c * 3 + 2], 0.f);
			normal = normalMat * normal;
			normals[c * 3 + 0] = normal.x;
			normals[c * 3 + 1] = normal.y;
			normals[c * 3 + 2] = normal.z;
		}
	}

	// colors
	if (submesh.getColors()) {
		memcpy(&colors[lastVertex * 4], submesh.getColors(), sizeof(float) * submesh.getNumVertices() * 4);
	}

	// tangents
	if (submesh.getTangents()) {
		memcpy(&tangents[lastVertex * 3], submesh.getTangents(), sizeof(float) * submesh.getNumVertices() * 3);
		for (unsigned int c = lastVertex; c < lastVertex + submesh.getNumVertices(); ++c) {
			glm::vec4 tangent(tangents[c * 3], tangents[c * 3 + 1], tangents[c * 3 + 2], 0.f);
			tangent = normalMat * tangent;
			tangents[c * 3 + 0] = tangent.x;
			tangents[c * 3 + 1] = tangent.y;
			tangents[c * 3 + 2] = tangent.z;
		}
	}

	// indices
	if (submesh.getIndices()) {
		memcpy(&indices[lastIndex], submesh.getIndices(), sizeof(GLuint) * submesh.getNumIndices());
		for (unsigned int c = lastIndex; c < lastIndex + submesh.getNumIndices(); ++c) {
			indices[c] += lastVertex;
		}
	}

	lastVertex += submesh.getNumVertices();
	lastIndex += submesh.getNumIndices();
}

Mesh::SubMesh::SubMesh(const VoxelMeshData& data) {
	for (int i = 0; i < BUFFER_TYPE_LENGTH; ++i) {
		vbo[static_cast<buffer_t>(i)] = 0;
	}

	elementCount = (unsigned int)data.indexCount;
	numVertices = (unsigned int)data.vertexCount;

	// positions
	{
		vertices = new float[numVertices * 3];
		memcpy(vertices, data.positions.get(), sizeof(float) * numVertices * 3);
	}

	// colors
	{
		colors = new float[numVertices * 3];
		memcpy(colors, data.colors.get(), sizeof(float) * numVertices * 3);
	}

	// normals
	{
		normals = new float[numVertices * 3];
		memcpy(normals, data.normals.get(), sizeof(float) * numVertices * 3);
	}

	// indices
	{
		indices = new GLuint[elementCount];
		memcpy(indices, data.indices.get(), sizeof(GLuint) * elementCount);
	}
}

Mesh::SubMesh::SubMesh(const aiScene* _scene, aiMesh* mesh) {
	scene = _scene;

	for (int i = 0; i < BUFFER_TYPE_LENGTH; ++i) {
		vbo[static_cast<buffer_t>(i)] = 0;
	}

	numVertices = mesh->mNumVertices;
	elementCount = mesh->mNumFaces * 3 * 2;

	if (mesh->HasPositions()) {
		if (vertices)
			delete[] vertices;
		vertices = new float[mesh->mNumVertices * 3];
		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
			vertices[i * 3] = mesh->mVertices[i].x;
			vertices[i * 3 + 1] = mesh->mVertices[i].y;
			vertices[i * 3 + 2] = mesh->mVertices[i].z;

			if (i == 0) {
				minBox.x = maxBox.x =  vertices[i * 3];
				minBox.y = maxBox.y =  vertices[i * 3 + 2];
				minBox.z = maxBox.z = -vertices[i * 3 + 1];
			} else {
				minBox.x = std::min(minBox.x,  vertices[i * 3]);
				minBox.y = std::min(minBox.y,  vertices[i * 3 + 2]);
				minBox.z = std::min(minBox.z, -vertices[i * 3 + 1]);
				maxBox.x = std::max(maxBox.x,  vertices[i * 3]);
				maxBox.y = std::max(maxBox.y,  vertices[i * 3 + 2]);
				maxBox.z = std::max(maxBox.z, -vertices[i * 3 + 1]);
			}
		}
	}

	if (mesh->HasTextureCoords(0)) {
		if (texCoords)
			delete[] texCoords;
		texCoords = new float[mesh->mNumVertices * 2];
		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
			texCoords[i * 2] = mesh->mTextureCoords[0][i].x;
			texCoords[i * 2 + 1] = mesh->mTextureCoords[0][i].y;
		}
	}

	if (mesh->HasNormals()) {
		if (normals)
			delete[] normals;
		normals = new float[mesh->mNumVertices * 3];
		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
			normals[i * 3] = mesh->mNormals[i].x;
			normals[i * 3 + 1] = mesh->mNormals[i].y;
			normals[i * 3 + 2] = mesh->mNormals[i].z;
		}
	}

	if (mesh->HasVertexColors(0)) {
		if (colors)
			delete[] colors;
		colors = new float[mesh->mNumVertices * 4];
		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
			colors[i * 4] = mesh->mColors[0][i].r;
			colors[i * 4 + 1] = mesh->mColors[0][i].g;
			colors[i * 4 + 2] = mesh->mColors[0][i].b;
			colors[i * 4 + 3] = mesh->mColors[0][i].a;
		}
	}

	if (mesh->HasBones()) {
		if (vertexbonedata)
			delete[] vertexbonedata;
		vertexbonedata = new VertexBoneData[mesh->mNumVertices];
		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
			for (unsigned int k = 0; k < numBonesPerVertex; ++k) {
				vertexbonedata[i].ids[k] = 0;
				vertexbonedata[i].weights[k] = 0.f;
			}
		}
		for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
			const char* boneName = mesh->mBones[i]->mName.data;
			unsigned int boneIndex = 0;

			if (!boneMapping.exists(boneName)) {
				boneIndex = numBones;
				++numBones;
				boneinfo_t bi;
				bi.name = boneName;
				bi.offset = glm::mat4();
				bi.real = true;
				bones.push(bi);

				boneMapping.insert(boneName, boneIndex);
			} else {
				boneIndex = *boneMapping[boneName];
				*boneMapping[boneName] = boneIndex;
			}

			bones[boneIndex].offset = glm::transpose(glm::make_mat4(&mesh->mBones[i]->mOffsetMatrix.a1));

			for (unsigned int j = 0; j < mesh->mBones[i]->mNumWeights; ++j) {
				unsigned int id = mesh->mBones[i]->mWeights[j].mVertexId;
				float weight = mesh->mBones[i]->mWeights[j].mWeight;
				vertexbonedata[id].addBoneData(boneIndex, weight);
			}
		}

		// maps nodes that might not be considered "bones" per-se
		if (scene) {
			mapBones(scene->mRootNode);
		}
	}

	if (mesh->HasTangentsAndBitangents()) {
		if (tangents)
			delete[] tangents;
		tangents = new float[mesh->mNumVertices * 3];
		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
			tangents[i * 3] = mesh->mTangents[i].x;
			tangents[i * 3 + 1] = mesh->mTangents[i].y;
			tangents[i * 3 + 2] = mesh->mTangents[i].z;
		}
	}

	if (mesh->HasFaces()) {
		if (indices)
			delete[] indices;
		indices = new GLuint[mesh->mNumFaces * 3 * 2];

		int index = 0;
		for (unsigned int i = 0; i < mesh->mNumFaces; ++i, ++index) {
			auto* p = &indices[index * 6];
			if(mesh->mFaces[i].mNumIndices == 3) {
				p[0] = mesh->mFaces[i].mIndices[0];
				p[2] = mesh->mFaces[i].mIndices[1];
				p[4] = mesh->mFaces[i].mIndices[2];
				p[1] = findAdjacentIndex(*mesh, p[0], p[2], p[4]);
				p[3] = findAdjacentIndex(*mesh, p[2], p[4], p[0]);
				p[5] = findAdjacentIndex(*mesh, p[4], p[0], p[2]);
			} else {
				p[0] = 0;
				p[1] = 0;
				p[2] = 0;
				p[3] = 0;
				p[4] = 0;
				p[5] = 0;
			}
		}
	}
}

unsigned int Mesh::SubMesh::boneIndexForName(const char* name) const {
	if (boneMapping.exists(name)) {
		return *boneMapping.find(name);
	} else {
		return UINT32_MAX;
	}
}

GLuint Mesh::SubMesh::findAdjacentIndex(const aiMesh& mesh, GLuint index1, GLuint index2, GLuint index3) {
	GLuint indices[6];
	for (unsigned int i = 0; i < mesh.mNumFaces; ++i) {
		unsigned int*& faceIndices = mesh.mFaces[i].mIndices;
		indices[0] = faceIndices[0];
		indices[1] = faceIndices[1];
		indices[2] = faceIndices[2];
		indices[3] = faceIndices[0];
		indices[4] = faceIndices[1];
		indices[5] = faceIndices[2];
		for (int edge = 0; edge < 3; ++edge) {
			GLuint v1 = indices[edge]; // first edge index
			GLuint v2 = indices[edge + 1]; // second edge index
			GLuint vOpp = indices[edge + 2]; // index of opposite vertex

			// if the edge matches the search edge and the opposite vertex does not match
			if (((v1 == index1 && v2 == index2) || (v2 == index1 && v1 == index2)) && vOpp != index3) {
				return vOpp; // we have found the adjacent vertex
			}
		}
	}

	// no opposite edge found
	return index3;
}

void Mesh::SubMesh::VertexBoneData::addBoneData(unsigned int boneID, float weight) {
	for (unsigned int i = 0; i < numBonesPerVertex; ++i) {
		if (weights[i] == 0.f) {
			ids[i] = boneID;
			weights[i] = weight;
			return;
		}
	}

	// should never get here - more bones than we have space for
	mainEngine->fmsg(Engine::MSG_WARN, "mesh loaded with more than %d bones per vertex", numBonesPerVertex);
	assert(0);
}

void Mesh::SubMesh::mapBones(const aiNode* node) {
	const char* nodeName = node->mName.data;
	if (!boneMapping[nodeName]) {
		boneMapping.insert(nodeName, numBones);
		boneinfo_t bi;
		bi.name = nodeName;
		bi.offset = glm::mat4();
		bi.real = false;
		bones.push(bi);
		++numBones;
	}
	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		mapBones(node->mChildren[i]);
	}
}

void Mesh::SubMesh::boneTransform(const AnimationMap& animations, skincache_t& skin) const {
	if (!scene || !scene->HasAnimations())
		return;
	const glm::mat4 identity(1.f);

	skin.anims.resize(numBones);
	skin.offsets.resize(numBones);

	readNodeHierarchy(&animations, &skin, scene->mRootNode, &identity);
}

#include <future>
#include <vector>
#include <mutex>

void Mesh::SubMesh::readNodeHierarchy(const AnimationMap* animations, skincache_t* skin, const aiNode* node, const glm::mat4* rootTransform) const {
	assert(scene);

	aiMatrix4x4 nodeTransform = node->mTransformation;
	const char* nodeName = node->mName.data;
	const aiNodeAnim* nodeAnim = findNodeAnim(scene->mAnimations[0], nodeName);

	if (nodeAnim) {
		aiVector3D scaling;
		aiQuaternion rotationQ;
		aiVector3D translation;

		// interpolate scaling, rotation, and position for each animation
		bool first = true;
		for (auto& pair : *animations) {
			const AnimationState& anim = pair.b;
			float weight = anim.getWeight(nodeAnim->mNodeName.data);

			calcInterpolatedScaling(scaling, anim, weight, nodeAnim);
			calcInterpolatedRotation(rotationQ, anim, weight, nodeAnim, first);
			calcInterpolatedPosition(translation, anim, weight, nodeAnim);
		}
		rotationQ.Normalize();

		// build transforms from scaling, rotation, and translation
		aiMatrix4x4 scalingM;
		aiMatrix4x4::Scaling(scaling, scalingM);
		aiMatrix4x4 rotationM(rotationQ.GetMatrix());
		aiMatrix4x4 translationM;
		aiMatrix4x4::Translation(translation, translationM);

		// combine the above transformations
		nodeTransform = translationM * rotationM * scalingM;
	}

	glm::mat4 glmTransform = glm::transpose(glm::make_mat4(&nodeTransform.a1));
	glm::mat4 globalTransform = *rootTransform * glmTransform;

	const unsigned int* boneIndexPtr = boneMapping[nodeName];
	if (boneIndexPtr) {
		unsigned int boneIndex = *boneIndexPtr;
		skin->offsets[boneIndex] = globalTransform;
		skin->anims[boneIndex] = globalTransform * bones[boneIndex].offset;
	}

	if (node->mNumChildren > 1) {
		std::vector<std::future<void>> jobs;
		for (unsigned int i = 0; i < node->mNumChildren; ++i) {
			jobs.push_back(std::async(std::launch::async, &Mesh::SubMesh::readNodeHierarchy, this, animations, skin, node->mChildren[i], &globalTransform));
		}
		for (auto& job : jobs) {
			job.wait();
		}
	} else if (node->mNumChildren == 1) {
		readNodeHierarchy(animations, skin, node->mChildren[0], &globalTransform);
	}
}

const aiNodeAnim* Mesh::SubMesh::findNodeAnim(const aiAnimation* animation, const char* str) const {
	for (unsigned int i = 0; i < animation->mNumChannels; ++i) {
		const char* curStr = animation->mChannels[i]->mNodeName.data;

		if (strcmp(curStr, str) == 0) {
			return animation->mChannels[i];
		}
	}

	return nullptr;
}

void Mesh::SubMesh::calcInterpolatedPosition(aiVector3D& out, const AnimationState& anim, float weight, const aiNodeAnim* nodeAnim) const {
	if (weight <= 0.f) {
		return;
	}
	if (nodeAnim->mNumPositionKeys == 1) {
		out += nodeAnim->mPositionKeys[0].mValue * weight;
		return;
	}

	if (anim.getLength() > 1.f) {
		if (anim.isLoop() || anim.getTicks() < anim.getLength()) {
			float timeCur = anim.getBegin() + anim.getTicks();
			float timeNext = anim.isLoop() ?
				anim.getBegin() + fmod(anim.getTicks() + anim.getTicksRate(), anim.getLength()) :
				anim.getBegin() + anim.getTicks() + anim.getTicksRate();
			unsigned int indexCur = findPosition(timeCur, nodeAnim);
			unsigned int indexNext = timeCur == timeNext ? indexCur : findPosition(timeNext, nodeAnim);
			const aiVector3D& start = nodeAnim->mPositionKeys[indexCur].mValue;
			const aiVector3D& end = nodeAnim->mPositionKeys[indexNext].mValue;
			aiVector3D delta = end - start;
			out += (start + delta * 0.5f) * weight;
		} else {
			float timeEnd = anim.getEnd();
			unsigned int indexCur = findPosition(timeEnd, nodeAnim);
			const aiVector3D& end = nodeAnim->mPositionKeys[indexCur].mValue;
			out += end * weight;
		}
	} else {
		float timeBeg = anim.getBegin();
		unsigned int indexCur = findPosition(timeBeg, nodeAnim);
		const aiVector3D& start = nodeAnim->mPositionKeys[indexCur].mValue;
		out += start * weight;
	}
}

void Mesh::SubMesh::calcInterpolatedRotation(aiQuaternion& out, const AnimationState& anim, float weight, const aiNodeAnim* nodeAnim, bool& first) const {
	if (weight <= 0.f) {
		return;
	}
	if (nodeAnim->mNumRotationKeys == 1) {
		const aiQuaternion& rotationQ = nodeAnim->mRotationKeys[0].mValue;
		if (first) {
			out = rotationQ;
			first = false;
		} else {
			aiQuaternion::Interpolate(out, aiQuaternion(out), rotationQ, weight);
		}
		return;
	}

	if (anim.getLength() > 1.f) {
		if (anim.isLoop() || anim.getTicks() < anim.getLength()) {
			float timeCur = anim.getBegin() + anim.getTicks();
			float timeNext = anim.isLoop() ?
				anim.getBegin() + fmod(anim.getTicks() + anim.getTicksRate(), anim.getLength()) :
				anim.getBegin() + anim.getTicks() + anim.getTicksRate();
			unsigned int indexCur = findRotation(timeCur, nodeAnim);
			unsigned int indexNext = timeCur == timeNext ? indexCur : findRotation(timeNext, nodeAnim);
			const aiQuaternion& startRotationQ = nodeAnim->mRotationKeys[indexCur].mValue;
			const aiQuaternion& endRotationQ = nodeAnim->mRotationKeys[indexNext].mValue;
			aiQuaternion rotationQ;
			aiQuaternion::Interpolate(rotationQ, startRotationQ, endRotationQ, 0.5f);
			if (first) {
				out = rotationQ;
				first = false;
			} else {
				aiQuaternion::Interpolate(out, aiQuaternion(out), rotationQ, weight);
			}
		} else {
			float timeEnd = anim.getEnd();
			unsigned int indexCur = findRotation(timeEnd, nodeAnim);
			const aiQuaternion& endRotationQ = nodeAnim->mRotationKeys[indexCur].mValue;
			if (first) {
				out = endRotationQ;
				first = false;
			} else {
				aiQuaternion::Interpolate(out, aiQuaternion(out), endRotationQ, weight);
			}
		}
	} else {
		float timeBeg = anim.getBegin();
		unsigned int indexCur = findRotation(timeBeg, nodeAnim);
		const aiQuaternion& startRotationQ = nodeAnim->mRotationKeys[indexCur].mValue;
		if (first) {
			out = startRotationQ;
			first = false;
		} else {
			aiQuaternion::Interpolate(out, aiQuaternion(out), startRotationQ, weight);
		}
	}
}

void Mesh::SubMesh::calcInterpolatedScaling(aiVector3D& out, const AnimationState& anim, float weight, const aiNodeAnim* nodeAnim) const {
	if (weight <= 0.f) {
		return;
	}
	if (nodeAnim->mNumScalingKeys == 1) {
		out += nodeAnim->mScalingKeys[0].mValue * weight;
		return;
	}

	if (anim.getLength() > 1.f) {
		if (anim.isLoop() || anim.getTicks() < anim.getLength()) {
			float timeCur = anim.getBegin() + anim.getTicks();
			float timeNext = anim.isLoop() ?
				anim.getBegin() + fmod(anim.getTicks() + anim.getTicksRate(), anim.getLength()) :
				anim.getBegin() + anim.getTicks() + anim.getTicksRate();
			unsigned int indexCur = findScaling(timeCur, nodeAnim);
			unsigned int indexNext = timeCur == timeNext ? indexCur : findScaling(timeNext, nodeAnim);
			const aiVector3D& start = nodeAnim->mScalingKeys[indexCur].mValue;
			const aiVector3D& end = nodeAnim->mScalingKeys[indexNext].mValue;
			aiVector3D delta = end - start;
			out += (start + delta * 0.5f) * weight;
		} else {
			float timeEnd = anim.getEnd();
			unsigned int indexCur = findScaling(timeEnd, nodeAnim);
			const aiVector3D& end = nodeAnim->mScalingKeys[indexCur].mValue;
			out += end * weight;
		}
	} else {
		float timeBeg = anim.getBegin();
		unsigned int indexCur = findScaling(timeBeg, nodeAnim);
		const aiVector3D& start = nodeAnim->mScalingKeys[indexCur].mValue;
		out += start * weight;
	}
}

unsigned int Mesh::SubMesh::findPosition(float animationTime, const aiNodeAnim* nodeAnim) const {
	for (unsigned int i = 0; i < nodeAnim->mNumPositionKeys - 1; ++i) {
		if (animationTime < (float)nodeAnim->mPositionKeys[i + 1].mTime) {
			return i;
		}
	}
	return nodeAnim->mNumPositionKeys - 1;
}
unsigned int Mesh::SubMesh::findRotation(float animationTime, const aiNodeAnim* nodeAnim) const {
	assert(nodeAnim->mNumRotationKeys > 0);
	for (unsigned int i = 0; i < nodeAnim->mNumRotationKeys - 1; ++i) {
		if (animationTime < (float)nodeAnim->mRotationKeys[i + 1].mTime) {
			return i;
		}
	}
	return nodeAnim->mNumRotationKeys - 1;
}
unsigned int Mesh::SubMesh::findScaling(float animationTime, const aiNodeAnim* nodeAnim) const {
	assert(nodeAnim->mNumScalingKeys > 0);
	for (unsigned int i = 0; i < nodeAnim->mNumScalingKeys - 1; ++i) {
		if (animationTime < (float)nodeAnim->mScalingKeys[i + 1].mTime) {
			return i;
		}
	}
	return nodeAnim->mNumScalingKeys - 1;
}

Mesh::SubMesh::~SubMesh() {
	if (vbo[VERTEX_BUFFER]) {
		glDeleteBuffers(1, &vbo[VERTEX_BUFFER]);
	}

	if (vbo[TEXCOORD_BUFFER]) {
		glDeleteBuffers(1, &vbo[TEXCOORD_BUFFER]);
	}

	if (vbo[NORMAL_BUFFER]) {
		glDeleteBuffers(1, &vbo[NORMAL_BUFFER]);
	}

	if (vbo[COLOR_BUFFER]) {
		glDeleteBuffers(1, &vbo[COLOR_BUFFER]);
	}

	if (vbo[BONE_BUFFER]) {
		glDeleteBuffers(1, &vbo[BONE_BUFFER]);
	}

	if (vbo[TANGENT_BUFFER]) {
		glDeleteBuffers(1, &vbo[TANGENT_BUFFER]);
	}

	if (vbo[INDEX_BUFFER]) {
		glDeleteBuffers(1, &vbo[INDEX_BUFFER]);
	}

	glDeleteVertexArrays(1, &vao);

	bones.clear();

	if (vertices)
		delete[] vertices;
	if (texCoords)
		delete[] texCoords;
	if (normals)
		delete[] normals;
	if (colors)
		delete[] colors;
	if (indices)
		delete[] indices;
	if (tangents)
		delete[] tangents;
	if (vertexbonedata)
		delete[] vertexbonedata;
}

void Mesh::SubMesh::draw(const Camera& camera) {
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES_ADJACENCY, elementCount, GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);
}
