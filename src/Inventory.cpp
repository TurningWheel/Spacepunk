#include "Inventory.hpp"

#include "Entity.hpp"
#include "Engine.hpp"
#include "Client.hpp"

void Inventory::serialize(FileInterface * file)
{
	file->property("nanoMatter", nanoMatter);
	file->property("bioMatter", bioMatter);
	file->property("neuroThread", neuroThread);
	items.serialize(file);
}

void Inventory::setVisibility(bool visible)
{
	Frame* inventory = (mainEngine->getLocalClient()->getGUI()->findFrame("inventory"));
	if (visible == false)
	{
		if (inventory != nullptr)
		{
			inventory->clear();
		}
	}
	else
	{
		if (inventory == nullptr)
		{
			Rect<int> invSize;
			invSize.h = 900;
			invSize.w = 1600;

			Client* client = mainEngine->getLocalClient();
			Frame* gui = client->getGUI();
			Frame* inventory = gui->addFrame("inventory");
			inventory->setSize(invSize);
			
		}

		// Setting sizes and adding each frame
		{
			Frame* head = inventory->addFrame("Head");
			Frame* suit = inventory->addFrame("Suit");
			Frame* gloves = inventory->addFrame("Gloves");
			Frame* boots = inventory->addFrame("Boots");
			Frame* back = inventory->addFrame("Back");
			Frame* rightHip = inventory->addFrame("RightHip");
			Frame* leftHip = inventory->addFrame("LeftHip");
			Frame* waist = inventory->addFrame("Waist");
			Frame* rightHand = inventory->addFrame("RightHand");
			Frame* leftHand = inventory->addFrame("LeftHand");

			Rect<int> frameSize;
			frameSize.w = 250;
			frameSize.h = 75;

			String slotName = "Head";
			Slot* slot = *items.find(slotName);
			head->addField(slotName, 20)->setText(slot->entity->getName());
			head->setSize(frameSize);
			head->setPos(200, 200);

			slotName = "Suit";
			slot = *items.find(slotName);
			suit->addField(slotName, 20)->setText(slot->entity->getName());
			suit->setSize(frameSize);
			suit->setPos(100, 300);

			slotName = "Gloves";
			slot = *items.find(slotName);
			gloves->addField(slotName, 20)->setText(slot->entity->getName());
			gloves->setSize(frameSize);
			gloves->setPos(350, 300);

			slotName = "Boots";
			slot = *items.find(slotName);
			boots->addField(slotName, 20)->setText(slot->entity->getName());
			boots->setSize(frameSize);
			boots->setPos(200, 400);

			slotName = "Back";
			slot = *items.find(slotName);
			back->addField(slotName, 20)->setText(slot->entity->getName());
			back->setSize(frameSize);
			back->setPos(1250, 200);

			slotName = "RightHip";
			slot = *items.find(slotName);
			rightHip->addField(slotName, 20)->setText(slot->entity->getName());
			rightHip->setSize(frameSize);
			rightHip->setPos(1300, 300);

			slotName = "LeftHip";
			slot = *items.find(slotName);
			leftHip->addField(slotName, 20)->setText(slot->entity->getName());
			leftHip->setSize(frameSize);
			leftHip->setPos(1200, 300);

			slotName = "Waist";
			slot = *items.find(slotName);
			waist->addField(slotName, 20)->setText(slot->entity->getName());
			waist->setSize(frameSize);
			waist->setPos(1250, 400);

			slotName = "RightHand";
			slot = *items.find(slotName);
			rightHand->addField(slotName, 20)->setText(slot->entity->getName());
			rightHand->setSize(frameSize);
			rightHand->setPos(200, 600);

			slotName = "LeftHand";
			slot = *items.find(slotName);
			leftHand->addField(slotName, 20)->setText(slot->entity->getName());
			leftHand->setSize(frameSize);
			leftHand->setPos(1250, 600);
		}
	}
}

void Inventory::Slot::serialize(FileInterface * file)
{
	file->property("locked", locked);
	file->propertyName("entities");
	if (file->isReading()) {
		Uint32 numEntities = 0;
		file->beginArray(numEntities);
		for (Uint32 index = 0; index < numEntities; ++index) {
			Entity* entity = new Entity(nullptr);
			file->value(*entity);

			// can only read one entity into our slot!
			if (this->entity) {
				delete entity;
			}
			this->entity = entity;
		}
		file->endArray();
	}
	else {
		if (entity)	{
			Uint32 one = 1;
			Uint32& oneref = one;
			file->beginArray(oneref);
			file->value(*entity);
			file->endArray();
		}
		else {
			Uint32 zero = 0;
			Uint32& zeroref = zero;
			file->beginArray(zero);
			file->endArray();
		}
		

	}
}
