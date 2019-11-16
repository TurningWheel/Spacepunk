// Chunk.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "World.hpp"
#include "Chunk.hpp"
#include "Renderer.hpp"
#include "Console.hpp"

#include <unordered_set>

Chunk::Chunk() {
	for( int i=0; i<BUFFER_TYPE_LENGTH; ++i ) {
		vbo[static_cast<buffer_t>(i)] = 0;
	}
	for( int i=0; i<size*size; ++i ) {
		tiles[i] = nullptr;
	}
}

Chunk::~Chunk() {
	for( int i=VERTEX_BUFFER; i<BUFFER_TYPE_LENGTH; ++i ) {
		if( vbo[static_cast<buffer_t>(i)] ) {
			glDeleteBuffers(1,&vbo[static_cast<buffer_t>(i)]);
			vbo[static_cast<buffer_t>(i)] = 0;
		}
	}
	if( vao ) {
		glDeleteVertexArrays(1,&vao);
		vao = 0;
	}
	{
		Node<Entity*>* nextnode;
		Node<Entity*>* node = ePopulation.getFirst();
		for( ; node != nullptr; node = nextnode ) {
			Entity* entity = node->getData();
			nextnode = node->getNext();
			entity->clearChunkNode();
		}
	}
	{
		Node<Component*>* nextnode;
		Node<Component*>* node = cPopulation.getFirst();
		for( ; node != nullptr; node = nextnode ) {
			Component* component = node->getData();
			nextnode = node->getNext();
			component->clearChunkNode();
		}
	}
}

Uint32 Chunk::calculateVertices() const {
	Uint32 result = 0;
	for( int i=0; i<size*size; ++i ) {
		if( tiles[i] ) {
			result += tiles[i]->getNumVertices();
		}
	}
	return result;
}

Node<Entity*>* Chunk::addEPopulation(Entity* entity) {
	return ePopulation.addNodeLast(entity);
}

Node<Component*>* Chunk::addCPopulation(Component* component) {
	return cPopulation.addNodeLast(component);
}

void Chunk::buildBuffers() {
	numVertices = calculateVertices();
	numIndices = numVertices * 2;

	// clear buffers
	vertexBuffer.clear();
	diffuseMapBuffer.clear();
	normalMapBuffer.clear();
	effectsMapBuffer.clear();
	normalBuffer.clear();
	tangentBuffer.clear();
	indexBuffer.clear();
	vertexBuffer.alloc(numVertices);
	diffuseMapBuffer.alloc(numVertices);
	normalMapBuffer.alloc(numVertices);
	effectsMapBuffer.alloc(numVertices);
	normalBuffer.alloc(numVertices);
	tangentBuffer.alloc(numVertices);
	indexBuffer.alloc(numIndices);

	// fill buffers
	GLuint index=0;
	for( int j=0; j<size*size; ++j ) {
		if( !tiles[j] )
			continue;
		Tile& tile = *tiles[j];
		if( !tile.hasVolume() )
			continue;

		colorChannels[j][0].r = tile.getShaderVars().customColorR.getArray()[0];
		colorChannels[j][0].g = tile.getShaderVars().customColorR.getArray()[1];
		colorChannels[j][0].b = tile.getShaderVars().customColorR.getArray()[2];
		colorChannels[j][1].r = tile.getShaderVars().customColorG.getArray()[0];
		colorChannels[j][1].g = tile.getShaderVars().customColorG.getArray()[1];
		colorChannels[j][1].b = tile.getShaderVars().customColorG.getArray()[2];
		colorChannels[j][2].r = tile.getShaderVars().customColorB.getArray()[0];
		colorChannels[j][2].g = tile.getShaderVars().customColorB.getArray()[1];
		colorChannels[j][2].b = tile.getShaderVars().customColorB.getArray()[2];

		for( int i=0; i<10; ++i ) {
			LinkedList<Tile::vertex_t>* list = tile.intForVertices(i);
			if( !mainEngine->isEditorRunning() ) {
				switch( i ) {
				case 0:
				{
					tile.compileCeilingVertices();
					break;
				}
				case 1:
				{
					tile.compileFloorVertices();
					break;
				}
				case 2:
				{
					Tile* neighbor = tile.findNeighbor(Tile::SIDE_EAST);
					if( neighbor ) {
						tile.compileUpperVertices(*neighbor, Tile::SIDE_EAST);
					}
					break;
				}
				case 3:
				{
					Tile* neighbor = tile.findNeighbor(Tile::SIDE_SOUTH);
					if( neighbor ) {
						tile.compileUpperVertices(*neighbor, Tile::SIDE_SOUTH);
					}
					break;
				}
				case 4:
				{
					Tile* neighbor = tile.findNeighbor(Tile::SIDE_WEST);
					if( neighbor ) {
						tile.compileUpperVertices(*neighbor, Tile::SIDE_WEST);
					}
					break;
				}
				case 5:
				{
					Tile* neighbor = tile.findNeighbor(Tile::SIDE_NORTH);
					if( neighbor ) {
						tile.compileUpperVertices(*neighbor, Tile::SIDE_NORTH);
					}
					break;
				}
				case 6:
				{
					Tile* neighbor = tile.findNeighbor(Tile::SIDE_EAST);
					if( neighbor ) {
						tile.compileLowerVertices(*neighbor, Tile::SIDE_EAST);
					}
					break;
				}
				case 7:
				{
					Tile* neighbor = tile.findNeighbor(Tile::SIDE_SOUTH);
					if( neighbor ) {
						tile.compileLowerVertices(*neighbor, Tile::SIDE_SOUTH);
					}
					break;
				}
				case 8:
				{
					Tile* neighbor = tile.findNeighbor(Tile::SIDE_WEST);
					if( neighbor ) {
						tile.compileLowerVertices(*neighbor, Tile::SIDE_WEST);
					}
					break;
				}
				case 9:
				{
					Tile* neighbor = tile.findNeighbor(Tile::SIDE_NORTH);
					if( neighbor ) {
						tile.compileLowerVertices(*neighbor, Tile::SIDE_NORTH);
					}
					break;
				}
				}
			}

			Node<Tile::vertex_t>* node;
			for( node=list->getFirst(); node!=nullptr; node=node->getNext() ) {
				Tile::vertex_t& vert = node->getData();
				vertexBuffer.push(glm::vec3(vert.pos.x,-vert.pos.z,vert.pos.y));

				Tile* neighbor = nullptr;
				switch( i ) {
					case 0:
						// ceiling
						{
							glm::vec3 xy( (float)(size - (j / size)), (float)(j % size), 0.f );
							glm::vec3 wh( 1.f, 1.f, 1.f );
							GLfloat u = 1.0 - (vert.pos.x-tile.getX())/Tile::size;
							GLfloat v = (vert.pos.y-tile.getY())/Tile::size;

							const String& string = mainEngine->getTextureDictionary().getWords()[tile.getCeilingTexture()];
							Texture* texture = mainEngine->getTextureResource().dataForString(string.get());
							if( texture == nullptr ) {
								texture = mainEngine->getTextureResource().dataForString(Texture::defaultTexture);
							}
							assert(texture != nullptr);

							GLuint diffuse = mainEngine->getTileDiffuseTextures().indexForName(texture->getTextures()[0]->getName());
							glm::vec3 diffuseUV( u, v, diffuse );
							diffuseMapBuffer.push( diffuseUV*wh + xy );

							GLuint normal = mainEngine->getTileNormalTextures().indexForName(texture->getTextures()[1]->getName());
							glm::vec3 normalUV( u, v, normal );
							normalMapBuffer.push( normalUV*wh + xy );

							GLuint effects = mainEngine->getTileEffectsTextures().indexForName(texture->getTextures()[2]->getName());
							glm::vec3 effectsUV( u, v, effects );
							effectsMapBuffer.push( effectsUV*wh + xy );
						}
						if( tile.getCeilingSlopeSize()==0 ) {
							normalBuffer.push(glm::vec3(0,-1,0));
						} else {
							switch( tile.getCeilingSlopeSide() ) {
								case Tile::SIDE_EAST:
									normalBuffer.push(glm::normalize(glm::vec3(-1 * tile.getCeilingSlopeSize(), -1 * Tile::size, 0)));
									break;
								case Tile::SIDE_SOUTH:
									normalBuffer.push(glm::normalize(glm::vec3(0, -1 * Tile::size, -1 * tile.getCeilingSlopeSize())));
									break;
								case Tile::SIDE_WEST:
									normalBuffer.push(glm::normalize(glm::vec3(1 * tile.getCeilingSlopeSize(), -1 * Tile::size, 0)));
									break;
								case Tile::SIDE_NORTH:
									normalBuffer.push(glm::normalize(glm::vec3(0, -1 * Tile::size, 1 * tile.getCeilingSlopeSize())));
									break;
								default:
									normalBuffer.push(glm::vec3(0,-1,0));
									break;
							}
						}
						break;
					case 1:
						// floor
						{
							glm::vec3 xy( (float)(j / size), (float)(j % size), 0.f );
							glm::vec3 wh( 1.f, 1.f, 1.f );
							GLfloat u = (vert.pos.x-tile.getX())/Tile::size;
							GLfloat v = (vert.pos.y-tile.getY())/Tile::size;

							const String& string = mainEngine->getTextureDictionary().getWords()[tile.getFloorTexture()];
							Texture* texture = mainEngine->getTextureResource().dataForString(string.get());
							if( texture == nullptr ) {
								texture = mainEngine->getTextureResource().dataForString(Texture::defaultTexture);
							}
							assert(texture != nullptr);

							GLuint diffuse = mainEngine->getTileDiffuseTextures().indexForName(texture->getTextures()[0]->getName());
							glm::vec3 diffuseUV( u, v, diffuse );
							diffuseMapBuffer.push( diffuseUV*wh + xy );

							GLuint normal = mainEngine->getTileNormalTextures().indexForName(texture->getTextures()[1]->getName());
							glm::vec3 normalUV( u, v, normal );
							normalMapBuffer.push( normalUV*wh + xy );

							GLuint effects = mainEngine->getTileEffectsTextures().indexForName(texture->getTextures()[2]->getName());
							glm::vec3 effectsUV( u, v, effects );
							effectsMapBuffer.push( effectsUV*wh + xy );
						}
						if( tile.getFloorSlopeSize()==0 ) {
							normalBuffer.push(glm::vec3(0,1,0));
						} else {
							switch( tile.getFloorSlopeSide() ) {
								case Tile::SIDE_EAST:
									normalBuffer.push(glm::normalize(glm::vec3(1 * tile.getFloorSlopeSize(), 1 * Tile::size, 0)));
									break;
								case Tile::SIDE_SOUTH:
									normalBuffer.push(glm::normalize(glm::vec3(0, 1 * Tile::size, 1 * tile.getFloorSlopeSize())));
									break;
								case Tile::SIDE_WEST:
									normalBuffer.push(glm::normalize(glm::vec3(-1 * tile.getFloorSlopeSize(), 1 * Tile::size, 0)));
									break;
								case Tile::SIDE_NORTH:
									normalBuffer.push(glm::normalize(glm::vec3(0, 1 * Tile::size, -1 * tile.getFloorSlopeSize())));
									break;
								default:
									normalBuffer.push(glm::vec3(0,1,0));
									break;
							}
						}
						break;
					case 2:
						// upper wall east
						if( (neighbor = tile.findNeighbor(Tile::SIDE_EAST)) == nullptr ) {
							continue;
						}
						{
							glm::vec3 xy( (float)(j % size), 0.f, 0.f );
							glm::vec3 wh( 1.f, 1.f, 1.f );
							GLfloat u = (vert.pos.y-tile.getY())/Tile::size;
							GLfloat v = vert.pos.z/Tile::size;

							const String& string = mainEngine->getTextureDictionary().getWords()[tile.getUpperTexture(Tile::SIDE_EAST)];
							Texture* texture = mainEngine->getTextureResource().dataForString(string.get());
							if( texture == nullptr ) {
								texture = mainEngine->getTextureResource().dataForString(Texture::defaultTexture);
							}
							assert(texture != nullptr);

							GLuint diffuse = mainEngine->getTileDiffuseTextures().indexForName(texture->getTextures()[0]->getName());
							glm::vec3 diffuseUV( u, v, diffuse );
							diffuseMapBuffer.push( diffuseUV*wh + xy );

							GLuint normal = mainEngine->getTileNormalTextures().indexForName(texture->getTextures()[1]->getName());
							glm::vec3 normalUV( u, v, normal );
							normalMapBuffer.push( normalUV*wh + xy );

							GLuint effects = mainEngine->getTileEffectsTextures().indexForName(texture->getTextures()[2]->getName());
							glm::vec3 effectsUV( u, v, effects );
							effectsMapBuffer.push( effectsUV*wh + xy );
						}
						normalBuffer.push(glm::vec3(-1,0,0));
						break;
					case 3:
						// upper wall south
						if( (neighbor = tile.findNeighbor(Tile::SIDE_SOUTH)) == nullptr ) {
							continue;
						}
						{
							glm::vec3 xy( (float)(size - (j / size)), 0.f, 0.f );
							glm::vec3 wh( 1.f, 1.f, 1.f );
							GLfloat u = (Tile::size-(vert.pos.x-tile.getX()))/Tile::size;
							GLfloat v = vert.pos.z/Tile::size;

							const String& string = mainEngine->getTextureDictionary().getWords()[tile.getUpperTexture(Tile::SIDE_SOUTH)];
							Texture* texture = mainEngine->getTextureResource().dataForString(string.get());
							if( texture == nullptr ) {
								texture = mainEngine->getTextureResource().dataForString(Texture::defaultTexture);
							}
							assert(texture != nullptr);

							GLuint diffuse = mainEngine->getTileDiffuseTextures().indexForName(texture->getTextures()[0]->getName());
							glm::vec3 diffuseUV( u, v, diffuse );
							diffuseMapBuffer.push( diffuseUV*wh + xy );

							GLuint normal = mainEngine->getTileNormalTextures().indexForName(texture->getTextures()[1]->getName());
							glm::vec3 normalUV( u, v, normal );
							normalMapBuffer.push( normalUV*wh + xy );

							GLuint effects = mainEngine->getTileEffectsTextures().indexForName(texture->getTextures()[2]->getName());
							glm::vec3 effectsUV( u, v, effects );
							effectsMapBuffer.push( effectsUV*wh + xy );
						}
						normalBuffer.push(glm::vec3(0,0,-1));
						break;
					case 4:
						// upper wall west
						if( (neighbor = tile.findNeighbor(Tile::SIDE_WEST)) == nullptr ) {
							continue;
						}
						{
							glm::vec3 xy( (float)(size - (j % size)), 0.f, 0.f );
							glm::vec3 wh( 1.f, 1.f, 1.f );
							GLfloat u = (Tile::size-(vert.pos.y-tile.getY()))/Tile::size;
							GLfloat v = vert.pos.z/Tile::size;

							const String& string = mainEngine->getTextureDictionary().getWords()[tile.getUpperTexture(Tile::SIDE_WEST)];
							Texture* texture = mainEngine->getTextureResource().dataForString(string.get());
							if( texture == nullptr ) {
								texture = mainEngine->getTextureResource().dataForString(Texture::defaultTexture);
							}
							assert(texture != nullptr);

							GLuint diffuse = mainEngine->getTileDiffuseTextures().indexForName(texture->getTextures()[0]->getName());
							glm::vec3 diffuseUV( u, v, diffuse );
							diffuseMapBuffer.push( diffuseUV*wh + xy );

							GLuint normal = mainEngine->getTileNormalTextures().indexForName(texture->getTextures()[1]->getName());
							glm::vec3 normalUV( u, v, normal );
							normalMapBuffer.push( normalUV*wh + xy );

							GLuint effects = mainEngine->getTileEffectsTextures().indexForName(texture->getTextures()[2]->getName());
							glm::vec3 effectsUV( u, v, effects );
							effectsMapBuffer.push( effectsUV*wh + xy );
						}
						normalBuffer.push(glm::vec3(1,0,0));
						break;
					case 5:
						// upper wall north
						if( (neighbor = tile.findNeighbor(Tile::SIDE_NORTH)) == nullptr ) {
							continue;
						}
						{
							glm::vec3 xy( (float)(j / size), 0.f, 0.f );
							glm::vec3 wh( 1.f, 1.f, 1.f );
							GLfloat u = (vert.pos.x-tile.getX())/Tile::size;
							GLfloat v = vert.pos.z/Tile::size;

							const String& string = mainEngine->getTextureDictionary().getWords()[tile.getUpperTexture(Tile::SIDE_NORTH)];
							Texture* texture = mainEngine->getTextureResource().dataForString(string.get());
							if( texture == nullptr ) {
								texture = mainEngine->getTextureResource().dataForString(Texture::defaultTexture);
							}
							assert(texture != nullptr);

							GLuint diffuse = mainEngine->getTileDiffuseTextures().indexForName(texture->getTextures()[0]->getName());
							glm::vec3 diffuseUV( u, v, diffuse );
							diffuseMapBuffer.push( diffuseUV*wh + xy );

							GLuint normal = mainEngine->getTileNormalTextures().indexForName(texture->getTextures()[1]->getName());
							glm::vec3 normalUV( u, v, normal );
							normalMapBuffer.push( normalUV*wh + xy );

							GLuint effects = mainEngine->getTileEffectsTextures().indexForName(texture->getTextures()[2]->getName());
							glm::vec3 effectsUV( u, v, effects );
							effectsMapBuffer.push( effectsUV*wh + xy );
						}
						normalBuffer.push(glm::vec3(0,0,1));
						break;
					case 6:
						// lower wall east
						if( (neighbor = tile.findNeighbor(Tile::SIDE_EAST)) == nullptr ) {
							continue;
						}
						{
							glm::vec3 xy( (float)(j % size), 0.f, 0.f );
							glm::vec3 wh( 1.f, 1.f, 1.f );
							GLfloat u = (vert.pos.y-tile.getY())/Tile::size;
							GLfloat v = vert.pos.z/Tile::size;

							const String& string = mainEngine->getTextureDictionary().getWords()[tile.getLowerTexture(Tile::SIDE_EAST)];
							Texture* texture = mainEngine->getTextureResource().dataForString(string.get());
							if( texture == nullptr ) {
								texture = mainEngine->getTextureResource().dataForString(Texture::defaultTexture);
							}
							assert(texture != nullptr);

							GLuint diffuse = mainEngine->getTileDiffuseTextures().indexForName(texture->getTextures()[0]->getName());
							glm::vec3 diffuseUV( u, v, diffuse );
							diffuseMapBuffer.push( diffuseUV*wh + xy );

							GLuint normal = mainEngine->getTileNormalTextures().indexForName(texture->getTextures()[1]->getName());
							glm::vec3 normalUV( u, v, normal );
							normalMapBuffer.push( normalUV*wh + xy );

							GLuint effects = mainEngine->getTileEffectsTextures().indexForName(texture->getTextures()[2]->getName());
							glm::vec3 effectsUV( u, v, effects );
							effectsMapBuffer.push( effectsUV*wh + xy );
						}
						normalBuffer.push(glm::vec3(-1,0,0));
						break;
					case 7:
						// lower wall south
						if( (neighbor = tile.findNeighbor(Tile::SIDE_SOUTH)) == nullptr ) {
							continue;
						}
						{
							glm::vec3 xy( (float)(size - (j / size)), 0.f, 0.f );
							glm::vec3 wh( 1.f, 1.f, 1.f );
							GLfloat u = (Tile::size-(vert.pos.x-tile.getX()))/Tile::size;
							GLfloat v = vert.pos.z/Tile::size;

							const String& string = mainEngine->getTextureDictionary().getWords()[tile.getLowerTexture(Tile::SIDE_SOUTH)];
							Texture* texture = mainEngine->getTextureResource().dataForString(string.get());
							if( texture == nullptr ) {
								texture = mainEngine->getTextureResource().dataForString(Texture::defaultTexture);
							}
							assert(texture != nullptr);

							GLuint diffuse = mainEngine->getTileDiffuseTextures().indexForName(texture->getTextures()[0]->getName());
							glm::vec3 diffuseUV( u, v, diffuse );
							diffuseMapBuffer.push( diffuseUV*wh + xy );

							GLuint normal = mainEngine->getTileNormalTextures().indexForName(texture->getTextures()[1]->getName());
							glm::vec3 normalUV( u, v, normal );
							normalMapBuffer.push( normalUV*wh + xy );

							GLuint effects = mainEngine->getTileEffectsTextures().indexForName(texture->getTextures()[2]->getName());
							glm::vec3 effectsUV( u, v, effects );
							effectsMapBuffer.push( effectsUV*wh + xy );
						}
						normalBuffer.push(glm::vec3(0,0,-1));
						break;
					case 8:
						// lower wall west
						if( (neighbor = tile.findNeighbor(Tile::SIDE_WEST)) == nullptr ) {
							continue;
						}
						{
							glm::vec3 xy( (float)(size - (j % size)), 0.f, 0.f );
							glm::vec3 wh( 1.f, 1.f, 1.f );
							GLfloat u = (Tile::size-(vert.pos.y-tile.getY()))/Tile::size;
							GLfloat v = vert.pos.z/Tile::size;

							const String& string = mainEngine->getTextureDictionary().getWords()[tile.getLowerTexture(Tile::SIDE_WEST)];
							Texture* texture = mainEngine->getTextureResource().dataForString(string.get());
							if( texture == nullptr ) {
								texture = mainEngine->getTextureResource().dataForString(Texture::defaultTexture);
							}
							assert(texture != nullptr);

							GLuint diffuse = mainEngine->getTileDiffuseTextures().indexForName(texture->getTextures()[0]->getName());
							glm::vec3 diffuseUV( u, v, diffuse );
							diffuseMapBuffer.push( diffuseUV*wh + xy );

							GLuint normal = mainEngine->getTileNormalTextures().indexForName(texture->getTextures()[1]->getName());
							glm::vec3 normalUV( u, v, normal );
							normalMapBuffer.push( normalUV*wh + xy );

							GLuint effects = mainEngine->getTileEffectsTextures().indexForName(texture->getTextures()[2]->getName());
							glm::vec3 effectsUV( u, v, effects );
							effectsMapBuffer.push( effectsUV*wh + xy );
						}
						normalBuffer.push(glm::vec3(1,0,0));
						break;
					case 9:
						// lower wall north
						if( (neighbor = tile.findNeighbor(Tile::SIDE_NORTH)) == nullptr ) {
							continue;
						}
						{
							glm::vec3 xy( (float)(j / size), 0.f, 0.f );
							glm::vec3 wh( 1.f, 1.f, 1.f );
							GLfloat u = (vert.pos.x-tile.getX())/Tile::size;
							GLfloat v = vert.pos.z/Tile::size;

							const String& string = mainEngine->getTextureDictionary().getWords()[tile.getLowerTexture(Tile::SIDE_NORTH)];
							Texture* texture = mainEngine->getTextureResource().dataForString(string.get());
							if( texture == nullptr ) {
								texture = mainEngine->getTextureResource().dataForString(Texture::defaultTexture);
							}
							assert(texture != nullptr);

							GLuint diffuse = mainEngine->getTileDiffuseTextures().indexForName(texture->getTextures()[0]->getName());
							glm::vec3 diffuseUV( u, v, diffuse );
							diffuseMapBuffer.push( diffuseUV*wh + xy );

							GLuint normal = mainEngine->getTileNormalTextures().indexForName(texture->getTextures()[1]->getName());
							glm::vec3 normalUV( u, v, normal );
							normalMapBuffer.push( normalUV*wh + xy );

							GLuint effects = mainEngine->getTileEffectsTextures().indexForName(texture->getTextures()[2]->getName());
							glm::vec3 effectsUV( u, v, effects );
							effectsMapBuffer.push( effectsUV*wh + xy );
						}
						normalBuffer.push(glm::vec3(0,0,1));
						break;
					default:
						assert(0);
						break;
				}

				indexBuffer.push(index);
				indexBuffer.push(index);
				index++;
			}
		}

		if( !mainEngine->isEditorRunning() ) {
			tile.cleanVertexBuffers();
		}
	}

	edges.clear();

	buildTangents();
	uploadBuffers();

	optimized = false;
}

void Chunk::buildTangents() {
	for( unsigned int i = 0; i < vertexBuffer.getSize(); i += 3 ) {
		glm::vec3& v0 = vertexBuffer[i+0];
		glm::vec3& v1 = vertexBuffer[i+1];
		glm::vec3& v2 = vertexBuffer[i+2];

		glm::vec2 uv0 = normalMapBuffer[i+0];
		glm::vec2 uv1 = normalMapBuffer[i+1];
		glm::vec2 uv2 = normalMapBuffer[i+2];

		// position delta
		glm::vec3 deltaPos0 = v1-v0;
		glm::vec3 deltaPos1 = v2-v0;

		// UV delta
		glm::vec2 deltaUV0 = uv1-uv0;
		glm::vec2 deltaUV1 = uv2-uv0;

		float r = 1.0f / (deltaUV0.x * deltaUV1.y - deltaUV0.y * deltaUV1.x);
		glm::vec3 tangent = (deltaPos0 * deltaUV1.y - deltaPos1 * deltaUV0.y)*r;
		tangentBuffer.push(tangent);
		tangentBuffer.push(tangent);
		tangentBuffer.push(tangent);
	}
}

void Chunk::optimizeBuffers() {
	if (optimized) {
		return;
	}
	if (!numVertices || !numIndices) {
		return;
	}
	edges.clear();

	// step 1: combine vertices
	combineVertices();

	// step 2: find edges
	/*EdgeList& minimal = edges;
	EdgeList all;
	findEdges(all, true);
	findEdges(minimal, false);

	// step 4: combine collinear edges
	combineEdges(all);
	combineEdges(minimal);

	// step 3: reduce edge set
	std::unordered_set<GLuint> minimalIndices;
	for( Uint32 c = 0; c < minimal.getSize(); ++c ) {
		minimalIndices.insert(minimal[c].a);
		minimalIndices.insert(minimal[c].b);
	}
	for( Uint32 c = 0; c < all.getSize(); ++c ) {
		if( minimalIndices.find(all[c].a) == minimalIndices.end() ||
			minimalIndices.find(all[c].b) == minimalIndices.end() ) {
			all.remove(c);
			--c;
		}
	}*/

	// step 5: find vertex adjacencies
	for( Uint32 i = 0; i < numIndices; i += 6 ) {
		indexBuffer[i+1] = findAdjacentIndex(indexBuffer[i],indexBuffer[i+2],indexBuffer[i+4]);
		indexBuffer[i+3] = findAdjacentIndex(indexBuffer[i+2],indexBuffer[i+4],indexBuffer[i]);
		indexBuffer[i+5] = findAdjacentIndex(indexBuffer[i+4],indexBuffer[i],indexBuffer[i+2]);
	}

	uploadBuffers();

	optimized = true;
}

void Chunk::combineVertices() {
	ArrayList<bool> conjoinedBool;
	conjoinedBool.resize(numVertices);
	for( Uint32 c = 0; c < conjoinedBool.getSize(); ++c ) {
		conjoinedBool[c] = false;
	}

	ArrayList<ArrayList<Uint32>> conjoinedMem;
	for( Uint32 i = 0; i < numVertices; ++i ) {
		if( conjoinedBool[i] ) {
			continue;
		}
		conjoinedMem.push(ArrayList<Uint32>());
		conjoinedMem.peek().push(i*2);
		for( Uint32 j = i + 1; j < numVertices; ++j ) {
			if( conjoinedBool[j] ) {
				continue;
			}
			if( vertexBuffer[i] == vertexBuffer[j] &&
				normalBuffer[i] == normalBuffer[j] &&
				diffuseMapBuffer[i] == diffuseMapBuffer[j] &&
				normalMapBuffer[i] == normalMapBuffer[j] &&
				effectsMapBuffer[i] == effectsMapBuffer[j] ) {
				conjoinedBool[j] = true;
				conjoinedMem.peek().push(j*2);
			}
		}
	}
	for( Uint32 j = 0, i = 0; i < numVertices; ++i, ++j ) {
		if( conjoinedBool[i] ) {
			vertexBuffer.removeAndRearrange(j);
			normalBuffer.removeAndRearrange(j);
			tangentBuffer.removeAndRearrange(j);
			diffuseMapBuffer.removeAndRearrange(j);
			normalMapBuffer.removeAndRearrange(j);
			effectsMapBuffer.removeAndRearrange(j);
			--j;
		}
	}
	numVertices = vertexBuffer.getSize();
	for( Uint32 index = 0; index < numIndices; index += 2 ) {
		bool found = false;
		for( Uint32 i = 0; i < conjoinedMem.getSize(); ++i ) {
			for( Uint32 j = 0; j < conjoinedMem[i].getSize(); ++j ) {
				if( conjoinedMem[i][j] == index ) {
					indexBuffer[index    ] = (GLuint)i;
					indexBuffer[index + 1] = (GLuint)i;
					found = true;
					break;
				}
				if( found ) {
					break;
				}
			}
			if( found ) {
				break;
			}
		}
		assert(found && "failed to find chunk index!");
	}
}

void Chunk::combineEdges(EdgeList& edges) {
	for( Uint32 i = 0; i < edges.getSize(); ++i ) {
		for( Uint32 j = 0; j < edges.getSize(); ++j ) {
			if( i == j ) {
				continue;
			}
			Edge& edge0 = edges[i];
			Edge& edge1 = edges[j];

			if( edge0.shares(edge1) ) {
				glm::vec3 edgeVec0 = glm::normalize(vertexBuffer[edge0.b] - vertexBuffer[edge0.a]);
				glm::vec3 edgeVec1 = glm::normalize(vertexBuffer[edge1.b] - vertexBuffer[edge1.a]);

				float dot = glm::dot(edgeVec0, edgeVec1);
				if( dot >= 0.999f ) {
					if( edge0.a == edge1.a ) {
						edge0.a = edge1.b;
					} else if( edge0.a == edge1.b ) {
						edge0.a = edge1.a;
					} else if( edge0.b == edge1.a ) {
						edge0.b = edge1.b;
					} else if( edge0.b == edge1.b ) {
						edge0.b = edge1.a;
					}
					edges.removeAndRearrange(j);
					if( j < i ) {
						--i;
					}
					--j;
				}
			}
		}
	}
}

void Chunk::findEdges(EdgeList& edges, bool all) {
	for( Uint32 index = 0; index < numIndices; index += 6 ) {
		findEdge(Edge(indexBuffer[index    ], indexBuffer[index + 2]), index, edges, all);
		findEdge(Edge(indexBuffer[index + 2], indexBuffer[index + 4]), index, edges, all);
		findEdge(Edge(indexBuffer[index + 4], indexBuffer[index    ]), index, edges, all);
	}
}

void Chunk::findEdge(const Edge& edge, Uint32 skip, EdgeList& edges, bool all) {
	bool foundMatch = false;
	for( Uint32 index = all ? skip + 6 : 0; index < numIndices; index += 6 ) {
		if( index == skip ) {
			continue;
		}
		Edge e0(indexBuffer[index    ], indexBuffer[index + 2]);
		Edge e1(indexBuffer[index + 2], indexBuffer[index + 4]);
		Edge e2(indexBuffer[index + 4], indexBuffer[index    ]);
		if( e0 == edge || e1 == edge || e2 == edge ) {
			foundMatch = true;
			break;
		}
	}
	if( !foundMatch ) {
		edges.push(edge);
	}
}

GLuint Chunk::findAdjacentIndex(GLuint index1, GLuint index2, GLuint index3) {
	GLuint indices[6];
	for (Uint32 index = 0; index < numIndices; index += 6) {
		indices[0] = indexBuffer[index];
		indices[1] = indexBuffer[index + 2];
		indices[2] = indexBuffer[index + 4];
		indices[3] = indexBuffer[index];
		indices[4] = indexBuffer[index + 2];
		indices[5] = indexBuffer[index + 4];
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

void Chunk::uploadBuffers() {
	if( vao ) {
		glDeleteVertexArrays(1,&vao);
		vao = 0;
	}
	if( !numVertices )
		return;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// upload vertex data
	if( vbo[VERTEX_BUFFER] ) {
		glDeleteBuffers(1,&vbo[VERTEX_BUFFER]);
		vbo[VERTEX_BUFFER] = 0;
	}
	glGenBuffers(1, &vbo[VERTEX_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[VERTEX_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec3), &vertexBuffer[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	// upload normal data
	if( vbo[NORMAL_BUFFER] ) {
		glDeleteBuffers(1,&vbo[NORMAL_BUFFER]);
		vbo[NORMAL_BUFFER] = 0;
	}
	glGenBuffers(1, &vbo[NORMAL_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[NORMAL_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec3), &normalBuffer[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);

	// upload tangent data
	if( vbo[TANGENT_BUFFER] ) {
		glDeleteBuffers(1,&vbo[TANGENT_BUFFER]);
		vbo[TANGENT_BUFFER] = 0;
	}
	glGenBuffers(1, &vbo[TANGENT_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[TANGENT_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec3), &tangentBuffer[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(2);

	// upload diffuse map data
	if( vbo[DIFFUSEMAP_BUFFER] ) {
		glDeleteBuffers(1,&vbo[DIFFUSEMAP_BUFFER]);
		vbo[DIFFUSEMAP_BUFFER] = 0;
	}
	glGenBuffers(1, &vbo[DIFFUSEMAP_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[DIFFUSEMAP_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec3), &diffuseMapBuffer[0], GL_STATIC_DRAW);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(3);

	// upload normal map data
	if( vbo[NORMALMAP_BUFFER] ) {
		glDeleteBuffers(1,&vbo[NORMALMAP_BUFFER]);
		vbo[NORMALMAP_BUFFER] = 0;
	}
	glGenBuffers(1, &vbo[NORMALMAP_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[NORMALMAP_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec3), &normalMapBuffer[0], GL_STATIC_DRAW);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(4);

	// upload effects map data
	if( vbo[EFFECTSMAP_BUFFER] ) {
		glDeleteBuffers(1,&vbo[EFFECTSMAP_BUFFER]);
		vbo[EFFECTSMAP_BUFFER] = 0;
	}
	glGenBuffers(1, &vbo[EFFECTSMAP_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[EFFECTSMAP_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec3), &effectsMapBuffer[0], GL_STATIC_DRAW);
	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(5);

	// upload index data
	if( vbo[INDEX_BUFFER] ) {
		glDeleteBuffers(1,&vbo[INDEX_BUFFER]);
		vbo[INDEX_BUFFER] = 0;
	}
	glGenBuffers(1, &vbo[INDEX_BUFFER]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(GLuint), &indexBuffer[0], GL_STATIC_DRAW);

	glBindVertexArray(0);
}

static Cvar cvar_showChunkEdges("showchunkedges", "show the computed edge list on chunks", "0");

void Chunk::draw(Camera& camera, ShaderProgram& shader) const {
	if( !numIndices )
		return;

	// upload colors
	if( camera.getDrawMode() == Camera::DRAW_STANDARD ||
		camera.getDrawMode() == Camera::DRAW_GLOW ) {
		glUniformMatrix3fv(shader.getUniformLocation("gTileColors"), size*size, GL_FALSE, glm::value_ptr(colorChannels[0]));
	}

	// draw elements
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES_ADJACENCY, (GLsizei)numIndices, GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);

	// draw edges
	if( cvar_showChunkEdges.toInt() ) {
		ShaderProgram* shader = const_cast<ShaderProgram*>(ShaderProgram::getCurrentShader()); // purely for debug
		for( Uint32 c = 0; c < edges.getSize(); ++c ) {
			glm::vec3 a = vertexBuffer[edges[c].a] - normalBuffer[edges[c].a];
			glm::vec3 b = vertexBuffer[edges[c].b] - normalBuffer[edges[c].b];
			glm::vec3 src(a.x, a.z, -a.y);
			glm::vec3 dst(b.x, b.z, -b.y);

			glm::vec3 n = normalBuffer[edges[c].a];
			glm::vec4 color(fabs(n.x), fabs(n.y), fabs(n.z), 1.f);
			camera.drawLine3D( 2.f, src, dst, color );
		}
		if( shader ) {
			shader->mount();
		}
	}
}
