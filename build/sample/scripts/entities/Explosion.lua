-- explosion

require "base/scripts/constants"
require "base/scripts/vector"

-- this function executes when the entity is created
function init()
	entity:setRot(Rotation(0.2, 0.0, 0.0))
    speaker = entity:findSpeakerByName("speaker")

    soundToPlay = entity:getKeyValueAsString("soundToPlay")
    if soundToPlay ~= "" then
        speaker:playSound(soundToPlay, false, 1000)
    end
end

-- this function executes once per frame before any entities run process() or postprocess()
function preprocess()
end

-- this function executes once per frame, after all entities run preprocess()
function process()
    scale = entity:getScale()
    scale = vecAdd(scale, vecPow(0.05))
    entity:setScale(scale)
    if speaker:isPlayingAnything() == false then
        entity:remove()
    end
end

-- this function executes once per frame after all entities have executed preprocess() and process()
function postprocess()
end
