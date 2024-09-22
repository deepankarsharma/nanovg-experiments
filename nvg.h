#pragma once


//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//


/*
OUI - A minimal semi-immediate GUI handling & layouting library

Copyright (c) 2014 Leonard Ritter <leonard.ritter@duangle.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 */

/*
Blendish - Blender 2.5 UI based theming functions for NanoVG

Copyright (c) 2014 Leonard Ritter <leonard.ritter@duangle.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/



#include <stdbool.h>

/*
Order
. oui.h 
. nanovg.h
. nanovg_gl.h
. blendish.h
. perf.h
. demo.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <GLES3/gl32.h>

// FILE: oui.h





/*
Revision 4 (2014-12-17)

OUI (short for "Open UI", spoken like the french "oui" for "yes") is a
platform agnostic single-header C library for layouting GUI elements and
handling related user input. Together with a set of widget drawing and logic
routines it can be used to build complex user interfaces.

OUI is a semi-immediate GUI. Widget declarations are persistent for the duration
of the setup and evaluation, but do not need to be kept around longer than one
frame.

OUI has no widget types; instead, it provides only one kind of element, "Items",
which can be tailored to the application by the user and expanded with custom
buffers and event handlers to behave as containers, buttons, sliders, radio
buttons, and so on.

OUI also does not draw anything; Instead it provides a set of functions to
iterate and query the layouted items in order to allow client code to render
each widget with its current state using a preferred graphics library.

See example.cpp in the repository for a full usage example.

A basic setup for OUI usage in C looks like this:
=================================================

// a header for each widget
typedef struct Data {
    int type;
    UIhandler handler;
} Data;

/// global event dispatch
void ui_handler(int item, UIevent event) {
    Data *data = (Data *)uiGetHandle(item);
    if (data && data->handler) {
        data->handler(item, event);
    }
}

void app_main(...) {
    UIcontext *context = uiCreateContext(4096, 1<<20);
    uiMakeCurrent(context);
    uiSetHandler(ui_handler);

    while (app_running()) {
        // update position of mouse cursor; the ui can also be updated
        // from received events.
        uiSetCursor(app_get_mouse_x(), app_get_mouse_y());

        // update button state
        for (int i = 0; i < 3; ++i)
            uiSetButton(i, app_get_button_state(i));

        // you can also send keys and scroll events; see example.cpp for more

        // --------------
        // this section does not have to be regenerated on frame; a good
        // policy is to invalidate it on events, as this usually alters
        // structure and layout.

        // begin new UI declarations
        uiBeginLayout();

        // - UI setup code goes here -
        app_setup_ui();

        // layout UI
        uiEndLayout();

        // --------------

        // draw UI, starting with the first item, index 0
        app_draw_ui(render_context,0);

        // update states and fire handlers
        uiProcess(get_time_ms());
    }

    uiDestroyContext(context);
}

Here's an example setup for a checkbox control:
===============================================

typedef struct CheckBoxData {
    Data head;
    const char *label;
    bool *checked;
} CheckBoxData;

// called when the item is clicked (see checkbox())
void app_checkbox_handler(int item, UIevent event) {
    // retrieve custom data (see checkbox())
    CheckBoxData *data = (CheckBoxData *)uiGetHandle(item);

    switch(event) {
    default: break;
    case UI_BUTTON0_DOWN: {
        // toggle value
        *data->checked = !(*data->checked);
    } break;
    }
}

// creates a checkbox control for a pointer to a boolean
int checkbox(const char *label, bool *checked) {

    // create new ui item
    int item = uiItem();

    // set minimum size of wiget; horizontal size is dynamic, vertical is fixed
    uiSetSize(item, 0, APP_WIDGET_HEIGHT);

    // store some custom data with the checkbox that we use for rendering
    // and value changes.
    CheckBoxData *data = (CheckBoxData *)uiAllocHandle(item, sizeof(CheckBoxData));

    // assign a custom typeid to the data so the renderer knows how to
    // render this control, and our event handler
    data->head.type = APP_WIDGET_CHECKBOX;
    data->head.handler = app_checkbox_handler;
    data->label = label;
    data->checked = checked;

    // set to fire as soon as the left button is
    // pressed; UI_BUTTON0_HOT_UP is also a popular alternative.
    uiSetEvents(item, UI_BUTTON0_DOWN);

    return item;
}

A simple recursive drawing routine can look like this:
======================================================

void app_draw_ui(AppRenderContext *ctx, int item) {
    // retrieve custom data and cast it to Data; we assume the first member
    // of every widget data item to be a Data field.
    Data *head = (Data *)uiGetHandle(item);

    // if a handle is set, this is a specialized widget
    if (head) {
        // get the widgets absolute rectangle
        UIrect rect = uiGetRect(item);

        switch(head->type) {
            default: break;
            case APP_WIDGET_LABEL: {
                // ...
            } break;
            case APP_WIDGET_BUTTON: {
                // ...
            } break;
            case APP_WIDGET_CHECKBOX: {
                // cast to the full data type
                CheckBoxData *data = (CheckBoxData*)head;

                // get the widgets current state
                int state = uiGetState(item);

                // if the value is set, the state is always active
                if (*data->checked)
                    state = UI_ACTIVE;

                // draw the checkbox
                app_draw_checkbox(ctx, rect, state, data->label);
            } break;
        }
    }

    // iterate through all children and draw
    int kid = uiFirstChild(item);
    while (kid != -1) {
        app_draw_ui(ctx, kid);
        kid = uiNextSibling(kid);
    }
}

Layouting items works like this:
================================

void layout_window(int w, int h) {
    // create root item; the first item always has index 0
    int parent = uiItem();
    // assign fixed size
    uiSetSize(parent, w, h);

    // create column box and use as new parent
    parent = uiInsert(parent, uiItem());
    // configure as column
    uiSetBox(parent, UI_COLUMN);
    // span horizontally, attach to top
    uiSetLayout(parent, UI_HFILL | UI_TOP);

    // add a label - we're assuming custom control functions to exist
    int item = uiInsert(parent, label("Hello World"));
    // set a fixed height for the label
    uiSetSize(item, 0, APP_WIDGET_HEIGHT);
    // span the label horizontally
    uiSetLayout(item, UI_HFILL);

    static bool checked = false;

    // add a checkbox to the same parent as item; this is faster than
    // calling uiInsert on the same parent repeatedly.
    item = uiAppend(item, checkbox("Checked:", &checked));
    // set a fixed height for the checkbox
    uiSetSize(item, 0, APP_WIDGET_HEIGHT);
    // span the checkbox in the same way as the label
    uiSetLayout(item, UI_HFILL);
}



 */

// you can override this from the outside to pick
// the export level you need
#ifndef OUI_EXPORT
#define OUI_EXPORT
#endif

// some language bindings (e.g. terra) have no good support
// for unions or unnamed structs;
// #define OUI_USE_UNION_VECTORS 0 to disable.
#ifndef OUI_USE_UNION_VECTORS
#define OUI_USE_UNION_VECTORS 1
#endif

// limits

enum {
    // maximum size in bytes of a single data buffer passed to uiAllocData().
    UI_MAX_DATASIZE = 4096,
    // maximum depth of nested containers
    UI_MAX_DEPTH = 64,
    // maximum number of buffered input events
    UI_MAX_INPUT_EVENTS = 64,
    // consecutive click threshold in ms
    UI_CLICK_THRESHOLD = 250,
};

typedef unsigned int UIuint;

// opaque UI context
typedef struct UIcontext UIcontext;

// item states as returned by uiGetState()

typedef enum UIitemState {
    // the item is inactive
    UI_COLD = 0,
    // the item is inactive, but the cursor is hovering over this item
    UI_HOT = 1,
    // the item is toggled, activated, focused (depends on item kind)
    UI_ACTIVE = 2,
    // the item is unresponsive
    UI_FROZEN = 3,
} UIitemState;

// container flags to pass to uiSetBox()
typedef enum UIboxFlags {
    // flex-direction (bit 0+1)

    // left to right
    UI_ROW = 0x002,
    // top to bottom
    UI_COLUMN = 0x003,

    // model (bit 1)

    // free layout
    UI_LAYOUT = 0x000,
    // flex model
    UI_FLEX = 0x002,

    // flex-wrap (bit 2)

    // single-line
    UI_NOWRAP = 0x000,
    // multi-line, wrap left to right
    UI_WRAP = 0x004,


    // justify-content (start, end, center, space-between)
    // at start of row/column
    UI_START = 0x008,
    // at center of row/column
    UI_MIDDLE = 0x000,
    // at end of row/column
    UI_END = 0x010,
    // insert spacing to stretch across whole row/column
    UI_JUSTIFY = 0x018,

    // align-items
    // can be implemented by putting a flex container in a layout container,
    // then using UI_TOP, UI_DOWN, UI_VFILL, UI_VCENTER, etc.
    // FILL is equivalent to stretch/grow

    // align-content (start, end, center, stretch)
    // can be implemented by putting a flex container in a layout container,
    // then using UI_TOP, UI_DOWN, UI_VFILL, UI_VCENTER, etc.
    // FILL is equivalent to stretch; space-between is not supported.
} UIboxFlags;

// child layout flags to pass to uiSetLayout()
typedef enum UIlayoutFlags {
    // attachments (bit 5-8)
    // fully valid when parent uses UI_LAYOUT model
    // partially valid when in UI_FLEX model

    // anchor to left item or left side of parent
    UI_LEFT = 0x020,
    // anchor to top item or top side of parent
    UI_TOP = 0x040,
    // anchor to right item or right side of parent
    UI_RIGHT = 0x080,
    // anchor to bottom item or bottom side of parent
    UI_DOWN = 0x100,
    // anchor to both left and right item or parent borders
    UI_HFILL = 0x0a0,
    // anchor to both top and bottom item or parent borders
    UI_VFILL = 0x140,
    // center horizontally, with left margin as offset
    UI_HCENTER = 0x000,
    // center vertically, with top margin as offset
    UI_VCENTER = 0x000,
    // center in both directions, with left/top margin as offset
    UI_CENTER = 0x000,
    // anchor to all four directions
    UI_FILL = 0x1e0,
    // when wrapping, put this element on a new line
    // wrapping layout code auto-inserts UI_BREAK flags,
    // drawing routines can read them with uiGetLayout()
    UI_BREAK = 0x200
} UIlayoutFlags;

// event flags
typedef enum UIevent {
    // on button 0 down
    UI_BUTTON0_DOWN = 0x0400,
    // on button 0 up
    // when this event has a handler, uiGetState() will return UI_ACTIVE as
    // long as button 0 is down.
    UI_BUTTON0_UP = 0x0800,
    // on button 0 up while item is hovered
    // when this event has a handler, uiGetState() will return UI_ACTIVE
    // when the cursor is hovering the items rectangle; this is the
    // behavior expected for buttons.
    UI_BUTTON0_HOT_UP = 0x1000,
    // item is being captured (button 0 constantly pressed);
    // when this event has a handler, uiGetState() will return UI_ACTIVE as
    // long as button 0 is down.
    UI_BUTTON0_CAPTURE = 0x2000,
    // on button 2 down (right mouse button, usually triggers context menu)
    UI_BUTTON2_DOWN = 0x4000,
    // item has received a scrollwheel event
    // the accumulated wheel offset can be queried with uiGetScroll()
    UI_SCROLL = 0x8000,
    // item is focused and has received a key-down event
    // the respective key can be queried using uiGetKey() and uiGetModifier()
    UI_KEY_DOWN = 0x10000,
    // item is focused and has received a key-up event
    // the respective key can be queried using uiGetKey() and uiGetModifier()
    UI_KEY_UP = 0x20000,
    // item is focused and has received a character event
    // the respective character can be queried using uiGetKey()
    UI_CHAR = 0x40000,
} UIevent;

enum {
    // these bits, starting at bit 24, can be safely assigned by the
    // application, e.g. as item types, other event types, drop targets, etc.
    // they can be set and queried using uiSetFlags() and uiGetFlags()
    UI_USERMASK = 0xff000000,

    // a special mask passed to uiFindItem()
    UI_ANY = 0xffffffff,
};

// handler callback; event is one of UI_EVENT_*
typedef void (*UIhandler)(UIcontext* ui_context, int item, UIevent event);

// for cursor positions, mainly
typedef struct UIvec2 {
#if OUI_USE_UNION_VECTORS
    union {
        int v[2];
        struct { int x, y; };
    };
#else
    int x, y;
#endif
} UIvec2;

// layout rectangle
typedef struct UIrect {
#if OUI_USE_UNION_VECTORS
    union {
        int v[4];
        struct { int x, y, w, h; };
    };
#else
    int x, y, w, h;
#endif
} UIrect;

// unless declared otherwise, all operations have the complexity O(1).

// Context Management
// ------------------

// create a new UI context; call uiMakeCurrent() to make this context the
// current context. The context is managed by the client and must be released
// using uiDestroyContext()
// item_capacity is the maximum of number of items that can be declared.
// buffer_capacity is the maximum total size of bytes that can be allocated
// using uiAllocHandle(); you may pass 0 if you don't need to allocate
// handles.
// 4096 and (1<<20) are good starting values.
OUI_EXPORT UIcontext *uiCreateContext(
        unsigned int item_capacity,
        unsigned int buffer_capacity);

// release the memory of an UI context created with uiCreateContext(); if the
// context is the current context, the current context will be set to NULL
OUI_EXPORT void uiDestroyContext(UIcontext *ctx);

// User Data
OUI_EXPORT void uiSetContextHandle(UIcontext *ui_context, void *handle);
OUI_EXPORT void *uiGetContextHandle(UIcontext *ui_context);

// Input Control
// -------------

// sets the current cursor position (usually belonging to a mouse) to the
// screen coordinates at (x,y)
OUI_EXPORT void uiSetCursor(UIcontext *ui_context, int x, int y);

// returns the current cursor position in screen coordinates as set by
// uiSetCursor()
OUI_EXPORT UIvec2 uiGetCursor(UIcontext *ui_context);

// returns the offset of the cursor relative to the last call to uiProcess()
OUI_EXPORT UIvec2 uiGetCursorDelta(UIcontext *ui_context);

// returns the beginning point of a drag operation.
OUI_EXPORT UIvec2 uiGetCursorStart(UIcontext *ui_context);

// returns the offset of the cursor relative to the beginning point of a drag
// operation.
OUI_EXPORT UIvec2 uiGetCursorStartDelta(UIcontext *ui_context);

// sets a mouse or gamepad button as pressed/released
// button is in the range 0..63 and maps to an application defined input
// source.
// mod is an application defined set of flags for modifier keys
// enabled is 1 for pressed, 0 for released
OUI_EXPORT void uiSetButton(UIcontext *ui_context, unsigned int button, unsigned int mod, bool enabled);

// returns the current state of an application dependent input button
// as set by uiSetButton().
// the function returns 1 if the button has been set to pressed, 0 for released.
OUI_EXPORT int uiGetButton(UIcontext *ui_context, unsigned int button);

// returns the number of chained clicks; 1 is a single click,
// 2 is a double click, etc.
OUI_EXPORT int uiGetClicks(UIcontext *ui_context);

// sets a key as down/up; the key can be any application defined keycode
// mod is an application defined set of flags for modifier keys
// enabled is 1 for key down, 0 for key up
// all key events are being buffered until the next call to uiProcess()
OUI_EXPORT void uiSetKey(UIcontext *ui_context, unsigned int key, unsigned int mod, bool enabled);

// sends a single character for text input; the character is usually in the
// unicode range, but can be application defined.
// all char events are being buffered until the next call to uiProcess()
OUI_EXPORT void uiSetChar(UIcontext *ui_context, unsigned int value);

// accumulates scroll wheel offsets for the current frame
// all offsets are being accumulated until the next call to uiProcess()
OUI_EXPORT void uiSetScroll(UIcontext *ui_context, int x, int y);

// returns the currently accumulated scroll wheel offsets for this frame
OUI_EXPORT UIvec2 uiGetScroll(UIcontext *ui_context);





// Stages
// ------

// clear the item buffer; uiBeginLayout() should be called before the first
// UI declaration for this frame to avoid concatenation of the same UI multiple
// times.
// After the call, all previously declared item IDs are invalid, and all
// application dependent context data has been freed.
// uiBeginLayout() must be followed by uiEndLayout().
OUI_EXPORT void uiBeginLayout(UIcontext *ui_context);

// layout all added items starting from the root item 0.
// after calling uiEndLayout(), no further modifications to the item tree should
// be done until the next call to uiBeginLayout().
// It is safe to immediately draw the items after a call to uiEndLayout().
// this is an O(N) operation for N = number of declared items.
OUI_EXPORT void uiEndLayout(UIcontext *ui_context);

// update the current hot item; this only needs to be called if items are kept
// for more than one frame and uiEndLayout() is not called
OUI_EXPORT void uiUpdateHotItem(UIcontext *ui_context);

// update the internal state according to the current cursor position and
// button states, and call all registered handlers.
// timestamp is the time in milliseconds relative to the last call to uiProcess()
// and is used to estimate the threshold for double-clicks
// after calling uiProcess(), no further modifications to the item tree should
// be done until the next call to uiBeginLayout().
// Items should be drawn before a call to uiProcess()
// this is an O(N) operation for N = number of declared items.
OUI_EXPORT void uiProcess(UIcontext *ui_context, int timestamp);

// reset the currently stored hot/active etc. handles; this should be called when
// a re-declaration of the UI changes the item indices, to avoid state
// related glitches because item identities have changed.
OUI_EXPORT void uiClearState(UIcontext *ui_context);

// UI Declaration
// --------------

// create a new UI item and return the new items ID.
OUI_EXPORT int uiItem(UIcontext *ui_context);

// set an items state to frozen; the UI will not recurse into frozen items
// when searching for hot or active items; subsequently, frozen items and
// their child items will not cause mouse event notifications.
// The frozen state is not applied recursively; uiGetState() will report
// UI_COLD for child items. Upon encountering a frozen item, the drawing
// routine needs to handle rendering of child items appropriately.
// see example.cpp for a demonstration.
OUI_EXPORT void uiSetFrozen(UIcontext *ui_context, int item, bool enable);

// set the application-dependent handle of an item.
// handle is an application defined 64-bit handle. If handle is NULL, the item
// will not be interactive.
OUI_EXPORT void uiSetHandle(UIcontext *ui_context, int item, void *handle);

// allocate space for application-dependent context data and assign it
// as the handle to the item.
// The memory of the pointer is managed by the UI context and released
// upon the next call to uiBeginLayout()
OUI_EXPORT void *uiAllocHandle(UIcontext *ui_context, int item, unsigned int size);

// set the global handler callback for interactive items.
// the handler will be called for each item whose event flags are set using
// uiSetEvents.
OUI_EXPORT void uiSetHandler(UIcontext *ui_context, UIhandler handler);

// flags is a combination of UI_EVENT_* and designates for which events the
// handler should be called.
OUI_EXPORT void uiSetEvents(UIcontext *ui_context, int item, unsigned int flags);

// flags is a user-defined set of flags defined by UI_USERMASK.
OUI_EXPORT void uiSetFlags(UIcontext *ui_context, int item, unsigned int flags);

// assign an item to a container.
// an item ID of 0 refers to the root item.
// the function returns the child item ID
// if the container has already added items, the function searches
// for the last item and calls uiAppend() on it, which is an
// O(N) operation for N siblings.
// it is usually more efficient to call uiInsert() for the first child,
// then chain additional siblings using uiAppend().
OUI_EXPORT int uiInsert(UIcontext *ui_context, int item, int child);

// assign an item to the same container as another item
// sibling is inserted after item.
OUI_EXPORT int uiAppend(UIcontext *ui_context, int item, int sibling);

// insert child into container item like uiInsert(), but prepend
// it to the first child item, effectively putting it in
// the background.
// it is efficient to call uiInsertBack() repeatedly
// in cases where drawing or layout order doesn't matter.
OUI_EXPORT int uiInsertBack(UIcontext *ui_context, int item, int child);

// same as uiInsert()
OUI_EXPORT int uiInsertFront(UIcontext *ui_context, int item, int child);

// set the size of the item; a size of 0 indicates the dimension to be
// dynamic; if the size is set, the item can not expand beyond that size.
OUI_EXPORT void uiSetSize(UIcontext *ui_context, int item, int w, int h);

// set the anchoring behavior of the item to one or multiple UIlayoutFlags
OUI_EXPORT void uiSetLayout(UIcontext *ui_context, int item, unsigned int flags);

// set the box model behavior of the item to one or multiple UIboxFlags
OUI_EXPORT void uiSetBox(UIcontext *ui_context, int item, unsigned int flags);

// set the left, top, right and bottom margins of an item; when the item is
// anchored to the parent or another item, the margin controls the distance
// from the neighboring element.
OUI_EXPORT void uiSetMargins(UIcontext *ui_context, int item, short l, short t, short r, short b);

// set item as recipient of all keyboard events; if item is -1, no item will
// be focused.
OUI_EXPORT void uiFocus(UIcontext *ui_context, int item);

// Iteration
// ---------

// returns the first child item of a container item. If the item is not
// a container or does not contain any items, -1 is returned.
// if item is 0, the first child item of the root item will be returned.
OUI_EXPORT int uiFirstChild(UIcontext *ui_context, int item);

// returns an items next sibling in the list of the parent containers children.
// if item is 0 or the item is the last child item, -1 will be returned.
OUI_EXPORT int uiNextSibling(UIcontext *ui_context, int item);

// Querying
// --------

// return the total number of allocated items
OUI_EXPORT int uiGetItemCount(UIcontext *ui_context);

// return the total bytes that have been allocated by uiAllocHandle()
OUI_EXPORT unsigned int uiGetAllocSize(UIcontext *ui_context);

// return the current state of the item. This state is only valid after
// a call to uiProcess().
// The returned value is one of UI_COLD, UI_HOT, UI_ACTIVE, UI_FROZEN.
OUI_EXPORT UIitemState uiGetState(UIcontext *ui_context, int item);

// return the application-dependent handle of the item as passed to uiSetHandle()
// or uiAllocHandle().
OUI_EXPORT void *uiGetHandle(UIcontext *ui_context, int item);

// return the item that is currently under the cursor or -1 for none
OUI_EXPORT int uiGetHotItem(UIcontext *ui_context);

// return the item that is currently focused or -1 for none
OUI_EXPORT int uiGetFocusedItem(UIcontext *ui_context);

// returns the topmost item containing absolute location (x,y), starting with
// item as parent, using a set of flags and masks as filter:
// if both flags and mask are UI_ANY, the first topmost item is returned.
// if mask is UI_ANY, the first topmost item matching *any* of flags is returned.
// otherwise the first item matching (item.flags & flags) == mask is returned.
// you may combine box, layout, event and user flags.
// frozen items will always be ignored.
OUI_EXPORT int uiFindItem(UIcontext *ui_context, int item, int x, int y,
        unsigned int flags, unsigned int mask);

// return the handler callback as passed to uiSetHandler()
OUI_EXPORT UIhandler uiGetHandler(UIcontext *ui_context);
// return the event flags for an item as passed to uiSetEvents()
OUI_EXPORT unsigned int uiGetEvents(UIcontext *ui_context, int item);
// return the user-defined flags for an item as passed to uiSetFlags()
OUI_EXPORT unsigned int uiGetFlags(UIcontext *ui_context, int item);

// when handling a KEY_DOWN/KEY_UP event: the key that triggered this event
OUI_EXPORT unsigned int uiGetKey(UIcontext *ui_context);
// when handling a keyboard or mouse event: the active modifier keys
OUI_EXPORT unsigned int uiGetModifier(UIcontext *ui_context);

// returns the items layout rectangle in absolute coordinates. If
// uiGetRect() is called before uiEndLayout(), the values of the returned
// rectangle are undefined.
OUI_EXPORT UIrect uiGetRect(UIcontext *ui_context, int item);

// returns 1 if an items absolute rectangle contains a given coordinate
// otherwise 0
OUI_EXPORT int uiContains(UIcontext *ui_context, int item, int x, int y);

// return the width of the item as set by uiSetSize()
OUI_EXPORT int uiGetWidth(UIcontext *ui_context, int item);
// return the height of the item as set by uiSetSize()
OUI_EXPORT int uiGetHeight(UIcontext *ui_context, int item);

// return the anchoring behavior as set by uiSetLayout()
OUI_EXPORT unsigned int uiGetLayout(UIcontext *ui_context, int item);
// return the box model as set by uiSetBox()
OUI_EXPORT unsigned int uiGetBox(UIcontext *ui_context, int item);

// return the left margin of the item as set with uiSetMargins()
OUI_EXPORT short uiGetMarginLeft(UIcontext *ui_context, int item);
// return the top margin of the item as set with uiSetMargins()
OUI_EXPORT short uiGetMarginTop(UIcontext *ui_context, int item);
// return the right margin of the item as set with uiSetMargins()
OUI_EXPORT short uiGetMarginRight(UIcontext *ui_context, int item);
// return the bottom margin of the item as set with uiSetMargins()
OUI_EXPORT short uiGetMarginDown(UIcontext *ui_context, int item);

// when uiBeginLayout() is called, the most recently declared items are retained.
// when uiEndLayout() completes, it matches the old item hierarchy to the new one
// and attempts to map old items to new items as well as possible.
// when passed an item Id from the previous frame, uiRecoverItem() returns the
// items new assumed Id, or -1 if the item could not be mapped.
// it is valid to pass -1 as item.
OUI_EXPORT int uiRecoverItem(UIcontext *ui_context, int olditem);

// in cases where it is important to recover old state over changes in
// the view, and the built-in remapping fails, the UI declaration can manually
// remap old items to new IDs in cases where e.g. the previous item ID has been
// temporarily saved; uiRemapItem() would then be called after creating the
// new item using uiItem().
OUI_EXPORT void uiRemapItem(UIcontext *ui_context, int olditem, int newitem);

// returns the number if items that have been allocated in the last frame
OUI_EXPORT int uiGetLastItemCount(UIcontext *ui_context);

enum {
    // extra item flags

    // bit 0-2
    UI_ITEM_BOX_MODEL_MASK = 0x000007,
    // bit 0-4
    UI_ITEM_BOX_MASK       = 0x00001F,
    // bit 5-8
    UI_ITEM_LAYOUT_MASK    = 0x0003E0,
    // bit 9-18
    UI_ITEM_EVENT_MASK     = 0x07FC00,
    // item is frozen (bit 19)
    UI_ITEM_FROZEN         = 0x080000,
    // item handle is pointer to data (bit 20)
    UI_ITEM_DATA           = 0x100000,
    // item has been inserted (bit 21)
    UI_ITEM_INSERTED       = 0x200000,
    // horizontal size has been explicitly set (bit 22)
    UI_ITEM_HFIXED         = 0x400000,
    // vertical size has been explicitly set (bit 23)
    UI_ITEM_VFIXED         = 0x800000,
    // bit 22-23
    UI_ITEM_FIXED_MASK     = 0xC00000,

    // which flag bits will be compared
    UI_ITEM_COMPARE_MASK = UI_ITEM_BOX_MODEL_MASK
        | (UI_ITEM_LAYOUT_MASK & ~UI_BREAK)
        | UI_ITEM_EVENT_MASK
        | UI_USERMASK,
};

typedef struct UIitem {
    // data handle
    void *handle;

    // about 27 bits worth of flags
    unsigned int flags;

    // index of first kid
    // if old item: index of equivalent new item
    int firstkid;
    // index of next sibling with same parent
    int nextitem;

    // margin offsets, interpretation depends on flags
    // after layouting, the first two components are absolute coordinates
    short margins[4];
    // size
    short size[2];
} UIitem;

typedef enum UIstate {
    UI_STATE_IDLE = 0,
    UI_STATE_CAPTURE,
} UIstate;

typedef enum UIstage {
    UI_STAGE_LAYOUT = 0,
    UI_STAGE_POST_LAYOUT,
    UI_STAGE_PROCESS,
} UIstage;

typedef struct UIhandleEntry {
    unsigned int key;
    int item;
} UIhandleEntry;

typedef struct UIinputEvent {
    unsigned int key;
    unsigned int mod;
    UIevent event;
} UIinputEvent;

struct UIcontext {
    unsigned int item_capacity;
    unsigned int buffer_capacity;

    // handler
    UIhandler handler;
    // User data
    void *handle;

    // button state in this frame
    unsigned long long buttons;
    // button state in the previous frame
    unsigned long long last_buttons;

    // where the cursor was at the beginning of the active state
    UIvec2 start_cursor;
    // where the cursor was last frame
    UIvec2 last_cursor;
    // where the cursor is currently
    UIvec2 cursor;
    // accumulated scroll wheel offsets
    UIvec2 scroll;

    int active_item;
    int focus_item;
    int last_hot_item;
    int last_click_item;
    int hot_item;

    UIstate state;
    UIstage stage;
    unsigned int active_key;
    unsigned int active_modifier;
    unsigned int active_button_modifier;
    int last_timestamp;
    int last_click_timestamp;
    int clicks;

    int count;
    int last_count;
    int eventcount;
    unsigned int datasize;

    UIitem *items;
    unsigned char *data;
    UIitem *last_items;
    int *item_map;
    UIinputEvent events[UI_MAX_INPUT_EVENTS];
};




// FILE: nanovg.h
//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#define NVG_PI 3.14159265358979323846264338327f

typedef struct NVGcontext NVGcontext;

struct NVGcolor {
	union {
		float rgba[4];
		struct {
			float r,g,b,a;
		};
	};
};
typedef struct NVGcolor NVGcolor;

struct NVGpaint {
	float xform[6];
	float extent[2];
	float radius;
	float feather;
	NVGcolor innerColor;
	NVGcolor outerColor;
	int image;
};
typedef struct NVGpaint NVGpaint;

enum NVGwinding {
	NVG_CCW = 1,			// Winding for solid shapes
	NVG_CW = 2,				// Winding for holes
};

enum NVGsolidity {
	NVG_SOLID = 1,			// CCW
	NVG_HOLE = 2,			// CW
};

enum NVGlineCap {
	NVG_BUTT,
	NVG_ROUND,
	NVG_SQUARE,
	NVG_BEVEL,
	NVG_MITER,
};

enum NVGalign {
	// Horizontal align
	NVG_ALIGN_LEFT 		= 1<<0,	// Default, align text horizontally to left.
	NVG_ALIGN_CENTER 	= 1<<1,	// Align text horizontally to center.
	NVG_ALIGN_RIGHT 	= 1<<2,	// Align text horizontally to right.
	// Vertical align
	NVG_ALIGN_TOP 		= 1<<3,	// Align text vertically to top.
	NVG_ALIGN_MIDDLE	= 1<<4,	// Align text vertically to middle.
	NVG_ALIGN_BOTTOM	= 1<<5,	// Align text vertically to bottom.
	NVG_ALIGN_BASELINE	= 1<<6, // Default, align text vertically to baseline.
};

enum NVGblendFactor {
	NVG_ZERO = 1<<0,
	NVG_ONE = 1<<1,
	NVG_SRC_COLOR = 1<<2,
	NVG_ONE_MINUS_SRC_COLOR = 1<<3,
	NVG_DST_COLOR = 1<<4,
	NVG_ONE_MINUS_DST_COLOR = 1<<5,
	NVG_SRC_ALPHA = 1<<6,
	NVG_ONE_MINUS_SRC_ALPHA = 1<<7,
	NVG_DST_ALPHA = 1<<8,
	NVG_ONE_MINUS_DST_ALPHA = 1<<9,
	NVG_SRC_ALPHA_SATURATE = 1<<10,
};

enum NVGcompositeOperation {
	NVG_SOURCE_OVER,
	NVG_SOURCE_IN,
	NVG_SOURCE_OUT,
	NVG_ATOP,
	NVG_DESTINATION_OVER,
	NVG_DESTINATION_IN,
	NVG_DESTINATION_OUT,
	NVG_DESTINATION_ATOP,
	NVG_LIGHTER,
	NVG_COPY,
	NVG_XOR,
};

struct NVGcompositeOperationState {
	int srcRGB;
	int dstRGB;
	int srcAlpha;
	int dstAlpha;
};
typedef struct NVGcompositeOperationState NVGcompositeOperationState;

struct NVGglyphPosition {
	const char* str;	// Position of the glyph in the input string.
	float x;			// The x-coordinate of the logical glyph position.
	float minx, maxx;	// The bounds of the glyph shape.
};
typedef struct NVGglyphPosition NVGglyphPosition;

struct NVGtextRow {
	const char* start;	// Pointer to the input text where the row starts.
	const char* end;	// Pointer to the input text where the row ends (one past the last character).
	const char* next;	// Pointer to the beginning of the next row.
	float width;		// Logical width of the row.
	float minx, maxx;	// Actual bounds of the row. Logical with and bounds can differ because of kerning and some parts over extending.
};
typedef struct NVGtextRow NVGtextRow;

enum NVGimageFlags {
    NVG_IMAGE_GENERATE_MIPMAPS	= 1<<0,     // Generate mipmaps during creation of the image.
	NVG_IMAGE_REPEATX			= 1<<1,		// Repeat image in X direction.
	NVG_IMAGE_REPEATY			= 1<<2,		// Repeat image in Y direction.
	NVG_IMAGE_FLIPY				= 1<<3,		// Flips (inverses) image in Y direction when rendered.
	NVG_IMAGE_PREMULTIPLIED		= 1<<4,		// Image data has premultiplied alpha.
	NVG_IMAGE_NEAREST			= 1<<5,		// Image interpolation is Nearest instead Linear
};

// Begin drawing a new frame
// Calls to nanovg drawing API should be wrapped in nvgBeginFrame() & nvgEndFrame()
// nvgBeginFrame() defines the size of the window to render to in relation currently
// set viewport (i.e. glViewport on GL backends). Device pixel ration allows to
// control the rendering on Hi-DPI devices.
// For example, GLFW returns two dimension for an opened window: window size and
// frame buffer size. In that case you would set windowWidth/Height to the window size
// devicePixelRatio to: frameBufferWidth / windowWidth.
void nvgBeginFrame(NVGcontext* ctx, float windowWidth, float windowHeight, float devicePixelRatio);

// Cancels drawing the current frame.
void nvgCancelFrame(NVGcontext* ctx);

// Ends drawing flushing remaining render state.
void nvgEndFrame(NVGcontext* ctx);

//
// Composite operation
//
// The composite operations in NanoVG are modeled after HTML Canvas API, and
// the blend func is based on OpenGL (see corresponding manuals for more info).
// The colors in the blending state have premultiplied alpha.

// Sets the composite operation. The op parameter should be one of NVGcompositeOperation.
void nvgGlobalCompositeOperation(NVGcontext* ctx, int op);

// Sets the composite operation with custom pixel arithmetic. The parameters should be one of NVGblendFactor.
void nvgGlobalCompositeBlendFunc(NVGcontext* ctx, int sfactor, int dfactor);

// Sets the composite operation with custom pixel arithmetic for RGB and alpha components separately. The parameters should be one of NVGblendFactor.
void nvgGlobalCompositeBlendFuncSeparate(NVGcontext* ctx, int srcRGB, int dstRGB, int srcAlpha, int dstAlpha);

//
// Color utils
//
// Colors in NanoVG are stored as unsigned ints in ABGR format.

// Returns a color value from red, green, blue values. Alpha will be set to 255 (1.0f).
NVGcolor nvgRGB(unsigned char r, unsigned char g, unsigned char b);

// Returns a color value from red, green, blue values. Alpha will be set to 1.0f.
NVGcolor nvgRGBf(float r, float g, float b);


// Returns a color value from red, green, blue and alpha values.
NVGcolor nvgRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

// Returns a color value from red, green, blue and alpha values.
NVGcolor nvgRGBAf(float r, float g, float b, float a);


// Linearly interpolates from color c0 to c1, and returns resulting color value.
NVGcolor nvgLerpRGBA(NVGcolor c0, NVGcolor c1, float u);

// Sets transparency of a color value.
NVGcolor nvgTransRGBA(NVGcolor c0, unsigned char a);

// Sets transparency of a color value.
NVGcolor nvgTransRGBAf(NVGcolor c0, float a);

// Returns color value specified by hue, saturation and lightness.
// HSL values are all in range [0..1], alpha will be set to 255.
NVGcolor nvgHSL(float h, float s, float l);

// Returns color value specified by hue, saturation and lightness and alpha.
// HSL values are all in range [0..1], alpha in range [0..255]
NVGcolor nvgHSLA(float h, float s, float l, unsigned char a);

//
// State Handling
//
// NanoVG contains state which represents how paths will be rendered.
// The state contains transform, fill and stroke styles, text and font styles,
// and scissor clipping.

// Pushes and saves the current render state into a state stack.
// A matching nvgRestore() must be used to restore the state.
void nvgSave(NVGcontext* ctx);

// Pops and restores current render state.
void nvgRestore(NVGcontext* ctx);

// Resets current render state to default values. Does not affect the render state stack.
void nvgReset(NVGcontext* ctx);

//
// Render styles
//
// Fill and stroke render style can be either a solid color or a paint which is a gradient or a pattern.
// Solid color is simply defined as a color value, different kinds of paints can be created
// using nvgLinearGradient(), nvgBoxGradient(), nvgRadialGradient() and nvgImagePattern().
//
// Current render style can be saved and restored using nvgSave() and nvgRestore().

// Sets whether to draw antialias for nvgStroke() and nvgFill(). It's enabled by default.
void nvgShapeAntiAlias(NVGcontext* ctx, int enabled);

// Sets current stroke style to a solid color.
void nvgStrokeColor(NVGcontext* ctx, NVGcolor color);

// Sets current stroke style to a paint, which can be a one of the gradients or a pattern.
void nvgStrokePaint(NVGcontext* ctx, NVGpaint paint);

// Sets current fill style to a solid color.
void nvgFillColor(NVGcontext* ctx, NVGcolor color);

// Sets current fill style to a paint, which can be a one of the gradients or a pattern.
void nvgFillPaint(NVGcontext* ctx, NVGpaint paint);

// Sets the miter limit of the stroke style.
// Miter limit controls when a sharp corner is beveled.
void nvgMiterLimit(NVGcontext* ctx, float limit);

// Sets the stroke width of the stroke style.
void nvgStrokeWidth(NVGcontext* ctx, float size);

// Sets how the end of the line (cap) is drawn,
// Can be one of: NVG_BUTT (default), NVG_ROUND, NVG_SQUARE.
void nvgLineCap(NVGcontext* ctx, int cap);

// Sets how sharp path corners are drawn.
// Can be one of NVG_MITER (default), NVG_ROUND, NVG_BEVEL.
void nvgLineJoin(NVGcontext* ctx, int join);

// Sets the transparency applied to all rendered shapes.
// Already transparent paths will get proportionally more transparent as well.
void nvgGlobalAlpha(NVGcontext* ctx, float alpha);

//
// Transforms
//
// The paths, gradients, patterns and scissor region are transformed by an transformation
// matrix at the time when they are passed to the API.
// The current transformation matrix is a affine matrix:
//   [sx kx tx]
//   [ky sy ty]
//   [ 0  0  1]
// Where: sx,sy define scaling, kx,ky skewing, and tx,ty translation.
// The last row is assumed to be 0,0,1 and is not stored.
//
// Apart from nvgResetTransform(), each transformation function first creates
// specific transformation matrix and pre-multiplies the current transformation by it.
//
// Current coordinate system (transformation) can be saved and restored using nvgSave() and nvgRestore().

// Resets current transform to a identity matrix.
void nvgResetTransform(NVGcontext* ctx);

// Premultiplies current coordinate system by specified matrix.
// The parameters are interpreted as matrix as follows:
//   [a c e]
//   [b d f]
//   [0 0 1]
void nvgTransform(NVGcontext* ctx, float a, float b, float c, float d, float e, float f);

// Translates current coordinate system.
void nvgTranslate(NVGcontext* ctx, float x, float y);

// Rotates current coordinate system. Angle is specified in radians.
void nvgRotate(NVGcontext* ctx, float angle);

// Skews the current coordinate system along X axis. Angle is specified in radians.
void nvgSkewX(NVGcontext* ctx, float angle);

// Skews the current coordinate system along Y axis. Angle is specified in radians.
void nvgSkewY(NVGcontext* ctx, float angle);

// Scales the current coordinate system.
void nvgScale(NVGcontext* ctx, float x, float y);

// Stores the top part (a-f) of the current transformation matrix in to the specified buffer.
//   [a c e]
//   [b d f]
//   [0 0 1]
// There should be space for 6 floats in the return buffer for the values a-f.
void nvgCurrentTransform(NVGcontext* ctx, float* xform);


// The following functions can be used to make calculations on 2x3 transformation matrices.
// A 2x3 matrix is represented as float[6].

// Sets the transform to identity matrix.
void nvgTransformIdentity(float* dst);

// Sets the transform to translation matrix matrix.
void nvgTransformTranslate(float* dst, float tx, float ty);

// Sets the transform to scale matrix.
void nvgTransformScale(float* dst, float sx, float sy);

// Sets the transform to rotate matrix. Angle is specified in radians.
void nvgTransformRotate(float* dst, float a);

// Sets the transform to skew-x matrix. Angle is specified in radians.
void nvgTransformSkewX(float* dst, float a);

// Sets the transform to skew-y matrix. Angle is specified in radians.
void nvgTransformSkewY(float* dst, float a);

// Sets the transform to the result of multiplication of two transforms, of A = A*B.
void nvgTransformMultiply(float* dst, const float* src);

// Sets the transform to the result of multiplication of two transforms, of A = B*A.
void nvgTransformPremultiply(float* dst, const float* src);

// Sets the destination to inverse of specified transform.
// Returns 1 if the inverse could be calculated, else 0.
int nvgTransformInverse(float* dst, const float* src);

// Transform a point by given transform.
void nvgTransformPoint(float* dstx, float* dsty, const float* xform, float srcx, float srcy);

// Converts degrees to radians and vice versa.
float nvgDegToRad(float deg);
float nvgRadToDeg(float rad);

//
// Images
//
// NanoVG allows you to load jpg, png, psd, tga, pic and gif files to be used for rendering.
// In addition you can upload your own image. The image loading is provided by stb_image.
// The parameter imageFlags is combination of flags defined in NVGimageFlags.

// Creates image by loading it from the disk from specified file name.
// Returns handle to the image.
int nvgCreateImage(NVGcontext* ctx, const char* filename, int imageFlags);

// Creates image by loading it from the specified chunk of memory.
// Returns handle to the image.
int nvgCreateImageMem(NVGcontext* ctx, int imageFlags, unsigned char* data, int ndata);

// Creates image from specified image data.
// Returns handle to the image.
int nvgCreateImageRGBA(NVGcontext* ctx, int w, int h, int imageFlags, const unsigned char* data);

int nvgCreateImageAlpha(NVGcontext* ctx, int w, int h, int imageFlags, const unsigned char* data);

// Updates image data specified by image handle.
void nvgUpdateImage(NVGcontext* ctx, int image, const unsigned char* data);

// Returns the dimensions of a created image.
void nvgImageSize(NVGcontext* ctx, int image, int* w, int* h);

// Deletes created image.
void nvgDeleteImage(NVGcontext* ctx, int image);

//
// Paints
//
// NanoVG supports four types of paints: linear gradient, box gradient, radial gradient and image pattern.
// These can be used as paints for strokes and fills.

// Creates and returns a linear gradient. Parameters (sx,sy)-(ex,ey) specify the start and end coordinates
// of the linear gradient, icol specifies the start color and ocol the end color.
// The gradient is transformed by the current transform when it is passed to nvgFillPaint() or nvgStrokePaint().
NVGpaint nvgLinearGradient(NVGcontext* ctx, float sx, float sy, float ex, float ey,
						   NVGcolor icol, NVGcolor ocol);

// Creates and returns a box gradient. Box gradient is a feathered rounded rectangle, it is useful for rendering
// drop shadows or highlights for boxes. Parameters (x,y) define the top-left corner of the rectangle,
// (w,h) define the size of the rectangle, r defines the corner radius, and f feather. Feather defines how blurry
// the border of the rectangle is. Parameter icol specifies the inner color and ocol the outer color of the gradient.
// The gradient is transformed by the current transform when it is passed to nvgFillPaint() or nvgStrokePaint().
NVGpaint nvgBoxGradient(NVGcontext* ctx, float x, float y, float w, float h,
						float r, float f, NVGcolor icol, NVGcolor ocol);

// Creates and returns a radial gradient. Parameters (cx,cy) specify the center, inr and outr specify
// the inner and outer radius of the gradient, icol specifies the start color and ocol the end color.
// The gradient is transformed by the current transform when it is passed to nvgFillPaint() or nvgStrokePaint().
NVGpaint nvgRadialGradient(NVGcontext* ctx, float cx, float cy, float inr, float outr,
						   NVGcolor icol, NVGcolor ocol);

// Creates and returns an image pattern. Parameters (ox,oy) specify the left-top location of the image pattern,
// (ex,ey) the size of one image, angle rotation around the top-left corner, image is handle to the image to render.
// The gradient is transformed by the current transform when it is passed to nvgFillPaint() or nvgStrokePaint().
NVGpaint nvgImagePattern(NVGcontext* ctx, float ox, float oy, float ex, float ey,
						 float angle, int image, float alpha);

//
// Scissoring
//
// Scissoring allows you to clip the rendering into a rectangle. This is useful for various
// user interface cases like rendering a text edit or a timeline.

// Sets the current scissor rectangle.
// The scissor rectangle is transformed by the current transform.
void nvgScissor(NVGcontext* ctx, float x, float y, float w, float h);

// Intersects current scissor rectangle with the specified rectangle.
// The scissor rectangle is transformed by the current transform.
// Note: in case the rotation of previous scissor rect differs from
// the current one, the intersection will be done between the specified
// rectangle and the previous scissor rectangle transformed in the current
// transform space. The resulting shape is always rectangle.
void nvgIntersectScissor(NVGcontext* ctx, float x, float y, float w, float h);

// Reset and disables scissoring.
void nvgResetScissor(NVGcontext* ctx);

//
// Paths
//
// Drawing a new shape starts with nvgBeginPath(), it clears all the currently defined paths.
// Then you define one or more paths and sub-paths which describe the shape. The are functions
// to draw common shapes like rectangles and circles, and lower level step-by-step functions,
// which allow to define a path curve by curve.
//
// NanoVG uses even-odd fill rule to draw the shapes. Solid shapes should have counter clockwise
// winding and holes should have counter clockwise order. To specify winding of a path you can
// call nvgPathWinding(). This is useful especially for the common shapes, which are drawn CCW.
//
// Finally you can fill the path using current fill style by calling nvgFill(), and stroke it
// with current stroke style by calling nvgStroke().
//
// The curve segments and sub-paths are transformed by the current transform.

// Clears the current path and sub-paths.
void nvgBeginPath(NVGcontext* ctx);

// Starts new sub-path with specified point as first point.
void nvgMoveTo(NVGcontext* ctx, float x, float y);

// Adds line segment from the last point in the path to the specified point.
void nvgLineTo(NVGcontext* ctx, float x, float y);

// Adds cubic bezier segment from last point in the path via two control points to the specified point.
void nvgBezierTo(NVGcontext* ctx, float c1x, float c1y, float c2x, float c2y, float x, float y);

// Adds quadratic bezier segment from last point in the path via a control point to the specified point.
void nvgQuadTo(NVGcontext* ctx, float cx, float cy, float x, float y);

// Adds an arc segment at the corner defined by the last path point, and two specified points.
void nvgArcTo(NVGcontext* ctx, float x1, float y1, float x2, float y2, float radius);

// Closes current sub-path with a line segment.
void nvgClosePath(NVGcontext* ctx);

// Sets the current sub-path winding, see NVGwinding and NVGsolidity.
void nvgPathWinding(NVGcontext* ctx, int dir);

// Creates new circle arc shaped sub-path. The arc center is at cx,cy, the arc radius is r,
// and the arc is drawn from angle a0 to a1, and swept in direction dir (NVG_CCW, or NVG_CW).
// Angles are specified in radians.
void nvgArc(NVGcontext* ctx, float cx, float cy, float r, float a0, float a1, int dir);

void nvgBarc(NVGcontext* ctx, float cx, float cy, float r, float a0, float a1, int dir, int join);

// Creates new rectangle shaped sub-path.
void nvgRect(NVGcontext* ctx, float x, float y, float w, float h);

// Creates new rounded rectangle shaped sub-path.
void nvgRoundedRect(NVGcontext* ctx, float x, float y, float w, float h, float r);

// Creates new rounded rectangle shaped sub-path with varying radii for each corner.
void nvgRoundedRectVarying(NVGcontext* ctx, float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);

// Creates new ellipse shaped sub-path.
void nvgEllipse(NVGcontext* ctx, float cx, float cy, float rx, float ry);

// Creates new circle shaped sub-path.
void nvgCircle(NVGcontext* ctx, float cx, float cy, float r);

// Fills the current path with current fill style.
void nvgFill(NVGcontext* ctx);

// Fills the current path with current stroke style.
void nvgStroke(NVGcontext* ctx);


//
// Text
//
// NanoVG allows you to load .ttf files and use the font to render text.
//
// The appearance of the text can be defined by setting the current text style
// and by specifying the fill color. Common text and font settings such as
// font size, letter spacing and text align are supported. Font blur allows you
// to create simple text effects such as drop shadows.
//
// At render time the font face can be set based on the font handles or name.
//
// Font measure functions return values in local space, the calculations are
// carried in the same resolution as the final rendering. This is done because
// the text glyph positions are snapped to the nearest pixels sharp rendering.
//
// The local space means that values are not rotated or scale as per the current
// transformation. For example if you set font size to 12, which would mean that
// line height is 16, then regardless of the current scaling and rotation, the
// returned line height is always 16. Some measures may vary because of the scaling
// since aforementioned pixel snapping.
//
// While this may sound a little odd, the setup allows you to always render the
// same way regardless of scaling. I.e. following works regardless of scaling:
//
//		const char* txt = "Text me up.";
//		nvgTextBounds(vg, x,y, txt, NULL, bounds);
//		nvgBeginPath(vg);
//		nvgRoundedRect(vg, bounds[0],bounds[1], bounds[2]-bounds[0], bounds[3]-bounds[1]);
//		nvgFill(vg);
//
// Note: currently only solid color fill is supported for text.

// Creates font by loading it from the disk from specified file name.
// Returns handle to the font.
int nvgCreateFont(NVGcontext* ctx, const char* name, const char* filename);

// fontIndex specifies which font face to load from a .ttf/.ttc file.
int nvgCreateFontAtIndex(NVGcontext* ctx, const char* name, const char* filename, const int fontIndex);

// Creates font by loading it from the specified memory chunk.
// Returns handle to the font.
int nvgCreateFontMem(NVGcontext* ctx, const char* name, unsigned char* data, int ndata, int freeData);

// fontIndex specifies which font face to load from a .ttf/.ttc file.
int nvgCreateFontMemAtIndex(NVGcontext* ctx, const char* name, unsigned char* data, int ndata, int freeData, const int fontIndex);

// Finds a loaded font of specified name, and returns handle to it, or -1 if the font is not found.
int nvgFindFont(NVGcontext* ctx, const char* name);

// Adds a fallback font by handle.
int nvgAddFallbackFontId(NVGcontext* ctx, int baseFont, int fallbackFont);

// Adds a fallback font by name.
int nvgAddFallbackFont(NVGcontext* ctx, const char* baseFont, const char* fallbackFont);

// Resets fallback fonts by handle.
void nvgResetFallbackFontsId(NVGcontext* ctx, int baseFont);

// Resets fallback fonts by name.
void nvgResetFallbackFonts(NVGcontext* ctx, const char* baseFont);

// Sets the font size of current text style.
void nvgFontSize(NVGcontext* ctx, float size);

// Sets the blur of current text style.
void nvgFontBlur(NVGcontext* ctx, float blur);

// Sets the letter spacing of current text style.
void nvgTextLetterSpacing(NVGcontext* ctx, float spacing);

// Sets the proportional line height of current text style. The line height is specified as multiple of font size.
void nvgTextLineHeight(NVGcontext* ctx, float lineHeight);

// Sets the text align of current text style, see NVGalign for options.
void nvgTextAlign(NVGcontext* ctx, int align);

// Sets the font face based on specified id of current text style.
void nvgFontFaceId(NVGcontext* ctx, int font);

// Sets the font face based on specified name of current text style.
void nvgFontFace(NVGcontext* ctx, const char* font);

// Draws text string at specified location. If end is specified only the sub-string up to the end is drawn.
float nvgText(NVGcontext* ctx, float x, float y, const char* string, const char* end);

// Draws multi-line text string at specified location wrapped at the specified width. If end is specified only the sub-string up to the end is drawn.
// White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
// Words longer than the max width are slit at nearest character (i.e. no hyphenation).
void nvgTextBox(NVGcontext* ctx, float x, float y, float breakRowWidth, const char* string, const char* end);

// Measures the specified text string. Parameter bounds should be a pointer to float[4],
// if the bounding box of the text should be returned. The bounds value are [xmin,ymin, xmax,ymax]
// Returns the horizontal advance of the measured text (i.e. where the next character should drawn).
// Measured values are returned in local coordinate space.
float nvgTextBounds(NVGcontext* ctx, float x, float y, const char* string, const char* end, float* bounds);

// Measures the specified multi-text string. Parameter bounds should be a pointer to float[4],
// if the bounding box of the text should be returned. The bounds value are [xmin,ymin, xmax,ymax]
// Measured values are returned in local coordinate space.
void nvgTextBoxBounds(NVGcontext* ctx, float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds);

// Calculates the glyph x positions of the specified text. If end is specified only the sub-string will be used.
// Measured values are returned in local coordinate space.
int nvgTextGlyphPositions(NVGcontext* ctx, float x, float y, const char* string, const char* end, NVGglyphPosition* positions, int maxPositions);

// Returns the vertical metrics based on the current text style.
// Measured values are returned in local coordinate space.
void nvgTextMetrics(NVGcontext* ctx, float* ascender, float* descender, float* lineh);

// Breaks the specified text into lines. If end is specified only the sub-string will be used.
// White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
// Words longer than the max width are slit at nearest character (i.e. no hyphenation).
int nvgTextBreakLines(NVGcontext* ctx, const char* string, const char* end, float breakRowWidth, NVGtextRow* rows, int maxRows);

//
// Internal Render API
//
enum NVGtexture {
	NVG_TEXTURE_ALPHA = 0x01,
	NVG_TEXTURE_RGBA = 0x02,
};

struct NVGscissor {
	float xform[6];
	float extent[2];
};
typedef struct NVGscissor NVGscissor;

struct NVGvertex {
	float x,y,u,v;
};
typedef struct NVGvertex NVGvertex;

struct NVGpath {
	int first;
	int count;
	unsigned char closed;
	int nbevel;
	NVGvertex* fill;
	int nfill;
	NVGvertex* stroke;
	int nstroke;
	int winding;
	int convex;
};
typedef struct NVGpath NVGpath;

struct NVGparams {
	void* userPtr;
	int edgeAntiAlias;
	int (*renderCreate)(void* uptr);
	int (*renderCreateTexture)(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data);
	int (*renderDeleteTexture)(void* uptr, int image);
	int (*renderUpdateTexture)(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data);
	int (*renderGetTextureSize)(void* uptr, int image, int* w, int* h);
	void (*renderViewport)(void* uptr, float width, float height, float devicePixelRatio);
	void (*renderCancel)(void* uptr);
	void (*renderFlush)(void* uptr);
	void (*renderFill)(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths);
	void (*renderStroke)(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths);
	void (*renderTriangles)(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, const NVGvertex* verts, int nverts, float fringe);
	void (*renderDelete)(void* uptr);
};
typedef struct NVGparams NVGparams;

// Constructor and destructor, called by the render back-end.
NVGcontext* nvgCreateInternal(NVGparams* params);
void nvgDeleteInternal(NVGcontext* ctx);

NVGparams* nvgInternalParams(NVGcontext* ctx);

// Debug function to dump cached path data.
void nvgDebugDumpPathCache(NVGcontext* ctx);



#define NVG_NOTUSED(v) for (;;) { (void)(1 ? (void)0 : ( (void)(v) ) ); break; }








// FILE: nanovg_gl.h


// Create flags

enum NVGcreateFlags {
	// Flag indicating if geometry based anti-aliasing is used (may not be needed when using MSAA).
	NVG_ANTIALIAS = 1<<0,
	// Flag indicating if strokes should be drawn using stencil buffer. The rendering will be a little
	// slower, but path overlaps (i.e. self-intersecting or sharp turns) will be drawn just once.
	NVG_STENCIL_STROKES = 1<<1,
	// Flag indicating that additional debug checks are done.
	NVG_DEBUG = 1<<2,
};

// Define VTable with pointers to the functions for a each OpenGL (ES) version.

typedef struct {
	const char *name;
	NVGcontext* (*createContext)(int flags);
	void (*deleteContext) (NVGcontext* ctx);
	int (*createImageFromHandle) (NVGcontext* ctx, unsigned int textureId, int w, int h, int flags);
	unsigned int (*getImageHandle) (NVGcontext* ctx, int image);
} NanoVG_GL_Functions_VTable;

// Create NanoVG contexts for different OpenGL (ES) versions.

NVGcontext* nvgCreateGLES3(int flags);
void nvgDeleteGLES3(NVGcontext* ctx);

int nvglCreateImageFromHandleGLES3(NVGcontext* ctx, GLuint textureId, int w, int h, int flags);
GLuint nvglImageHandleGLES3(NVGcontext* ctx, int image);

// These are additional flags on top of NVGimageFlags.
enum NVGimageFlagsGL {
	NVG_IMAGE_NODELETE = 1<<16, // Do not delete GL texture handle.
};

// Create VTables for different OpenGL (ES) versions.
extern const NanoVG_GL_Functions_VTable NanoVG_GLES3_Functions_VTable;





// FILE: blendish.h



/*

Revision 6 (2014-09-21)

Summary
-------

Blendish is a small collection of drawing functions for NanoVG, designed to
replicate the look of the Blender 2.5+ User Interface. You can use these
functions to theme your UI library. Several metric constants for faithful
reproduction are also included.

Blendish supports the original Blender icon sheet; As the licensing of Blenders
icons is unclear, they are not included in Blendishes repository, but a SVG
template, "icons_template.svg" is provided, which you can use to build your own
icon sheet.

To use icons, you must first load the icon sheet using one of the
nvgCreateImage*() functions and then pass the image handle to bndSetIconImage();
otherwise, no icons will be drawn. See bndSetIconImage() for more information.

Blendish will not render text until a suitable UI font has been passed to
bndSetFont() has been called. See bndSetFont() for more information.


Drawbacks
---------

There is no support for varying dpi resolutions yet. The library is hardcoded
to the equivalent of 72 dpi in the Blender system settings.

Support for label truncation is missing. Text rendering breaks when widgets are
too short to contain their labels.

Usage
-----

To use this header file in implementation mode, define BLENDISH_IMPLEMENTATION
before including blendish.h, otherwise the file will be in header-only mode.

*/

// you can override this from the outside to pick
// the export level you need
#ifndef BND_EXPORT
#define BND_EXPORT
#endif

// if that typedef is provided elsewhere, you may define
// BLENDISH_NO_NVG_TYPEDEFS before including the header.
#ifndef BLENDISH_NO_NVG_TYPEDEFS
typedef struct NVGcontext NVGcontext;
typedef struct NVGcolor NVGcolor;
typedef struct NVGglyphPosition NVGglyphPosition;
#endif

// describes the theme used to draw a single widget or widget box;
// these values correspond to the same values that can be retrieved from
// the Theme panel in the Blender preferences
typedef struct BNDwidgetTheme {
    // color of widget box outline
    NVGcolor outlineColor;
    // color of widget item (meaning changes depending on class)
    NVGcolor itemColor;
    // fill color of widget box
    NVGcolor innerColor;
    // fill color of widget box when active
    NVGcolor innerSelectedColor;
    // color of text label
    NVGcolor textColor;
    // color of text label when active
    NVGcolor textSelectedColor;
    // delta modifier for upper part of gradient (-100 to 100)
    int shadeTop;
    // delta modifier for lower part of gradient (-100 to 100)
    int shadeDown;
} BNDwidgetTheme;

// describes the theme used to draw nodes
typedef struct BNDnodeTheme {
    // inner color of selected node (and downarrow)
    NVGcolor nodeSelectedColor;
    // outline of wires
    NVGcolor wiresColor;
    // color of text label when active
    NVGcolor textSelectedColor;

    // inner color of active node (and dragged wire)
    NVGcolor activeNodeColor;
    // color of selected wire
    NVGcolor wireSelectColor;
    // color of background of node
    NVGcolor nodeBackdropColor;

    // how much a noodle curves (0 to 10)
    int noodleCurving;
} BNDnodeTheme;

// describes the theme used to draw widgets
typedef struct BNDtheme {
    // the background color of panels and windows
    NVGcolor backgroundColor;
    // theme for labels
    BNDwidgetTheme regularTheme;
    // theme for tool buttons
    BNDwidgetTheme toolTheme;
    // theme for radio buttons
    BNDwidgetTheme radioTheme;
    // theme for text fields
    BNDwidgetTheme textFieldTheme;
    // theme for option buttons (checkboxes)
    BNDwidgetTheme optionTheme;
    // theme for choice buttons (comboboxes)
    // Blender calls them "menu buttons"
    BNDwidgetTheme choiceTheme;
    // theme for number fields
    BNDwidgetTheme numberFieldTheme;
    // theme for slider controls
    BNDwidgetTheme sliderTheme;
    // theme for scrollbars
    BNDwidgetTheme scrollBarTheme;
    // theme for tooltips
    BNDwidgetTheme tooltipTheme;
    // theme for menu backgrounds
    BNDwidgetTheme menuTheme;
    // theme for menu items
    BNDwidgetTheme menuItemTheme;
    // theme for nodes
    BNDnodeTheme nodeTheme;
} BNDtheme;

// how text on a control is aligned
typedef enum BNDtextAlignment {
    BND_LEFT = 0,
    BND_CENTER,
} BNDtextAlignment;

// states altering the styling of a widget
typedef enum BNDwidgetState {
    // not interacting
    BND_DEFAULT = 0,
    // the mouse is hovering over the control
    BND_HOVER,
    // the widget is activated (pressed) or in an active state (toggled)
    BND_ACTIVE
} BNDwidgetState;

// flags indicating which corners are sharp (for grouping widgets)
typedef enum BNDcornerFlags {
    // all corners are round
    BND_CORNER_NONE = 0,
    // sharp top left corner
    BND_CORNER_TOP_LEFT = 1,
    // sharp top right corner
    BND_CORNER_TOP_RIGHT = 2,
    // sharp bottom right corner
    BND_CORNER_DOWN_RIGHT = 4,
    // sharp bottom left corner
    BND_CORNER_DOWN_LEFT = 8,
    // all corners are sharp;
    // you can invert a set of flags using ^= BND_CORNER_ALL
    BND_CORNER_ALL = 0xF,
    // top border is sharp
    BND_CORNER_TOP = 3,
    // bottom border is sharp
    BND_CORNER_DOWN = 0xC,
    // left border is sharp
    BND_CORNER_LEFT = 9,
    // right border is sharp
    BND_CORNER_RIGHT = 6
} BNDcornerFlags;

// build an icon ID from two coordinates into the icon sheet, where
// (0,0) designates the upper-leftmost icon, (1,0) the one right next to it,
// and so on.
#define BND_ICONID(x,y) ((x)|((y)<<8))
// alpha of disabled widget groups
// can be used in conjunction with nvgGlobalAlpha()
#define BND_DISABLED_ALPHA 0.5

enum {
    // default widget height
    BND_WIDGET_HEIGHT = 21,
    // default toolbutton width (if icon only)
    BND_TOOL_WIDTH = 20,

    // default radius of node ports
    BND_NODE_PORT_RADIUS = 5,
    // top margin of node content
    BND_NODE_MARGIN_TOP = 25,
    // bottom margin of node content
    BND_NODE_MARGIN_DOWN = 5,
    // left and right margin of node content
    BND_NODE_MARGIN_SIDE = 10,
    // height of node title bar
    BND_NODE_TITLE_HEIGHT = 20,
    // width of node title arrow click area
    BND_NODE_ARROW_AREA_WIDTH = 20,

    // size of splitter corner click area
    BND_SPLITTER_AREA_SIZE = 12,

    // width of vertical scrollbar
    BND_SCROLLBAR_WIDTH = 13,
    // height of horizontal scrollbar
    BND_SCROLLBAR_HEIGHT = 14,

    // default vertical spacing
    BND_VSPACING = 1,
    // default vertical spacing between groups
    BND_VSPACING_GROUP = 8,
    // default horizontal spacing
    BND_HSPACING = 8,
};

typedef enum BNDicon {
    BND_ICON_NONE = BND_ICONID(0,29),
    BND_ICON_QUESTION = BND_ICONID(1,29),
    BND_ICON_ERROR = BND_ICONID(2,29),
    BND_ICON_CANCEL = BND_ICONID(3,29),
    BND_ICON_TRIA_RIGHT = BND_ICONID(4,29),
    BND_ICON_TRIA_DOWN = BND_ICONID(5,29),
    BND_ICON_TRIA_LEFT = BND_ICONID(6,29),
    BND_ICON_TRIA_UP = BND_ICONID(7,29),
    BND_ICON_ARROW_LEFTRIGHT = BND_ICONID(8,29),
    BND_ICON_PLUS = BND_ICONID(9,29),
    BND_ICON_DISCLOSURE_TRI_DOWN = BND_ICONID(10,29),
    BND_ICON_DISCLOSURE_TRI_RIGHT = BND_ICONID(11,29),
    BND_ICON_RADIOBUT_OFF = BND_ICONID(12,29),
    BND_ICON_RADIOBUT_ON = BND_ICONID(13,29),
    BND_ICON_MENU_PANEL = BND_ICONID(14,29),
    BND_ICON_BLENDER = BND_ICONID(15,29),
    BND_ICON_GRIP = BND_ICONID(16,29),
    BND_ICON_DOT = BND_ICONID(17,29),
    BND_ICON_COLLAPSEMENU = BND_ICONID(18,29),
    BND_ICON_X = BND_ICONID(19,29),
    BND_ICON_GO_LEFT = BND_ICONID(21,29),
    BND_ICON_PLUG = BND_ICONID(22,29),
    BND_ICON_UI = BND_ICONID(23,29),
    BND_ICON_NODE = BND_ICONID(24,29),
    BND_ICON_NODE_SEL = BND_ICONID(25,29),

    BND_ICON_FULLSCREEN = BND_ICONID(0,28),
    BND_ICON_SPLITSCREEN = BND_ICONID(1,28),
    BND_ICON_RIGHTARROW_THIN = BND_ICONID(2,28),
    BND_ICON_BORDERMOVE = BND_ICONID(3,28),
    BND_ICON_VIEWZOOM = BND_ICONID(4,28),
    BND_ICON_ZOOMIN = BND_ICONID(5,28),
    BND_ICON_ZOOMOUT = BND_ICONID(6,28),
    BND_ICON_PANEL_CLOSE = BND_ICONID(7,28),
    BND_ICON_COPY_ID = BND_ICONID(8,28),
    BND_ICON_EYEDROPPER = BND_ICONID(9,28),
    BND_ICON_LINK_AREA = BND_ICONID(10,28),
    BND_ICON_AUTO = BND_ICONID(11,28),
    BND_ICON_CHECKBOX_DEHLT = BND_ICONID(12,28),
    BND_ICON_CHECKBOX_HLT = BND_ICONID(13,28),
    BND_ICON_UNLOCKED = BND_ICONID(14,28),
    BND_ICON_LOCKED = BND_ICONID(15,28),
    BND_ICON_UNPINNED = BND_ICONID(16,28),
    BND_ICON_PINNED = BND_ICONID(17,28),
    BND_ICON_SCREEN_BACK = BND_ICONID(18,28),
    BND_ICON_RIGHTARROW = BND_ICONID(19,28),
    BND_ICON_DOWNARROW_HLT = BND_ICONID(20,28),
    BND_ICON_DOTSUP = BND_ICONID(21,28),
    BND_ICON_DOTSDOWN = BND_ICONID(22,28),
    BND_ICON_LINK = BND_ICONID(23,28),
    BND_ICON_INLINK = BND_ICONID(24,28),
    BND_ICON_PLUGIN = BND_ICONID(25,28),

    BND_ICON_HELP = BND_ICONID(0,27),
    BND_ICON_GHOST_ENABLED = BND_ICONID(1,27),
    BND_ICON_COLOR = BND_ICONID(2,27),
    BND_ICON_LINKED = BND_ICONID(3,27),
    BND_ICON_UNLINKED = BND_ICONID(4,27),
    BND_ICON_HAND = BND_ICONID(5,27),
    BND_ICON_ZOOM_ALL = BND_ICONID(6,27),
    BND_ICON_ZOOM_SELECTED = BND_ICONID(7,27),
    BND_ICON_ZOOM_PREVIOUS = BND_ICONID(8,27),
    BND_ICON_ZOOM_IN = BND_ICONID(9,27),
    BND_ICON_ZOOM_OUT = BND_ICONID(10,27),
    BND_ICON_RENDER_REGION = BND_ICONID(11,27),
    BND_ICON_BORDER_RECT = BND_ICONID(12,27),
    BND_ICON_BORDER_LASSO = BND_ICONID(13,27),
    BND_ICON_FREEZE = BND_ICONID(14,27),
    BND_ICON_STYLUS_PRESSURE = BND_ICONID(15,27),
    BND_ICON_GHOST_DISABLED = BND_ICONID(16,27),
    BND_ICON_NEW = BND_ICONID(17,27),
    BND_ICON_FILE_TICK = BND_ICONID(18,27),
    BND_ICON_QUIT = BND_ICONID(19,27),
    BND_ICON_URL = BND_ICONID(20,27),
    BND_ICON_RECOVER_LAST = BND_ICONID(21,27),
    BND_ICON_FULLSCREEN_ENTER = BND_ICONID(23,27),
    BND_ICON_FULLSCREEN_EXIT = BND_ICONID(24,27),
    BND_ICON_BLANK1 = BND_ICONID(25,27),

    BND_ICON_LAMP = BND_ICONID(0,26),
    BND_ICON_MATERIAL = BND_ICONID(1,26),
    BND_ICON_TEXTURE = BND_ICONID(2,26),
    BND_ICON_ANIM = BND_ICONID(3,26),
    BND_ICON_WORLD = BND_ICONID(4,26),
    BND_ICON_SCENE = BND_ICONID(5,26),
    BND_ICON_EDIT = BND_ICONID(6,26),
    BND_ICON_GAME = BND_ICONID(7,26),
    BND_ICON_RADIO = BND_ICONID(8,26),
    BND_ICON_SCRIPT = BND_ICONID(9,26),
    BND_ICON_PARTICLES = BND_ICONID(10,26),
    BND_ICON_PHYSICS = BND_ICONID(11,26),
    BND_ICON_SPEAKER = BND_ICONID(12,26),
    BND_ICON_TEXTURE_SHADED = BND_ICONID(13,26),

    BND_ICON_VIEW3D = BND_ICONID(0,25),
    BND_ICON_IPO = BND_ICONID(1,25),
    BND_ICON_OOPS = BND_ICONID(2,25),
    BND_ICON_BUTS = BND_ICONID(3,25),
    BND_ICON_FILESEL = BND_ICONID(4,25),
    BND_ICON_IMAGE_COL = BND_ICONID(5,25),
    BND_ICON_INFO = BND_ICONID(6,25),
    BND_ICON_SEQUENCE = BND_ICONID(7,25),
    BND_ICON_TEXT = BND_ICONID(8,25),
    BND_ICON_IMASEL = BND_ICONID(9,25),
    BND_ICON_SOUND = BND_ICONID(10,25),
    BND_ICON_ACTION = BND_ICONID(11,25),
    BND_ICON_NLA = BND_ICONID(12,25),
    BND_ICON_SCRIPTWIN = BND_ICONID(13,25),
    BND_ICON_TIME = BND_ICONID(14,25),
    BND_ICON_NODETREE = BND_ICONID(15,25),
    BND_ICON_LOGIC = BND_ICONID(16,25),
    BND_ICON_CONSOLE = BND_ICONID(17,25),
    BND_ICON_PREFERENCES = BND_ICONID(18,25),
    BND_ICON_CLIP = BND_ICONID(19,25),
    BND_ICON_ASSET_MANAGER = BND_ICONID(20,25),

    BND_ICON_OBJECT_DATAMODE = BND_ICONID(0,24),
    BND_ICON_EDITMODE_HLT = BND_ICONID(1,24),
    BND_ICON_FACESEL_HLT = BND_ICONID(2,24),
    BND_ICON_VPAINT_HLT = BND_ICONID(3,24),
    BND_ICON_TPAINT_HLT = BND_ICONID(4,24),
    BND_ICON_WPAINT_HLT = BND_ICONID(5,24),
    BND_ICON_SCULPTMODE_HLT = BND_ICONID(6,24),
    BND_ICON_POSE_HLT = BND_ICONID(7,24),
    BND_ICON_PARTICLEMODE = BND_ICONID(8,24),
    BND_ICON_LIGHTPAINT = BND_ICONID(9,24),

    BND_ICON_SCENE_DATA = BND_ICONID(0,23),
    BND_ICON_RENDERLAYERS = BND_ICONID(1,23),
    BND_ICON_WORLD_DATA = BND_ICONID(2,23),
    BND_ICON_OBJECT_DATA = BND_ICONID(3,23),
    BND_ICON_MESH_DATA = BND_ICONID(4,23),
    BND_ICON_CURVE_DATA = BND_ICONID(5,23),
    BND_ICON_META_DATA = BND_ICONID(6,23),
    BND_ICON_LATTICE_DATA = BND_ICONID(7,23),
    BND_ICON_LAMP_DATA = BND_ICONID(8,23),
    BND_ICON_MATERIAL_DATA = BND_ICONID(9,23),
    BND_ICON_TEXTURE_DATA = BND_ICONID(10,23),
    BND_ICON_ANIM_DATA = BND_ICONID(11,23),
    BND_ICON_CAMERA_DATA = BND_ICONID(12,23),
    BND_ICON_PARTICLE_DATA = BND_ICONID(13,23),
    BND_ICON_LIBRARY_DATA_DIRECT = BND_ICONID(14,23),
    BND_ICON_GROUP = BND_ICONID(15,23),
    BND_ICON_ARMATURE_DATA = BND_ICONID(16,23),
    BND_ICON_POSE_DATA = BND_ICONID(17,23),
    BND_ICON_BONE_DATA = BND_ICONID(18,23),
    BND_ICON_CONSTRAINT = BND_ICONID(19,23),
    BND_ICON_SHAPEKEY_DATA = BND_ICONID(20,23),
    BND_ICON_CONSTRAINT_BONE = BND_ICONID(21,23),
    BND_ICON_CAMERA_STEREO = BND_ICONID(22,23),
    BND_ICON_PACKAGE = BND_ICONID(23,23),
    BND_ICON_UGLYPACKAGE = BND_ICONID(24,23),

    BND_ICON_BRUSH_DATA = BND_ICONID(0,22),
    BND_ICON_IMAGE_DATA = BND_ICONID(1,22),
    BND_ICON_FILE = BND_ICONID(2,22),
    BND_ICON_FCURVE = BND_ICONID(3,22),
    BND_ICON_FONT_DATA = BND_ICONID(4,22),
    BND_ICON_RENDER_RESULT = BND_ICONID(5,22),
    BND_ICON_SURFACE_DATA = BND_ICONID(6,22),
    BND_ICON_EMPTY_DATA = BND_ICONID(7,22),
    BND_ICON_SETTINGS = BND_ICONID(8,22),
    BND_ICON_RENDER_ANIMATION = BND_ICONID(9,22),
    BND_ICON_RENDER_STILL = BND_ICONID(10,22),
    BND_ICON_BOIDS = BND_ICONID(12,22),
    BND_ICON_STRANDS = BND_ICONID(13,22),
    BND_ICON_LIBRARY_DATA_INDIRECT = BND_ICONID(14,22),
    BND_ICON_GREASEPENCIL = BND_ICONID(15,22),
    BND_ICON_LINE_DATA = BND_ICONID(16,22),
    BND_ICON_GROUP_BONE = BND_ICONID(18,22),
    BND_ICON_GROUP_VERTEX = BND_ICONID(19,22),
    BND_ICON_GROUP_VCOL = BND_ICONID(20,22),
    BND_ICON_GROUP_UVS = BND_ICONID(21,22),
    BND_ICON_RNA = BND_ICONID(24,22),
    BND_ICON_RNA_ADD = BND_ICONID(25,22),

    BND_ICON_OUTLINER_OB_EMPTY = BND_ICONID(0,20),
    BND_ICON_OUTLINER_OB_MESH = BND_ICONID(1,20),
    BND_ICON_OUTLINER_OB_CURVE = BND_ICONID(2,20),
    BND_ICON_OUTLINER_OB_LATTICE = BND_ICONID(3,20),
    BND_ICON_OUTLINER_OB_META = BND_ICONID(4,20),
    BND_ICON_OUTLINER_OB_LAMP = BND_ICONID(5,20),
    BND_ICON_OUTLINER_OB_CAMERA = BND_ICONID(6,20),
    BND_ICON_OUTLINER_OB_ARMATURE = BND_ICONID(7,20),
    BND_ICON_OUTLINER_OB_FONT = BND_ICONID(8,20),
    BND_ICON_OUTLINER_OB_SURFACE = BND_ICONID(9,20),
    BND_ICON_OUTLINER_OB_SPEAKER = BND_ICONID(10,20),
    BND_ICON_RESTRICT_VIEW_OFF = BND_ICONID(19,20),
    BND_ICON_RESTRICT_VIEW_ON = BND_ICONID(20,20),
    BND_ICON_RESTRICT_SELECT_OFF = BND_ICONID(21,20),
    BND_ICON_RESTRICT_SELECT_ON = BND_ICONID(22,20),
    BND_ICON_RESTRICT_RENDER_OFF = BND_ICONID(23,20),
    BND_ICON_RESTRICT_RENDER_ON = BND_ICONID(24,20),

    BND_ICON_OUTLINER_DATA_EMPTY = BND_ICONID(0,19),
    BND_ICON_OUTLINER_DATA_MESH = BND_ICONID(1,19),
    BND_ICON_OUTLINER_DATA_CURVE = BND_ICONID(2,19),
    BND_ICON_OUTLINER_DATA_LATTICE = BND_ICONID(3,19),
    BND_ICON_OUTLINER_DATA_META = BND_ICONID(4,19),
    BND_ICON_OUTLINER_DATA_LAMP = BND_ICONID(5,19),
    BND_ICON_OUTLINER_DATA_CAMERA = BND_ICONID(6,19),
    BND_ICON_OUTLINER_DATA_ARMATURE = BND_ICONID(7,19),
    BND_ICON_OUTLINER_DATA_FONT = BND_ICONID(8,19),
    BND_ICON_OUTLINER_DATA_SURFACE = BND_ICONID(9,19),
    BND_ICON_OUTLINER_DATA_SPEAKER = BND_ICONID(10,19),
    BND_ICON_OUTLINER_DATA_POSE = BND_ICONID(11,19),

    BND_ICON_MESH_PLANE = BND_ICONID(0,18),
    BND_ICON_MESH_CUBE = BND_ICONID(1,18),
    BND_ICON_MESH_CIRCLE = BND_ICONID(2,18),
    BND_ICON_MESH_UVSPHERE = BND_ICONID(3,18),
    BND_ICON_MESH_ICOSPHERE = BND_ICONID(4,18),
    BND_ICON_MESH_GRID = BND_ICONID(5,18),
    BND_ICON_MESH_MONKEY = BND_ICONID(6,18),
    BND_ICON_MESH_CYLINDER = BND_ICONID(7,18),
    BND_ICON_MESH_TORUS = BND_ICONID(8,18),
    BND_ICON_MESH_CONE = BND_ICONID(9,18),
    BND_ICON_LAMP_POINT = BND_ICONID(12,18),
    BND_ICON_LAMP_SUN = BND_ICONID(13,18),
    BND_ICON_LAMP_SPOT = BND_ICONID(14,18),
    BND_ICON_LAMP_HEMI = BND_ICONID(15,18),
    BND_ICON_LAMP_AREA = BND_ICONID(16,18),
    BND_ICON_META_EMPTY = BND_ICONID(19,18),
    BND_ICON_META_PLANE = BND_ICONID(20,18),
    BND_ICON_META_CUBE = BND_ICONID(21,18),
    BND_ICON_META_BALL = BND_ICONID(22,18),
    BND_ICON_META_ELLIPSOID = BND_ICONID(23,18),
    BND_ICON_META_CAPSULE = BND_ICONID(24,18),

    BND_ICON_SURFACE_NCURVE = BND_ICONID(0,17),
    BND_ICON_SURFACE_NCIRCLE = BND_ICONID(1,17),
    BND_ICON_SURFACE_NSURFACE = BND_ICONID(2,17),
    BND_ICON_SURFACE_NCYLINDER = BND_ICONID(3,17),
    BND_ICON_SURFACE_NSPHERE = BND_ICONID(4,17),
    BND_ICON_SURFACE_NTORUS = BND_ICONID(5,17),
    BND_ICON_CURVE_BEZCURVE = BND_ICONID(9,17),
    BND_ICON_CURVE_BEZCIRCLE = BND_ICONID(10,17),
    BND_ICON_CURVE_NCURVE = BND_ICONID(11,17),
    BND_ICON_CURVE_NCIRCLE = BND_ICONID(12,17),
    BND_ICON_CURVE_PATH = BND_ICONID(13,17),
    BND_ICON_COLOR_RED = BND_ICONID(19,17),
    BND_ICON_COLOR_GREEN = BND_ICONID(20,17),
    BND_ICON_COLOR_BLUE = BND_ICONID(21,17),

    BND_ICON_FORCE_FORCE = BND_ICONID(0,16),
    BND_ICON_FORCE_WIND = BND_ICONID(1,16),
    BND_ICON_FORCE_VORTEX = BND_ICONID(2,16),
    BND_ICON_FORCE_MAGNETIC = BND_ICONID(3,16),
    BND_ICON_FORCE_HARMONIC = BND_ICONID(4,16),
    BND_ICON_FORCE_CHARGE = BND_ICONID(5,16),
    BND_ICON_FORCE_LENNARDJONES = BND_ICONID(6,16),
    BND_ICON_FORCE_TEXTURE = BND_ICONID(7,16),
    BND_ICON_FORCE_CURVE = BND_ICONID(8,16),
    BND_ICON_FORCE_BOID = BND_ICONID(9,16),
    BND_ICON_FORCE_TURBULENCE = BND_ICONID(10,16),
    BND_ICON_FORCE_DRAG = BND_ICONID(11,16),
    BND_ICON_FORCE_SMOKEFLOW = BND_ICONID(12,16),

    BND_ICON_MODIFIER = BND_ICONID(0,12),
    BND_ICON_MOD_WAVE = BND_ICONID(1,12),
    BND_ICON_MOD_BUILD = BND_ICONID(2,12),
    BND_ICON_MOD_DECIM = BND_ICONID(3,12),
    BND_ICON_MOD_MIRROR = BND_ICONID(4,12),
    BND_ICON_MOD_SOFT = BND_ICONID(5,12),
    BND_ICON_MOD_SUBSURF = BND_ICONID(6,12),
    BND_ICON_HOOK = BND_ICONID(7,12),
    BND_ICON_MOD_PHYSICS = BND_ICONID(8,12),
    BND_ICON_MOD_PARTICLES = BND_ICONID(9,12),
    BND_ICON_MOD_BOOLEAN = BND_ICONID(10,12),
    BND_ICON_MOD_EDGESPLIT = BND_ICONID(11,12),
    BND_ICON_MOD_ARRAY = BND_ICONID(12,12),
    BND_ICON_MOD_UVPROJECT = BND_ICONID(13,12),
    BND_ICON_MOD_DISPLACE = BND_ICONID(14,12),
    BND_ICON_MOD_CURVE = BND_ICONID(15,12),
    BND_ICON_MOD_LATTICE = BND_ICONID(16,12),
    BND_ICON_CONSTRAINT_DATA = BND_ICONID(17,12),
    BND_ICON_MOD_ARMATURE = BND_ICONID(18,12),
    BND_ICON_MOD_SHRINKWRAP = BND_ICONID(19,12),
    BND_ICON_MOD_CAST = BND_ICONID(20,12),
    BND_ICON_MOD_MESHDEFORM = BND_ICONID(21,12),
    BND_ICON_MOD_BEVEL = BND_ICONID(22,12),
    BND_ICON_MOD_SMOOTH = BND_ICONID(23,12),
    BND_ICON_MOD_SIMPLEDEFORM = BND_ICONID(24,12),
    BND_ICON_MOD_MASK = BND_ICONID(25,12),

    BND_ICON_MOD_CLOTH = BND_ICONID(0,11),
    BND_ICON_MOD_EXPLODE = BND_ICONID(1,11),
    BND_ICON_MOD_FLUIDSIM = BND_ICONID(2,11),
    BND_ICON_MOD_MULTIRES = BND_ICONID(3,11),
    BND_ICON_MOD_SMOKE = BND_ICONID(4,11),
    BND_ICON_MOD_SOLIDIFY = BND_ICONID(5,11),
    BND_ICON_MOD_SCREW = BND_ICONID(6,11),
    BND_ICON_MOD_VERTEX_WEIGHT = BND_ICONID(7,11),
    BND_ICON_MOD_DYNAMICPAINT = BND_ICONID(8,11),
    BND_ICON_MOD_REMESH = BND_ICONID(9,11),
    BND_ICON_MOD_OCEAN = BND_ICONID(10,11),
    BND_ICON_MOD_WARP = BND_ICONID(11,11),
    BND_ICON_MOD_SKIN = BND_ICONID(12,11),
    BND_ICON_MOD_TRIANGULATE = BND_ICONID(13,11),
    BND_ICON_MOD_WIREFRAME = BND_ICONID(14,11),

    BND_ICON_REC = BND_ICONID(0,10),
    BND_ICON_PLAY = BND_ICONID(1,10),
    BND_ICON_FF = BND_ICONID(2,10),
    BND_ICON_REW = BND_ICONID(3,10),
    BND_ICON_PAUSE = BND_ICONID(4,10),
    BND_ICON_PREV_KEYFRAME = BND_ICONID(5,10),
    BND_ICON_NEXT_KEYFRAME = BND_ICONID(6,10),
    BND_ICON_PLAY_AUDIO = BND_ICONID(7,10),
    BND_ICON_PLAY_REVERSE = BND_ICONID(8,10),
    BND_ICON_PREVIEW_RANGE = BND_ICONID(9,10),
    BND_ICON_ACTION_TWEAK = BND_ICONID(10,10),
    BND_ICON_PMARKER_ACT = BND_ICONID(11,10),
    BND_ICON_PMARKER_SEL = BND_ICONID(12,10),
    BND_ICON_PMARKER = BND_ICONID(13,10),
    BND_ICON_MARKER_HLT = BND_ICONID(14,10),
    BND_ICON_MARKER = BND_ICONID(15,10),
    BND_ICON_SPACE2 = BND_ICONID(16,10),
    BND_ICON_SPACE3 = BND_ICONID(17,10),
    BND_ICON_KEYINGSET = BND_ICONID(18,10),
    BND_ICON_KEY_DEHLT = BND_ICONID(19,10),
    BND_ICON_KEY_HLT = BND_ICONID(20,10),
    BND_ICON_MUTE_IPO_OFF = BND_ICONID(21,10),
    BND_ICON_MUTE_IPO_ON = BND_ICONID(22,10),
    BND_ICON_VISIBLE_IPO_OFF = BND_ICONID(23,10),
    BND_ICON_VISIBLE_IPO_ON = BND_ICONID(24,10),
    BND_ICON_DRIVER = BND_ICONID(25,10),

    BND_ICON_SOLO_OFF = BND_ICONID(0,9),
    BND_ICON_SOLO_ON = BND_ICONID(1,9),
    BND_ICON_FRAME_PREV = BND_ICONID(2,9),
    BND_ICON_FRAME_NEXT = BND_ICONID(3,9),
    BND_ICON_NLA_PUSHDOWN = BND_ICONID(4,9),
    BND_ICON_IPO_CONSTANT = BND_ICONID(5,9),
    BND_ICON_IPO_LINEAR = BND_ICONID(6,9),
    BND_ICON_IPO_BEZIER = BND_ICONID(7,9),
    BND_ICON_IPO_SINE = BND_ICONID(8,9),
    BND_ICON_IPO_QUAD = BND_ICONID(9,9),
    BND_ICON_IPO_CUBIC = BND_ICONID(10,9),
    BND_ICON_IPO_QUART = BND_ICONID(11,9),
    BND_ICON_IPO_QUINT = BND_ICONID(12,9),
    BND_ICON_IPO_EXPO = BND_ICONID(13,9),
    BND_ICON_IPO_CIRC = BND_ICONID(14,9),
    BND_ICON_IPO_BOUNCE = BND_ICONID(15,9),
    BND_ICON_IPO_ELASTIC = BND_ICONID(16,9),
    BND_ICON_IPO_BACK = BND_ICONID(17,9),
    BND_ICON_IPO_EASE_IN = BND_ICONID(18,9),
    BND_ICON_IPO_EASE_OUT = BND_ICONID(19,9),
    BND_ICON_IPO_EASE_IN_OUT = BND_ICONID(20,9),

    BND_ICON_VERTEXSEL = BND_ICONID(0,8),
    BND_ICON_EDGESEL = BND_ICONID(1,8),
    BND_ICON_FACESEL = BND_ICONID(2,8),
    BND_ICON_LOOPSEL = BND_ICONID(3,8),
    BND_ICON_ROTATE = BND_ICONID(5,8),
    BND_ICON_CURSOR = BND_ICONID(6,8),
    BND_ICON_ROTATECOLLECTION = BND_ICONID(7,8),
    BND_ICON_ROTATECENTER = BND_ICONID(8,8),
    BND_ICON_ROTACTIVE = BND_ICONID(9,8),
    BND_ICON_ALIGN = BND_ICONID(10,8),
    BND_ICON_SMOOTHCURVE = BND_ICONID(12,8),
    BND_ICON_SPHERECURVE = BND_ICONID(13,8),
    BND_ICON_ROOTCURVE = BND_ICONID(14,8),
    BND_ICON_SHARPCURVE = BND_ICONID(15,8),
    BND_ICON_LINCURVE = BND_ICONID(16,8),
    BND_ICON_NOCURVE = BND_ICONID(17,8),
    BND_ICON_RNDCURVE = BND_ICONID(18,8),
    BND_ICON_PROP_OFF = BND_ICONID(19,8),
    BND_ICON_PROP_ON = BND_ICONID(20,8),
    BND_ICON_PROP_CON = BND_ICONID(21,8),
    BND_ICON_SCULPT_DYNTOPO = BND_ICONID(22,8),
    BND_ICON_PARTICLE_POINT = BND_ICONID(23,8),
    BND_ICON_PARTICLE_TIP = BND_ICONID(24,8),
    BND_ICON_PARTICLE_PATH = BND_ICONID(25,8),

    BND_ICON_MAN_TRANS = BND_ICONID(0,7),
    BND_ICON_MAN_ROT = BND_ICONID(1,7),
    BND_ICON_MAN_SCALE = BND_ICONID(2,7),
    BND_ICON_MANIPUL = BND_ICONID(3,7),
    BND_ICON_SNAP_OFF = BND_ICONID(4,7),
    BND_ICON_SNAP_ON = BND_ICONID(5,7),
    BND_ICON_SNAP_NORMAL = BND_ICONID(6,7),
    BND_ICON_SNAP_INCREMENT = BND_ICONID(7,7),
    BND_ICON_SNAP_VERTEX = BND_ICONID(8,7),
    BND_ICON_SNAP_EDGE = BND_ICONID(9,7),
    BND_ICON_SNAP_FACE = BND_ICONID(10,7),
    BND_ICON_SNAP_VOLUME = BND_ICONID(11,7),
    BND_ICON_STICKY_UVS_LOC = BND_ICONID(13,7),
    BND_ICON_STICKY_UVS_DISABLE = BND_ICONID(14,7),
    BND_ICON_STICKY_UVS_VERT = BND_ICONID(15,7),
    BND_ICON_CLIPUV_DEHLT = BND_ICONID(16,7),
    BND_ICON_CLIPUV_HLT = BND_ICONID(17,7),
    BND_ICON_SNAP_PEEL_OBJECT = BND_ICONID(18,7),
    BND_ICON_GRID = BND_ICONID(19,7),

    BND_ICON_PASTEDOWN = BND_ICONID(0,6),
    BND_ICON_COPYDOWN = BND_ICONID(1,6),
    BND_ICON_PASTEFLIPUP = BND_ICONID(2,6),
    BND_ICON_PASTEFLIPDOWN = BND_ICONID(3,6),
    BND_ICON_SNAP_SURFACE = BND_ICONID(8,6),
    BND_ICON_AUTOMERGE_ON = BND_ICONID(9,6),
    BND_ICON_AUTOMERGE_OFF = BND_ICONID(10,6),
    BND_ICON_RETOPO = BND_ICONID(11,6),
    BND_ICON_UV_VERTEXSEL = BND_ICONID(12,6),
    BND_ICON_UV_EDGESEL = BND_ICONID(13,6),
    BND_ICON_UV_FACESEL = BND_ICONID(14,6),
    BND_ICON_UV_ISLANDSEL = BND_ICONID(15,6),
    BND_ICON_UV_SYNC_SELECT = BND_ICONID(16,6),

    BND_ICON_BBOX = BND_ICONID(0,5),
    BND_ICON_WIRE = BND_ICONID(1,5),
    BND_ICON_SOLID = BND_ICONID(2,5),
    BND_ICON_SMOOTH = BND_ICONID(3,5),
    BND_ICON_POTATO = BND_ICONID(4,5),
    BND_ICON_ORTHO = BND_ICONID(6,5),
    BND_ICON_LOCKVIEW_OFF = BND_ICONID(9,5),
    BND_ICON_LOCKVIEW_ON = BND_ICONID(10,5),
    BND_ICON_AXIS_SIDE = BND_ICONID(12,5),
    BND_ICON_AXIS_FRONT = BND_ICONID(13,5),
    BND_ICON_AXIS_TOP = BND_ICONID(14,5),
    BND_ICON_NDOF_DOM = BND_ICONID(15,5),
    BND_ICON_NDOF_TURN = BND_ICONID(16,5),
    BND_ICON_NDOF_FLY = BND_ICONID(17,5),
    BND_ICON_NDOF_TRANS = BND_ICONID(18,5),
    BND_ICON_LAYER_USED = BND_ICONID(19,5),
    BND_ICON_LAYER_ACTIVE = BND_ICONID(20,5),

    BND_ICON_SORTALPHA = BND_ICONID(0,3),
    BND_ICON_SORTBYEXT = BND_ICONID(1,3),
    BND_ICON_SORTTIME = BND_ICONID(2,3),
    BND_ICON_SORTSIZE = BND_ICONID(3,3),
    BND_ICON_LONGDISPLAY = BND_ICONID(4,3),
    BND_ICON_SHORTDISPLAY = BND_ICONID(5,3),
    BND_ICON_GHOST = BND_ICONID(6,3),
    BND_ICON_IMGDISPLAY = BND_ICONID(7,3),
    BND_ICON_SAVE_AS = BND_ICONID(8,3),
    BND_ICON_SAVE_COPY = BND_ICONID(9,3),
    BND_ICON_BOOKMARKS = BND_ICONID(10,3),
    BND_ICON_FONTPREVIEW = BND_ICONID(11,3),
    BND_ICON_FILTER = BND_ICONID(12,3),
    BND_ICON_NEWFOLDER = BND_ICONID(13,3),
    BND_ICON_OPEN_RECENT = BND_ICONID(14,3),
    BND_ICON_FILE_PARENT = BND_ICONID(15,3),
    BND_ICON_FILE_REFRESH = BND_ICONID(16,3),
    BND_ICON_FILE_FOLDER = BND_ICONID(17,3),
    BND_ICON_FILE_BLANK = BND_ICONID(18,3),
    BND_ICON_FILE_BLEND = BND_ICONID(19,3),
    BND_ICON_FILE_IMAGE = BND_ICONID(20,3),
    BND_ICON_FILE_MOVIE = BND_ICONID(21,3),
    BND_ICON_FILE_SCRIPT = BND_ICONID(22,3),
    BND_ICON_FILE_SOUND = BND_ICONID(23,3),
    BND_ICON_FILE_FONT = BND_ICONID(24,3),
    BND_ICON_FILE_TEXT = BND_ICONID(25,3),

    BND_ICON_RECOVER_AUTO = BND_ICONID(0,2),
    BND_ICON_SAVE_PREFS = BND_ICONID(1,2),
    BND_ICON_LINK_BLEND = BND_ICONID(2,2),
    BND_ICON_APPEND_BLEND = BND_ICONID(3,2),
    BND_ICON_IMPORT = BND_ICONID(4,2),
    BND_ICON_EXPORT = BND_ICONID(5,2),
    BND_ICON_EXTERNAL_DATA = BND_ICONID(6,2),
    BND_ICON_LOAD_FACTORY = BND_ICONID(7,2),
    BND_ICON_LOOP_BACK = BND_ICONID(13,2),
    BND_ICON_LOOP_FORWARDS = BND_ICONID(14,2),
    BND_ICON_BACK = BND_ICONID(15,2),
    BND_ICON_FORWARD = BND_ICONID(16,2),
    BND_ICON_FILE_BACKUP = BND_ICONID(24,2),
    BND_ICON_DISK_DRIVE = BND_ICONID(25,2),

    BND_ICON_MATPLANE = BND_ICONID(0,1),
    BND_ICON_MATSPHERE = BND_ICONID(1,1),
    BND_ICON_MATCUBE = BND_ICONID(2,1),
    BND_ICON_MONKEY = BND_ICONID(3,1),
    BND_ICON_HAIR = BND_ICONID(4,1),
    BND_ICON_ALIASED = BND_ICONID(5,1),
    BND_ICON_ANTIALIASED = BND_ICONID(6,1),
    BND_ICON_MAT_SPHERE_SKY = BND_ICONID(7,1),
    BND_ICON_WORDWRAP_OFF = BND_ICONID(12,1),
    BND_ICON_WORDWRAP_ON = BND_ICONID(13,1),
    BND_ICON_SYNTAX_OFF = BND_ICONID(14,1),
    BND_ICON_SYNTAX_ON = BND_ICONID(15,1),
    BND_ICON_LINENUMBERS_OFF = BND_ICONID(16,1),
    BND_ICON_LINENUMBERS_ON = BND_ICONID(17,1),
    BND_ICON_SCRIPTPLUGINS = BND_ICONID(18,1),

    BND_ICON_SEQ_SEQUENCER = BND_ICONID(0,0),
    BND_ICON_SEQ_PREVIEW = BND_ICONID(1,0),
    BND_ICON_SEQ_LUMA_WAVEFORM = BND_ICONID(2,0),
    BND_ICON_SEQ_CHROMA_SCOPE = BND_ICONID(3,0),
    BND_ICON_SEQ_HISTOGRAM = BND_ICONID(4,0),
    BND_ICON_SEQ_SPLITVIEW = BND_ICONID(5,0),
    BND_ICON_IMAGE_RGB = BND_ICONID(9,0),
    BND_ICON_IMAGE_RGB_ALPHA = BND_ICONID(10,0),
    BND_ICON_IMAGE_ALPHA = BND_ICONID(11,0),
    BND_ICON_IMAGE_ZDEPTH = BND_ICONID(12,0),
    BND_ICON_IMAGEFILE = BND_ICONID(13,0),
} BNDicon;

////////////////////////////////////////////////////////////////////////////////

// set the current theme all widgets will be drawn with.
// the default Blender 2.6 theme is set by default.
BND_EXPORT void bndSetTheme(BNDtheme theme);

// Returns the currently set theme
BND_EXPORT const BNDtheme *bndGetTheme();

// designates an image handle as returned by nvgCreateImage*() as the themes'
// icon sheet. The icon sheet format must be compatible to Blender 2.6's icon
// sheet; the order of icons does not matter.
// A valid icon sheet is e.g. shown at
// http://wiki.blender.org/index.php/Dev:2.5/Doc/How_to/Add_an_icon
BND_EXPORT void bndSetIconImage(int image);

// designates an image handle as returned by nvgCreateFont*() as the themes'
// UI font. Blender's original UI font Droid Sans is perfectly suited and
// available here:
// https://svn.blender.org/svnroot/bf-blender/trunk/blender/release/datafiles/fonts/
BND_EXPORT void bndSetFont(int font);

////////////////////////////////////////////////////////////////////////////////

// High Level Functions
// --------------------
// Use these functions to draw themed widgets with your NVGcontext.

// Draw a label with its lower left origin at (x,y) and size of (w,h).
// if iconid >= 0, an icon will be added to the widget
// if label is not NULL, a label will be added to the widget
// widget looks best when height is BND_WIDGET_HEIGHT
BND_EXPORT void bndLabel(NVGcontext *ctx,
    float x, float y, float w, float h, int iconid, const char *label);

// Draw a tool button  with its lower left origin at (x,y) and size of (w,h),
// where flags is one or multiple flags from BNDcornerFlags and state denotes
// the widgets current UI state.
// if iconid >= 0, an icon will be added to the widget
// if label is not NULL, a label will be added to the widget
// widget looks best when height is BND_WIDGET_HEIGHT
BND_EXPORT void bndToolButton(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, BNDwidgetState state,
    int iconid, const char *label);

// Draw a radio button with its lower left origin at (x,y) and size of (w,h),
// where flags is one or multiple flags from BNDcornerFlags and state denotes
// the widgets current UI state.
// if iconid >= 0, an icon will be added to the widget
// if label is not NULL, a label will be added to the widget
// widget looks best when height is BND_WIDGET_HEIGHT
BND_EXPORT void bndRadioButton(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, BNDwidgetState state,
    int iconid, const char *label);


// Calculate the corresponding text position for given coordinates px/py
// in a text field.
// See bndTextField for more info.
BND_EXPORT int bndTextFieldTextPosition(NVGcontext *ctx, float x, float y, float w, float h,
    int iconid, const char *text, int px, int py);

// Draw a text field with its lower left origin at (x,y) and size of (w,h),
// where flags is one or multiple flags from BNDcornerFlags and state denotes
// the widgets current UI state.
// if iconid >= 0, an icon will be added to the widget
// if text is not NULL, text will be printed to the widget
// cbegin must be >= 0 and <= strlen(text) and denotes the beginning of the caret
// cend must be >= cbegin and <= strlen(text) and denotes the end of the caret
// if cend < cbegin, then no caret will be drawn
// widget looks best when height is BND_WIDGET_HEIGHT
BND_EXPORT void bndTextField(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, BNDwidgetState state,
    int iconid, const char *text, int cbegin, int cend);

// Draw an option button with its lower left origin at (x,y) and size of (w,h),
// where flags is one or multiple flags from BNDcornerFlags and state denotes
// the widgets current UI state.
// if label is not NULL, a label will be added to the widget
// widget looks best when height is BND_WIDGET_HEIGHT
BND_EXPORT void bndOptionButton(NVGcontext *ctx,
    float x, float y, float w, float h, BNDwidgetState state,
    const char *label);

// Draw a choice button with its lower left origin at (x,y) and size of (w,h),
// where flags is one or multiple flags from BNDcornerFlags and state denotes
// the widgets current UI state.
// if iconid >= 0, an icon will be added to the widget
// if label is not NULL, a label will be added to the widget
// widget looks best when height is BND_WIDGET_HEIGHT
BND_EXPORT void bndChoiceButton(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, BNDwidgetState state,
    int iconid, const char *label);

// Draw a color button  with its lower left origin at (x,y) and size of (w,h),
// where flags is one or multiple flags from BNDcornerFlags and state denotes
// the widgets current UI state.
// widget looks best when height is BND_WIDGET_HEIGHT
BND_EXPORT void bndColorButton(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, NVGcolor color);

// Draw a number field with its lower left origin at (x,y) and size of (w,h),
// where flags is one or multiple flags from BNDcornerFlags and state denotes
// the widgets current UI state.
// if label is not NULL, a label will be added to the widget
// if value is not NULL, a value will be added to the widget, along with
// a ":" separator
// widget looks best when height is BND_WIDGET_HEIGHT
BND_EXPORT void bndNumberField(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, BNDwidgetState state,
    const char *label, const char *value);

// Draw slider control with its lower left origin at (x,y) and size of (w,h),
// where flags is one or multiple flags from BNDcornerFlags and state denotes
// the widgets current UI state.
// progress must be in the range 0..1 and controls the size of the slider bar
// if label is not NULL, a label will be added to the widget
// if value is not NULL, a value will be added to the widget, along with
// a ":" separator
// widget looks best when height is BND_WIDGET_HEIGHT
BND_EXPORT void bndSlider(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, BNDwidgetState state,
    float progress, const char *label, const char *value);

// Draw scrollbar with its lower left origin at (x,y) and size of (w,h),
// where state denotes the widgets current UI state.
// offset is in the range 0..1 and controls the position of the scroll handle
// size is in the range 0..1 and controls the size of the scroll handle
// horizontal widget looks best when height is BND_SCROLLBAR_HEIGHT,
// vertical looks best when width is BND_SCROLLBAR_WIDTH
BND_EXPORT void bndScrollBar(NVGcontext *ctx,
    float x, float y, float w, float h, BNDwidgetState state,
    float offset, float size);

// Draw a menu background with its lower left origin at (x,y) and size of (w,h),
// where flags is one or multiple flags from BNDcornerFlags.
BND_EXPORT void bndMenuBackground(NVGcontext *ctx,
    float x, float y, float w, float h, int flags);

// Draw a menu label with its lower left origin at (x,y) and size of (w,h).
// if iconid >= 0, an icon will be added to the widget
// if label is not NULL, a label will be added to the widget
// widget looks best when height is BND_WIDGET_HEIGHT
BND_EXPORT void bndMenuLabel(NVGcontext *ctx,
    float x, float y, float w, float h, int iconid, const char *label);

// Draw a menu item with its lower left origin at (x,y) and size of (w,h),
// where state denotes the widgets current UI state.
// if iconid >= 0, an icon will be added to the widget
// if label is not NULL, a label will be added to the widget
// widget looks best when height is BND_WIDGET_HEIGHT
BND_EXPORT void bndMenuItem(NVGcontext *ctx,
    float x, float y, float w, float h, BNDwidgetState state,
    int iconid, const char *label);

// Draw a tooltip background with its lower left origin at (x,y) and size of (w,h)
BND_EXPORT void bndTooltipBackground(NVGcontext *ctx, float x, float y, float w, float h);

// Draw a node port at the given position filled with the given color
BND_EXPORT void bndNodePort(NVGcontext *ctx, float x, float y, BNDwidgetState state,
    NVGcolor color);

// Draw a node wire originating at (x0,y0) and floating to (x1,y1), with
// a colored gradient based on the states state0 and state1:
// BND_DEFAULT: default wire color
// BND_HOVER: selected wire color
// BND_ACTIVE: dragged wire color
BND_EXPORT void bndNodeWire(NVGcontext *ctx, float x0, float y0, float x1, float y1,
    BNDwidgetState state0, BNDwidgetState state1);

// Draw a node wire originating at (x0,y0) and floating to (x1,y1), with
// a colored gradient based on the two colors color0 and color1
BND_EXPORT void bndColoredNodeWire(NVGcontext *ctx, float x0, float y0, float x1, float y1,
    NVGcolor color0, NVGcolor color1);

// Draw a node background with its upper left origin at (x,y) and size of (w,h)
// where titleColor provides the base color for the title bar
BND_EXPORT void bndNodeBackground(NVGcontext *ctx, float x, float y, float w, float h,
    BNDwidgetState state, int iconid, const char *label, NVGcolor titleColor);

// Draw a window with the upper right and lower left splitter widgets into
// the rectangle at origin (x,y) and size (w, h)
BND_EXPORT void bndSplitterWidgets(NVGcontext *ctx, float x, float y, float w, float h);

// Draw the join area overlay stencil into the rectangle
// at origin (x,y) and size (w,h)
// vertical is 0 or 1 and designates the arrow orientation,
// mirror is 0 or 1 and flips the arrow side
BND_EXPORT void bndJoinAreaOverlay(NVGcontext *ctx, float x, float y, float w, float h,
    int vertical, int mirror);

////////////////////////////////////////////////////////////////////////////////

// Estimator Functions
// -------------------
// Use these functions to estimate sizes for widgets with your NVGcontext.

// returns the ideal width for a label with given icon and text
BND_EXPORT float bndLabelWidth(NVGcontext *ctx, int iconid, const char *label);

// returns the height for a label with given icon, text and width; this
// function is primarily useful in conjunction with multiline labels and textboxes
BND_EXPORT float bndLabelHeight(NVGcontext *ctx, int iconid, const char *label,
    float width);

////////////////////////////////////////////////////////////////////////////////

// Low Level Functions
// -------------------
// these are part of the implementation detail and can be used to theme
// new kinds of controls in a similar fashion.

// make color transparent using the default alpha value
BND_EXPORT NVGcolor bndTransparent(NVGcolor color);

// offset a color by a given integer delta in the range -100 to 100
BND_EXPORT NVGcolor bndOffsetColor(NVGcolor color, int delta);

// assigns radius r to the four entries of array radiuses depending on whether
// the corner is marked as sharp or not; see BNDcornerFlags for possible
// flag values.
BND_EXPORT void bndSelectCorners(float *radiuses, float r, int flags);

// computes the upper and lower gradient colors for the inner box from a widget
// theme and the widgets state. If flipActive is set and the state is
// BND_ACTIVE, the upper and lower colors will be swapped.
BND_EXPORT void bndInnerColors(NVGcolor *shade_top, NVGcolor *shade_down,
    const BNDwidgetTheme *theme, BNDwidgetState state, int flipActive);

// computes the text color for a widget label from a widget theme and the
// widgets state.
BND_EXPORT NVGcolor bndTextColor(const BNDwidgetTheme *theme, BNDwidgetState state);

// computes the bounds of the scrollbar handle from the scrollbar size
// and the handles offset and size.
// offset is in the range 0..1 and defines the position of the scroll handle
// size is in the range 0..1 and defines the size of the scroll handle
BND_EXPORT void bndScrollHandleRect(float *x, float *y, float *w, float *h,
    float offset, float size);

// Add a rounded box path at position (x,y) with size (w,h) and a separate
// radius for each corner listed in clockwise order, so that cr0 = top left,
// cr1 = top right, cr2 = bottom right, cr3 = bottom left;
// this is a low level drawing function: the path must be stroked or filled
// to become visible.
BND_EXPORT void bndRoundedBox(NVGcontext *ctx, float x, float y, float w, float h,
    float cr0, float cr1, float cr2, float cr3);

// Draw a flat panel without any decorations at position (x,y) with size (w,h)
// and fills it with backgroundColor
BND_EXPORT void bndBackground(NVGcontext *ctx, float x, float y, float w, float h);

// Draw a beveled border at position (x,y) with size (w,h) shaded with
// lighter and darker versions of backgroundColor
BND_EXPORT void bndBevel(NVGcontext *ctx, float x, float y, float w, float h);

// Draw a lower inset for a rounded box at position (x,y) with size (w,h)
// that gives the impression the surface has been pushed in.
// cr2 and cr3 contain the radiuses of the bottom right and bottom left
// corners of the rounded box.
BND_EXPORT void bndBevelInset(NVGcontext *ctx, float x, float y, float w, float h,
    float cr2, float cr3);

// Draw an icon with (x,y) as its upper left coordinate; the iconid selects
// the icon from the sheet; use the BND_ICONID macro to build icon IDs.
BND_EXPORT void bndIcon(NVGcontext *ctx, float x, float y, int iconid);

// Draw a drop shadow around the rounded box at (x,y) with size (w,h) and
// radius r, with feather as its maximum range in pixels.
// No shadow will be painted inside the rounded box.
BND_EXPORT void bndDropShadow(NVGcontext *ctx, float x, float y, float w, float h,
    float r, float feather, float alpha);

// Draw the inner part of a widget box, with a gradient from shade_top to
// shade_down. If h>w, the gradient will be horizontal instead of
// vertical.
BND_EXPORT void bndInnerBox(NVGcontext *ctx, float x, float y, float w, float h,
    float cr0, float cr1, float cr2, float cr3,
    NVGcolor shade_top, NVGcolor shade_down);

// Draw the outline part of a widget box with the given color
BND_EXPORT void bndOutlineBox(NVGcontext *ctx, float x, float y, float w, float h,
    float cr0, float cr1, float cr2, float cr3, NVGcolor color);

// Draw an optional icon specified by <iconid> and an optional label with
// given alignment (BNDtextAlignment), fontsize and color within a widget box.
// if iconid is >= 0, an icon will be drawn and the labels remaining space
// will be adjusted.
// if label is not NULL, it will be drawn with the specified alignment, fontsize
// and color.
// if value is not NULL, label and value will be drawn with a ":" separator
// inbetween.
BND_EXPORT void bndIconLabelValue(NVGcontext *ctx, float x, float y, float w, float h,
    int iconid, NVGcolor color, int align, float fontsize, const char *label,
    const char *value);

// Draw an optional icon specified by <iconid> and an optional label with
// given alignment (BNDtextAlignment), fontsize and color within a node title bar
// if iconid is >= 0, an icon will be drawn
// if label is not NULL, it will be drawn with the specified alignment, fontsize
// and color.
BND_EXPORT void bndNodeIconLabel(NVGcontext *ctx, float x, float y, float w, float h,
    int iconid, NVGcolor color, NVGcolor shadowColor, int align,
    float fontsize, const char *label);

// Calculate the corresponding text position for given coordinates px/py
// in an iconLabel.
// See bndIconLabelCaret for more info.
BND_EXPORT int bndIconLabelTextPosition(NVGcontext *ctx, float x, float y, float w, float h,
    int iconid, float fontsize, const char *label, int px, int py);

// Draw an optional icon specified by <iconid>, an optional label and
// a caret with given fontsize and color within a widget box.
// if iconid is >= 0, an icon will be drawn and the labels remaining space
// will be adjusted.
// if label is not NULL, it will be drawn with the specified alignment, fontsize
// and color.
// cbegin must be >= 0 and <= strlen(text) and denotes the beginning of the caret
// cend must be >= cbegin and <= strlen(text) and denotes the end of the caret
// if cend < cbegin, then no caret will be drawn
BND_EXPORT void bndIconLabelCaret(NVGcontext *ctx, float x, float y, float w, float h,
    int iconid, NVGcolor color, float fontsize, const char *label,
    NVGcolor caretcolor, int cbegin, int cend);

// Draw a checkmark for an option box with the given upper left coordinates
// (ox,oy) with the specified color.
BND_EXPORT void bndCheck(NVGcontext *ctx, float ox, float oy, NVGcolor color);

// Draw a horizontal arrow for a number field with its center at (x,y) and
// size s; if s is negative, the arrow points to the left.
BND_EXPORT void bndArrow(NVGcontext *ctx, float x, float y, float s, NVGcolor color);

// Draw an up/down arrow for a choice box with its center at (x,y) and size s
BND_EXPORT void bndUpDownArrow(NVGcontext *ctx, float x, float y, float s, NVGcolor color);

// Draw a node down-arrow with its tip at (x,y) and size s
BND_EXPORT void bndNodeArrowDown(NVGcontext *ctx, float x, float y, float s, NVGcolor color);

// return the color of a node wire based on state
// BND_HOVER indicates selected state,
// BND_ACTIVE indicates dragged state
BND_EXPORT NVGcolor bndNodeWireColor(const BNDnodeTheme *theme, BNDwidgetState state);









// FILE: perf.h

enum GraphrenderStyle {
    GRAPH_RENDER_FPS,
    GRAPH_RENDER_MS,
    GRAPH_RENDER_PERCENT,
};

#define GRAPH_HISTORY_COUNT 100
struct PerfGraph {
    int style;
    char name[32];
    float values[GRAPH_HISTORY_COUNT];
    int head;
};
typedef struct PerfGraph PerfGraph;

void initGraph(PerfGraph* fps, int style, const char* name);
void updateGraph(PerfGraph* fps, float frameTime);
void renderGraph(NVGcontext* vg, float x, float y, PerfGraph* fps);
float getGraphAverage(PerfGraph* fps);

#define GPU_QUERY_COUNT 5
struct GPUtimer {
    int supported;
    int cur, ret;
    unsigned int queries[GPU_QUERY_COUNT];
};
typedef struct GPUtimer GPUtimer;

void initGPUTimer(GPUtimer* timer);
void startGPUTimer(GPUtimer* timer);
int stopGPUTimer(GPUtimer* timer, float* times, int maxTimes);




// FILE: demo.h


struct DemoData {
    int fontNormal, fontBold, fontIcons, fontEmoji, fontMono;
    int images[12];
};
typedef struct DemoData DemoData;

int loadDemoData(NVGcontext* vg, DemoData* data);
void freeDemoData(NVGcontext* vg, DemoData* data);
void renderDemo(NVGcontext* vg, float mx, float my, float width, float height, float t, int blowup, DemoData* data);

void saveScreenShot(int w, int h, int premult, const char* name);
