// Asset.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Asset.hpp"
#include "File.hpp"

const char* Asset::typeStr[Asset::ASSET_NUM] = {
	"invalid",
	"animation",
	"cubemap",
	"directory",
	"image",
	"material",
	"mesh",
	"shader",
	"shaderprogram",
	"sound",
	"text",
	"font",
	"texture",
	"framebuffer"
};

Asset::Asset(const char* _name) {
	assert(_name);
	name = _name;
}

bool Asset::finalize() {
	return loaded = true;
}

bool Asset::valid(const char* name) {
	if (!name || name[0] == '\0') {
		return false;
	}
	if (name[0] == '#') {
		return true;
	}
	String path = mainEngine->buildPath(name).get();
	FILE* fp = nullptr;
	if ((fp = fopen(path.get(), "r")) != nullptr) {
		fclose(fp);
		return true;
	} else {
		return false;
	}
}

void Asset::serialize(FileInterface * file) {
	mainEngine->fmsg(Engine::MSG_WARN, "serialize() called on unsupported asset '%s'", name.get());
}