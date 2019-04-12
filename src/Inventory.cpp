#include "Inventory.hpp"

void Inventory::serialize(FileInterface * file)
{
	file->property("nanoMatter", nanoMatter);
	file->property("bioMatter", bioMatter);
	file->property("neuroThread", neuroThread);
	items.serialize(file);
}
