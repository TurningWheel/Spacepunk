// Input.hpp

#pragma once

#include "Main.hpp"
#include "String.hpp"
#include "Map.hpp"

class Input {
public:
	Input() {}
	~Input() {}

	enum bindingenum_t {
		INVALID,

		MOVE_FORWARD,
		MOVE_LEFT,
		MOVE_BACKWARD,
		MOVE_RIGHT,
		MOVE_UP,
		MOVE_DOWN,

		LOOK_UP,
		LOOK_LEFT,
		LOOK_DOWN,
		LOOK_RIGHT,

		LEAN_MODIFIER,
		LEAN_LEFT,
		LEAN_RIGHT,
			
		INTERACT,
		HAND_LEFT,
		HAND_RIGHT,

		INVENTORY1,
		INVENTORY2,
		INVENTORY3,
		INVENTORY4,
		DROP_MODIFIER,
		STATUS,

		MENU_UP,
		MENU_LEFT,
		MENU_DOWN,
		MENU_RIGHT,
		MENU_CONFIRM,
		MENU_CANCEL,
		MENU_TOGGLE,
		MENU_LOBBY,
		MENU_PAGE_LEFT,
		MENU_PAGE_RIGHT,

		BINDINGENUM_TYPE_LENGTH
	};
	static const char* bindingName[BINDINGENUM_TYPE_LENGTH];

	// input mapping
	struct binding_t {
		String input = "";
		float analog = 0.f;
		bool binary = false;
		bool consumed = false;

		// bind type
		enum bindtype_t {
			INVALID,
			KEYBOARD,
			CONTROLLER_AXIS,
			CONTROLLER_BUTTON,
			MOUSE_BUTTON,
			NUM
		};
		bindtype_t type = INVALID;

		// keyboard binding info
		SDL_Scancode scancode = SDL_Scancode::SDL_SCANCODE_UNKNOWN;

		// gamepad binding info
		SDL_GameController* pad = nullptr;
		SDL_GameControllerAxis padAxis = SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID;
		SDL_GameControllerButton padButton = SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID;
		bool padAxisNegative = false;

		// mouse button info
		int mouseButton = 0;
	};

	// gets the analog value of a particular input binding
	// @param binding the binding to query
	// @return the analog value (range = -1.f : +1.f)
	float analog(bindingenum_t binding) const;

	// gets the binary value of a particular input binding
	// @param binding the binding to query
	// @return the bool value (false = not pressed, true = pressed)
	bool binary(bindingenum_t binding) const;

	// gets the binary value of a particular input binding, if it's not been consumed
	// releasing the input and retriggering it "unconsumes"
	// @param binding the binding to query
	// @return the bool value (false = not pressed, true = pressed)
	bool binaryToggle(bindingenum_t binding) const;

	// @param binding the binding to flag consumed
	void consumeBinaryToggle(bindingenum_t binding);

	// gets the input mapped to a particular input binding
	// @param binding the binding to query
	// @return the input mapped to the given binding
	const char* binding(bindingenum_t binding) const;

	// rebind the given action to the given input
	// @param name the action to rebind
	// @param input the input to rebind to the action
	void rebind(bindingenum_t binding, const char* input);

	// find the binding with the given name
	// @param name the name of the binding
	// @return the binding enum
	static bindingenum_t enumForName(const char* name);

	// updates the state of all current bindings
	void update();

	// getters & setters
	const bool					isInverted() const			{ return inverted; }
	void						setInverted(bool b)			{ inverted = b; }

private:
	binding_t bindings[BINDINGENUM_TYPE_LENGTH];
	bool inverted = false;

	// converts the given input to a boolean value
	// @return the converted value
	static bool binaryOf(binding_t& binding);

	// converts the given input to a float value
	// @return the converted value
	static float analogOf(binding_t& binding);

	// map of scancodes to input names
	static Map<SDL_Scancode> scancodeNames;
	static SDL_Scancode getScancodeFromName(const char* name);
};
