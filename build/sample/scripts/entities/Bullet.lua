-- boolet

require "base/scripts/constants"
require "base/scripts/vector"

-- this function executes when the entity is created
function init()
    speed = vecPow(20)
    dir = entity:getAng():toVector()
    vel = vecMul(dir, speed)
	entity:setVel(vel)

    lifespan = 0
    bounceNum = 0

    tankWhoShotMe = entity:getKeyValueAsString("owner")
    tankChar = {"RedTank", "BlueTank"}
end

-- this function executes once per frame before any entities run process() or postprocess()
function preprocess()
end

-- this function executes once per frame, after all entities run preprocess()
function process()


    -- lifespan is 5 seconds or 3 bounces
    seconds = 60
    lifespan = lifespan + 1
    if lifespan > 5 * seconds then
        entity:remove()
    end

    if bounceNum > 2 then
        entity:remove()
    end



    -- line trace
    hit = entity:lineTrace(entity:getPos(), vecAdd(entity:getPos(), vecMul(dir, vecPow(40))))


    -- see if we hit something, and that it wasn't the tank who fired us:

    if hit.manifest and hit.manifest.entity ~= nil then
        if hit.manifest.entity:getName():get() ~= tankWhoShotMe then

            bounce = entity:findSpeakerByName("speaker")
            bounce:playSound("ricochet.wav", false, 1000)

            -- bounce
            dir = dir:reflect(hit.normal)
            vel = vecMul(dir, speed)
        	entity:setVel(vel)

             -- this way, a bullet can STILL hit the tank who shot it once it has bounced off something
            tankWhoShotMe = nil
            bounceNum = bounceNum + 1
        end

        count = 0
        while (count <= #tankChar)
        do

            if hit.manifest.entity:getName():get() == tankChar[count] then
                explode = entity:findSpeakerByName("speaker")
                explode:playSound("explosion.wav", false, 1000)

                hit.manifest.entity:remove()
                entity:remove()
            end
            count = count +1
        end
    end


end

-- this function executes once per frame after all entities have executed preprocess() and process()
function postprocess()
end
