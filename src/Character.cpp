// Character.cpp

#include "Main.hpp"
#include "Character.hpp"
#include "Engine.hpp"

const char* Character::sexStr[SEX_MAX] = {
	"male",
	"female",
	"none"
};

Character::Character(Entity& entity, Component* parent) :
	Component(entity, parent) {

	name = typeStr[COMPONENT_CHARACTER];

	//General
	hp = DEFAULT_HP;
	mp = DEFAULT_MP;
	sex = DEFAULT_SEX;
	level = DEFAULT_LEVEL;
	xp = DEFAULT_XP;
	hunger = DEFAULT_HUNGER;

	//Resources
	nanoMatter = DEFAULT_NANO_MATTER;
	bioMatter = DEFAULT_BIO_MATTER;
	neuroThread = DEFAULT_NEURO_THREAD;
	gold = DEFAULT_GOLD;

	//Attributes
	strength = DEFAULT_STRENGTH;
	dexterity = DEFAULT_DEXTERITY;
	intelligence = DEFAULT_INTELLIGENCE;
	constitution = DEFAULT_CONSTITUTION;
	perception = DEFAULT_PERCEPTION;
	charisma = DEFAULT_CHARISMA;
	luck = DEFAULT_LUCK;

	// exposed attributes
	attributes.push(new AttributeInt("HP", hp));
	attributes.push(new AttributeInt("MP", mp));
	attributes.push(new AttributeEnum<sex_t>("Sex", sexStr, sex_t::SEX_MAX, sex));
	attributes.push(new AttributeInt("Level", level));
	attributes.push(new AttributeInt("XP", xp));
	attributes.push(new AttributeInt("Hunger", hunger));
	attributes.push(new AttributeInt("Nanomatter", nanoMatter));
	attributes.push(new AttributeInt("Biomatter", bioMatter));
	attributes.push(new AttributeInt("Neurothread", neuroThread));
	attributes.push(new AttributeInt("Gold", gold));
	attributes.push(new AttributeInt("Strength", strength));
	attributes.push(new AttributeInt("Dexterity", dexterity));
	attributes.push(new AttributeInt("Intelligence", intelligence));
	attributes.push(new AttributeInt("Constitution", constitution));
	attributes.push(new AttributeInt("Perception", perception));
	attributes.push(new AttributeInt("Charisma", charisma));
	attributes.push(new AttributeInt("Luck", luck));
}

Character::~Character() {
}

void Character::load(FILE* fp) {
	Component::load(fp);

	//General
	Uint32 reserved = 0;
	Engine::freadl(&reserved, sizeof(Uint32), 1, fp, nullptr, "Character::load()");

	Engine::freadl(&hp, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	Engine::freadl(&mp, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	Sint32 sexRead;
	Engine::freadl(&sexRead, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	sex = static_cast<sex_t>(sexRead);
	Engine::freadl(&level, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	Engine::freadl(&xp, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	Engine::freadl(&hunger, sizeof(Sint32), 1, fp, nullptr, "Character::load()");

	//Resources
	reserved = 0;
	Engine::freadl(&reserved, sizeof(Uint32), 1, fp, nullptr, "Character::load()");

	Engine::freadl(&nanoMatter, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	Engine::freadl(&bioMatter, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	Engine::freadl(&neuroThread, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	Engine::freadl(&gold, sizeof(Sint32), 1, fp, nullptr, "Character::load()");

	//Attributes
	reserved = 0;
	Engine::freadl(&reserved, sizeof(Uint32), 1, fp, nullptr, "Character::load()");

	Engine::freadl(&strength, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	Engine::freadl(&dexterity, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	Engine::freadl(&intelligence, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	Engine::freadl(&constitution, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	Engine::freadl(&perception, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	Engine::freadl(&charisma, sizeof(Sint32), 1, fp, nullptr, "Character::load()");
	Engine::freadl(&luck, sizeof(Sint32), 1, fp, nullptr, "Character::load()");

	reserved = 0;
	Engine::freadl(&reserved, sizeof(Uint32), 1, fp, nullptr, "Character::load()");

	loadSubComponents(fp);
}

void Character::serialize(FileInterface * file) {
	Component::serialize(file);

	Uint32 version = 0;
	file->property("Character::version", version);

	// base stats
	file->property("hp", hp);
	file->property("mp", mp);
	file->property("sex", sex);
	file->property("level", level);
	file->property("xp", xp);
	file->property("hunger", hunger);

	//Resources
	file->property("nanoMatter", nanoMatter);
	file->property("bioMatter", bioMatter);
	file->property("neuroThread", neuroThread);
	file->property("gold", gold);

	//Attributes
	file->property("strength", strength);
	file->property("dexterity", dexterity);
	file->property("intelligence", intelligence);
	file->property("constitution", constitution);
	file->property("perception", perception);
	file->property("charisma", charisma);
	file->property("luck", luck);
}