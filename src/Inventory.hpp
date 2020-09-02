//! @file Inventory.hpp

#pragma once

#include "Map.hpp"
#include "String.hpp"
#include "Rect.hpp"

class Item;
class Frame;

class Inventory {
public:
	Inventory() = default;
	Inventory(const Inventory&) = default;
	Inventory(Inventory&&) = default;
	~Inventory() = default;

	Inventory& operator=(const Inventory&) = default;
	Inventory& operator=(Inventory&&) = default;

	float nanoMatter;
	float bioMatter;
	float neuroThread;

	struct Slot {
		bool locked = false; //! if the slot is locked
		class Entity* entity = nullptr;

		void serialize(FileInterface * file);
	};

	Map<String, Slot*> items;

	//! save/load this object to a file
	//! @param file interface to serialize with
	void serialize(FileInterface * file);

	void setVisibility(bool visible);

	void setupInvSlotDisplay(String slotName, Frame* frame, Rect<int> frameSize, int xPos, int yPos);
};