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
	Frame* inventory = mainEngine->getLocalClient()->getGUI()->findFrame("inventory");
	if (visible == false)
	{
		if (inventory != nullptr)
		{
			inventory->clear();
		}
	} else
	{
		if (inventory == nullptr)
		{
			Rect<int> invSize;
			invSize.h = 900;
			invSize.w = 1600;

			Client* client = mainEngine->getLocalClient();
			Frame* gui = client->getGUI();
			inventory = gui->addFrame("inventory");
			inventory->setSize(invSize);

		}

		// Setting sizes and adding each frame
		{
			Frame* head = inventory->addFrame("Helmet");
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

			setupInvSlotDisplay("Helmet", head, frameSize, 175, 200);
			setupInvSlotDisplay("Suit", suit, frameSize, 50, 300);
			setupInvSlotDisplay("Gloves", gloves, frameSize, 300, 300);
			setupInvSlotDisplay("Boots", boots, frameSize, 175, 400);
			setupInvSlotDisplay("Back", back, frameSize, 1175, 200);
			setupInvSlotDisplay("RightHip", rightHip, frameSize, 1300, 300);
			setupInvSlotDisplay("Waist", waist, frameSize, 1175, 400);
			setupInvSlotDisplay("RightHand", rightHand, frameSize, 175, 600);
			setupInvSlotDisplay("LeftHand", leftHand, frameSize, 1175, 600);
		}
	}
}

void Inventory::setupInvSlotDisplay(String slotName, Frame* frame, Rect<int> frameSize, int xPos, int yPos)
{
	Slot* slot = *items.find(slotName);
	//frame->addField(slotName, 20)->setText(slot-> entity->getName().length() > 0 ? slot->entity->getName() : "");
	frame->setSize(frameSize);
	frame->setColor(glm::vec4(.25f, .25f, .25f, 1.f));
	frame->setPos(xPos, yPos);
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
	} else {
		if (entity) {
			Uint32 one = 1;
			Uint32& oneref = one;
			file->beginArray(oneref);
			file->value(*entity);
			file->endArray();
		} else {
			Uint32 zero = 0;
			Uint32& zeroref = zero;
			file->beginArray(zero);
			file->endArray();
		}


	}
}
