// Material.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Client.hpp"
#include "Renderer.hpp"
#include "Material.hpp"
#include "Image.hpp"
#include "Cubemap.hpp"

Material::Material(const char* _name) : Asset(_name), shader(_name) {
	path = mainEngine->buildPath(_name).get();

	FILE* fp = NULL;
	mainEngine->fmsg(Engine::MSG_DEBUG,"loading material '%s'...",_name);
	if( (fp=fopen(path.get(),"rb"))==NULL ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to load material '%s'",_name);
		return;
	}
	clearerr(fp);

	for( int line=1; !feof(fp); ++line ) {
		char buf[1024] = { 0 };
		if( fgets(buf,1024,fp)==NULL ) {
			break;
		}

		// skip empty lines
		if( buf[0] == '#' || buf[0]=='\n' || buf[0]=='\r' ) {
			continue;
		}

		// null terminate the end of the string
		size_t len = strlen(buf)-1;
		if( buf[len] == '\n' || buf[len] == '\r' ) {
			buf[len] = 0;
			if( len>=1 ) {
				if( buf[len-1] == '\n' || buf[len-1] == '\r' ) {
					buf[len-1] = 0;
				}
			}
		}

		// shader
		if( strncmp( buf, "shader = ", 9 )==0 ) {
			String filename((char *)(buf+9));
			shader.addShader(filename.get());

			continue;
		}

		// standard texture
		else if( strncmp( buf, "texture = ", 10 )==0 ) {
			String filename((char *)(buf+10));
			Image* image = mainEngine->getImageResource().dataForString(filename.get());
			if( image ) {
				stdTextures.addNodeLast(image);
			}

			continue;
		}

		// glow texture
		else if( strncmp( buf, "glow = ", 7 )==0 ) {
			String filename((char *)(buf+7));
			Image* image = mainEngine->getImageResource().dataForString(filename.get());
			if( image ) {
				glowTextures.addNodeLast(image);
			}

			continue;
		}

		// cubemap texture
		else if( strncmp( buf, "cubemap = ", 10 )==0 ) {
			String filename((char *)(buf+10));
			Cubemap* cubemap = mainEngine->getCubemapResource().dataForString(filename.get());
			if( cubemap ) {
				cubemaps.addNodeLast(cubemap);
			}

			continue;
		}
	}

	fclose(fp);

	shader.link();
	loaded = true;
}

Material::~Material() {
	stdTextures.removeAll();
	glowTextures.removeAll();
}

void Material::bindTextures(texturekind_t textureKind) {
	LinkedList<Image*>& images = (textureKind==STANDARD) ? stdTextures : glowTextures;
	unsigned int textureNum = 0;

	// bind normal textures
	if( images.getSize()==0 ) {
		Client* client = mainEngine->getLocalClient();
		if( !client ) {
			return;
		}
		Renderer* renderer = client->getRenderer();
		if( !renderer ) {
			return;
		} else if( !renderer->isInitialized() ) {
			return;
		}

		glUniform1i(shader.getUniformLocation("gTexture"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,renderer->getNullImage()->getTexID());
		++textureNum;
	} else if( images.getSize()==1 ) {
		glUniform1i(shader.getUniformLocation("gTexture"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,images.getFirst()->getData()->getTexID());
		++textureNum;
	} else if( images.getSize()>1 ) {
		Node<Image*>* node;
		for( node=images.getFirst(); node!=nullptr && textureNum < 32; node=node->getNext(), ++textureNum ) {
			Image* image = node->getData();

			char buf[32] = { 0 };
			snprintf(buf,32,"gTexture[%d]",textureNum);

			glUniform1i(shader.getUniformLocation(buf), textureNum);
			glActiveTexture(GL_TEXTURE0+textureNum);
			glBindTexture(GL_TEXTURE_2D, image->getTexID());
		}
	}

	// bind cubemap textures
	if( cubemaps.getSize() == 1 ) {
		glUniform1i(shader.getUniformLocation("gCubemap"), textureNum);
		glActiveTexture(GL_TEXTURE0 + std::min(textureNum, 31U));
		glBindTexture(GL_TEXTURE_CUBE_MAP,cubemaps.getFirst()->getData()->getTexID());
	} else if( cubemaps.getSize() > 1 ) {
		Node<Cubemap*>* node;
		for( node=cubemaps.getFirst(); node!=nullptr && textureNum < 32; node=node->getNext(), ++textureNum ) {
			Cubemap* cubemap = node->getData();

			char buf[32] = { 0 };
			snprintf(buf,32,"gCubemap[%d]",textureNum);

			glUniform1i(shader.getUniformLocation(buf), textureNum);
			glActiveTexture(GL_TEXTURE0+textureNum);
			glBindTexture(GL_TEXTURE_CUBE_MAP,cubemap->getTexID());
		}
	}
}