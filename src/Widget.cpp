// Widget.cpp

#include "Widget.hpp"
#include "Engine.hpp"
#include "Frame.hpp"

Widget::~Widget() {
	if (parent) {
		for (auto node = parent->widgets.getFirst(); node != nullptr; node = node->getNext()) {
			if (node->getData() == this) {
				parent->widgets.removeNode(node);
				break;
			}
		}
	}
}

void Widget::select() {
	if (selected) {
		return;
	}
	Widget* head = findHead();
	if (head && head->getType() == WIDGET_FRAME) {
		Frame* f = static_cast<Frame*>(head);
		f->deselect(); // this deselects everything in the gui
	}
	selected = true;
}

void Widget::deselect() {
	selected = false;
}

void Widget::activate() {
	// no-op
}

Frame* Widget::findSearchRoot() {
	Widget* gui = findHead();
	if (gui && gui->getType() == WIDGET_FRAME) {
		if (widgetSearchParent.empty()) {
			return static_cast<Frame*>(gui);
		} else {
			auto search = gui->findWidget(widgetSearchParent.get(), true);
			if (search && search->getType() == WIDGET_FRAME) {
				return static_cast<Frame*>(search);
			} else {
				return static_cast<Frame*>(gui);
			}
		}
	} else {
		return nullptr;
	}
}

Widget* Widget::handleInput() {
	Widget* result = nullptr;
	while (selected) {
		Input& input = mainEngine->getInput(owner);

		// find search root
		Frame* root = nullptr;

		// tab to next element
		if (mainEngine->pressKey(SDL_SCANCODE_TAB)) {
			if (!widgetTab.empty()) {
				root = root ? root : findSearchRoot();
				result = root->findWidget(widgetTab.get(), true);
				if (result) {
					break;
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
				if (moves[c][1] && moves[c][1][0] != '\0') {
					root = root ? root : findSearchRoot();
					result = root->findWidget(moves[c][1], true);
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
				root = root ? root : findSearchRoot();
				result = root->findWidget(widgetPageRight.get(), true);
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
				root = root ? root : findSearchRoot();
				result = root->findWidget(widgetPageLeft.get(), true);
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
				root = root ? root : findSearchRoot();
				result = root->findWidget(widgetBack.get(), true);
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
    if (parent && parent->owner == owner) {
        return parent->findHead();
    } else {
        return this;
    }
}

Widget* Widget::findWidget(const char* name, bool recursive) {
	for (auto widget : widgets) {
		if (widget->owner != owner) {
			continue;
		}
		if (widget->name == name) {
			return widget;
		} else if (recursive) {
			auto result = widget->findWidget(name, recursive);
			if (result) {
				return result;
			}
		}
	}
	return nullptr;
}

void Widget::adoptWidget(Widget& widget) {
	if (widget.parent) {
		for (auto node = widget.parent->widgets.getFirst(); node != nullptr; node = node->getNext()) {
			if (node->getData() == &widget) {
				widget.parent->widgets.removeNode(node);
				break;
			}
		}
	}
	widget.parent = this;
	widgets.addNodeLast(&widget);
}
