// Input.hpp

#pragma once

#include "Main.hpp"
#include "String.hpp"
#include "Map.hpp"

class Input {
public:
	Input() {}
	~Input() {}

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
	float analog(const char* binding) const;

	// gets the binary value of a particular input binding
	// @param binding the binding to query
	// @return the bool value (false = not pressed, true = pressed)
	bool binary(const char* binding) const;

	// gets the binary value of a particular input binding, if it's not been consumed
	// releasing the input and retriggering it "unconsumes"
	// @param binding the binding to query
	// @return the bool value (false = not pressed, true = pressed)
	bool binaryToggle(const char* binding) const;

	// @param binding the binding to flag consumed
	void consumeBinaryToggle(const char* binding);

	// gets the input mapped to a particular input binding
	// @param binding the binding to query
	// @return the input mapped to the given binding
	const char* binding(const char* binding) const;

	// rebind the given action to the given input
	// @param name the action to rebind
	// @param input the input to rebind to the action
	void rebind(const char* binding, const char* input);

	// refresh bindings (eg after a new controller is detected)
	void refresh();

	// updates the state of all current bindings
	void update();

	// getters & setters
	const bool					isInverted() const			{ return inverted; }
	void						setInverted(bool b)			{ inverted = b; }

private:
	Map<StringBuf<64>, binding_t> bindings;
	bool inverted = false;

	// converts the given input to a boolean value
	// @return the converted value
	static bool binaryOf(binding_t& binding);

	// converts the given input to a float value
	// @return the converted value
	static float analogOf(binding_t& binding);

	// map of scancodes to input names
	static Map<String, SDL_Scancode> scancodeNames;
	static SDL_Scancode getScancodeFromName(const char* name);
};
