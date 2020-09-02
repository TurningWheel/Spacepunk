//! @file Character.hpp

#pragma once

#include "Main.hpp"
#include "Component.hpp"

//! A Character is an Entity Component that essentially contains all the data for an RPG character.
class Character : public Component {
public:

	enum sex_t {
		SEX_MALE,
		SEX_FEMALE,
		SEX_NONE,
		SEX_MAX
	};
	static const char* sexStr[SEX_MAX];

	//General Defaults
	static const Sint32 DEFAULT_HP = 100;
	static const Sint32 DEFAULT_MP = 100;
	static const sex_t DEFAULT_SEX = SEX_NONE;
	static const Sint32 DEFAULT_LEVEL = 0;
	static const Sint32 DEFAULT_XP = 0;
	static const Sint32 DEFAULT_HUNGER = 1000;

	//Resource Defaults
	static const Sint32 DEFAULT_NANO_MATTER = 0;
	static const Sint32 DEFAULT_BIO_MATTER = 0;
	static const Sint32 DEFAULT_NEURO_THREAD = 0;
	static const Sint32 DEFAULT_GOLD = 0;

	//Attribute Defaults
	static const Sint32 DEFAULT_STRENGTH = 0;
	static const Sint32 DEFAULT_DEXTERITY = 0;
	static const Sint32 DEFAULT_INTELLIGENCE = 0;
	static const Sint32 DEFAULT_CONSTITUTION = 0;
	static const Sint32 DEFAULT_PERCEPTION = 0;
	static const Sint32 DEFAULT_CHARISMA = 0;
	static const Sint32 DEFAULT_LUCK = 0;

	Character() = delete;
	Character(Entity& entity, Component* parent);
	Character(const Character&) = delete;
	Character(Character&&) = delete;
	virtual ~Character();

	virtual void load(FILE* fp);

	//! save/load this object to a file
	//! @param file interface to serialize with
	virtual void serialize(FileInterface * file) override;

	//General getters
	virtual type_t getType() const override { return COMPONENT_CHARACTER; }
	Sint32		getHp() const { return hp; }
	Sint32		getMp() const { return mp; }
	sex_t		getSex() const { return sex; }
	Sint32		getLevel() const { return level; }
	Sint32		getXp() const { return xp; }
	Sint32		getHunger() const { return hunger; }

	//Resource getters
	Sint32		getNanoMatter() const { return nanoMatter; }
	Sint32		getBioMatter() const { return bioMatter; }
	Sint32		getNeuroThread() const { return neuroThread; }
	Sint32		getGold() const { return gold; }

	//Attribute getters
	Sint32		getStrength() const { return strength; }
	Sint32		getDexterity() const { return dexterity; }
	Sint32		getIntelligence() const { return intelligence; }
	Sint32		getConstitution() const { return constitution; }
	Sint32		getPerception() const { return perception; }
	Sint32		getCharisma() const { return charisma; }
	Sint32		getLuck() const { return luck; }

	//General setters
	void		setHp(Sint32 hp) { this->hp = hp; }
	void		setMp(Sint32 mp) { this->mp = mp; }
	void		setSex(sex_t sex) { this->sex = sex; }
	void		setLevel(Sint32 level) { this->level = level; }
	void		setXp(Sint32 xp) { this->xp = xp; }
	void		setHunger(Sint32 hunger) { this->hunger = hunger; }

	//Resource setters
	void		setNanoMatter(Sint32 nanoMatter) { this->nanoMatter = nanoMatter; }
	void		setBioMatter(Sint32 bioMatter) { this->bioMatter = bioMatter; }
	void		setNeuroThread(Sint32 neuroThread) { this->neuroThread = neuroThread; }
	void		setGold(Sint32 gold) { this->gold = gold; }

	//Attribute setters
	void		setStrength(Sint32 strength) { this->strength = strength; }
	void		setDexterity(Sint32 dexterity) { this->dexterity = dexterity; }
	void		setIntelligence(Sint32 intelligence) { this->intelligence = intelligence; }
	void		setConstitution(Sint32 constitution) { this->constitution = constitution; }
	void		setPerception(Sint32 perception) { this->perception = perception; }
	void		setCharisma(Sint32 charisma) { this->charisma = charisma; }
	void		setLuck(Sint32 luck) { this->luck = luck; }

	Character& operator=(const Character& src) {
		hp = src.hp;
		mp = src.mp;
		sex = src.sex;
		level = src.level;
		xp = src.xp;
		hunger = src.hunger;

		nanoMatter = src.nanoMatter;
		bioMatter = src.bioMatter;
		neuroThread = src.neuroThread;
		gold = src.gold;

		strength = src.strength;
		dexterity = src.dexterity;
		intelligence = src.intelligence;
		constitution = src.constitution;
		perception = src.perception;
		charisma = src.charisma;
		luck = src.luck;

		updateNeeded = true;
		return *this;
	}

	Character& operator=(Character&&) = delete;

private:
	//General
	Sint32 hp;
	Sint32 mp;
	sex_t sex;
	Sint32 level;
	Sint32 xp;
	Sint32 hunger;

	//Resources
	Sint32 nanoMatter;
	Sint32 bioMatter;
	Sint32 neuroThread;
	Sint32 gold;

	//Attributes
	Sint32 strength;
	Sint32 dexterity;
	Sint32 intelligence;
	Sint32 constitution;
	Sint32 perception;
	Sint32 charisma;
	Sint32 luck;
};
