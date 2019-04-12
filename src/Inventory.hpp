// Inventory.hpp

#pragma once

#include "Map.hpp"
#include "String.hpp"

class Item;

class Inventory {
public:
	float nanoMatter;
	float bioMatter;
	float neuroThread;

	class Slot {
		bool locked; // if the slot is locked
		Item* item;
	};

	Map<String, Slot*> items;
};