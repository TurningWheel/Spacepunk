-- bullet

require "base/scripts/constants"
require "base/scripts/vector"

function die()
    local world = entity:getWorld()
    local explosion = world:spawnEntity("Explosion", entity:getPos(), entity:getAng():toRotation())
    explosion:setKeyValue("soundToPlay", "boom.wav")
    entity:remove()
    return explosion
end

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
    canHitOwner = false

    -- get a handle on the Interface entity
    local world = entity:getWorld()
    entities = world:getEntitiesByName("Interface");
    if entities:getSize() > 0 then
        interface = entities:get(0)
    else
        interface = nil
    end
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
        die()
    end

    -- line trace
    hit = entity:lineTrace(entity:getPos(), vecAdd(entity:getPos(), vecMul(dir, vecPow(40))))

    -- see if we hit something
    if hit.manifest and hit.manifest.entity ~= nil then

        -- check for collision with a tank
        touchedTank = false
        count = 1
        while (count <= #tankChar)
        do
            if hit.manifest.entity:getName():get() == tankChar[count] then
                touchedTank = true
                tank = hit.manifest.entity

                -- destroy the tank
                local world = entity:getWorld()
                local explosion = world:spawnEntity("Explosion", tank:getPos(), tank:getAng():toRotation())
                explosion:setKeyValue("soundToPlay", "explosion.wav")
                explosion:setFlag(FLAG.DEPTHFAIL)
                tank:remove()

                -- adjust score
                if tankWhoShotMe == "RedTank" then
                    if tank:getName():get() == "BlueTank" then
                        local score = interface:getKeyValueAsInt("red_score")
                        score = score + 1 -- score a kill: gain a point
                        interface:setKeyValue("red_score", tostring(score))
                    elseif tank:getName():get() == "RedTank" then
                        local score = interface:getKeyValueAsInt("red_score")
                        score = score - 1 -- suicide: lose a point
                        interface:setKeyValue("red_score", tostring(score))
                    end
                elseif tankWhoShotMe == "BlueTank" then
                    if tank:getName():get() == "RedTank" then
                        local score = interface:getKeyValueAsInt("blue_score")
                        score = score + 1 -- score a kill: gain a point
                        interface:setKeyValue("blue_score", tostring(score))
                    elseif tank:getName():get() == "BlueTank" then
                        local score = interface:getKeyValueAsInt("blue_score")
                        score = score - 1 -- suicide: lose a point
                        interface:setKeyValue("blue_score", tostring(score))
                    end
                end

                -- explode bullet
                die()
            end
            count = count + 1
        end

        -- collision with something that was NOT a tank
        if touchedTank == false then
            if hit.manifest.entity:getName():get() ~= tankWhoShotMe or canHitOwner then

                -- a bullet can bounce 3 times before it explodes
                bounceNum = bounceNum + 1
                if bounceNum > 2 then
                    die()
                else
                    -- ricochet sound
                    speaker = entity:findSpeakerByName("speaker")
                    speaker:playSound("ricochet.wav", false, 1000)

                    -- bounce
                    dir = dir:reflect(hit.normal)
                    vel = vecMul(dir, speed)
                    entity:setVel(vel)

                    canHitOwner = true
                end
            end
        end

    end

end

-- this function executes once per frame after all entities have executed preprocess() and process()
function postprocess()
end
