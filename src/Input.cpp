// Input.cpp

#include "Main.hpp"
#include "Input.hpp"
#include "Engine.hpp"
#include "Console.hpp"

Cvar cvar_sensitivity("input.sensitivity", "look sensitivity", "1.0");
Cvar cvar_deadzone("input.deadzone", "controller stick dead-zone range", "0.2");

Map<String, SDL_Scancode> Input::scancodeNames;

float Input::analog(const char* binding) const {
	auto b = bindings[binding];
	return b ? b->analog : 0.f;
}

bool Input::binary(const char* binding) const {
	auto b = bindings[binding];
	return b ? b->binary : false;
}

bool Input::binaryToggle(const char* binding) const {
	auto b = bindings[binding];
	return b ? b->binary && !b->consumed : false;
}

void Input::consumeBinaryToggle(const char* binding) {
	auto b = bindings[binding];
	if (b && b->binary) {
		b->consumed = true;
	}
}

const char* Input::binding(const char* binding) const {
	auto b = bindings[binding];
	return b ? b->input.get() : "";
}

void Input::refresh() {
	for (auto& pair : bindings) {
		rebind(pair.a, pair.b.input.get());
	}
}

void Input::rebind(const char* binding, const char* input) {
	auto b = bindings[binding];
	if (!b) {
		bindings.insert(binding, binding_t());
		b = bindings[binding];
	}
	b->input.assign(input);
	if (input == nullptr) {
		b->type = binding_t::INVALID;
		return;
	}

	size_t len = strlen(input);
	if (len >= 3 && strncmp(input, "Pad", 3) == 0) {
		// game controller

		char* type = nullptr;
		Uint32 index = strtol((const char*)(input + 3), &type, 10);
		LinkedList<SDL_GameController*>& list = mainEngine->getControllers();
		Node<SDL_GameController*>* node = list.nodeForIndex(index);

		if (node) {
			b->pad = node->getData();
			SDL_GameController* pad = node->getData();
			if (strncmp(type, "Button", 6) == 0) {
				if (strcmp((const char*)(type + 6), "A") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_A;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "B") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_B;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "X") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_X;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "Y") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_Y;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "Back") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_BACK;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "Start") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_START;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "LeftStick") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_LEFTSTICK;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "RightStick") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "LeftBumper") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "RightBumper") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else {
					b->type = binding_t::INVALID;
					return;
				}
			} else if (strncmp(type, "StickLeft", 9) == 0) {
				if (strcmp((const char*)(type + 9), "X-") == 0) {
					b->padAxisNegative = true;
					b->padAxis = SDL_CONTROLLER_AXIS_LEFTX;
					b->type = binding_t::CONTROLLER_AXIS;
					return;
				} else if (strcmp((const char*)(type + 9), "X+") == 0) {
					b->padAxisNegative = false;
					b->padAxis = SDL_CONTROLLER_AXIS_LEFTX;
					b->type = binding_t::CONTROLLER_AXIS;
					return;
				} else if (strcmp((const char*)(type + 9), "Y-") == 0) {
					b->padAxisNegative = true;
					b->padAxis = SDL_CONTROLLER_AXIS_LEFTY;
					b->type = binding_t::CONTROLLER_AXIS;
					return;
				} else if (strcmp((const char*)(type + 9), "Y+") == 0) {
					b->padAxisNegative = false;
					b->padAxis = SDL_CONTROLLER_AXIS_LEFTY;
					b->type = binding_t::CONTROLLER_AXIS;
					return;
				} else {
					b->type = binding_t::INVALID;
					return;
				}
			} else if (strncmp(type, "StickRight", 10) == 0) {
				if (strcmp((const char*)(type + 10), "X-") == 0) {
					b->padAxisNegative = true;
					b->padAxis = SDL_CONTROLLER_AXIS_RIGHTX;
					b->type = binding_t::CONTROLLER_AXIS;
					return;
				} else if (strcmp((const char*)(type + 10), "X+") == 0) {
					b->padAxisNegative = false;
					b->padAxis = SDL_CONTROLLER_AXIS_RIGHTX;
					b->type = binding_t::CONTROLLER_AXIS;
					return;
				} else if (strcmp((const char*)(type + 10), "Y-") == 0) {
					b->padAxisNegative = true;
					b->padAxis = SDL_CONTROLLER_AXIS_RIGHTY;
					b->type = binding_t::CONTROLLER_AXIS;
					return;
				} else if (strcmp((const char*)(type + 10), "Y+") == 0) {
					b->padAxisNegative = false;
					b->padAxis = SDL_CONTROLLER_AXIS_RIGHTY;
					b->type = binding_t::CONTROLLER_AXIS;
					return;
				} else {
					b->type = binding_t::INVALID;
					return;
				}
			} else if (strncmp(type, "Dpad", 4) == 0) {
				if (strcmp((const char*)(type + 4), "X-") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 4), "X+") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 4), "Y-") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_DPAD_UP;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 4), "Y+") == 0) {
					b->padButton = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
					b->type = binding_t::CONTROLLER_BUTTON;
					return;
				} else {
					b->type = binding_t::INVALID;
					return;
				}
			} else if (strncmp(type, "LeftTrigger", 11) == 0) {
				b->padAxisNegative = false;
				b->padAxis = SDL_CONTROLLER_AXIS_TRIGGERLEFT;
				b->type = binding_t::CONTROLLER_AXIS;
				return;
			} else if (strncmp(type, "RightTrigger", 12) == 0) {
				b->padAxisNegative = false;
				b->padAxis = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
				b->type = binding_t::CONTROLLER_AXIS;
				return;
			} else {
				b->type = binding_t::INVALID;
				return;
			}
		} else {
			b->type = binding_t::INVALID;
			return;
		}
	} else if (len >= 3 && strncmp(input, "Joy", 3) == 0) {
		// joystick

		char* type = nullptr;
		Uint32 index = strtol((const char*)(input + 3), &type, 10);
		LinkedList<SDL_Joystick*>& list = mainEngine->getJoysticks();
		Node<SDL_Joystick*>* node = list.nodeForIndex(index);

		if (node) {
			b->joystick = node->getData();
			SDL_Joystick* joystick = node->getData();
			if (strncmp(type, "Button", 6) == 0) {
				b->type = binding_t::JOYSTICK_BUTTON;
				b->joystickButton = strtol((const char*)(type + 6), nullptr, 10);
				return;
			} else if (strncmp(type, "Axis-", 5) == 0) {
				b->type = binding_t::JOYSTICK_AXIS;
				b->joystickAxisNegative = true;
				b->joystickAxis = strtol((const char*)(type + 5), nullptr, 10);
				return;
			} else if (strncmp(type, "Axis+", 5) == 0) {
				b->type = binding_t::JOYSTICK_AXIS;
				b->joystickAxisNegative = false;
				b->joystickAxis = strtol((const char*)(type + 5), nullptr, 10);
				return;
			} else if (strncmp(type, "Hat", 3) == 0) {
				b->type = binding_t::JOYSTICK_HAT;
				b->joystickHat = strtol((const char*)(type + 3), nullptr, 10);
				if (type[3]) {
					if (strncmp((const char*)(type + 4), "LeftUp", 6) == 0) {
						b->joystickHatState = SDL_HAT_LEFTUP;
						return;
					} else if (strncmp((const char*)(type + 4), "Up", 2) == 0) {
						b->joystickHatState = SDL_HAT_UP;
						return;
					} else if (strncmp((const char*)(type + 4), "RightUp", 7) == 0) {
						b->joystickHatState = SDL_HAT_RIGHTUP;
						return;
					} else if (strncmp((const char*)(type + 4), "Right", 5) == 0) {
						b->joystickHatState = SDL_HAT_RIGHT;
						return;
					} else if (strncmp((const char*)(type + 4), "RightDown", 9) == 0) {
						b->joystickHatState = SDL_HAT_RIGHTDOWN;
						return;
					} else if (strncmp((const char*)(type + 4), "Down", 4) == 0) {
						b->joystickHatState = SDL_HAT_DOWN;
						return;
					} else if (strncmp((const char*)(type + 4), "LeftDown", 8) == 0) {
						b->joystickHatState = SDL_HAT_LEFTDOWN;
						return;
					} else if (strncmp((const char*)(type + 4), "Left", 4) == 0) {
						b->joystickHatState = SDL_HAT_LEFT;
						return;
					} else if (strncmp((const char*)(type + 4), "Centered", 8) == 0) {
						b->joystickHatState = SDL_HAT_CENTERED;
						return;
					} else {
						b->type = binding_t::INVALID;
						return;
					}
				}
			} else {
				b->type = binding_t::INVALID;
				return;
			}
		}

		return;
	} else if (len >= 5 && strncmp(input, "Mouse", 5) == 0) {
		// mouse
		Uint32 index = strtol((const char*)(input + 5), nullptr, 10);
		int result = min(max(0U, index), 4U);

		b->type = binding_t::MOUSE_BUTTON;
		b->mouseButton = result;
		return;
	} else {
		// keyboard
		b->type = binding_t::KEYBOARD;
		b->scancode = getScancodeFromName(input);
		return;
	}
}

void Input::update() {
	for (auto& pair : bindings) {
		auto& binding = pair.b;
		binding.analog = analogOf(binding);
		bool oldBinary = binding.binary;
		binding.binary = binaryOf(binding);
		if (oldBinary != binding.binary) {
			// unconsume the input whenever it's released or pressed again.
			binding.consumed = false;
		}
	}
}

bool Input::binaryOf(binding_t& binding) {
	if (binding.type == binding_t::CONTROLLER_AXIS ||
		binding.type == binding_t::CONTROLLER_BUTTON) {
		SDL_GameController* pad = binding.pad;
		if (binding.type == binding_t::CONTROLLER_BUTTON) {
			return SDL_GameControllerGetButton(pad, binding.padButton) == 1;
		} else {
			if (binding.padAxisNegative) {
				return SDL_GameControllerGetAxis(pad, binding.padAxis) < -16384;
			} else {
				return SDL_GameControllerGetAxis(pad, binding.padAxis) > 16384;
			}
		}
	} else if (
		binding.type == binding_t::JOYSTICK_AXIS ||
		binding.type == binding_t::JOYSTICK_BUTTON ||
		binding.type == binding_t::JOYSTICK_HAT) {
		SDL_Joystick* joystick = binding.joystick;
		if (binding.type == binding_t::JOYSTICK_BUTTON) {
			return SDL_JoystickGetButton(joystick, binding.joystickButton) == 1;
		} else if (binding.type == binding_t::JOYSTICK_AXIS) {
			if (binding.joystickAxisNegative) {
				return SDL_JoystickGetAxis(joystick, binding.joystickAxis) < -16384;
			} else {
				return SDL_JoystickGetAxis(joystick, binding.joystickAxis) > 16384;
			}
		} else {
			return SDL_JoystickGetHat(joystick, binding.joystickHat) == binding.joystickHatState;
		}
	} else if (binding.type == binding_t::MOUSE_BUTTON) {
		return mainEngine->getMouseStatus(binding.mouseButton);
	} else if (binding.type == binding_t::KEYBOARD) {
		SDL_Scancode key = binding.scancode;
		if (key != SDL_SCANCODE_UNKNOWN) {
			return mainEngine->getKeyStatus((int)key);
		}
	}

	return false;
}

float Input::analogOf(binding_t& binding) {
	if (binding.type == binding_t::CONTROLLER_AXIS ||
		binding.type == binding_t::CONTROLLER_BUTTON) {
		SDL_GameController* pad = binding.pad;
		if (binding.type == binding_t::CONTROLLER_BUTTON) {
			return SDL_GameControllerGetButton(pad, binding.padButton) ? 1.f : 0.f;
		} else {
			if (binding.padAxisNegative) {
				float result = min(SDL_GameControllerGetAxis(pad, binding.padAxis) / 32768.f, 0.f) * -1.f;
				return (fabs(result) > cvar_deadzone.toFloat()) ? result : 0.f;
			} else {
				float result = max(SDL_GameControllerGetAxis(pad, binding.padAxis) / 32767.f, 0.f);
				return (fabs(result) > cvar_deadzone.toFloat()) ? result : 0.f;
			}
		}
	} else if (
		binding.type == binding_t::JOYSTICK_AXIS ||
		binding.type == binding_t::JOYSTICK_BUTTON ||
		binding.type == binding_t::JOYSTICK_HAT) {
		SDL_Joystick* joystick = binding.joystick;
		if (binding.type == binding_t::JOYSTICK_BUTTON) {
			return SDL_JoystickGetButton(joystick, binding.joystickButton) ? 1.f : 0.f;
		} else if (binding.type == binding_t::JOYSTICK_AXIS) {
			if (binding.joystickAxisNegative) {
				float result = min(SDL_JoystickGetAxis(joystick, binding.joystickAxis) / 32768.f, 0.f) * -1.f;
				return (fabs(result) > cvar_deadzone.toFloat()) ? result : 0.f;
			} else {
				float result = max(SDL_JoystickGetAxis(joystick, binding.joystickAxis) / 32767.f, 0.f);
				return (fabs(result) > cvar_deadzone.toFloat()) ? result : 0.f;
			}
		} else {
			return SDL_JoystickGetHat(joystick, binding.joystickHat) == binding.joystickHatState ? 1.f : 0.f;
		}
	} else if (binding.type == binding_t::MOUSE_BUTTON) {
		return mainEngine->getMouseStatus(binding.mouseButton) ? 1.f : 0.f;
	} else if (binding.type == binding_t::KEYBOARD) {
		SDL_Scancode key = binding.scancode;
		if (key != SDL_SCANCODE_UNKNOWN) {
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
	if (argc < 3) {
		mainEngine->fmsg(Engine::MSG_ERROR, "Not enough args. ex: bind 0 MoveForward W");
		return 1;
	}
	int num = argv[0][0] - '0';
	if (num >= 0 && num < 4) {
		const char* binding = argv[1];
		StringBuf<32> input(argv[2]);
		for (int c = 3; c < argc; ++c) {
			input.appendf(" %s", argv[c]);
		}
		mainEngine->getInput(num).rebind(binding, input.get());
		return 0;
	} else {
		return 1;
	}
}

static Ccmd ccmd_bind("bind", "bind an input to an action", &console_bind);
