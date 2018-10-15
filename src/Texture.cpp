// Texture.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Texture.hpp"

const char* Texture::defaultTexture = "images/tile/null.txt";
const char* Texture::defaultDiffuse = "images/tile/diffuse/null.png";
const char* Texture::defaultNormal = "images/tile/normal/flat.png";
const char* Texture::defaultEffects = "images/tile/effects/empty.png";

Texture::Texture(const char* _name) : Asset(_name) {
	path = mainEngine->buildPath(_name).get();

	FILE* fp = NULL;
	mainEngine->fmsg(Engine::MSG_DEBUG,"loading texture '%s'...",_name);
	if( (fp=fopen(path.get(),"rb"))==NULL ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to load texture '%s'",_name);
		return;
	}
	clearerr(fp);

	// prepare for three images: diffuse, normal, effects
	textures.alloc(3);

	int index = 0;
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

		// load texture
		Image* image = mainEngine->getImageResource().dataForString(buf);
		if( image ) {
			textures.push(image);
			++index;
		} else {
			// fail case: load default texture. failing that, mark texture as failed to load
			switch( index ) {
				case 0:
					image = mainEngine->getImageResource().dataForString(defaultDiffuse);
					if( image ) {
						textures.push(image);
						++index;
					} else {
						mainEngine->fmsg(Engine::MSG_ERROR,"failed to load texture '%s': no diffuse texture available!",_name);
						fclose(fp);
						return;
					}
					break;
				case 1:
					image = mainEngine->getImageResource().dataForString(defaultDiffuse);
					if( image ) {
						textures.push(image);
						++index;
					} else {
						mainEngine->fmsg(Engine::MSG_ERROR,"failed to load texture '%s': no diffuse texture available!",_name);
						fclose(fp);
						return;
					}
					break;
				case 2:
					image = mainEngine->getImageResource().dataForString(defaultDiffuse);
					if( image ) {
						textures.push(image);
						++index;
					} else {
						mainEngine->fmsg(Engine::MSG_ERROR,"failed to load texture '%s': no diffuse texture available!",_name);
						fclose(fp);
						return;
					}
					break;
				default:
					mainEngine->fmsg(Engine::MSG_ERROR,"failed to load texture '%s': missing texture no. %d!",_name,index);
					fclose(fp);
					return;
			}
		}
	}
	fclose(fp);

	if( textures.getSize() < 3 ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to load texture '%s': 3 textures needed! (diffuse, normal, effects)",_name);
		return;
	}

	loaded = true;
}

Texture::~Texture() {
	textures.clear();
}