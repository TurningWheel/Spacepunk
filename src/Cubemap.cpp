// Cubemap.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Cubemap.hpp"

Cubemap::Cubemap(const char* _name) : Asset(_name) {
	path = mainEngine->buildPath(_name).get();

	FILE* fp = NULL;
	mainEngine->fmsg(Engine::MSG_DEBUG,"loading cubemap '%s'...",_name);
	if( (fp=fopen(path.get(),"rb"))==NULL ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to load cubemap '%s'",_name);
		return;
	}
	clearerr(fp);

	glGenTextures(1,&texid);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texid);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	
	int index = 0;
	for( int line = 1; !feof(fp) && index<6; ++line, ++index ) {
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

		// load image
		String path = mainEngine->buildPath(buf).get();
		if( (surfs[index]=IMG_Load(path.get())) == nullptr ) {
			mainEngine->fmsg(Engine::MSG_ERROR,"failed to load cubemap image '%s'",buf);
			fclose(fp);
			return;
		}

		// translate the original surface to an RGBA surface
		SDL_Surface* newSurf = SDL_CreateRGBSurface(0, surfs[index]->w, surfs[index]->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
		SDL_BlitSurface(surfs[index], nullptr, newSurf, nullptr); // blit onto a purely RGBA Surface
		SDL_FreeSurface(surfs[index]);
		surfs[index] = newSurf;

		// load the new surface as a GL texture
		SDL_LockSurface(surfs[index]);
		if (!index)
		{
			glTexStorage2D(GL_TEXTURE_CUBE_MAP, 4, GL_RGBA8, surfs[index]->w, surfs[index]->h);
		}
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, 0, 0, 0, surfs[index]->w, surfs[index]->h, GL_RGBA, GL_UNSIGNED_BYTE, surfs[index]->pixels);
		SDL_UnlockSurface(surfs[index]);
	}
	fclose(fp);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	if( index < 6 ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"failed to load cubemap '%s': not enough images! (must be six)",_name);
		return;
	}

	loaded = true;
}

Cubemap::~Cubemap() {
	for( int c = 0; c < 6; ++c ) {
		if( surfs[c] ) {
			SDL_FreeSurface(surfs[c]);
			surfs[c] = nullptr;
		}
	}
	if( texid ) {
		glDeleteTextures(1,&texid);
		texid = 0;
	}
}