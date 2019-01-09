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

//These functions are not in the script.cpp file since they drastically increase compile time and memory usage due to heavy template usage.
void Script::exposeCharacter() {
	{
		auto characterType = lua.create_simple_usertype<Character>(sol::constructors<Character(Entity&, Component*)>(),
			sol::base_classes, sol::bases<Component>()
		);

		//Bind all of the editor's member functions.
		characterType.set("getHp", &Character::getHp);
		characterType.set("getMp", &Character::getMp);
		characterType.set("getSex", &Character::getSex);
		characterType.set("getLevel", &Character::getLevel);
		characterType.set("getXp", &Character::getXp);
		characterType.set("getHunger", &Character::getHunger);
		characterType.set("getNanoMatter", &Character::getNanoMatter);
		characterType.set("getBioMatter", &Character::getBioMatter);
		characterType.set("getNeuroThread", &Character::getNeuroThread);
		characterType.set("getGold", &Character::getGold);
		characterType.set("getStrength", &Character::getStrength);
		characterType.set("getDexterity", &Character::getDexterity);
		characterType.set("getIntelligence", &Character::getIntelligence);
		characterType.set("getConstitution", &Character::getConstitution);
		characterType.set("getPerception", &Character::getPerception);
		characterType.set("getCharisma", &Character::getCharisma);
		characterType.set("getLuck", &Character::getLuck);
		characterType.set("setHp", &Character::setHp);
		characterType.set("setMp", &Character::setMp);
		characterType.set("setSex", &Character::setSex);
		characterType.set("setLevel", &Character::setLevel);
		characterType.set("setXp", &Character::setXp);
		characterType.set("setHunger", &Character::setHunger);
		characterType.set("setNanoMatter", &Character::setNanoMatter);
		characterType.set("setBioMatter", &Character::setBioMatter);
		characterType.set("setNeuroThread", &Character::setNeuroThread);
		characterType.set("setGold", &Character::setGold);
		characterType.set("setStrength", &Character::setStrength);
		characterType.set("setDexterity", &Character::setDexterity);
		characterType.set("setIntelligence", &Character::setIntelligence);
		characterType.set("setConstitution", &Character::setConstitution);
		characterType.set("setPerception", &Character::setPerception);
		characterType.set("setCharisma", &Character::setCharisma);
		characterType.set("setLuck", &Character::setLuck);

		//Finally register the thing.
		lua.set_usertype("Character", characterType);
	}

	LinkedList<Character*>::exposeToScript(lua, "LinkedListCharacterPtr", "NodeCharacterPtr");
	ArrayList<Character*>::exposeToScript(lua, "ArrayListCharacterPtr");
}
