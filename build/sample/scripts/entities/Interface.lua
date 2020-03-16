-- user interface

require "base/scripts/constants"
require "base/scripts/vector"

function updateStatus()
    red_score:setText(entity:getKeyValueAsString("red_score"))
    blue_score:setText(entity:getKeyValueAsString("blue_score"))
    if blueTank ~= nil then
        local reload = blueTank:getKeyValueAsInt("shootTimer")
        size.x = (xres / 4 - 45) * (reload / (60 * 2))
        size.w = (xres / 4 - 45) - (xres / 4 - 45) * (reload / (60 * 2))
        blue_reload_bar:setSize(size)
    else
        size.x = 0
        size.w = 0
        blue_reload_bar:setSize(size)
    end
    if redTank ~= nil then
        local reload = redTank:getKeyValueAsInt("shootTimer")
        size.x = 0
        size.w = (xres / 4 - 45) - (xres / 4 - 45) * (reload / (60 * 2))
        red_reload_bar:setSize(size)
    else
        size.x = 0
        size.w = 0
        red_reload_bar:setSize(size)
    end
end

function spawnTank(name)
    local pos = Vector(math.random(-400, 400), math.random(-400, 400), 145)
    local ang = Rotation(math.random(0, math.pi * 2), 0, 0)
    local tank = world:spawnEntity(name, pos, ang)
    local spawnShield = 60 * 2
    tank:setKeyValue("spawnShield", tostring(spawnShield))
    return tank
end

-- this function executes when the entity is created
function init()
    world = entity:getWorld()
    blueSpawnTimer = 0
    redSpawnTimer = 0

    -- server goes no further
    if entity:isServerObj() then
        do return end
    end

    localClient = engine:getLocalClient()
    gui = localClient:getGUI()
    xres = engine:getXres()
    yres = engine:getYres()
    size = RectSint32()

    -- red status
    red_status = gui:addFrame("red_status", "")
    size.w = xres / 4
    size.h = yres / 20
    size.x = xres / 100
    size.y = yres / 100
    red_status:setSize(size)
    red_status:setColor(WideVector(0.7, 0.0, 0.0, 1.0))

    -- red score
    size.w = 40
    size.h = yres / 20
    size.x = 0
    size.y = 0
    red_score = red_status:addField("red_score", 5)
    red_score:setSize(size)
    red_score:setJustify(JUSTIFY.CENTER)
    entity:setKeyValue("red_score", "0")

    -- red reload
    size.w = xres / 4 - 45
    size.h = yres / 20 - 10
    size.x = 40
    size.y = 5
    red_reload = red_status:addFrame("red_reload", "")
    red_reload:setSize(size)
    red_reload:setColor(WideVector(0.7, 0.0, 0.0, 1.0))
    red_reload:setHigh(false)

    -- red reload bar
    size.x = 0
    size.y = 0
    red_reload_bar = red_reload:addFrame("red_reload_bar", "")
    red_reload_bar:setSize(size)
    red_reload_bar:setColor(WideVector(0.7, 0.7, 0.7, 0.5))
    red_reload_bar:setBorder(0)

    -- blue status
    blue_status = gui:addFrame("blue_status", "")
    size.w = xres / 4
    size.h = yres / 20
    size.x = xres - xres / 100 - size.w
    size.y = yres / 100
    blue_status:setSize(size)
    blue_status:setColor(WideVector(0.0, 0.0, 0.7, 1.0))

    -- blue score
    size.w = 40
    size.h = yres / 20
    size.x = xres / 4 - size.w
    size.y = 0
    blue_score = blue_status:addField("blue_score", 5)
    blue_score:setSize(size)
    blue_score:setJustify(JUSTIFY.CENTER)
    entity:setKeyValue("blue_score", "0")

    -- blue reload
    size.w = xres / 4 - 45
    size.h = yres / 20 - 10
    size.x = 5
    size.y = 5
    blue_reload = blue_status:addFrame("blue_reload", "")
    blue_reload:setSize(size)
    blue_reload:setColor(WideVector(0.0, 0.0, 0.7, 1.0))
    blue_reload:setHigh(false)

    -- blue reload bar
    size.x = 0
    size.y = 0
    blue_reload_bar = blue_reload:addFrame("blue_reload_bar", "")
    blue_reload_bar:setSize(size)
    blue_reload_bar:setColor(WideVector(0.7, 0.7, 0.7, 0.5))
    blue_reload_bar:setBorder(0)
end

-- this function executes once per frame before any entities run process() or postprocess()
function preprocess()
end

-- this function executes once per frame, after all entities run preprocess()
function process()

    -- respawn blue tank
    blueTanks = world:getEntitiesByName("BlueTank")
    if blueTanks:getSize() == 0 then
        blueTank = nil
        blueSpawnTimer = blueSpawnTimer + 1
        if blueSpawnTimer > 60 * 2 then
            blueSpawnTimer = 0
            spawnTank("BlueTank")
        end
    else
        blueTank = blueTanks:get(0)
    end

    -- respawn red tank
    redTanks = world:getEntitiesByName("RedTank")
    if redTanks:getSize() == 0 then
        redTank = nil
        redSpawnTimer = redSpawnTimer + 1
        if redSpawnTimer > 60 * 2 then
            redSpawnTimer = 0
            spawnTank("RedTank")
        end
    else
        redTank = redTanks:get(0)
    end

    -- server goes no further
    if entity:isServerObj() then
        do return end
    end

    updateStatus()
end

-- this function executes once per frame after all entities have executed preprocess() and process()
function postprocess()
end
