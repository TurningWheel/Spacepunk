// Inventory.hpp

#pragma once

#include "ArrayList.hpp"

class Item;

class Inventory {
public:
	float nanoMatter;
	float bioMatter;
	float neuroThread;

	ArrayList<Item*> items;
};