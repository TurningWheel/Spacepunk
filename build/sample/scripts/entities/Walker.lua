-- a simple walking character with player controls

require "base/scripts/constants"

function init()
	input = engine:getInput(0)
end

function preprocess()
end

function process()
	--local cpos = entity:getPos()
	--local pos = Vector(cpos.x, cpos.y, cpos.z)
	local vel = Vector(1.0 * input:analog("MoveForward"), 0.0, 0.0)
	entity:setVel(vel)
end

function postprocess()
end