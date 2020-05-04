//! @file Slider.hpp

#pragma once

#include "Main.hpp"
#include "String.hpp"
#include "Rect.hpp"
#include "WideVector.hpp"
#include "Script.hpp"

class Frame;
class Renderer;

//! a Slider lives in a frame and allows a user to select a range of values
class Slider {
public:
    Slider(Frame& _parent) :
        parent(&_parent)
        {}
    ~Slider() {}

    //! the result of the slider process
    struct result_t {
        bool highlighted;				//!< was highlighted this frame
        bool clicked;					//!< was modified this frame
        Uint32 highlightTime;			//!< time since slider was highlighted
        const char* tooltip = nullptr;	//!< slider tooltip to be displayed
    };

    //! draws the slider
    //! @param renderer the renderer object used to draw the slider
    //! @param _size size and position of slider's parent frame
    //! @param _actualSize offset into the parent frame space (scroll)
    void draw(Renderer& renderer, Rect<int> _size, Rect<int> _actualSize);

    //! handles slider clicks, etc.
    //! @param _size size and position of slider's parent frame
    //! @param _actualSize offset into the parent frame space (scroll)
    //! @param usable true if another object doesn't have the mouse's attention, false otherwise
    //! @return resultant state of the slider after processing
    result_t process(Rect<int> _size, Rect<int> _actualSize, const bool usable);

    const char*                 getName() const { return name.get(); }
    float                       getValue() const { return value; }
    float                       getMaxValue() const { return maxValue; }
    float                       getMinValue() const { return minValue; }
    int                         getBorder() const { return border; }
    const Rect<int>&            getHandleSize() const { return handleSize; }
    const Rect<int>&            getRailSize() const { return railSize; }
    const char*                 getTooltip() const { return tooltip.get(); }
    bool                        isPressed() const { return pressed; }
    bool                        isHighlighted() const { return highlighted; }
    bool                        isDisabled() const { return disabled; }
    const WideVector&           getColor() const { return color; }
    const Script::Function*     getCallback() const { return callback; }

    void    setName(const char* _name) { name = _name; }
    void    setValue(float _value) { value = _value; }
    void    setMaxValue(float _value) { maxValue = _value; }
    void    setMinValue(float _value) { minValue = _value; }
    void    setBorder(int _border) { border = _border; }
    void    setHandleSize(const Rect<int>& rect) { handleSize = rect; }
    void    setRailSize(const Rect<int>& rect) { railSize = rect; }
    void    setTooltip(const char* _tooltip) { tooltip = _tooltip; }
    void    setDisabled(bool _disabled) { disabled = _disabled; }
    void    setColor(const WideVector& _color) { color = _color; }
    void	setCallback(const Script::Function* fn) { callback = fn; }

private:
    Frame* parent = nullptr;                        //!< parent frame
    const Script::Function* callback = nullptr;		//!< native callback for clicking
    String name;                                    //!< internal name of the slider
    float value = 0.f;                              //!< value
    float maxValue = 0.f;                           //!< maximum value
    float minValue = 0.f;                           //!< minimum value
    int border = 3;                                 //!< border size in pixels
    Rect<int> handleSize;                           //!< size of the handle in pixels
    Rect<int> railSize;                             //!< size of the rail in pixels
    String tooltip;								    //!< if empty, button has no tooltip; otherwise, it does
    bool pressed = false;							//!< slider clicked state
    bool highlighted = false;						//!< true if mouse is hovering over slider; false otherwise
    bool disabled = false;							//!< if true, the slider is dimmed and unusable
    WideVector color;								//!< the slider's color
    Uint32 highlightTime = 0;						//!< records the time since the slider was highlighted
};