-- a full screen camera

require "base/scripts/constants"

-- this function executes when the entity is created
function init()
	camera = entity:findCameraByName("camera")
	win = RectSint32()
    camera:setListener()
end

-- this function executes once per frame before any entities run process() or postprocess()
function preprocess()
end

-- this function executes once per frame, after all entities run preprocess()
function process()
	win.x = 0
	win.y = 0
	win.w = engine:getXres()
	win.h = engine:getYres()
	camera:setWin(win)
end

-- this function executes once per frame after all entities have executed preprocess() and process()
function postprocess()
end
