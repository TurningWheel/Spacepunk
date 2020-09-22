// Widget.cpp

#include "Widget.hpp"
#include "Engine.hpp"

void Widget::select() {
	selected = true;
}

void Widget::deselect() {
	selected = false;
}

void Widget::activate() {
	// no-op
}

Widget* Widget::handleInput() {
	Widget* result = nullptr;
	while (selected) {
		Input& input = mainEngine->getInput(owner);
		Frame* gui = static_cast<Frame*>(findHead());

		// tab to next element
		if (mainEngine->pressKey(SDL_SCANCODE_TAB)) {
			if (!widgetTab.empty()) {
				if (widgetTabParent.empty()) {
					result = gui->findWidget(widgetTab.get(), true);
					if (result) {
						break;
					}
				} else {
					auto p = gui->findWidget(widgetTabParent.get(), true);
					if (p && p->getType() == WIDGET_FRAME) {
						auto f = static_cast<Frame*>(p);
						result = f->findWidget(widgetTab.get(), true);
						if (result) {
							break;
						}
					}
				}
			}
		}

		// directional move to next element
		const char* moves[4][2] = {
			{ "MenuRight", widgetRight.get() },
			{ "MenuDown", widgetDown.get() },
			{ "MenuLeft", widgetLeft.get() },
			{ "MenuUp", widgetUp.get() }
		};
		for (int c = 0; c < sizeof(moves) / sizeof(moves[0]); ++c) {
			if (input.binaryToggle(moves[c][0])) {
				if (moves[c][1] && moves[c][1] != '\0') {
					result = gui->findWidget(moves[c][1], true);
					if (result) {
						input.consumeBinaryToggle(moves[c][0]);
						break;
					}
				}
			}
		}

		// next tab
		if (input.binaryToggle("MenuPageRight")) {
			if (!widgetPageRight.empty()) {
				result = gui->findWidget(widgetPageRight.get(), true);
				if (result) {
					input.consumeBinaryToggle("MenuPageRight");
					result->activate();
					break;
				}
			}
		}

		// previous tab
		if (input.binaryToggle("MenuPageLeft")) {
			if (!widgetPageLeft.empty()) {
				result = gui->findWidget(widgetPageLeft.get(), true);
				if (result) {
					input.consumeBinaryToggle("MenuPageLeft");
					result->activate();
					break;
				}
			}
		}

		// confirm selection
		if (input.binaryToggle("MenuConfirm")) {
			input.consumeBinaryToggle("MenuConfirm");
			activate();
			break;
		}

		// cancel selection
		if (input.binaryToggle("MenuCancel")) {
			if (!widgetBack.empty()) {
				result = gui->findWidget(widgetBack.get(), true);
				if (result) {
					input.consumeBinaryToggle("MenuCancel");
					result->activate();
					break;
				}
			}
		}

		break;
	}
	return result;
}

Widget* Widget::findHead() {
    if (parent) {
        return parent->findHead();
    } else {
        return this;
    }
}