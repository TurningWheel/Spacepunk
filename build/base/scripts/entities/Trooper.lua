-- Trooper

function animate(name, blend)
	model:animate(name, blend)
	gun:animate(name, blend)
end

function init()
	model = entity:findModelByName("model")
	gun = entity:findModelByName("gun")
	bbox = entity:findBBoxByName("physics")

	animate("walk forward", false)
end

function process()
	local forwardAng = Angle(entity:getAng().yaw,entity:getAng().pitch,entity:getAng().roll)
	forwardAng.pitch = 0
	forwardAng.roll = 0
	entity:setVel(forwardAng:toVector())
end

function postprocess()
end

function preprocess()
end