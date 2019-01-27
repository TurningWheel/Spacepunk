// Item.hpp

#pragma once

#include "Main.hpp"
#include "String.hpp"
#include "ArrayList.hpp"

class Item {
private:
	float		weight				= 1.f;				// weight
	float		value				= 1.f;				// base price

	// a thrown item calls thrown(item, victim) on contact
	bool		throwable			= false;			// hold action + drop button throws the item
	float		throwLeadInTime		= 1.f;				// time it takes to animate a throw in secs
	float		throwRecovery		= 1.f;				// time after throwing before another action can be done
	bool		throwDetonates		= false;			// whether it detonates on contact after throwing

	bool		explodeOnDamage		= false;			//

	float		distance			= 0.f;				// effect distance
	float		spread				= 0.f;				// effect spread cone arc
	float		radius				= 0.f;				// effect radius

	float		animLeadSpeed		= 0.f;				// pre-effect animation time
	float		animRecovSpeed		= 0.f;				// post-effect animation time

	Sint32		spendCharges		= 0;				// ammo to spend per use
	Sint32		currCharges			= 0;				// current ammo capacity
	Sint32		maxCharges			= 0;				// max ammo capacity
	Sint32		rechargeRate		= 0;				// recharge rate

	float		currCooldown		= 0.f;				// current cooldown time between uses
	float		maxCooldown			= 0.f;				// max cooldown time between uses

	ArrayList<damageinfo_t> damages;

	// in script:
	// passiveQuick() function
	// passiveHold() function
	// passiveEquip() function
};