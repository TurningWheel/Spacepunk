-- Defines several constants common across all script files

function protect(tbl)
    return setmetatable({}, {
        __index = tbl,
        __newindex = function(t, key, value)
            error("attempting to change constant " ..
                   tostring(key) .. " to " .. tostring(value), 2)
        end
    })
end

-- BBox collision shapes
SHAPE = {
	BOX = 0,
	SPHERE = 1,
	CAPSULE = 2,
	CYLINDER = 3,
	CONE = 4,
	MESH = 5,
}
SHAPE = protect(SHAPE)

-- Button styles
STYLE = {
	NORMAL = 0,
	TOGGLE = 1,
	CHECKBOX = 2,
}
STYLE = protect(STYLE)

-- Character sex
SEX = {
	MALE = 0,
	FEMALE = 1,
	NONE = 2,
}
SEX = protect(SEX)

-- Component type
COMPONENT = {
	BASIC = 0,
	BBOX = 1,
	MODEL = 2,
	LIGHT = 3,
	CAMERA = 4,
	SPEAKER = 5,
	EMITTER = 6,
	CHARACTER = 7,
}
COMPONENT = protect(COMPONENT)

-- Editor mode
EDITINGMODE = {
	TILES = 0,
	TEXTURES = 1,
	ENTITIES = 2,
	SECTORS = 3,
}
EDITINGMODE = protect(EDITINGMODE)

-- Editor widget mode
WIDGETMODE = {
	NONE = 0,
	TRANSLATE = 1,
	ROTATE = 2,
	SCALE = 3,
}
WIDGETMODE = protect(WIDGETMODE)

-- Log message type
MSG = {
	DEBUG = 0,
	INFO = 1,
	WARN = 2,
	ERROR = 3,
	CRITICAL = 4,
	FATAL = 5,
	NOTE = 6,
	CHAT = 7,
}
MSG = protect(MSG)

-- Entity flag type
FLAG = {
	VISIBLE = 1,
	PASSABLE = 2,
	LOCAL = 4,
	UPDATE = 8,
	GENIUS = 16,
	OVERDRAW = 32,
	SPRITE = 64,
	ALLOWTRACE = 128,
	SHADOW = 256,
	FULLYLIT = 512,
	GLOWING = 1024,
	DEPTHFAIL = 2048,
	OCCLUDE = 4096,
	INTERACTABLE = 8192,
}
FLAG = protect(FLAG)

-- Field justification
JUSTIFY = {
	LEFT = 0,
	CENTER = 1,
	RIGHT = 2,
}
JUSTIFY = protect(JUSTIFY)

-- Player input binding
BINDING = {
	INVALID = 0,

	MOVE_FORWARD = 1,
	MOVE_LEFT = 2,
	MOVE_BACKWARD = 3,
	MOVE_RIGHT = 4,
	MOVE_UP = 5,
	MOVE_DOWN = 6,

	LOOK_UP = 7,
	LOOK_LEFT = 8,
	LOOK_DOWN = 9,
	LOOK_RIGHT = 10,

	LEAN_MODIFIER = 11,
	LEAN_LEFT = 12,
	LEAN_RIGHT = 13,

	INTERACT = 14,
	HAND_LEFT = 15,
	HAND_RIGHT = 16,

	INVENTORY1 = 17,
	INVENTORY2 = 18,
	INVENTORY3 = 19,
	INVENTORY4 = 20,
	DROP_MODIFIER = 21,
	STATUS = 22,

	MENU_UP = 23,
	MENU_LEFT = 24,
	MENU_DOWN = 25,
	MENU_RIGHT = 26,
	MENU_CONFIRM = 27,
	MENU_CANCEL = 28,
	MENU_TOGGLE = 29,
	MENU_LOBBY = 30,
	MENU_PAGE_LEFT = 31,
	MENU_PAGE_RIGHT = 32,
}
BINDING = protect(BINDING)

-- Tile size
TILE = {
	SIZE = 128,
}
TILE = protect(TILE)

-- Math
MATH = {
	PI = 3.14159265358979323,
}
MATH = protect(MATH)