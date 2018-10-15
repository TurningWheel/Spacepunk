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
		bindings[binding].analog = analogOf(bindings[binding].input.get());
		bool obinary = bindings[binding].binary;
		bindings[binding].binary = binaryOf(bindings[binding].input.get());
		if ( obinary != bindings[binding].binary )
		{
			//Unconsume the input whenever it's released or pressed again.
			bindings[binding].consumed = false;
		}
	}
}

bool Input::binaryOf(const char* input) {
	if( input == nullptr )
		return false;

	size_t len = strlen(input);
	if( len >= 3 && strncmp(input, "Pad", 3) == 0 ) {
		// game controller

		char* type = nullptr;
		Uint32 index = strtol((const char*)(input+3), &type, 10);
		LinkedList<SDL_GameController*>& list = mainEngine->getControllers();
		Node<SDL_GameController*>* node = list.nodeForIndex(index);

		if( node ) {
			SDL_GameController* pad = node->getData();
			if( strncmp(type, "Button", 6) == 0 ) {
				if( strcmp((const char*)(type+6), "A") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A) == 1;
				}
				else if( strcmp((const char*)(type+6), "B") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B) == 1;
				}
				else if( strcmp((const char*)(type+6), "X") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_X) == 1;
				}
				else if( strcmp((const char*)(type+6), "Y") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_Y) == 1;
				}
				else if( strcmp((const char*)(type+6), "Back") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_BACK) == 1;
				}
				else if( strcmp((const char*)(type+6), "Start") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START) == 1;
				}
				else if( strcmp((const char*)(type+6), "LeftStick") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_LEFTSTICK) == 1;
				}
				else if( strcmp((const char*)(type+6), "RightStick") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSTICK) == 1;
				}
				else if( strcmp((const char*)(type+6), "LeftBumper") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) == 1;
				}
				else if( strcmp((const char*)(type+6), "RightBumper") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) == 1;
				}
			}
			else if( strncmp(type, "StickLeft", 9) == 0 ) {
				if( strcmp((const char*)(type+9), "X-") == 0 ) {
					return (bool)(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX) < -16384);
				}
				else if( strcmp((const char*)(type+9), "X+") == 0 ) {
					return (bool)(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX) > 16384);
				}
				else if( strcmp((const char*)(type+9), "Y-") == 0 ) {
					return (bool)(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY) < -16384);
				}
				else if( strcmp((const char*)(type+9), "Y+") == 0 ) {
					return (bool)(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY) > 16384);
				}
			}
			else if( strncmp(type, "StickRight", 10) == 0 ) {
				if( strcmp((const char*)(type+10), "X-") == 0 ) {
					return (bool)(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTX) < -16384);
				}
				else if( strcmp((const char*)(type+10), "X+") == 0 ) {
					return (bool)(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTX) > 16384);
				}
				else if( strcmp((const char*)(type+10), "Y-") == 0 ) {
					return (bool)(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTY) < -16384);
				}
				else if( strcmp((const char*)(type+10), "Y+") == 0 ) {
					return (bool)(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTY) > 16384);
				}
			}
			else if( strncmp(type, "Dpad", 4) == 0 ) {
				if( strcmp((const char*)(type+4), "X-") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT) == 1;
				}
				else if( strcmp((const char*)(type+4), "X+") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) == 1;
				}
				else if( strcmp((const char*)(type+4), "Y-") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP) == 1;
				}
				else if( strcmp((const char*)(type+4), "Y+") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN) == 1;
				}
			}
			else if( strncmp(type, "LeftTrigger", 11) == 0 ) {
				return (bool)(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 16384);
			}
			else if( strncmp(type, "RightTrigger", 12) == 0 ) {
				return (bool)(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 16384);
			}
		}
	}
	else if( len >= 5 && strncmp(input, "Mouse", 5) == 0 ) {
		// mouse
		char* button = nullptr;
		Uint32 index = strtol((const char*)(input+5), &button, 10);
		int result = min( max( 0, (int)strtol(button, nullptr, 10) ), 4 );
		return mainEngine->getMouseStatus(result);
	}
	else {
		// keyboard
		SDL_Scancode key = SDL_GetScancodeFromName(input);
		if( key != SDL_SCANCODE_UNKNOWN ) {
			return mainEngine->getKeyStatus((int)key);
		}
	}

	return false;
}

float Input::analogOf(const char* input) {
	if( input == nullptr )
		return 0.f;

	size_t len = strlen(input);
	if( len >= 3 && strncmp(input, "Pad", 3) == 0 ) {
		// game controller

		char* type = nullptr;
		Uint32 index = strtol((const char*)(input+3), &type, 10);
		LinkedList<SDL_GameController*>& list = mainEngine->getControllers();
		Node<SDL_GameController*>* node = list.nodeForIndex(index);

		if( node ) {
			SDL_GameController* pad = node->getData();
			if( strncmp(type, "Button", 6) == 0 ) {
				if( strcmp((const char*)(type+6), "A") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A) ? 1.f : 0.f;
				}
				else if( strcmp((const char*)(type+6), "B") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B) ? 1.f : 0.f;
				}
				else if( strcmp((const char*)(type+6), "X") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_X) ? 1.f : 0.f;
				}
				else if( strcmp((const char*)(type+6), "Y") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_Y) ? 1.f : 0.f;
				}
				else if( strcmp((const char*)(type+6), "Back") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_BACK) ? 1.f : 0.f;
				}
				else if( strcmp((const char*)(type+6), "Start") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START) ? 1.f : 0.f;
				}
				else if( strcmp((const char*)(type+6), "LeftStick") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_LEFTSTICK) ? 1.f : 0.f;
				}
				else if( strcmp((const char*)(type+6), "RightStick") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSTICK) ? 1.f : 0.f;
				}
				else if( strcmp((const char*)(type+6), "LeftBumper") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) ? 1.f : 0.f;
				}
				else if( strcmp((const char*)(type+6), "RightBumper") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) ? 1.f : 0.f;
				}
			}
			else if( strncmp(type, "StickLeft", 9) == 0 ) {
				if( strcmp((const char*)(type+9), "X-") == 0 ) {
					float result = min(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX) / 32768.f, 0.f) * -1.f;
					return (result > cvar_deadzone.toFloat()) ? result : 0.f;
				}
				else if( strcmp((const char*)(type+9), "X+") == 0 ) {
					float result = max(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX) / 32767.f, 0.f);
					return (result > cvar_deadzone.toFloat()) ? result : 0.f;
				}
				else if( strcmp((const char*)(type+9), "Y-") == 0 ) {
					float result = min(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY) / 32768.f, 0.f) * -1.f;
					return (result > cvar_deadzone.toFloat()) ? result : 0.f;
				}
				else if( strcmp((const char*)(type+9), "Y+") == 0 ) {
					float result = max(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY) / 32767.f, 0.f);
					return (result > cvar_deadzone.toFloat()) ? result : 0.f;
				}
			}
			else if( strncmp(type, "StickRight", 10) == 0 ) {
				if( strcmp((const char*)(type+10), "X-") == 0 ) {
					float result = min(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTX) / 32768.f, 0.f) * -1.f;
					return (result > cvar_deadzone.toFloat()) ? result : 0.f;
				}
				else if( strcmp((const char*)(type+10), "X+") == 0 ) {
					float result = max(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTX) / 32767.f, 0.f);
					return (result > cvar_deadzone.toFloat()) ? result : 0.f;
				}
				else if( strcmp((const char*)(type+10), "Y-") == 0 ) {
					float result = min(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTY) / 32768.f, 0.f) * -1.f;
					return (result > cvar_deadzone.toFloat()) ? result : 0.f;
				}
				else if( strcmp((const char*)(type+10), "Y+") == 0 ) {
					float result = max(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTY) / 32767.f, 0.f);
					return (result > cvar_deadzone.toFloat()) ? result : 0.f;
				}
			}
			else if( strncmp(type, "Dpad", 4) == 0 ) {
				if( strcmp((const char*)(type+4), "X-") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ? 1.f : 0.f;
				}
				else if( strcmp((const char*)(type+4), "X+") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ? 1.f : 0.f;
				}
				else if( strcmp((const char*)(type+4), "Y-") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP) ? 1.f : 0.f;
				}
				else if( strcmp((const char*)(type+4), "Y+") == 0 ) {
					return SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ? 1.f : 0.f;
				}
			}
			else if( strncmp(type, "LeftTrigger", 11) == 0 ) {
				return SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 32767.f;
			}
			else if( strncmp(type, "RightTrigger", 12) == 0 ) {
				return SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 32767.f;
			}
		}
	}
	else if( len >= 5 && strncmp(input, "Mouse", 5) == 0 ) {
		// mouse
		char* button = nullptr;
		Uint32 index = strtol((const char*)(input+5), &button, 10);
		int result = min( max( 0, (int)strtol(button, nullptr, 10) ), 4 );
		return mainEngine->getMouseStatus(result) ? 1.f : 0.f;
	}
	else {
		// keyboard
		SDL_Scancode key = SDL_GetScancodeFromName(input);
		if( key != SDL_SCANCODE_UNKNOWN ) {
			return mainEngine->getKeyStatus((int)key) ? 1.f : 0.f;
		}
	}

	return 0.f;
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
