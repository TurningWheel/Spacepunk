// Item.cpp

#include "Entity.hpp"
#include "Item.hpp"
#include "Inventory.hpp"
#include "File.hpp"

void Item::serialize(FileInterface* file) {
	int version = 0;
	file->property("Item::version", version);
	file->property("icon", icon);
	file->property("weight", weight);
	file->property("value", value);
	file->property("throwable", throwable);
	file->property("throwLeadInTime", throwLeadInTime);
	file->property("throwRecovery", throwRecovery);
	file->property("throwDestroys", throwDestroys);
	file->property("takesDamage", takesDamage);
	file->property("health", health);
	file->property("damageImmunities", damageImmunities);
	file->property("detonates", detonates);
	file->property("detonationDamageType", detonationDamageType);
	file->property("detonationRadius", detonationRadius);
	file->property("detonationDamage", detonationDamage);
	file->property("detonationFalloff", detonationFalloff);
	file->property("slotEffects", slotEffects);
	file->property("distance", distance);
	file->property("spread", spread);
	file->property("radius", radius);
	file->property("animLeadSpeed", animLeadSpeed);
	file->property("animRecovSpeed", animRecovSpeed);
	file->property("spendCharges", spendCharges);
	file->property("currCharges", currCharges);
	file->property("maxCharges", maxCharges);
	file->property("rechargeRate", rechargeRate);
	file->property("currCooldown", currCooldown);
	file->property("maxCooldown", maxCooldown);
	file->property("slotRestrictions", slotRestrictions);
	//file->property("conditionalEffects", conditionalEffects);
	file->property("actions", actions);
}

void Item::InitInventory()
{
	Inventory.items.insert("Helmet", nullptr);
	Inventory.items.insert("Suit", nullptr);
	Inventory.items.insert("Gloves", nullptr);
	Inventory.items.insert("Boots", nullptr);
	Inventory.items.insert("Back", nullptr);
	Inventory.items.insert("RightHip", nullptr);
	Inventory.items.insert("LeftHip", nullptr);
	Inventory.items.insert("Waist", nullptr);
	Inventory.items.insert("RightHand", nullptr);
	Inventory.items.insert("LeftHand", nullptr);
}

void Item::depositItem(Entity* itemToDeposit, String invSlot)
{
	if (Inventory.items.find(invSlot))
	{
		Inventory::Slot slot;
		slot.entity = itemToDeposit;
		Inventory.items.insert(invSlot, &slot);
	}
}

bool Item::isSlotFilled(String invSlot)
{
	return getSlottedItem(invSlot) != nullptr;
}

Entity* Item::getSlottedItem(String invSlot)
{
	Inventory::Slot* slot = *Inventory.items.find(invSlot);
	return slot->entity;
}

void Item::Action::serialize(FileInterface* file) {
	int version = 0;
	file->property("Action::version", version);
	file->property("damage", damage);
	file->property("damageType", damageType);
	file->property("leadInTime", leadInTime);
	file->property("recoverTime", recoverTime);
	file->property("radius", radius);
	file->property("radiusFalloff", radiusFalloff);
	file->property("shootBullet", shootBullet);
	file->property("distance", distance);
	file->property("spread", spread);
	file->property("shootLaser", shootLaser);
	file->property("laserColor", laserColor);
	file->property("laserSize", laserSize);
	file->property("shootProjectile", shootProjectile);
	file->property("gravity", gravity);
	file->property("speed", speed);
	//file->property("projectile", projectile);
}

/*void Item::ConditionalEffect::serialize(FileInterface* file) {
	int version = 0;
	file->property("ConditionalEffect::version", version);
	file->property("effect", effect);
	file->property("ammoSpendRate", ammoSpendRate);
	file->property("ammoSpendUse", ammoSpendUse);
}*/