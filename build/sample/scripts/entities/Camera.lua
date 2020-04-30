-- a full screen camera

require "base/scripts/constants"

-- this function executes when the entity is created
function init()
	camera = entity:findCameraByName("camera")
    camera:setListener()
end

-- this function executes once per frame before any entities run process() or postprocess()
function preprocess()
end

-- this function executes once per frame, after all entities run preprocess()
function process()
end

-- this function executes once per frame after all entities have executed preprocess() and process()
function postprocess()
end
