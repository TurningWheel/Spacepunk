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

UINT8_MAX = 0xff; UINT8_MAX = protect(UINT8_MAX)
INT8_MAX = 0xff; INT8_MAX = protect(INT8_MAX)
UINT16_MAX = 0xffff; UINT16_MAX = protect(UINT16_MAX)
INT16_MAX = 0xffff; INT16_MAX = protect(INT16_MAX)
UINT32_MAX = 0xffffffff; UINT32_MAX = protect(UINT32_MAX)
INT32_MAX = 0xffffffff; INT32_MAX = protect(INT32_MAX)

-- Frame border styles
BORDER = {
    FLAT = 0,
    BEVEL_HIGH = 1,
    BEVEL_LOW = 2,
}
BORDER = protect(BORDER)

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
    DROPDOWN = 3,
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
    TOP = 0,
    BOTTOM = 1,
    LEFT = 2,
    RIGHT = 3,
    CENTER = 4,
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

    TOGGLE_INVENTORY = 33,
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

-- Keyboard scancodes
SCANCODE = {
    KEY_UNKNOWN = 0,
    KEY_A = 4,
    KEY_B = 5,
    KEY_C = 6,
    KEY_D = 7,
    KEY_E = 8,
    KEY_F = 9,
    KEY_G = 10,
    KEY_H = 11,
    KEY_I = 12,
    KEY_J = 13,
    KEY_K = 14,
    KEY_L = 15,
    KEY_M = 16,
    KEY_N = 17,
    KEY_O = 18,
    KEY_P = 19,
    KEY_Q = 20,
    KEY_R = 21,
    KEY_S = 22,
    KEY_T = 23,
    KEY_U = 24,
    KEY_V = 25,
    KEY_W = 26,
    KEY_X = 27,
    KEY_Y = 28,
    KEY_Z = 29,
    KEY_1 = 30,
    KEY_2 = 31,
    KEY_3 = 32,
    KEY_4 = 33,
    KEY_5 = 34,
    KEY_6 = 35,
    KEY_7 = 36,
    KEY_8 = 37,
    KEY_9 = 38,
    KEY_0 = 39,
    KEY_RETURN = 40,
    KEY_ESCAPE = 41,
    KEY_BACKSPACE = 42,
    KEY_TAB = 43,
    KEY_SPACE = 44,
    KEY_MINUS = 45,
    KEY_EQUALS = 46,
    KEY_LEFTBRACKET = 47,
    KEY_RIGHTBRACKET = 48,
    KEY_BACKSLASH = 49,
    KEY_NONUSHASH = 50,
    KEY_SEMICOLON = 51,
    KEY_APOSTROPHE = 52,
    KEY_GRAVE = 53,
    KEY_COMMA = 54,
    KEY_PERIOD = 55,
    KEY_SLASH = 56,
    KEY_CAPSLOCK = 57,
    KEY_F1 = 58,
    KEY_F2 = 59,
    KEY_F3 = 60,
    KEY_F4 = 61,
    KEY_F5 = 62,
    KEY_F6 = 63,
    KEY_F7 = 64,
    KEY_F8 = 65,
    KEY_F9 = 66,
    KEY_F10 = 67,
    KEY_F11 = 68,
    KEY_F12 = 69,
    KEY_PRINTSCREEN = 70,
    KEY_SCROLLLOCK = 71,
    KEY_PAUSE = 72,
    KEY_INSERT = 73,
    KEY_HOME = 74,
    KEY_PAGEUP = 75,
    KEY_DELETE = 76,
    KEY_END = 77,
    KEY_PAGEDOWN = 78,
    KEY_RIGHT = 79,
    KEY_LEFT = 80,
    KEY_DOWN = 81,
    KEY_UP = 82,
    KEY_NUMLOCKCLEAR = 83,
    KEY_KP_DIVIDE = 84,
    KEY_KP_MULTIPLY = 85,
    KEY_KP_MINUS = 86,
    KEY_KP_PLUS = 87,
    KEY_KP_ENTER = 88,
    KEY_KP_1 = 89,
    KEY_KP_2 = 90,
    KEY_KP_3 = 91,
    KEY_KP_4 = 92,
    KEY_KP_5 = 93,
    KEY_KP_6 = 94,
    KEY_KP_7 = 95,
    KEY_KP_8 = 96,
    KEY_KP_9 = 97,
    KEY_KP_0 = 98,
    KEY_KP_PERIOD = 99,
    KEY_NONUSBACKSLASH = 100,
    KEY_APPLICATION = 101,
    KEY_POWER = 102,
    KEY_KP_EQUALS = 103,
    KEY_F13 = 104,
    KEY_F14 = 105,
    KEY_F15 = 106,
    KEY_F16 = 107,
    KEY_F17 = 108,
    KEY_F18 = 109,
    KEY_F19 = 110,
    KEY_F20 = 111,
    KEY_F21 = 112,
    KEY_F22 = 113,
    KEY_F23 = 114,
    KEY_F24 = 115,
    KEY_EXECUTE = 116,
    KEY_HELP = 117,
    KEY_MENU = 118,
    KEY_SELECT = 119,
    KEY_STOP = 120,
    KEY_AGAIN = 121,
    KEY_UNDO = 122,
    KEY_CUT = 123,
    KEY_COPY = 124,
    KEY_PASTE = 125,
    KEY_FIND = 126,
    KEY_MUTE = 127,
    KEY_VOLUMEUP = 128,
    KEY_VOLUMEDOWN = 129,
    KEY_KP_COMMA = 133,
    KEY_KP_EQUALSAS400 = 134,
    KEY_INTERNATIONAL1 = 135,
    KEY_INTERNATIONAL2 = 136,
    KEY_INTERNATIONAL3 = 137,
    KEY_INTERNATIONAL4 = 138,
    KEY_INTERNATIONAL5 = 139,
    KEY_INTERNATIONAL6 = 140,
    KEY_INTERNATIONAL7 = 141,
    KEY_INTERNATIONAL8 = 142,
    KEY_INTERNATIONAL9 = 143,
    KEY_LANG1 = 144,
    KEY_LANG2 = 145,
    KEY_LANG3 = 146,
    KEY_LANG4 = 147,
    KEY_LANG5 = 148,
    KEY_LANG6 = 149,
    KEY_LANG7 = 150,
    KEY_LANG8 = 151,
    KEY_LANG9 = 152,
    KEY_ALTERASE = 153,
    KEY_SYSREQ = 154,
    KEY_CANCEL = 155,
    KEY_CLEAR = 156,
    KEY_PRIOR = 157,
    KEY_RETURN2 = 158,
    KEY_SEPARATOR = 159,
    KEY_OUT = 160,
    KEY_OPER = 161,
    KEY_CLEARAGAIN = 162,
    KEY_CRSEL = 163,
    KEY_EXSEL = 164,
    KEY_KP_00 = 176,
    KEY_KP_000 = 177,
    KEY_THOUSANDSSEPARATOR = 178,
    KEY_DECIMALSEPARATOR = 179,
    KEY_CURRENCYUNIT = 180,
    KEY_CURRENCYSUBUNIT = 181,
    KEY_KP_LEFTPAREN = 182,
    KEY_KP_RIGHTPAREN = 183,
    KEY_KP_LEFTBRACE = 184,
    KEY_KP_RIGHTBRACE = 185,
    KEY_KP_TAB = 186,
    KEY_KP_BACKSPACE = 187,
    KEY_KP_A = 188,
    KEY_KP_B = 189,
    KEY_KP_C = 190,
    KEY_KP_D = 191,
    KEY_KP_E = 192,
    KEY_KP_F = 193,
    KEY_KP_XOR = 194,
    KEY_KP_POWER = 195,
    KEY_KP_PERCENT = 196,
    KEY_KP_LESS = 197,
    KEY_KP_GREATER = 198,
    KEY_KP_AMPERSAND = 199,
    KEY_KP_DBLAMPERSAND = 200,
    KEY_KP_VERTICALBAR = 201,
    KEY_KP_DBLVERTICALBAR = 202,
    KEY_KP_COLON = 203,
    KEY_KP_HASH = 204,
    KEY_KP_SPACE = 205,
    KEY_KP_AT = 206,
    KEY_KP_EXCLAM = 207,
    KEY_KP_MEMSTORE = 208,
    KEY_KP_MEMRECALL = 209,
    KEY_KP_MEMCLEAR = 210,
    KEY_KP_MEMADD = 211,
    KEY_KP_MEMSUBTRACT = 212,
    KEY_KP_MEMMULTIPLY = 213,
    KEY_KP_MEMDIVIDE = 214,
    KEY_KP_PLUSMINUS = 215,
    KEY_KP_CLEAR = 216,
    KEY_KP_CLEARENTRY = 217,
    KEY_KP_BINARY = 218,
    KEY_KP_OCTAL = 219,
    KEY_KP_DECIMAL = 220,
    KEY_KP_HEXADECIMAL = 221,
    KEY_LCTRL = 224,
    KEY_LSHIFT = 225,
    KEY_LALT = 226,
    KEY_LGUI = 227,
    KEY_RCTRL = 228,
    KEY_RSHIFT = 229,
    KEY_RALT = 230,
    KEY_RGUI = 231,
    KEY_MODE = 257,
    KEY_AUDIONEXT = 258,
    KEY_AUDIOPREV = 259,
    KEY_AUDIOSTOP = 260,
    KEY_AUDIOPLAY = 261,
    KEY_AUDIOMUTE = 262,
    KEY_MEDIASELECT = 263,
    KEY_WWW = 264,
    KEY_MAIL = 265,
    KEY_CALCULATOR = 266,
    KEY_COMPUTER = 267,
    KEY_AC_SEARCH = 268,
    KEY_AC_HOME = 269,
    KEY_AC_BACK = 270,
    KEY_AC_FORWARD = 271,
    KEY_AC_STOP = 272,
    KEY_AC_REFRESH = 273,
    KEY_AC_BOOKMARKS = 274,
    KEY_BRIGHTNESSDOWN = 275,
    KEY_BRIGHTNESSUP = 276,
    KEY_DISPLAYSWITCH = 277,
    KEY_KBDILLUMTOGGLE = 278,
    KEY_KBDILLUMDOWN = 279,
    KEY_KBDILLUMUP = 280,
    KEY_EJECT = 281,
    KEY_SLEEP = 282,
    KEY_APP1 = 283,
    KEY_APP2 = 284,
}
SCANCODE = protect(SCANCODE)