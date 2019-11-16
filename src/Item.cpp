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
	Inventory::Slot slotHelmet;
	Inventory::Slot slotSuit;
	Inventory::Slot slotGloves;
	Inventory::Slot slotBoots;
	Inventory::Slot slotBack;
	Inventory::Slot slotRightHip;
	Inventory::Slot slotLeftHip;
	Inventory::Slot slotWaist;
	Inventory::Slot slotRightHand;
	Inventory::Slot slotLeftHand;

	inventory.items.insert("Helmet", &slotHelmet);
	inventory.items.insert("Suit", &slotSuit);
	inventory.items.insert("Gloves", &slotGloves);
	inventory.items.insert("Boots", &slotBoots);
	inventory.items.insert("Back", &slotBack);
	inventory.items.insert("RightHip", &slotRightHip);
	inventory.items.insert("LeftHip", &slotLeftHip);
	inventory.items.insert("Waist", &slotWaist);
	inventory.items.insert("RightHand", &slotRightHand);
	inventory.items.insert("LeftHand", &slotLeftHand);
}

void Item::depositItem(Entity* itemToDeposit, String invSlot)
{
	if (inventory.items.find(invSlot))
	{
		Inventory::Slot slot;
		slot.entity = itemToDeposit;
		inventory.items.insert(invSlot, &slot);
	}
}

bool Item::isSlotFilled(String invSlot)
{
	return getSlottedItem(invSlot) != nullptr;
}

Entity* Item::getSlottedItem(String invSlot)
{
	Inventory::Slot* slot = *inventory.items.find(invSlot);
	return slot->entity;
}

void Item::setInventoryVisibility(bool visible)
{
	inventory.setVisibility(visible);
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