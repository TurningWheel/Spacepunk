-- Player spawn point (invisible, uninteractable object)

require "base/scripts/constants"

function init()
	entity:setFlags(0)
	entity:setFlag(FLAG.PASSABLE)
	entity:setFlag(FLAG.LOCAL)
end

function process()
end

function postprocess()
end

function preprocess()
end