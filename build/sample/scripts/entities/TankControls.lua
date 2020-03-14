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

	input = Input()
	input:rebind("up", joystick .. "Axis-4")
	input:rebind("down", joystick .. "Axis+4")
	input:rebind("left", joystick .. "Axis-0")
	input:rebind("right", joystick .. "Axis+0")
	input:rebind("x", joystick .. "Button0")
	input:rebind("a", joystick .. "Button1")
	input:rebind("b", joystick .. "Button2")
	input:rebind("y", joystick .. "Button3")
	input:rebind("l", joystick .. "Button4")
	input:rebind("r", joystick .. "Button5")
	input:rebind("select", joystick .. "Button8")
	input:rebind("start", joystick .. "Button9")
end

-- this function executes once per frame before any entities run process() or postprocess()
function preprocess()
end

-- this function executes once per frame, after all entities run preprocess()
function process()
	input:update()
	local step = 60 -- ticks per second

	-- move forward and backward
	local forward = 2.0 * input:analog("up")
	local forwardVec = vecMul(entity:getAng():toVector(), Vector(forward, forward, forward))
	local backward = 2.0 * input:analog("down")
	local backwardVec = vecMul(entity:getAng():rotate(Rotation(math.pi, 0.0, 0.0)):toVector(), Vector(backward, backward, backward))
	local vel = vecAdd(forwardVec, backwardVec)
	entity:setVel(vel)

	-- shoot
    if input:binaryToggle("a") then
        world = entity:getWorld()
        barrel = entity:findComponentByName("Barrel")
        bullet = world:spawnEntity("bullet", barrel:getGlobalPos(), barrel:getGlobalAng():toRotation())
        bullet:setKeyValue("owner", entity:getName():get())
        input:consumeBinaryToggle("a")
        boom = entity:findSpeakerByName("speaker")
        boom:playSound("fire.wav", false, 1000)
    end

	-- turn left and right
	local right = math.pi / step * input:analog("right")
	local left = math.pi / step * input:analog("left")
	local rot = Rotation(right - left, 0.0, 0.0)
	entity:setRot(rot)
end

-- this function executes once per frame after all entities have executed preprocess() and process()
function postprocess()
end
