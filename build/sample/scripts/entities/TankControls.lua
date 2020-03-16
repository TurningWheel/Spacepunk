-- a simple moving character with tank controls

require "base/scripts/constants"
require "base/scripts/vector"

-- this function executes when the entity is created
function init()
	-- setup inputs
	local joystick = entity:getKeyValueAsString("joystick")
	if joystick == "" then
		joystick = "Joy0"
	end

	input = engine:getInput(entity:getKeyValueAsInt("player"))
	shootTimer = entity:getKeyValueAsInt("shootTimer") -- reload timer

    spawnShield = entity:getKeyValueAsInt("spawnShield")
    if spawnShield > 0 then
        speaker = entity:findSpeakerByName("speaker")
        speaker:playSound("pop.wav", false, 1000)
    end
end

-- this function executes once per frame before any entities run process() or postprocess()
function preprocess()
end

-- this function executes once per frame, after all entities run preprocess()
function process()
	local step = 60 -- ticks per second

    -- spawn shield
    spawnShield = entity:getKeyValueAsInt("spawnShield")
    if spawnShield > 0 then
        entity:resetFlag(FLAG.ALLOWTRACE)
        if entity:getTicks() % 6 < 3 then
            entity:resetFlag(FLAG.VISIBLE)
        else
            entity:setFlag(FLAG.VISIBLE)
        end
        spawnShield = spawnShield - 1
        entity:setKeyValue("spawnShield", tostring(spawnShield))
    else
        entity:setFlag(FLAG.ALLOWTRACE)
        entity:setFlag(FLAG.VISIBLE)
    end

	-- move forward and backward
	local forward = 2.0 * input:analog("Forward")
	local forwardVec = vecMul(entity:getAng():toVector(), Vector(forward, forward, forward))
	local backward = 2.0 * input:analog("Backward")
	local backwardVec = vecMul(entity:getAng():rotate(Rotation(math.pi, 0.0, 0.0)):toVector(), Vector(backward, backward, backward))
	local vel = vecAdd(forwardVec, backwardVec)
	entity:setVel(vel)

	-- shoot
    if input:binaryToggle("Fire") and shootTimer == 0 then
        world = entity:getWorld()
        barrel = entity:findComponentByName("Barrel")
        bullet = world:spawnEntity("Bullet", barrel:getGlobalPos(), barrel:getGlobalAng():toRotation())
        bullet:setKeyValue("owner", entity:getName():get())
        input:consumeBinaryToggle("Fire")
        speaker = entity:findSpeakerByName("speaker")
        speaker:playSound("fire.wav", false, 1000)
        shootTimer = step * 2 -- 2 second reload
    end

	-- turn left and right
	local right = math.pi / step * input:analog("TurnRight")
	local left = math.pi / step * input:analog("TurnLeft")
	local rot = Rotation(right - left, 0.0, 0.0)
	entity:setRot(rot)

	-- reload!
	if shootTimer > 0 then
		shootTimer = shootTimer - 1
	end
	entity:setKeyValue("shootTimer", tostring(shootTimer))
end

-- this function executes once per frame after all entities have executed preprocess() and process()
function postprocess()
end
