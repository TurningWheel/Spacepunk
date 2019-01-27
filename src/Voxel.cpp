// Voxel.cpp

#include "Voxel.hpp"
#include "LinkedList.hpp"

// voxel structure
typedef struct voxel_t
{
	~voxel_t() {
		delete data;
	}
	Sint32 sizex, sizey, sizez;
	Uint8* data;
	Uint8 palette[256][3];
} voxel_t;

// vertex structure
typedef struct vertex_t
{
	float x, y, z;
} vertex_t;

// quad structure
typedef struct polyquad_t
{
	vertex_t vertex[4];
	Uint8 r, g, b;
	int side;
} polyquad_t;

// triangle structure
typedef struct polytriangle_t
{
	vertex_t vertex[3];
	vertex_t normal[3];
	Uint8 r, g, b;
} polytriangle_t;

// polymodel structure
typedef struct polymodel_t
{
	polytriangle_t* faces;
	Uint32 numfaces;
	GLuint vbo;
	GLuint colors;
	GLuint va;
} polymodel_t;

VoxelMeshData generateVBOs(polymodel_t* model)
{
	VoxelMeshData result(model->numfaces);

	for ( Uint32 i = 0; i < model->numfaces; i++ )
	{
		const polytriangle_t *face = &model->faces[i];
		for (int vert_index = 0; vert_index < 3; vert_index++)
		{
			int data_index = i * 9 + vert_index * 3;
			const vertex_t *vert = &face->vertex[vert_index];
			const vertex_t *norm = &face->normal[vert_index];

			result.positions[data_index] = vert->x;
			result.positions[data_index + 1] = -vert->z;
			result.positions[data_index + 2] = vert->y;

			result.colors[data_index] = face->r / 255.f;
			result.colors[data_index + 1] = face->g / 255.f;
			result.colors[data_index + 2] = face->b / 255.f;

			result.normals[data_index] = norm->x;
			result.normals[data_index + 1] = -norm->z;
			result.normals[data_index + 2] = norm->y;
		}
	}

	return result;
}

VoxelMeshData generatePolyModel(voxel_t* model)
{
	Sint32 x, y, z;
	Uint32 i;
	Uint32 index, indexdown[3];
	Uint8 newcolor, oldcolor;
	bool buildingquad;
	polyquad_t* quad1, *quad2;
	Uint32 numquads;
	LinkedList<polyquad_t> quads;

	polymodel_t polymodel;

	numquads = 0;
	polymodel.numfaces = 0;
	indexdown[0] = model->sizez * model->sizey;
	indexdown[1] = model->sizez;
	indexdown[2] = 1;

	// find front faces
	for ( x = model->sizex - 1; x >= 0; x-- )
	{
		for ( z = 0; z < model->sizez; z++ )
		{
			oldcolor = 255;
			buildingquad = false;
			for ( y = 0; y < model->sizey; y++ )
			{
				index = z + y * model->sizez + x * model->sizey * model->sizez;
				newcolor = model->data[index];
				if ( buildingquad == true )
				{
					bool doit = false;
					if ( newcolor != oldcolor )
					{
						doit = true;
					}
					else if ( x < model->sizex - 1 )
						if ( model->data[index + indexdown[0]] >= 0 && model->data[index + indexdown[0]] < 255 )
						{
							doit = true;
						}
					if ( doit )
					{
						// add the last two vertices to the previous quad
						buildingquad = false;

						Node<polyquad_t>* currentNode = quads.getLast();
						quad1 = &currentNode->getData();
						quad1->vertex[1].x = x - model->sizex / 2.f + 1;
						quad1->vertex[1].y = y - model->sizey / 2.f;
						quad1->vertex[1].z = z - model->sizez / 2.f - 1;
						quad1->vertex[2].x = x - model->sizex / 2.f + 1;
						quad1->vertex[2].y = y - model->sizey / 2.f;
						quad1->vertex[2].z = z - model->sizez / 2.f;

						// optimize quad
						Node<polyquad_t>* node;
						for ( i = 0, node = quads.getFirst(); i < numquads - 1; i++, node = node->getNext() )
						{
							quad2 = &node->getData();
							if ( quad1->side == quad2->side )
							{
								if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
								{
									if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
									{
										if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
										{
											quad2->vertex[2].z++;
											quad2->vertex[3].z++;
											quads.removeNode(currentNode);
											numquads--;
											polymodel.numfaces -= 2;
											break;
										}
									}
								}
							}
						}
					}
				}
				if ( newcolor != oldcolor || !buildingquad )
				{
					if ( newcolor != 255 )
					{
						bool doit = false;
						if ( x == model->sizex - 1 )
						{
							doit = true;
						}
						else if ( model->data[index + indexdown[0]] == 255 )
						{
							doit = true;
						}
						if ( doit )
						{
							// start building a quad
							buildingquad = true;
							numquads++;
							polymodel.numfaces += 2;

							polyquad_t newQuad;
							quad1 = &newQuad;

							quad1->side = 0;
							quad1->vertex[0].x = x - model->sizex / 2.f + 1;
							quad1->vertex[0].y = y - model->sizey / 2.f;
							quad1->vertex[0].z = z - model->sizez / 2.f - 1;
							quad1->vertex[3].x = x - model->sizex / 2.f + 1;
							quad1->vertex[3].y = y - model->sizey / 2.f;
							quad1->vertex[3].z = z - model->sizez / 2.f;
							quad1->r = model->palette[model->data[index]][0];
							quad1->g = model->palette[model->data[index]][1];
							quad1->b = model->palette[model->data[index]][2];

							Node<polyquad_t>* newNode = quads.addNodeLast(newQuad);
						}
					}
				}
				oldcolor = newcolor;
			}
			if ( buildingquad == true )
			{
				// add the last two vertices to the previous quad
				buildingquad = false;

				Node<polyquad_t>* currentNode = quads.getLast();
				quad1 = &currentNode->getData();
				quad1->vertex[1].x = x - model->sizex / 2.f + 1;
				quad1->vertex[1].y = y - model->sizey / 2.f;
				quad1->vertex[1].z = z - model->sizez / 2.f - 1;
				quad1->vertex[2].x = x - model->sizex / 2.f + 1;
				quad1->vertex[2].y = y - model->sizey / 2.f;
				quad1->vertex[2].z = z - model->sizez / 2.f;

				// optimize quad
				Node<polyquad_t>* node;
				for ( i = 0, node = quads.getFirst(); i < numquads - 1; i++, node = node->getNext() )
				{
					quad2 = &node->getData();
					if ( quad1->side == quad2->side )
					{
						if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
						{
							if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
							{
								if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
								{
									quad2->vertex[2].z++;
									quad2->vertex[3].z++;
									quads.removeNode(currentNode);
									numquads--;
									polymodel.numfaces -= 2;
									break;
								}
							}
						}
					}
				}
			}
		}
	}

	// find back faces
	for ( x = 0; x < model->sizex; x++ )
	{
		for ( z = 0; z < model->sizez; z++ )
		{
			oldcolor = 255;
			buildingquad = false;
			for ( y = 0; y < model->sizey; y++ )
			{
				index = z + y * model->sizez + x * model->sizey * model->sizez;
				newcolor = model->data[index];
				if ( buildingquad == true )
				{
					bool doit = false;
					if ( newcolor != oldcolor )
					{
						doit = true;
					}
					else if ( x > 0 )
						if ( model->data[index - indexdown[0]] >= 0 && model->data[index - indexdown[0]] < 255 )
						{
							doit = true;
						}
					if ( doit )
					{
						// add the last two vertices to the previous quad
						buildingquad = false;

						Node<polyquad_t>* currentNode = quads.getLast();
						quad1 = &currentNode->getData();
						quad1->vertex[1].x = x - model->sizex / 2.f;
						quad1->vertex[1].y = y - model->sizey / 2.f;
						quad1->vertex[1].z = z - model->sizez / 2.f;
						quad1->vertex[2].x = x - model->sizex / 2.f;
						quad1->vertex[2].y = y - model->sizey / 2.f;
						quad1->vertex[2].z = z - model->sizez / 2.f - 1;

						// optimize quad
						Node<polyquad_t>* node;
						for ( i = 0, node = quads.getFirst(); i < numquads - 1; i++, node = node->getNext() )
						{
							quad2 = &node->getData();
							if ( quad1->side == quad2->side )
							{
								if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
								{
									if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
									{
										if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
										{
											quad2->vertex[0].z++;
											quad2->vertex[1].z++;
											quads.removeNode(currentNode);
											numquads--;
											polymodel.numfaces -= 2;
											break;
										}
									}
								}
							}
						}
					}
				}
				if ( newcolor != oldcolor || !buildingquad )
				{
					if ( newcolor != 255 )
					{
						bool doit = false;
						if ( x == 0 )
						{
							doit = true;
						}
						else if ( model->data[index - indexdown[0]] == 255 )
						{
							doit = true;
						}
						if ( doit )
						{
							// start building a quad
							buildingquad = true;
							numquads++;
							polymodel.numfaces += 2;

							polyquad_t newQuad;
							quad1 = &newQuad;

							quad1->side = 1;
							quad1->vertex[0].x = x - model->sizex / 2.f;
							quad1->vertex[0].y = y - model->sizey / 2.f;
							quad1->vertex[0].z = z - model->sizez / 2.f;
							quad1->vertex[3].x = x - model->sizex / 2.f;
							quad1->vertex[3].y = y - model->sizey / 2.f;
							quad1->vertex[3].z = z - model->sizez / 2.f - 1;
							quad1->r = model->palette[model->data[index]][0];
							quad1->g = model->palette[model->data[index]][1];
							quad1->b = model->palette[model->data[index]][2];

							Node<polyquad_t>* newNode = quads.addNodeLast(newQuad);
						}
					}
				}
				oldcolor = newcolor;
			}
			if ( buildingquad == true )
			{
				// add the last two vertices to the previous quad
				buildingquad = false;

				Node<polyquad_t>* currentNode = quads.getLast();
				quad1 = &currentNode->getData();
				quad1->vertex[1].x = x - model->sizex / 2.f;
				quad1->vertex[1].y = y - model->sizey / 2.f;
				quad1->vertex[1].z = z - model->sizez / 2.f;
				quad1->vertex[2].x = x - model->sizex / 2.f;
				quad1->vertex[2].y = y - model->sizey / 2.f;
				quad1->vertex[2].z = z - model->sizez / 2.f - 1;

				// optimize quad
				Node<polyquad_t>* node;
				for ( i = 0, node = quads.getFirst(); i < numquads - 1; i++, node = node->getNext() )
				{
					quad2 = &node->getData();
					if ( quad1->side == quad2->side )
					{
						if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
						{
							if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
							{
								if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
								{
									quad2->vertex[0].z++;
									quad2->vertex[1].z++;
									quads.removeNode(currentNode);
									numquads--;
									polymodel.numfaces -= 2;
									break;
								}
							}
						}
					}
				}
			}
		}
	}

	// find right faces
	for ( y = model->sizey - 1; y >= 0; y-- )
	{
		for ( z = 0; z < model->sizez; z++ )
		{
			oldcolor = 255;
			buildingquad = false;
			for ( x = 0; x < model->sizex; x++ )
			{
				index = z + y * model->sizez + x * model->sizey * model->sizez;
				newcolor = model->data[index];
				if ( buildingquad == true )
				{
					bool doit = false;
					if ( newcolor != oldcolor )
					{
						doit = true;
					}
					else if ( y < model->sizey - 1 )
						if ( model->data[index + indexdown[1]] >= 0 && model->data[index + indexdown[1]] < 255 )
						{
							doit = true;
						}
					if ( doit )
					{
						// add the last two vertices to the previous quad
						buildingquad = false;

						Node<polyquad_t>* currentNode = quads.getLast();
						quad1 = &currentNode->getData();
						quad1->vertex[1].x = x - model->sizex / 2.f;
						quad1->vertex[1].y = y - model->sizey / 2.f + 1;
						quad1->vertex[1].z = z - model->sizez / 2.f;
						quad1->vertex[2].x = x - model->sizex / 2.f;
						quad1->vertex[2].y = y - model->sizey / 2.f + 1;
						quad1->vertex[2].z = z - model->sizez / 2.f - 1;

						// optimize quad
						Node<polyquad_t>* node;
						for ( i = 0, node = quads.getFirst(); i < numquads - 1; i++, node = node->getNext() )
						{
							quad2 = &node->getData();
							if ( quad1->side == quad2->side )
							{
								if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
								{
									if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
									{
										if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
										{
											quad2->vertex[0].z++;
											quad2->vertex[1].z++;
											quads.removeNode(currentNode);
											numquads--;
											polymodel.numfaces -= 2;
											break;
										}
									}
								}
							}
						}
					}
				}
				if ( newcolor != oldcolor || !buildingquad )
				{
					if ( newcolor != 255 )
					{
						bool doit = false;
						if ( y == model->sizey - 1 )
						{
							doit = true;
						}
						else if ( model->data[index + indexdown[1]] == 255 )
						{
							doit = true;
						}
						if ( doit )
						{
							// start building a quad
							buildingquad = true;
							numquads++;
							polymodel.numfaces += 2;

							polyquad_t newQuad;
							quad1 = &newQuad;

							quad1->side = 2;
							quad1->vertex[0].x = x - model->sizex / 2.f;
							quad1->vertex[0].y = y - model->sizey / 2.f + 1;
							quad1->vertex[0].z = z - model->sizez / 2.f;
							quad1->vertex[3].x = x - model->sizex / 2.f;
							quad1->vertex[3].y = y - model->sizey / 2.f + 1;
							quad1->vertex[3].z = z - model->sizez / 2.f - 1;
							quad1->r = model->palette[model->data[index]][0];
							quad1->g = model->palette[model->data[index]][1];
							quad1->b = model->palette[model->data[index]][2];

							Node<polyquad_t>* newNode = quads.addNodeLast(newQuad);
						}
					}
				}
				oldcolor = newcolor;
			}
			if ( buildingquad == true )
			{
				// add the last two vertices to the previous quad
				buildingquad = false;
				Node<polyquad_t>* currentNode = quads.getLast();
				quad1 = &currentNode->getData();
				quad1->vertex[1].x = x - model->sizex / 2.f;
				quad1->vertex[1].y = y - model->sizey / 2.f + 1;
				quad1->vertex[1].z = z - model->sizez / 2.f;
				quad1->vertex[2].x = x - model->sizex / 2.f;
				quad1->vertex[2].y = y - model->sizey / 2.f + 1;
				quad1->vertex[2].z = z - model->sizez / 2.f - 1;

				// optimize quad
				Node<polyquad_t>* node;
				for ( i = 0, node = quads.getFirst(); i < numquads - 1; i++, node = node->getNext() )
				{
					quad2 = &node->getData();
					if ( quad1->side == quad2->side )
					{
						if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
						{
							if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
							{
								if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
								{
									quad2->vertex[0].z++;
									quad2->vertex[1].z++;
									quads.removeNode(currentNode);
									numquads--;
									polymodel.numfaces -= 2;
									break;
								}
							}
						}
					}
				}
			}
		}
	}

	// find left faces
	for ( y = 0; y < model->sizey; y++ )
	{
		for ( z = 0; z < model->sizez; z++ )
		{
			oldcolor = 255;
			buildingquad = false;
			for ( x = 0; x < model->sizex; x++ )
			{
				index = z + y * model->sizez + x * model->sizey * model->sizez;
				newcolor = model->data[index];
				if ( buildingquad == true )
				{
					bool doit = false;
					if ( newcolor != oldcolor )
					{
						doit = true;
					}
					else if ( y > 0 )
						if ( model->data[index - indexdown[1]] >= 0 && model->data[index - indexdown[1]] < 255 )
						{
							doit = true;
						}
					if ( doit )
					{
						// add the last two vertices to the previous quad
						buildingquad = false;

						Node<polyquad_t>* currentNode = quads.getLast();
						quad1 = &currentNode->getData();
						quad1->vertex[1].x = x - model->sizex / 2.f;
						quad1->vertex[1].y = y - model->sizey / 2.f;
						quad1->vertex[1].z = z - model->sizez / 2.f - 1;
						quad1->vertex[2].x = x - model->sizex / 2.f;
						quad1->vertex[2].y = y - model->sizey / 2.f;
						quad1->vertex[2].z = z - model->sizez / 2.f;

						// optimize quad
						Node<polyquad_t>* node;
						for ( i = 0, node = quads.getFirst(); i < numquads - 1; i++, node = node->getNext() )
						{
							quad2 = &node->getData();
							if ( quad1->side == quad2->side )
							{
								if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
								{
									if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
									{
										if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
										{
											quad2->vertex[2].z++;
											quad2->vertex[3].z++;
											quads.removeNode(currentNode);
											numquads--;
											polymodel.numfaces -= 2;
											break;
										}
									}
								}
							}
						}
					}
				}
				if ( newcolor != oldcolor || !buildingquad )
				{
					if ( newcolor != 255 )
					{
						bool doit = false;
						if ( y == 0 )
						{
							doit = true;
						}
						else if ( model->data[index - indexdown[1]] == 255 )
						{
							doit = true;
						}
						if ( doit )
						{
							// start building a quad
							buildingquad = true;
							numquads++;
							polymodel.numfaces += 2;

							polyquad_t newQuad;
							quad1 = &newQuad;

							quad1->side = 3;
							quad1->vertex[0].x = x - model->sizex / 2.f;
							quad1->vertex[0].y = y - model->sizey / 2.f;
							quad1->vertex[0].z = z - model->sizez / 2.f - 1;
							quad1->vertex[3].x = x - model->sizex / 2.f;
							quad1->vertex[3].y = y - model->sizey / 2.f;
							quad1->vertex[3].z = z - model->sizez / 2.f;
							quad1->r = model->palette[model->data[index]][0];
							quad1->g = model->palette[model->data[index]][1];
							quad1->b = model->palette[model->data[index]][2];

							Node<polyquad_t>* newNode = quads.addNodeLast(newQuad);
						}
					}
				}
				oldcolor = newcolor;
			}
			if ( buildingquad == true )
			{
				// add the last two vertices to the previous quad
				buildingquad = false;
				Node<polyquad_t>* currentNode = quads.getLast();
				quad1 = &currentNode->getData();
				quad1->vertex[1].x = x - model->sizex / 2.f;
				quad1->vertex[1].y = y - model->sizey / 2.f;
				quad1->vertex[1].z = z - model->sizez / 2.f - 1;
				quad1->vertex[2].x = x - model->sizex / 2.f;
				quad1->vertex[2].y = y - model->sizey / 2.f;
				quad1->vertex[2].z = z - model->sizez / 2.f;

				// optimize quad
				Node<polyquad_t>* node;
				for ( i = 0, node = quads.getFirst(); i < numquads - 1; i++, node = node->getNext() )
				{
					quad2 = &node->getData();
					if ( quad1->side == quad2->side )
					{
						if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
						{
							if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
							{
								if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
								{
									quad2->vertex[2].z++;
									quad2->vertex[3].z++;
									quads.removeNode(currentNode);
									numquads--;
									polymodel.numfaces -= 2;
									break;
								}
							}
						}
					}
				}
			}
		}
	}

	// find bottom faces
	for ( z = model->sizez - 1; z >= 0; z-- )
	{
		for ( y = 0; y < model->sizey; y++ )
		{
			oldcolor = 255;
			buildingquad = false;
			for ( x = 0; x < model->sizex; x++ )
			{
				index = z + y * model->sizez + x * model->sizey * model->sizez;
				newcolor = model->data[index];
				if ( buildingquad == true )
				{
					bool doit = false;
					if ( newcolor != oldcolor )
					{
						doit = true;
					}
					else if ( z < model->sizez - 1 )
						if ( model->data[index + indexdown[2]] >= 0 && model->data[index + indexdown[2]] < 255 )
						{
							doit = true;
						}
					if ( doit )
					{
						// add the last two vertices to the previous quad
						buildingquad = false;

						Node<polyquad_t>* currentNode = quads.getLast();
						quad1 = &currentNode->getData();
						quad1->vertex[1].x = x - model->sizex / 2.f;
						quad1->vertex[1].y = y - model->sizey / 2.f;
						quad1->vertex[1].z = z - model->sizez / 2.f;
						quad1->vertex[2].x = x - model->sizex / 2.f;
						quad1->vertex[2].y = y - model->sizey / 2.f + 1;
						quad1->vertex[2].z = z - model->sizez / 2.f;

						// optimize quad
						Node<polyquad_t>* node;
						for ( i = 0, node = quads.getFirst(); i < numquads - 1; i++, node = node->getNext() )
						{
							quad2 = &node->getData();
							if ( quad1->side == quad2->side )
							{
								if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
								{
									if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
									{
										if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
										{
											quad2->vertex[2].y++;
											quad2->vertex[3].y++;
											quads.removeNode(currentNode);
											numquads--;
											polymodel.numfaces -= 2;
											break;
										}
									}
								}
							}
						}
					}
				}
				if ( newcolor != oldcolor || !buildingquad )
				{
					if ( newcolor != 255 )
					{
						bool doit = false;
						if ( z == model->sizez - 1 )
						{
							doit = true;
						}
						else if ( model->data[index + indexdown[2]] == 255 )
						{
							doit = true;
						}
						if ( doit )
						{
							// start building a quad
							buildingquad = true;
							numquads++;
							polymodel.numfaces += 2;

							polyquad_t newQuad;
							quad1 = &newQuad;

							quad1->side = 4;
							quad1->vertex[0].x = x - model->sizex / 2.f;
							quad1->vertex[0].y = y - model->sizey / 2.f;
							quad1->vertex[0].z = z - model->sizez / 2.f;
							quad1->vertex[3].x = x - model->sizex / 2.f;
							quad1->vertex[3].y = y - model->sizey / 2.f + 1;
							quad1->vertex[3].z = z - model->sizez / 2.f;
							quad1->r = model->palette[model->data[index]][0];
							quad1->g = model->palette[model->data[index]][1];
							quad1->b = model->palette[model->data[index]][2];

							Node<polyquad_t>* newNode = quads.addNodeLast(newQuad);
						}
					}
				}
				oldcolor = newcolor;
			}
			if ( buildingquad == true )
			{
				// add the last two vertices to the previous quad
				buildingquad = false;

				Node<polyquad_t>* currentNode = quads.getLast();
				quad1 = &currentNode->getData();
				quad1->vertex[1].x = x - model->sizex / 2.f;
				quad1->vertex[1].y = y - model->sizey / 2.f;
				quad1->vertex[1].z = z - model->sizez / 2.f;
				quad1->vertex[2].x = x - model->sizex / 2.f;
				quad1->vertex[2].y = y - model->sizey / 2.f + 1;
				quad1->vertex[2].z = z - model->sizez / 2.f;

				// optimize quad
				Node<polyquad_t>* node;
				for ( i = 0, node = quads.getFirst(); i < numquads - 1; i++, node = node->getNext() )
				{
					quad2 = &node->getData();
					if ( quad1->side == quad2->side )
					{
						if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
						{
							if ( quad2->vertex[3].x == quad1->vertex[0].x && quad2->vertex[3].y == quad1->vertex[0].y && quad2->vertex[3].z == quad1->vertex[0].z )
							{
								if ( quad2->vertex[2].x == quad1->vertex[1].x && quad2->vertex[2].y == quad1->vertex[1].y && quad2->vertex[2].z == quad1->vertex[1].z )
								{
									quad2->vertex[2].y++;
									quad2->vertex[3].y++;
									quads.removeNode(currentNode);
									numquads--;
									polymodel.numfaces -= 2;
									break;
								}
							}
						}
					}
				}
			}
		}
	}

	// find top faces
	for ( z = 0; z < model->sizez; z++ )
	{
		for ( y = 0; y < model->sizey; y++ )
		{
			oldcolor = 255;
			buildingquad = false;
			for ( x = 0; x < model->sizex; x++ )
			{
				index = z + y * model->sizez + x * model->sizey * model->sizez;
				newcolor = model->data[index];
				if ( buildingquad == true )
				{
					bool doit = false;
					if ( newcolor != oldcolor )
					{
						doit = true;
					}
					else if ( z > 0 )
						if ( model->data[index - indexdown[2]] >= 0 && model->data[index - indexdown[2]] < 255 )
						{
							doit = true;
						}
					if ( doit )
					{
						// add the last two vertices to the previous quad
						buildingquad = false;

						Node<polyquad_t>* currentNode = quads.getLast();
						quad1 = &currentNode->getData();
						quad1->vertex[1].x = x - model->sizex / 2.f;
						quad1->vertex[1].y = y - model->sizey / 2.f + 1;
						quad1->vertex[1].z = z - model->sizez / 2.f - 1;
						quad1->vertex[2].x = x - model->sizex / 2.f;
						quad1->vertex[2].y = y - model->sizey / 2.f;
						quad1->vertex[2].z = z - model->sizez / 2.f - 1;

						// optimize quad
						Node<polyquad_t>* node;
						for ( i = 0, node = quads.getFirst(); i < numquads - 1; i++, node = node->getNext() )
						{
							quad2 = &node->getData();
							if ( quad1->side == quad2->side )
							{
								if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
								{
									if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
									{
										if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
										{
											quad2->vertex[0].y++;
											quad2->vertex[1].y++;
											quads.removeNode(currentNode);
											numquads--;
											polymodel.numfaces -= 2;
											break;
										}
									}
								}
							}
						}
					}
				}
				if ( newcolor != oldcolor || !buildingquad )
				{
					if ( newcolor != 255 )
					{
						bool doit = false;
						if ( z == 0 )
						{
							doit = true;
						}
						else if ( model->data[index - indexdown[2]] == 255 )
						{
							doit = true;
						}
						if ( doit )
						{
							// start building a quad
							buildingquad = true;
							numquads++;
							polymodel.numfaces += 2;

							polyquad_t newQuad;
							quad1 = &newQuad;

							quad1->side = 5;
							quad1->vertex[0].x = x - model->sizex / 2.f;
							quad1->vertex[0].y = y - model->sizey / 2.f + 1;
							quad1->vertex[0].z = z - model->sizez / 2.f - 1;
							quad1->vertex[3].x = x - model->sizex / 2.f;
							quad1->vertex[3].y = y - model->sizey / 2.f;
							quad1->vertex[3].z = z - model->sizez / 2.f - 1;
							quad1->r = model->palette[model->data[index]][0];
							quad1->g = model->palette[model->data[index]][1];
							quad1->b = model->palette[model->data[index]][2];

							Node<polyquad_t>* newNode = quads.addNodeLast(newQuad);
						}
					}
				}
				oldcolor = newcolor;
			}
			if ( buildingquad == true )
			{
				// add the last two vertices to the previous quad
				buildingquad = false;

				Node<polyquad_t>* currentNode = quads.getLast();
				quad1 = &currentNode->getData();
				quad1->vertex[1].x = x - model->sizex / 2.f;
				quad1->vertex[1].y = y - model->sizey / 2.f + 1;
				quad1->vertex[1].z = z - model->sizez / 2.f - 1;
				quad1->vertex[2].x = x - model->sizex / 2.f;
				quad1->vertex[2].y = y - model->sizey / 2.f;
				quad1->vertex[2].z = z - model->sizez / 2.f - 1;

				// optimize quad
				Node<polyquad_t>* node;
				for ( i = 0, node = quads.getFirst(); i < numquads - 1; i++, node = node->getNext() )
				{
					quad2 = &node->getData();
					if ( quad1->side == quad2->side )
					{
						if ( quad1->r == quad2->r && quad1->g == quad2->g && quad1->b == quad2->b )
						{
							if ( quad2->vertex[0].x == quad1->vertex[3].x && quad2->vertex[0].y == quad1->vertex[3].y && quad2->vertex[0].z == quad1->vertex[3].z )
							{
								if ( quad2->vertex[1].x == quad1->vertex[2].x && quad2->vertex[1].y == quad1->vertex[2].y && quad2->vertex[1].z == quad1->vertex[2].z )
								{
									quad2->vertex[0].y++;
									quad2->vertex[1].y++;
									quads.removeNode(currentNode);
									numquads--;
									polymodel.numfaces -= 2;
									break;
								}
							}
						}
					}
				}
			}
		}
	}

	// translate quads into triangles
	polymodel.faces = (polytriangle_t*) malloc(sizeof(polytriangle_t) * polymodel.numfaces);
	for ( i = 0; i < polymodel.numfaces; i++ )
	{
		Node<polyquad_t>* node = quads.nodeForIndex(i / 2);
		polyquad_t* quad = &node->getData();
		polymodel.faces[i].r = quad->r;
		polymodel.faces[i].g = quad->g;
		polymodel.faces[i].b = quad->b;
		if ( i % 2 )
		{
			polymodel.faces[i].vertex[0] = quad->vertex[0];
			polymodel.faces[i].vertex[1] = quad->vertex[1];
			polymodel.faces[i].vertex[2] = quad->vertex[2];
		}
		else
		{
			polymodel.faces[i].vertex[0] = quad->vertex[0];
			polymodel.faces[i].vertex[1] = quad->vertex[2];
			polymodel.faces[i].vertex[2] = quad->vertex[3];
		}

		for (size_t c = 0; c < 3; ++c) {
			polymodel.faces[i].normal[c].x = (quad->side == 0 ? 1.f : (quad->side == 1 ? -1.f : 0));
			polymodel.faces[i].normal[c].y = (quad->side == 2 ? 1.f : (quad->side == 3 ? -1.f : 0));
			polymodel.faces[i].normal[c].z = (quad->side == 4 ? 1.f : (quad->side == 5 ? -1.f : 0));
		}
	}

	// now store models into VBOs
	return generateVBOs(&polymodel);
}

voxel_t* loadVoxel(const char* filename)
{
	FILE* file;
	voxel_t* model;

	if (filename != NULL)
	{
		if ((file = fopen(filename, "rb")) == NULL)
		{
			return NULL;
		}
		model = new voxel_t();
		model->sizex = 0;
		fread(&model->sizex, sizeof(Sint32), 1, file);
		model->sizey = 0;
		fread(&model->sizey, sizeof(Sint32), 1, file);
		model->sizez = 0;
		fread(&model->sizez, sizeof(Sint32), 1, file);
		model->data = new Uint8[model->sizex * model->sizey * model->sizez];
		memset(model->data, 0, sizeof(Uint8)*model->sizex * model->sizey * model->sizez);
		fread(model->data, sizeof(Uint8), model->sizex * model->sizey * model->sizez, file);
		fread(&model->palette, sizeof(Uint8), 256 * 3, file);
		int c;
		for ( c = 0; c < 256; c++ )
		{
			model->palette[c][0] = model->palette[c][0] << 2;
			model->palette[c][1] = model->palette[c][1] << 2;
			model->palette[c][2] = model->palette[c][2] << 2;
		}
		fclose(file);

		return model;
	}
	else
	{
		return NULL;
	}
}

VoxelMeshData VoxelReader::readVoxel(const char* path) {
	voxel_t* voxelModel = loadVoxel(path);
	if (!voxelModel) {
		return false;
	}

	return generatePolyModel(voxelModel);
}

GLuint VoxelMeshData::findAdjacentIndex(GLuint index1, GLuint index2, GLuint index3) {
	for( size_t index = 0; index < indexCount; index += 6 ) {
		GLuint indices[3];
		indices[0] = this->indices[index    ];
		indices[1] = this->indices[index + 2];
		indices[2] = this->indices[index + 4];
		for( int edge = 0; edge < 3; ++edge ) {
			GLuint v1 = indices[edge]; // first edge index
			GLuint v2 = indices[(edge + 1) & 3]; // second edge index
			GLuint vOpp = indices[(edge + 2) & 3]; // index of opposite vertex

			// if the edge matches the search edge and the opposite vertex does not match
			if( ((v1 == index1 && v2 == index2) || (v2 == index1 && v1 == index2)) && vOpp != index3 ) {
				return vOpp; // we have found the adjacent vertex
			}
		}
	}

	// no opposite edge found
	return index3;
}