// Input.cpp

#include "Main.hpp"
#include "Input.hpp"
#include "Engine.hpp"
#include "Console.hpp"

Cvar cvar_sensitivity("input.sensitivity","look sensitivity","1.0");
Cvar cvar_deadzone("input.deadzone","controller stick dead-zone range","0.2");

const char* Input::bindingName[] = {
	"Invalid",

	"MoveForward",
	"MoveLeft",
	"MoveBackward",
	"MoveRight",
	"MoveUp",
	"MoveDown",

	"LookUp",
	"LookLeft",
	"LookDown",
	"LookRight",

	"Lean",
	"LeanLeft",
	"LeanRight",

	"Interact",
	"HandLeft",
	"HandRight",

	"Inventory1",
	"Inventory2",
	"Inventory3",
	"Inventory4",
	"Drop",
	"Status",

	"MenuUp",
	"MenuLeft",
	"MenuDown",
	"MenuRight",
	"MenuConfirm",
	"MenuCancel",
	"MenuToggle",
	"MenuLobby",
	"MenuPageLeft",
	"MenuPageRight"
};

Map<SDL_Scancode> Input::scancodeNames;

float Input::analog(Input::bindingenum_t binding) const {
	return bindings[binding].analog;
}

bool Input::binary(Input::bindingenum_t binding) const {
	return bindings[binding].binary;
}

bool Input::binaryToggle(bindingenum_t binding) const {
	return bindings[binding].binary && !bindings[binding].consumed;
}

void Input::consumeBinaryToggle(bindingenum_t binding) {
	if ( bindings[binding].binary )
	{
		bindings[binding].consumed = true;
	}
}

const char* Input::binding(Input::bindingenum_t binding) const {
	return bindings[binding].input.get();
}

void Input::rebind(Input::bindingenum_t binding, const char* input) {
	bindings[binding].input.assign(input);
	if( input == nullptr ) {
		bindings[binding].type = binding_t::INVALID;
		return;
	}

	size_t len = strlen(input);
	if( len >= 3 && strncmp(input, "Pad", 3) == 0 ) {
		// game controller

		char* type = nullptr;
		Uint32 index = strtol((const char*)(input+3), &type, 10);
		LinkedList<SDL_GameController*>& list = mainEngine->getControllers();
		Node<SDL_GameController*>* node = list.nodeForIndex(index);

		if( node ) {
			bindings[binding].pad = node->getData();
			SDL_GameController* pad = node->getData();
			if( strncmp(type, "Button", 6) == 0 ) {
				if( strcmp((const char*)(type+6), "A") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_A;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else if( strcmp((const char*)(type+6), "B") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_B;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else if( strcmp((const char*)(type+6), "X") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_X;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else if( strcmp((const char*)(type+6), "Y") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_Y;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else if( strcmp((const char*)(type+6), "Back") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_BACK;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else if( strcmp((const char*)(type+6), "Start") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_START;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else if( strcmp((const char*)(type+6), "LeftStick") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_LEFTSTICK;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else if( strcmp((const char*)(type+6), "RightStick") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else if( strcmp((const char*)(type+6), "LeftBumper") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else if( strcmp((const char*)(type+6), "RightBumper") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else {
					bindings[binding].type = binding_t::INVALID;
					return;
				}
			}
			else if( strncmp(type, "StickLeft", 9) == 0 ) {
				if( strcmp((const char*)(type+9), "X-") == 0 ) {
					bindings[binding].padAxisNegative = true;
					bindings[binding].padAxis = SDL_CONTROLLER_AXIS_LEFTX;
					bindings[binding].type = binding_t::CONTROLLER_AXIS;
					return;
				}
				else if( strcmp((const char*)(type+9), "X+") == 0 ) {
					bindings[binding].padAxisNegative = false;
					bindings[binding].padAxis = SDL_CONTROLLER_AXIS_LEFTX;
					bindings[binding].type = binding_t::CONTROLLER_AXIS;
					return;
				}
				else if( strcmp((const char*)(type+9), "Y-") == 0 ) {
					bindings[binding].padAxisNegative = true;
					bindings[binding].padAxis = SDL_CONTROLLER_AXIS_LEFTY;
					bindings[binding].type = binding_t::CONTROLLER_AXIS;
					return;
				}
				else if( strcmp((const char*)(type+9), "Y+") == 0 ) {
					bindings[binding].padAxisNegative = false;
					bindings[binding].padAxis = SDL_CONTROLLER_AXIS_LEFTY;
					bindings[binding].type = binding_t::CONTROLLER_AXIS;
					return;
				}
				else {
					bindings[binding].type = binding_t::INVALID;
					return;
				}
			}
			else if( strncmp(type, "StickRight", 10) == 0 ) {
				if( strcmp((const char*)(type+10), "X-") == 0 ) {
					bindings[binding].padAxisNegative = true;
					bindings[binding].padAxis = SDL_CONTROLLER_AXIS_RIGHTX;
					bindings[binding].type = binding_t::CONTROLLER_AXIS;
					return;
				}
				else if( strcmp((const char*)(type+10), "X+") == 0 ) {
					bindings[binding].padAxisNegative = false;
					bindings[binding].padAxis = SDL_CONTROLLER_AXIS_RIGHTX;
					bindings[binding].type = binding_t::CONTROLLER_AXIS;
					return;
				}
				else if( strcmp((const char*)(type+10), "Y-") == 0 ) {
					bindings[binding].padAxisNegative = true;
					bindings[binding].padAxis = SDL_CONTROLLER_AXIS_RIGHTY;
					bindings[binding].type = binding_t::CONTROLLER_AXIS;
					return;
				}
				else if( strcmp((const char*)(type+10), "Y+") == 0 ) {
					bindings[binding].padAxisNegative = false;
					bindings[binding].padAxis = SDL_CONTROLLER_AXIS_RIGHTY;
					bindings[binding].type = binding_t::CONTROLLER_AXIS;
					return;
				}
				else {
					bindings[binding].type = binding_t::INVALID;
					return;
				}
			}
			else if( strncmp(type, "Dpad", 4) == 0 ) {
				if( strcmp((const char*)(type+4), "X-") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else if( strcmp((const char*)(type+4), "X+") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else if( strcmp((const char*)(type+4), "Y-") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_DPAD_UP;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else if( strcmp((const char*)(type+4), "Y+") == 0 ) {
					bindings[binding].padButton = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
					bindings[binding].type = binding_t::CONTROLLER_BUTTON;
					return;
				}
				else {
					bindings[binding].type = binding_t::INVALID;
					return;
				}
			}
			else if( strncmp(type, "LeftTrigger", 11) == 0 ) {
				bindings[binding].padAxisNegative = false;
				bindings[binding].padAxis = SDL_CONTROLLER_AXIS_TRIGGERLEFT;
				bindings[binding].type = binding_t::CONTROLLER_AXIS;
				return;
			}
			else if( strncmp(type, "RightTrigger", 12) == 0 ) {
				bindings[binding].padAxisNegative = false;
				bindings[binding].padAxis = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
				bindings[binding].type = binding_t::CONTROLLER_AXIS;
				return;
			}
			else {
				bindings[binding].type = binding_t::INVALID;
				return;
			}
		} else {
			bindings[binding].type = binding_t::INVALID;
			return;
		}
	}
	else if( len >= 5 && strncmp(input, "Mouse", 5) == 0 ) {
		// mouse
		char* button = nullptr;
		Uint32 index = strtol((const char*)(input+5), &button, 10);
		int result = min( max( 0, (int)strtol(button, nullptr, 10) ), 4 );

		bindings[binding].type = binding_t::MOUSE_BUTTON;
		bindings[binding].mouseButton = result;
		return;
	}
	else {
		// keyboard
		bindings[binding].type = binding_t::KEYBOARD;
		bindings[binding].scancode = getScancodeFromName(input);
		return;
	}
}

Input::bindingenum_t Input::enumForName(const char* name) {
	if( !name ) {
		return INVALID;
	}
	size_t theirLen = strlen(name);

	for( int c = 0; c < (int)(BINDINGENUM_TYPE_LENGTH); ++c ) {
		size_t ourLen = strlen(bindingName[c]);
		if( theirLen != ourLen )
			continue;

		if( strncmp(bindingName[c], name, ourLen) == 0 ) {
			return (bindingenum_t)c;
		}
	}
	return INVALID;
}

void Input::update() {
	for( int c = 0; c < (int)(BINDINGENUM_TYPE_LENGTH); ++c ) {
		bindingenum_t binding = (bindingenum_t)c;
		bindings[binding].analog = analogOf(bindings[binding]);
		bool obinary = bindings[binding].binary;
		bindings[binding].binary = binaryOf(bindings[binding]);
		if ( obinary != bindings[binding].binary )
		{
			//Unconsume the input whenever it's released or pressed again.
			bindings[binding].consumed = false;
		}
	}
}

bool Input::binaryOf(binding_t& binding) {
	if( binding.type == binding_t::CONTROLLER_AXIS ||
		binding.type == binding_t::CONTROLLER_BUTTON ) {
		SDL_GameController* pad = binding.pad;
		if( binding.type == binding_t::CONTROLLER_BUTTON ) {
			return SDL_GameControllerGetButton(pad, binding.padButton) == 1;
		} else {
			if( binding.padAxisNegative ) {
				return SDL_GameControllerGetAxis(pad, binding.padAxis) < -16384;
			} else {
				return SDL_GameControllerGetAxis(pad, binding.padAxis) > 16384;
			}
		}
	} else if( binding.type == binding_t::MOUSE_BUTTON ) {
		return mainEngine->getMouseStatus(binding.mouseButton);
	} else if( binding.type == binding_t::KEYBOARD ) {
		SDL_Scancode key = binding.scancode;
		if( key != SDL_SCANCODE_UNKNOWN ) {
			return mainEngine->getKeyStatus((int)key);
		}
	}

	return false;
}

float Input::analogOf(binding_t& binding) {
	if( binding.type == binding_t::CONTROLLER_AXIS ||
		binding.type == binding_t::CONTROLLER_BUTTON ) {
		SDL_GameController* pad = binding.pad;
		if( binding.type == binding_t::CONTROLLER_BUTTON ) {
			return SDL_GameControllerGetButton(pad, binding.padButton) ? 1.f : 0.f;
		} else {
			if( binding.padAxisNegative ) {
				float result = min(SDL_GameControllerGetAxis(pad, binding.padAxis) / 32768.f, 0.f) * -1.f;
				return (result > cvar_deadzone.toFloat()) ? result : 0.f;
			} else {
				float result = max(SDL_GameControllerGetAxis(pad, binding.padAxis) / 32767.f, 0.f);
				return (result > cvar_deadzone.toFloat()) ? result : 0.f;
			}
		}
	} else if( binding.type == binding_t::MOUSE_BUTTON ) {
		return mainEngine->getMouseStatus(binding.mouseButton) ? 1.f : 0.f;
	} else if( binding.type == binding_t::KEYBOARD ) {
		SDL_Scancode key = binding.scancode;
		if( key != SDL_SCANCODE_UNKNOWN ) {
			return mainEngine->getKeyStatus((int)key) ? 1.f : 0.f;
		}
	}

	return 0.f;
}

SDL_Scancode Input::getScancodeFromName(const char* name) {
	SDL_Scancode* search = scancodeNames.find(name);
	if (!search) {
		SDL_Scancode scancode = SDL_GetScancodeFromName(name);
		if (scancode != SDL_SCANCODE_UNKNOWN) {
			scancodeNames.insert(name, scancode);
		}
		return scancode;
	} else {
		return *search;
	}
}

static int console_bind(int argc, const char** argv) {
	if( argc < 3 ) {
		mainEngine->fmsg(Engine::MSG_ERROR,"Not enough args. ex: bind 0 MoveForward W");
		return 1;
	}
	int num = argv[0][0] - '0';
	if( num >= 0 && num < 4 ) {
		Input::bindingenum_t binding = Input::enumForName(argv[1]);
		StringBuf<32> input(argv[2]);
		for( int c=3; c<argc; ++c ) {
			input.appendf(" %s",argv[c]);
		}
		mainEngine->getInput(num).rebind(binding, input.get());
		return 0;
	} else {
		return 1;
	}
}

static Ccmd ccmd_bind("bind","bind an input to an action",&console_bind);
