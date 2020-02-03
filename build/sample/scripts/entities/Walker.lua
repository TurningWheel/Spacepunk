-- a simple walking character with player controls

require "base/scripts/constants"
require "base/scripts/vector"

function init()
	input = Input()

	input:rebind("up", "Joy0Axis-4")
	input:rebind("down", "Joy0Axis+4")
	input:rebind("left", "Joy0Axis-0")
	input:rebind("right", "Joy0Axis+0")
	input:rebind("x", "Joy0Button0")
	input:rebind("a", "Joy0Button1")
	input:rebind("b", "Joy0Button2")
	input:rebind("y", "Joy0Button3")
	input:rebind("l", "Joy0Button4")
	input:rebind("r", "Joy0Button5")
	input:rebind("select", "Joy0Button8")
	input:rebind("start", "Joy0Button9")
end

function preprocess()
end

function process()
	input:update()
	local step = 60 -- ticks per second

	local forward = 2.0 * input:analog("up")
	local forwardVec = vecMul(entity:getAng():toVector(), Vector(forward, forward, forward))
	local backward = 2.0 * input:analog("down")
	local backwardVec = vecMul(entity:getAng():rotate(Rotation(math.pi, 0.0, 0.0)):toVector(), Vector(backward, backward, backward))
	local vel = vecAdd(forwardVec, backwardVec)
	entity:setVel(vel)

	local right = math.pi / step * input:analog("right")
	local left = math.pi / step * input:analog("left")
	local rot = Rotation(right - left, 0.0, 0.0)
	entity:setRot(rot)
end

function postprocess()
end