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
	"texture",
	"framebuffer"
};

Asset::Asset() {
}

Asset::Asset(const char* _name) {
	assert(_name);
	name = _name;
}

Asset::~Asset() {
}

bool Asset::finalize() {
	return loaded = true;
}

void Asset::serialize(FileInterface * file) {
	mainEngine->fmsg(Engine::MSG_WARN, "serialize() called on unsupported asset '%s'", name.get());
}