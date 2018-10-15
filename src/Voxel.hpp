// Voxel.hpp

#pragma once

#include "Main.hpp"

#include <memory>

class Mesh;

// this is the real meat
struct VoxelMeshData {
	VoxelMeshData(size_t numFaces) {
		vertexCount = 3 * numFaces;
		indexCount = vertexCount * 2;
		size = vertexCount * 3;
		positions.reset(new GLfloat[size]);
		colors.reset(new GLfloat[size]);
		normals.reset(new GLfloat[size]);
		indices.reset(new GLuint[indexCount]);
		for (size_t c = 0; c < vertexCount; ++c) {
			indices[c*2] = (GLuint)c;
		}
		for( size_t i = 0; i < indexCount; i += 6 ) {
			indices[i+1] = findAdjacentIndex(indices[i],indices[i+2],indices[i+4]);
			indices[i+3] = findAdjacentIndex(indices[i+2],indices[i+4],indices[i]);
			indices[i+5] = findAdjacentIndex(indices[i+4],indices[i],indices[i+2]);
		}
	}

	// 24bit positions, colors
	std::unique_ptr<GLfloat[]> positions;
	std::unique_ptr<GLfloat[]> colors;
	std::unique_ptr<GLfloat[]> normals;
	std::unique_ptr<GLuint[]> indices;
	size_t vertexCount;
	size_t indexCount;
	size_t size;

private:
	GLuint findAdjacentIndex(GLuint index1, GLuint index2, GLuint index3);
};

class VoxelReader {
public:
	// import a voxel model into a mesh
	// @param path full file path
	static VoxelMeshData readVoxel(const char* path);
};