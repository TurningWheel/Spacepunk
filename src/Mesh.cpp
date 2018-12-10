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
	mainEngine->fmsg(Engine::MSG_DEBUG,"loading mesh '%s'...",name.get());

	path = mainEngine->buildPath(_name).get();

	String extension = path.substr(path.length() - 4);
	if (extension == ".vox") {
		// load voxel mesh
		VoxelMeshData data = VoxelReader::readVoxel(path.get());
		Mesh::SubMesh* entry = new Mesh::SubMesh(name.get(), data);
		mainEngine->fmsg(Engine::MSG_DEBUG,"loaded voxel mesh: %d verts", entry->getNumVertices());
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
		flags |= aiProcess_FixInfacingNormals;
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
		if( !scene ) {
			mainEngine->fmsg(Engine::MSG_ERROR,"failed to load mesh '%s': %s",name.get(),importer->GetErrorString());
			return;
		} else {
			if( !scene->HasAnimations() ) {
				flags |= aiProcess_PreTransformVertices;
			}
			importer->ApplyPostProcessing(flags);
		}
		for( unsigned int i=0; i < scene->mNumMeshes; ++i ) {
			Mesh::SubMesh* entry = new Mesh::SubMesh(name.get(), *scene, scene->mMeshes[i]);
			minBox.x = min(minBox.x,entry->getMinBox().x);
			minBox.y = min(minBox.y,entry->getMinBox().y);
			minBox.z = min(minBox.z,entry->getMinBox().z);
			maxBox.x = max(maxBox.x,entry->getMaxBox().x);
			maxBox.y = max(maxBox.y,entry->getMaxBox().y);
			maxBox.z = max(maxBox.z,entry->getMaxBox().z);
			subMeshes.addNodeLast(entry);
			numBones += entry->getNumBones();
			numVertices += entry->getNumVertices();
			mainEngine->fmsg(Engine::MSG_DEBUG,"loaded submesh: %d verts, %d bones", entry->getNumVertices(), entry->getNumBones());
		}
	}
	mainEngine->fmsg(Engine::MSG_DEBUG,"loaded mesh '%s': %d entries, %d verts, %d bones", name.get(), subMeshes.getSize(), numVertices, numBones);

	loaded = true;
}

Mesh::~Mesh(void) {
	while( subMeshes.getFirst() ) {
		delete subMeshes.getFirst()->getData();
		subMeshes.removeNode(subMeshes.getFirst());
	}
	if( importer ) {
		if( scene ) {
			importer->FreeScene();
			scene = nullptr;
		}
		delete importer;
		importer = nullptr;
	}
}

unsigned int Mesh::boneIndexForName(const char* name) const {
	int result = 0;
	for( const Node<SubMesh*>* node = subMeshes.getFirst(); node != nullptr; node = node->getNext() ) {
		const SubMesh* entry = node->getData();

		int index = entry->boneIndexForName(name);
		if( index == UINT32_MAX ) {
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
	if( scene ) {
		if( scene->mNumAnimations > 0 && scene->mAnimations[0] ) {
			return scene->mAnimations[0]->mDuration;
		}
	}
	return 0.f;
}

ShaderProgram* Mesh::loadShader(const Component& component, Camera& camera, const ArrayList<Light*>& lights, Material* material, const Mesh::shadervars_t& shaderVars, const glm::mat4& matrix) {
	Client* client = mainEngine->getLocalClient();
	if( !client )
		return nullptr;
	Renderer* renderer = camera.getRenderer();
	if( !renderer )
		return nullptr;
	bool editor = false;
	if( mainEngine->isEditorRunning() && component.getEntity()->getWorld() ) {
		editor = component.getEntity()->getWorld()->isShowTools();
	}

	// don't highlight if lineWidth == 0
	if( camera.getDrawMode() == Camera::DRAW_SILHOUETTE ||
		camera.getDrawMode() == Camera::DRAW_TRIANGLES ) {
		if( shaderVars.lineWidth <= 0 ) {
			return nullptr;
		}
	}

	// get shader program
	Material* mat = nullptr;
	switch( camera.getDrawMode() ) {
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
			if( material == nullptr ) {
				mat = mainEngine->getMaterialResource().dataForString("shaders/actor/std.json");
			} else {
				mat = material;
			}
			break;
	}

	if( !mat ) {
		return nullptr;
	} else {
		ShaderProgram& shader = mat->getShader();
		if( &shader != ShaderProgram::getCurrentShader() ) {
			shader.mount();
		}

		// set line width
		if( shaderVars.lineWidth > 0 ) {
			glLineWidth(shaderVars.lineWidth);
		}

		// load highlight color into shader
		glUniform4fv(shader.getUniformLocation("gHighlightColor"), 1, glm::value_ptr(shaderVars.highlightColor));

		// load textures, if necessary
		unsigned int textureUnit = 0;
		if( camera.getDrawMode() == Camera::DRAW_STANDARD || camera.getDrawMode() == Camera::DRAW_DEPTHFAIL || camera.getDrawMode() == Camera::DRAW_GLOW ) {
			// load per-mesh shader vars
			glUniform1i(shader.getUniformLocation("gCustomColorEnabled"), shaderVars.customColorEnabled);
			if( shaderVars.customColorEnabled == GL_TRUE ) {
				glUniform4fv(shader.getUniformLocation("gCustomColorR"), 1, &shaderVars.customColorR[0]);
				glUniform4fv(shader.getUniformLocation("gCustomColorG"), 1, &shaderVars.customColorG[0]);
				glUniform4fv(shader.getUniformLocation("gCustomColorB"), 1, &shaderVars.customColorB[0]);
				glUniform4fv(shader.getUniformLocation("gCustomColorA"), 1, &shaderVars.customColorA[0]);
			}

			// bind textures
			if( mat ) {
				if( camera.getDrawMode() == Camera::DRAW_GLOW ) {
					textureUnit = mat->bindTextures(Material::GLOW);
				} else {
					textureUnit = mat->bindTextures(Material::STANDARD);
				}
			}
		}

		// load common shader vars
		if( camera.getDrawMode() == Camera::DRAW_DEPTH ) {

			// load model matrix into shader
			glUniformMatrix4fv(shader.getUniformLocation("gModel"), 1, GL_FALSE, glm::value_ptr(matrix));

			// load projection matrix into shader
			glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(camera.getProjViewMatrix()));
		} else if( camera.getDrawMode() == Camera::DRAW_SHADOW ) {

			// load model matrix into shader
			glUniformMatrix4fv(shader.getUniformLocation("gModel"), 1, GL_FALSE, glm::value_ptr(matrix));

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
		} else if( camera.getDrawMode() == Camera::DRAW_SILHOUETTE || camera.getDrawMode() == Camera::DRAW_TRIANGLES ) {

			// load model matrix into shader
			glUniformMatrix4fv(shader.getUniformLocation("gModel"), 1, GL_FALSE, glm::value_ptr(matrix));

			// load projection matrix into shader
			glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(camera.getProjViewMatrix()));

			// load camera position into shader
			glm::vec3 cameraPos( camera.getGlobalPos().x, -camera.getGlobalPos().z, camera.getGlobalPos().y );
			glUniform3fv(shader.getUniformLocation("gCameraPos"), 1, glm::value_ptr(cameraPos));

		} else if( camera.getDrawMode() == Camera::DRAW_STENCIL ) {

			// load model matrix into shader
			glUniformMatrix4fv(shader.getUniformLocation("gModel"), 1, GL_FALSE, glm::value_ptr(matrix));

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
		} else {

			// load model matrix into shader
			glUniformMatrix4fv(shader.getUniformLocation("gModel"), 1, GL_FALSE, glm::value_ptr(matrix));

			// load model normal matrix
			glm::mat4 normalMat = component.getGlobalMat();
			normalMat[3] = glm::vec4(0.f,0.f,0.f,1.f);
			glUniformMatrix4fv(shader.getUniformLocation("gNormalTransform"), 1, GL_FALSE, glm::value_ptr(normalMat));

			// load projection matrix into shader
			glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(camera.getProjViewMatrix()));
			glUniform2fv(shader.getUniformLocation("gClipPlanes"), 1, glm::value_ptr(glm::vec2(camera.getClipNear(), camera.getClipFar())));

			// load camera position into shader
			glm::vec3 cameraPos = glm::vec3( camera.getGlobalPos().x, -camera.getGlobalPos().z, camera.getGlobalPos().y );
			glUniform3fv(shader.getUniformLocation("gCameraPos"), 1, glm::value_ptr(cameraPos));

			// load light data into shader
			if( component.getEntity()->isFlag(Entity::flag_t::FLAG_FULLYLIT) || camera.getDrawMode() == Camera::DRAW_GLOW || camera.getDrawMode() == Camera::DRAW_DEPTHFAIL ) {
				glUniform1i(shader.getUniformLocation("gActiveLight"), GL_FALSE);
				glUniform1i(shader.getUniformLocation("gNumLights"), 0);
			} else {
				glUniform1i(shader.getUniformLocation("gActiveLight"), GL_TRUE);
				if( lights.getSize() ) {
					shader.uploadLights(camera, lights, maxLights, textureUnit);
				} else if( editor ) {
					glUniform3fv(shader.getUniformLocation("gLightPos[0]"), 1, glm::value_ptr(cameraPos));
					glUniform4fv(shader.getUniformLocation("gLightColor[0]"), 1, glm::value_ptr(glm::vec4(1,1,1,1)));
					glUniform1f(shader.getUniformLocation("gLightIntensity[0]"), 1);
					glUniform1f(shader.getUniformLocation("gLightRadius[0]"), 16384.f);
					glUniform3fv(shader.getUniformLocation("gLightScale[0]"), 1, glm::value_ptr(glm::vec3(1.f,1.f,1.f)));
					glUniform1i(shader.getUniformLocation("gLightShape[0]"), 0);
					glUniform1i(shader.getUniformLocation("gShadowmapEnabled[0]"), GL_FALSE);
					glUniform1i(shader.getUniformLocation("gNumLights"), 1);
				} else {
					glUniform1i(shader.getUniformLocation("gNumLights"), 0);
				}
			}
		}

		return &shader;
	}
}

void Mesh::skin( Map<AnimationState>& animations, ArrayList<skincache_t>& skincache ) {
	if( !hasAnimations() ) {
		return;
	}

	skincache.clear();
	skincache.resize(subMeshes.getSize());

	Uint32 index = 0;
	for( auto& entry : subMeshes ) {
		if( hasAnimations() ) {
			if( skincache[index].anims.empty() ) {
				entry->boneTransform(animations, skincache[index]);
			}
		}

		++index;
	}
}

static Cvar cvar_showBones("showbones", "displays bones in animated models as dots for debug purposes", "0");
static Cvar cvar_findBone("findbone", "used with showbones, displays only the bone with the given name", "");

void Mesh::draw( Camera& camera, const Component* component, ArrayList<skincache_t>& skincache, ShaderProgram* shader ) {
	if( skincache.getSize() < subMeshes.getSize() ) {
		skincache.resize(subMeshes.getSize());
	}

	Uint32 index = 0;
	for( auto& entry : subMeshes ) {
		if( shader ) {
			if( hasAnimations() ) {
				glUniform1i(shader->getUniformLocation("gAnimated"), GL_TRUE);

				unsigned int numBones = std::min( (unsigned int)skincache[index].anims.getSize(), (unsigned int)SubMesh::maxBones );

				for( unsigned int i=0; i<numBones; ++i ) {
					char name[128];
					snprintf(name,128,"gBones[%d]",i);
					glUniformMatrix4fv(shader->getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(skincache[index].anims[i]));

					// debug stuff
					if( cvar_showBones.toInt() && component ) {
						String findBone = cvar_findBone.toStr();
						if (findBone == "" || findBone == entry->getBones()[i].name) {
							glm::mat4 mat = component->getGlobalMat() * skincache[index].offsets[i];
							Vector pos0 = Vector( mat[3][0], mat[3][2], -mat[3][1] );
							Vector pos1 = pos0 + Vector( mat[0][0], mat[0][2], -mat[0][1] ).normal() * 10.f;
							Vector pos2 = pos0 + Vector( mat[1][0], mat[1][2], -mat[1][1] ).normal() * 10.f;
							Vector pos3 = pos0 + Vector( mat[2][0], mat[2][2], -mat[2][1] ).normal() * 10.f;
							camera.markPoint(pos0, glm::vec4(.5f, .5f, .5f, 1.f));
							camera.markLine(pos0, pos1, glm::vec4(.5f, 0.f, 0.f, 1.f));
							camera.markLine(pos0, pos2, glm::vec4(0.f, .5f, 0.f, 1.f));
							camera.markLine(pos0, pos3, glm::vec4(0.f, 0.f, .5f, 1.f));
						}
					}
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
	ArrayList<skincache_t> skincache;
	for( Uint32 c=0; c<subMeshes.getSize(); ++c ) {
		skincache.push(skincache_t());
	}
	draw(camera, component, skincache, shader);
}

Mesh::SubMesh::SubMesh(const char* name, const VoxelMeshData& data) {
	for( int i=0; i<BUFFER_TYPE_LENGTH; ++i ) {
		vbo[static_cast<buffer_t>(i)] = 0;
	}

	elementCount = (unsigned int)data.indexCount;
	numVertices = (unsigned int)data.vertexCount;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// positions
	{
		vertices = new float[numVertices * 3];
		memcpy(vertices, data.positions.get(), sizeof(float) * numVertices * 3);

		glGenBuffers(1, &vbo[VERTEX_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[VERTEX_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, data.size * sizeof(GLfloat), data.positions.get(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);
	}

	// colors
	{
		colors = new float[numVertices * 3];
		memcpy(colors, data.colors.get(), sizeof(float) * numVertices * 3);

		glGenBuffers(1, &vbo[COLOR_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[COLOR_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, data.size * sizeof(GLfloat), data.colors.get(), GL_STATIC_DRAW);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(1);
	}

	// normals
	{
		normals = new float[numVertices * 3];
		memcpy(normals, data.normals.get(), sizeof(float) * numVertices * 3);

		glGenBuffers(1, &vbo[NORMAL_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[NORMAL_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, data.size * sizeof(GLfloat), data.normals.get(), GL_STATIC_DRAW);

		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(2);
	}

	// indices
	{
		indices = new GLuint[elementCount];
		memcpy(indices, data.indices.get(), sizeof(GLuint) * elementCount);

		glGenBuffers(1, &vbo[INDEX_BUFFER]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[INDEX_BUFFER]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indexCount * sizeof(GLuint), data.indices.get(), GL_STATIC_DRAW);
	}

	glBindVertexArray(0);
}

Mesh::SubMesh::SubMesh(const char* name, const aiScene& _scene, aiMesh* mesh) {
	scene = &_scene;

	for( int i=0; i<BUFFER_TYPE_LENGTH; ++i ) {
		vbo[static_cast<buffer_t>(i)] = 0;
	}
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	numVertices = mesh->mNumVertices;
	elementCount = mesh->mNumFaces * 3 * 2;

	if( mesh->HasPositions() ) {
		if( vertices )
			delete[] vertices;
		vertices = new float[mesh->mNumVertices * 3];
		for( unsigned int i=0; i < mesh->mNumVertices; ++i ) {
			vertices[i * 3    ] = mesh->mVertices[i].x;
			vertices[i * 3 + 1] = mesh->mVertices[i].y;
			vertices[i * 3 + 2] = mesh->mVertices[i].z;

			if( i==0 ) {
				minBox.x = maxBox.x = vertices[i * 3    ];
				minBox.y = maxBox.y = vertices[i * 3 + 2];
				minBox.z = maxBox.z = vertices[i * 3 + 1];
			} else {
				minBox.x = min(minBox.x,(float)vertices[i * 3    ]);
				minBox.y = min(minBox.y,(float)vertices[i * 3 + 2]);
				minBox.z = min(minBox.z,(float)vertices[i * 3 + 1]);
				maxBox.x = max(maxBox.x,(float)vertices[i * 3    ]);
				maxBox.y = max(maxBox.y,(float)vertices[i * 3 + 2]);
				maxBox.z = max(maxBox.z,(float)vertices[i * 3 + 1]);
			}
		}

		glGenBuffers(1, &vbo[VERTEX_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[VERTEX_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, 3 * mesh->mNumVertices * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);
	}

	if( mesh->HasTextureCoords(0) ) {
		if( texCoords )
			delete[] texCoords;
		texCoords = new float[mesh->mNumVertices * 2];
		for( unsigned int i=0; i < mesh->mNumVertices; ++i ) {
			texCoords[i * 2] = mesh->mTextureCoords[0][i].x;
			texCoords[i * 2 + 1] = mesh->mTextureCoords[0][i].y;
		}

		glGenBuffers(1, &vbo[TEXCOORD_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[TEXCOORD_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, 2 * mesh->mNumVertices * sizeof(GLfloat), texCoords, GL_STATIC_DRAW);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(1);
	}

	if( mesh->HasNormals() ) {
		if( normals )
			delete[] normals;
		normals = new float[mesh->mNumVertices * 3];
		for( unsigned int i=0; i < mesh->mNumVertices; ++i ) {
			normals[i * 3] = mesh->mNormals[i].x;
			normals[i * 3 + 1] = mesh->mNormals[i].y;
			normals[i * 3 + 2] = mesh->mNormals[i].z;
		}

		glGenBuffers(1, &vbo[NORMAL_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[NORMAL_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, 3 * mesh->mNumVertices * sizeof(GLfloat), normals, GL_STATIC_DRAW);

		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(2);
	}

	if( mesh->HasVertexColors(0) ) {
		if( colors )
			delete[] colors;
		colors = new float[mesh->mNumVertices * 4];
		for( unsigned int i=0; i < mesh->mNumVertices; ++i ) {
			colors[i * 4] = mesh->mColors[0][i].r;
			colors[i * 4 + 1] = mesh->mColors[0][i].g;
			colors[i * 4 + 2] = mesh->mColors[0][i].b;
			colors[i * 4 + 3] = mesh->mColors[0][i].a;
		}

		glGenBuffers(1, &vbo[COLOR_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[COLOR_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, 4 * mesh->mNumVertices * sizeof(GLfloat), colors, GL_STATIC_DRAW);

		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(3);
	}

	if( mesh->HasBones() ) {
		VertexBoneData* vertexbonedata = new VertexBoneData[mesh->mNumVertices];
		for( unsigned int i=0; i < mesh->mNumVertices; ++i ) {
			for( unsigned int k=0; k < numBonesPerVertex; ++k ) {
				vertexbonedata[i].ids[k] = 0;
				vertexbonedata[i].weights[k] = 0.f;
			}
		}
		for( unsigned int i=0; i < mesh->mNumBones; ++i ) {
			const char* boneName = mesh->mBones[i]->mName.data;
			unsigned int boneIndex = 0;

			if( !boneMapping.exists(boneName) ) {
				boneIndex = numBones;
				++numBones;
				boneinfo_t bi;
				bi.name = boneName;
				bi.offset = glm::mat4();
				bones.push(bi);

				boneMapping.insert(boneName, boneIndex);
			} else {
				boneIndex = *boneMapping[boneName];
				*boneMapping[boneName] = boneIndex;
			}

			bones[boneIndex].offset = glm::transpose(glm::make_mat4(&mesh->mBones[i]->mOffsetMatrix.a1));

			for( unsigned int j=0; j < mesh->mBones[i]->mNumWeights; ++j ) {
				unsigned int id = mesh->mBones[i]->mWeights[j].mVertexId;
				float weight = mesh->mBones[i]->mWeights[j].mWeight;
				vertexbonedata[id].addBoneData(boneIndex,weight);
			}
		}

		glGenBuffers(1, &vbo[BONE_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[BONE_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexBoneData) * mesh->mNumVertices, vertexbonedata, GL_STATIC_DRAW);

		glVertexAttribIPointer(4, 4, GL_INT, sizeof(VertexBoneData), NULL);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)(sizeof(GLint)*4));
		glEnableVertexAttribArray(5);

		delete[] vertexbonedata;
	}

	if( mesh->HasTangentsAndBitangents() ) {
		if( tangents )
			delete[] tangents;
		tangents = new float[mesh->mNumVertices * 3];
		for( unsigned int i=0; i < mesh->mNumVertices; ++i ) {
			tangents[i * 3] = mesh->mTangents[i].x;
			tangents[i * 3 + 1] = mesh->mTangents[i].y;
			tangents[i * 3 + 2] = mesh->mTangents[i].z;
		}

		glGenBuffers(1, &vbo[TANGENT_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[TANGENT_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, 3 * mesh->mNumVertices * sizeof(GLfloat), tangents, GL_STATIC_DRAW);

		glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(6);
	}

	if( mesh->HasFaces() ) {
		if( indices )
			delete[] indices;
		indices = new GLuint[mesh->mNumFaces * 3 * 2];

		for( unsigned int i=0; i < mesh->mNumFaces; ++i ) {
			assert(mesh->mFaces[i].mNumIndices==3);
			indices[i*6]   = mesh->mFaces[i].mIndices[0];
			indices[i*6+2] = mesh->mFaces[i].mIndices[1];
			indices[i*6+4] = mesh->mFaces[i].mIndices[2];

			indices[i*6+1] = findAdjacentIndex( *mesh, indices[i*6], indices[i*6+2], indices[i*6+4] );
			indices[i*6+3] = findAdjacentIndex( *mesh, indices[i*6+2], indices[i*6+4], indices[i*6] );
			indices[i*6+5] = findAdjacentIndex( *mesh, indices[i*6+4], indices[i*6], indices[i*6+2] );
		}

		glGenBuffers(1, &vbo[INDEX_BUFFER]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[INDEX_BUFFER]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * 2 * mesh->mNumFaces * sizeof(GLuint), indices, GL_STATIC_DRAW);
	}

	glBindVertexArray(0);
}

unsigned int Mesh::SubMesh::boneIndexForName(const char* name) const {
	if( boneMapping.exists(name) ) {
		return *boneMapping.find(name);
	} else {
		return UINT32_MAX;
	}
}

GLuint Mesh::SubMesh::findAdjacentIndex(const aiMesh& mesh, GLuint index1, GLuint index2, GLuint index3) {
	for( unsigned int i=0; i<mesh.mNumFaces; ++i ) {
		unsigned int*& indices = mesh.mFaces[i].mIndices;
		for( int edge = 0; edge < 3; ++edge ) {
			unsigned int v1 = indices[edge]; // first edge index
			unsigned int v2 = indices[(edge + 1) % 3]; // second edge index
			unsigned int vOpp = indices[(edge + 2) % 3]; // index of opposite vertex

			// if the edge matches the search edge and the opposite vertex does not match
			if( ((v1 == index1 && v2 == index2) || (v2 == index1 && v1 == index2)) && vOpp != index3 ) {
				return vOpp; // we have found the adjacent vertex
			}
		}
	}

	// no opposite edge found
	return index3;
}

void Mesh::SubMesh::VertexBoneData::addBoneData(unsigned int boneID, float weight) {
    for( unsigned int i=0; i < numBonesPerVertex; ++i ) {
        if( weights[i] == 0.f ) {
            ids[i] = boneID;
            weights[i] = weight;
            return;
        }
    }

    // should never get here - more bones than we have space for
	mainEngine->fmsg(Engine::MSG_WARN,"mesh loaded with more than %d bones per vertex",numBonesPerVertex);
    assert(0);
}

void Mesh::SubMesh::boneTransform(Map<AnimationState>& animations, skincache_t& skin) {
	if( !scene || !scene->HasAnimations() )
		return;
	const glm::mat4 identity( 1.f );

	skin.anims.resize(numBones);
	skin.offsets.resize(numBones);
	readNodeHierarchy(animations, skin, scene->mRootNode, identity);
}

void Mesh::SubMesh::readNodeHierarchy(Map<AnimationState>& animations, skincache_t& skin, const aiNode* node, const glm::mat4& rootTransform) {
	aiMatrix4x4 nodeTransform = node->mTransformation;

	const char* nodeName = node->mName.data;
	const aiNodeAnim* nodeAnim = findNodeAnim(scene->mAnimations[0], nodeName);

	if( nodeAnim ) {
		// interpolate scaling and generate scaling transformation matrix
		aiVector3D scaling;
		calcInterpolatedScaling(scaling, animations, nodeAnim);
		aiMatrix4x4 scalingM;
		aiMatrix4x4::Scaling(scaling, scalingM);

		// interpolate rotation and generate rotation transformation matrix
		aiQuaternion rotationQ;
		calcInterpolatedRotation(rotationQ, animations, nodeAnim);
		aiMatrix4x4 rotationM(rotationQ.GetMatrix());

		// interpolate translation and generate translation transformation matrix
		aiVector3D translation;
		calcInterpolatedPosition(translation, animations, nodeAnim);
		aiMatrix4x4 translationM;
		aiMatrix4x4::Translation(translation,translationM);

		// combine the above transformations
		nodeTransform = translationM * rotationM * scalingM;
	}

	glm::mat4 glmTransform = glm::transpose(glm::make_mat4(&nodeTransform.a1));
	glm::mat4 globalTransform = rootTransform * glmTransform;

	unsigned int boneIndex = 0;
	unsigned int* boneIndexPtr = boneMapping[nodeName];
	if( !boneIndexPtr ) {
		boneIndex = numBones;
		skin.offsets.resize(numBones+1);
		skin.anims.resize(numBones+1);
		boneMapping.insert(nodeName, numBones);
		boneinfo_t bi;
		bi.name = nodeName;
		bi.offset = glm::mat4();
		bones.push(bi);
		++numBones;
	} else {
		boneIndex = *boneIndexPtr;
	}
	skin.offsets[boneIndex] = globalTransform;
	skin.anims[boneIndex] = globalTransform * bones[boneIndex].offset;

	for( unsigned int i=0; i < node->mNumChildren; ++i ) {
		readNodeHierarchy(animations, skin, node->mChildren[i], globalTransform);
	}
}

const aiNodeAnim* Mesh::SubMesh::findNodeAnim(const aiAnimation* animation, const char* str) {
	for( unsigned int i=0; i < animation->mNumChannels; ++i ) {
		const char* curStr = animation->mChannels[i]->mNodeName.data;

		if( strcmp(curStr,str)==0 ) {
			return animation->mChannels[i];
		}
	}

	return nullptr;
}

void Mesh::SubMesh::calcInterpolatedPosition(aiVector3D& out, Map<AnimationState>& animations, const aiNodeAnim* nodeAnim) {
	if( nodeAnim->mNumPositionKeys == 1 ) {
		out = nodeAnim->mPositionKeys[0].mValue;
		return;
	}

	for( auto& pair : animations ) {
		AnimationState& anim = pair.b;
		float weight = anim.getWeight(nodeAnim->mNodeName.data);
		if (weight <= 0.f) {
			continue;
		}

		if( anim.getLength() > 1.f ) {
			if( anim.isLoop() || anim.getTicks() < anim.getLength() ) {
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
}

void Mesh::SubMesh::calcInterpolatedRotation(aiQuaternion& out, Map<AnimationState>& animations, const aiNodeAnim* nodeAnim) {
	// we need at least two values to interpolate...
	if( nodeAnim->mNumRotationKeys == 1 ) {
		out = nodeAnim->mRotationKeys[0].mValue;
		return;
	}

	bool first = true;
	for( auto& pair : animations ) {
		AnimationState& anim = pair.b;
		float weight = anim.getWeight(nodeAnim->mNodeName.data);
		if (weight <= 0.f) {
			continue;
		}

		if( anim.getLength() > 1.f ) {
			if( anim.isLoop() || anim.getTicks() < anim.getLength() ) {
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
				} else {
					aiQuaternion::Interpolate(out, aiQuaternion(out), rotationQ, weight);
				}
			} else {
				float timeEnd = anim.getEnd();
				unsigned int indexCur = findRotation(timeEnd, nodeAnim);
				const aiQuaternion& endRotationQ = nodeAnim->mRotationKeys[indexCur].mValue;
				if (first) {
					out = endRotationQ;
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
			} else {
				aiQuaternion::Interpolate(out, aiQuaternion(out), startRotationQ, weight);
			}
		}
		first = false;
	}
	out = out.Normalize();
}

void Mesh::SubMesh::calcInterpolatedScaling(aiVector3D& out, Map<AnimationState>& animations, const aiNodeAnim* nodeAnim) {
	if( nodeAnim->mNumScalingKeys == 1 ) {
		out = nodeAnim->mScalingKeys[0].mValue;
		return;
	}

	for( auto& pair : animations ) {
		AnimationState& anim = pair.b;
		float weight = anim.getWeight(nodeAnim->mNodeName.data);
		if (weight <= 0.f) {
			continue;
		}

		if( anim.getLength() > 1.f ) {
			if( anim.isLoop() || anim.getTicks() < anim.getLength() ) {
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
}

unsigned int Mesh::SubMesh::findPosition(float animationTime, const aiNodeAnim* nodeAnim) {
	for( unsigned int i=0; i < nodeAnim->mNumPositionKeys - 1; ++i ) {
		if( animationTime < (float)nodeAnim->mPositionKeys[i + 1].mTime ) {
			return i;
		}
	}
	return nodeAnim->mNumPositionKeys - 1;
}
unsigned int Mesh::SubMesh::findRotation(float animationTime, const aiNodeAnim* nodeAnim) {
	assert(nodeAnim->mNumRotationKeys > 0);
	for( unsigned int i=0; i < nodeAnim->mNumRotationKeys - 1; ++i ) {
		if( animationTime < (float)nodeAnim->mRotationKeys[i + 1].mTime ) {
			return i;
		}
	}
	return nodeAnim->mNumRotationKeys - 1;
}
unsigned int Mesh::SubMesh::findScaling(float animationTime, const aiNodeAnim* nodeAnim) {
	assert(nodeAnim->mNumScalingKeys > 0);
	for( unsigned int i=0; i < nodeAnim->mNumScalingKeys - 1; ++i ) {
		if( animationTime < (float)nodeAnim->mScalingKeys[i + 1].mTime ) {
			return i;
		}
	}
	return nodeAnim->mNumScalingKeys - 1;
}

Mesh::SubMesh::~SubMesh() {
	if( vbo[VERTEX_BUFFER] ) {
		glDeleteBuffers(1, &vbo[VERTEX_BUFFER]);
	}

	if( vbo[TEXCOORD_BUFFER] ) {
		glDeleteBuffers(1, &vbo[TEXCOORD_BUFFER]);
	}

	if( vbo[NORMAL_BUFFER] ) {
		glDeleteBuffers(1, &vbo[NORMAL_BUFFER]);
	}

	if( vbo[COLOR_BUFFER] ) {
		glDeleteBuffers(1, &vbo[COLOR_BUFFER]);
	}

	if( vbo[BONE_BUFFER] ) {
		glDeleteBuffers(1, &vbo[BONE_BUFFER]);
	}

	if( vbo[INDEX_BUFFER] ) {
		glDeleteBuffers(1, &vbo[INDEX_BUFFER]);
	}

	glDeleteVertexArrays(1, &vao);

	bones.clear();

	if( vertices )
		delete[] vertices;
	if( texCoords )
		delete[] texCoords;
	if( normals )
		delete[] normals;
	if( colors )
		delete[] colors;
	if( indices )
		delete[] indices;
	if( tangents )
		delete[] tangents;
}

void Mesh::SubMesh::draw(const Camera& camera) {
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES_ADJACENCY, elementCount, GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);
}
