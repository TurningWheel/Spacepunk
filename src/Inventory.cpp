#include "Inventory.hpp"
#include "Entity.hpp"

void Inventory::serialize(FileInterface * file)
{
	file->property("nanoMatter", nanoMatter);
	file->property("bioMatter", bioMatter);
	file->property("neuroThread", neuroThread);
	items.serialize(file);
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
