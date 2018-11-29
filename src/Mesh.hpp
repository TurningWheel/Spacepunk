// Mesh.hpp

#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>

#define GLM_FORCE_RADIANS
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Asset.hpp"
#include "LinkedList.hpp"
#include "ArrayList.hpp"
#include "Node.hpp"
#include "Vector.hpp"
#include "String.hpp"
#include "AnimationState.hpp"
#include "Map.hpp"
#include "Voxel.hpp"

class ShaderProgram;
class Camera;
class Material;

class Mesh : public Asset {
public:
	Mesh() {}
	Mesh(const char* _name);
	virtual ~Mesh();

	// skin cache
	struct skincache_t {
		ArrayList<glm::mat4> anims;
		ArrayList<glm::mat4> offsets;
	};

	// shader vars
	struct shadervars_t {
		GLboolean customColorEnabled = GL_FALSE;
		ArrayList<GLfloat> customColorR = { 1.f, 0.f, 0.f, 1.f };
		ArrayList<GLfloat> customColorG = { 0.f, 1.f, 0.f, 1.f };
		ArrayList<GLfloat> customColorB = { 0.f, 0.f, 1.f, 1.f };
		ArrayList<GLfloat> customColorA = { 1.f, 0.f, 0.f, 1.f };
		glm::vec4 highlightColor = { 1.f, 1.f, 0.f, 1.f };
		GLfloat lineWidth = 4.f;

		// save/load this object to a file
		// @param file interface to serialize with
		void serialize(FileInterface * file) {
			Uint32 version = 0;
			file->property("Mesh::version", version);

			bool customColorBool = customColorEnabled == GL_TRUE;
			file->property("customColorEnabled", customColorBool);
			customColorEnabled = customColorBool ? GL_TRUE : GL_FALSE;

			file->property("customColorR", customColorR);
			file->property("customColorG", customColorG);
			file->property("customColorB", customColorB);
			file->property("customColorA", customColorA);
		}
	};

	// determines whether any of the mesh entries have animations or not
	// @return true if the mesh has animations, false otherwise
	const bool hasAnimations() const;

	// loads the appropriate shader to draw the mesh
	// @param component: the component that contains the mesh
	// @param camera: the camera object to render the scene with
	// @param light: the light object to light the scene with, or nullptr for no light source
	// @param material: path to the material asset used to render the mesh
	// @return the ShaderProgram object with the given name, or nullptr if no shader was loaded
	// @param matrix: model matrix
	ShaderProgram* loadShader(Component& component, Camera& camera, Light* light, Material* material, const shadervars_t& shaderVars, const glm::mat4& matrix);

	// draws the mesh without animating it
	// @param camera: the camera to render the mesh through
	// @param component: optional component tied to the mesh
	// @param shader: the shader program to draw the mesh with
	void draw( Camera& camera, const Component* component, ShaderProgram* shader );

	// draws the mesh
	// @param camera: the camera to render the mesh through
	// @param component: optional component tied to the mesh
	// @param skincache: skincache to render with
	// @param shader: the shader program to draw the mesh with
	void draw( Camera& camera, const Component* component, ArrayList<skincache_t>& skincache, ShaderProgram* shader );

	// skins the mesh
	// @param animations: animations to skin with
	// @param skincache: where to store resulting skin
	void skin( Map<AnimationState>& animations, ArrayList<skincache_t>& skincache );

	// find the bone with the given name
	// @param name: the name of the bone to search for
	// @return the index of the bone we are searching for, or UINT32_MAX if the bone could not be found
	unsigned int boneIndexForName( const char* name ) const;

	// submesh entry
	class SubMesh {
	public:
		enum buffer_t {
			VERTEX_BUFFER,
			TEXCOORD_BUFFER,
			NORMAL_BUFFER,
			COLOR_BUFFER,
			BONE_BUFFER,
			INDEX_BUFFER,
			TANGENT_BUFFER,
			BUFFER_TYPE_LENGTH
		};

		// max number of bones in a mesh
		static const int maxBones = 100;

		// max number of bone weight per vertex
		static const int numBonesPerVertex = 4;

		// the number of elements to be drawn
		unsigned int elementCount;

		SubMesh(const char* name, const VoxelMeshData& data);
		SubMesh(const char* name, const aiScene& _scene, aiMesh* mesh);
		~SubMesh();

		void draw(const Camera& camera);
		const Vector& getMaxBox() const { return maxBox; }
		const Vector& getMinBox() const { return minBox; }

		// subclass for vertex bones and weights
		class VertexBoneData {
			public:
				GLint ids[numBonesPerVertex];
				GLfloat weights[numBonesPerVertex];

				void addBoneData( unsigned int boneID, float weight );
		};

		// substruct for bone data
		struct boneinfo_t {
			glm::mat4 offset;
			String name;
		};

		unsigned int boneIndexForName( const char* name ) const;

		GLuint findAdjacentIndex(const aiMesh& mesh, GLuint index1, GLuint index2, GLuint index3);

		void boneTransform(Map<AnimationState>& animations, skincache_t& skin);
		void readNodeHierarchy(Map<AnimationState>& animations, skincache_t& skin, const aiNode* node, const glm::mat4& rootTransform);
		const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const char* str);

		void calcInterpolatedPosition(aiVector3D& out, Map<AnimationState>& animations, const aiNodeAnim* nodeAnim);
		void calcInterpolatedRotation(aiQuaternion& out, Map<AnimationState>& animations, const aiNodeAnim* nodeAnim);
		void calcInterpolatedScaling(aiVector3D& out, Map<AnimationState>& animations, const aiNodeAnim* nodeAnim);

		// getters & setters
		virtual const type_t				getType() const				{ return ASSET_MESH; }
		const unsigned int					getNumVertices() const		{ return numVertices; }
		const unsigned int					getNumIndices() const		{ return elementCount; }
		const unsigned int					getNumBones() const			{ return numBones; }
		const float*						getVertices() const			{ return vertices; }
		const float*						getTexCoords() const		{ return texCoords; }
		const float*						getNormals() const			{ return normals; }
		const float*						getColors() const			{ return colors; }
		const GLuint*						getIndices() const			{ return indices; }
		const ArrayList<boneinfo_t>&		getBones() const			{ return bones; }
		const aiNode*						getRootNode() const			{ return scene->mRootNode; }

	private:
		Map<unsigned int> boneMapping; // maps a bone name to its index
		ArrayList<boneinfo_t> bones;
    	unsigned int numBones = 0;
		unsigned int numVertices = 0;
		GLuint vao = 0;
		GLuint vbo[BUFFER_TYPE_LENGTH];
		const aiScene* scene = nullptr; // points to parent's aiScene object, DO NOT DELETE
		GLuint gBonesLocation[maxBones];
		Vector minBox, maxBox;

		// raw data
		float* vertices = nullptr;		// position:  3 floats per vertex
		float* texCoords = nullptr;		// texCoords: 2 floats per vertex
		float* normals = nullptr;		// normals:   3 floats per vertex
		float* colors = nullptr;		// colors:    4 floats per vertex
		float* tangents = nullptr;		// tangents:  3 floats per vertex
		GLuint* indices = nullptr;		// indices:   2 uints per vertex (first is vertex, second is adjacent vertex)
	};

	// getters & setters
	const LinkedList<Mesh::SubMesh*>&		getSubMeshes() const		{ return subMeshes; }
	const Vector&					getMinBox() const			{ return minBox; }
	const Vector&					getMaxBox() const			{ return maxBox; }
	float							getAnimLength() const;

private:
	Assimp::Importer* importer = nullptr;
	const aiScene* scene = nullptr;
	LinkedList<Mesh::SubMesh*> subMeshes;
	Vector minBox, maxBox;

	unsigned int numBones = 0;
	unsigned int numVertices = 0;
};
