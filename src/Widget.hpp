//! @file Widget.hpp

#pragma once

#include "Main.hpp"
#include "String.hpp"
#include "LinkedList.hpp"

class Frame;

class Widget {
public:
    Widget() = default;
    Widget(const Widget&) = delete;
    Widget(Widget&&) = delete;
    virtual ~Widget();

    Widget& operator=(const Widget&) = delete;
    Widget& operator=(Widget&&) = delete;

    //! widget type
    enum type_t {
        WIDGET_FRAME,
        WIDGET_BUTTON,
        WIDGET_FIELD,
        WIDGET_SLIDER
    };

    virtual type_t  getType() const = 0;
    const char*		getName() const { return name.get(); }
    bool			isPressed() const { return reallyPressed; }
    bool			isHighlighted() const { return selected | highlighted; }
    bool			isSelected() const { return selected; }
    bool			isDisabled() const { return disabled; }
    bool            isInvisible() const { return invisible; }
    Uint32          getHighlightTime() const { return highlightTime; }
    Sint32          getOwner() const { return owner; }
    const char*     getWidgetRight() const { return widgetRight.get(); }
    const char*     getWidgetDown() const { return widgetDown.get(); }
    const char*     getWidgetLeft() const { return widgetLeft.get(); }
    const char*     getWidgetUp() const { return widgetUp.get(); }
    const char*     getWidgetPageLeft() const { return widgetPageLeft.get(); }
    const char*     getWidgetPageRight() const { return widgetPageRight.get(); }
    const char*     getWidgetBack() const { return widgetBack.get(); }
    const char*     getWidgetSearchParent() const { return widgetSearchParent.get(); }
    const char*     getWidgetTab() const { return widgetTab.get(); }

    void	setName(const char* _name) { name = _name; }
    void	setPressed(bool _pressed) { reallyPressed = pressed = _pressed; }
    void    setSelected(bool _selected) { selected = _selected; }
    void	setDisabled(bool _disabled) { disabled = _disabled; }
    void    setInvisible(bool _invisible) { invisible = _invisible; }
    void    setOwner(Sint32 _owner) { owner = _owner; }
    void    setWidgetRight(const char* s) { widgetRight = s; }
    void    setWidgetDown(const char* s) { widgetDown = s; }
    void    setWidgetLeft(const char* s) { widgetLeft = s; }
    void    setWidgetUp(const char* s) { widgetUp = s; }
    void    setWidgetPageLeft(const char* s) { widgetPageLeft = s; }
    void    setWidgetPageRight(const char* s) { widgetPageRight = s; }
    void    setWidgetBack(const char* s) { widgetBack = s; }
    void    setWidgetSearchParent(const char* s) { widgetSearchParent = s; }
    void    setWidgetTab(const char* s) { widgetTab = s; }

    //! recursively locates the head widget for this widget
    //! @return the head widget, which may be this widget
    Widget* findHead();

    //! activate this widget
    virtual void activate();

    //! select this widget
    virtual void select();

    //! deselect this widget
    virtual void deselect();

    //! handle inputs on the widget
    //! @return the next widget to select, or nullptr if no widget was selected
    Widget* handleInput();

    //! adopt a new widget as one of our children
    //! @param widget the widget to adopt
    void adoptWidget(Widget& widget);

    //! find a widget amongst our children
    //! @param name the name of the widget to find
    //! @param recursive true to search recursively or not
    //! @return the widget found, or nullptr if it was not found
    Widget* findWidget(const char* name, bool recursive);

protected:
    Widget* parent = nullptr;                       //!< parent widget
    LinkedList<Widget*> widgets;                    //!< widget children
    String name;                                    //!< widget name
    bool pressed = false;							//!< pressed state
    bool reallyPressed = false;						//!< the "actual" pressed state, pre-mouse process
    bool highlighted = false;                       //!< if true, this widget has the mouse over it
    bool selected = false;							//!< if true, this widget has focus
    bool disabled = false;							//!< if true, the widget is unusable and grayed out
    bool invisible = false;                         //!< if true, widget is both unusable and invisible
    Uint32 highlightTime = 0u;						//!< records the time since the widget was highlighted
    Sint32 owner = 0;                               //!< which player owns this widget (0 = player 1, 1 = player 2, etc)

    String widgetSearchParent;                      //!< parent of widget to select (use to narrow search)
    String widgetRight;             				//!< next widget to select right
    String widgetDown;                              //!< next widget to select down
    String widgetLeft;                              //!< next widget to select left
    String widgetUp;                                //!< next widget to select up
    String widgetPageLeft;                          //!< widget to activate when you press MenuPageLeft
    String widgetPageRight;                         //!< widget to activate when you press MenuPageRight
    String widgetBack;                              //!< widget to activate when you press MenuCancel
    String widgetTab;                               //!< widget to select when you press tab

    Frame* findSearchRoot();
};