(function() {

  //Enums copied from ppb_input_event.h
  var PP_InputEvent_Type = {
    UNDEFINED: -1,
    MOUSEDOWN: 0,
    MOUSEUP: 1,
    MOUSEMOVE: 2,
    MOUSEENTER: 3,
    MOUSELEAVE: 4,
    WHEEL: 5,
    RAWKEYDOWN: 6,
    KEYDOWN: 7,
    KEYUP: 8,
    CHAR: 9,
    CONTEXTMENU: 10,
    IME_COMPOSITION_START: 11,
    IME_COMPOSITION_UPDATE: 12,
    IME_COMPOSITION_END: 13,
    IME_TEXT: 14,
    TOUCHSTART: 15,
    TOUCHMOVE: 16,
    TOUCHEND: 17,
    TOUCHCANCEL: 18
  };
  var PPIE_Type = PP_InputEvent_Type;

  var PP_InputEvent_MouseButton = {
    NONE: -1,
    LEFT: 0,
    MIDDLE: 1,
    RIGHT: 2
  };
  var PPIE_MouseButton = PP_InputEvent_MouseButton;

  var PP_InputEvent_Modifier = {
    SHIFTKEY: 1 << 0,
    CONTROLKEY: 1 << 1,
    ALTKEY: 1 << 2,
    METAKEY: 1 << 3,
    ISKEYPAD: 1 << 4,
    ISAUTOREPEAT: 1 << 5,
    LEFTBUTTONDOWN: 1 << 6,
    MIDDLEBUTTONDOWN: 1 << 7,
    RIGHTBUTTONDOWN: 1 << 8,
    CAPSLOCKKEY: 1 << 9,
    NUMLOCKKEY: 1 << 10,
    ISLEFT: 1 << 11,
    ISRIGHT: 1 << 12
  };
  var PPIE_Modifier = PP_InputEvent_Modifier;

  var PP_InputEvent_Class = {
    MOUSE: 1 << 0,
    KEYBOARD: 1 << 1,
    WHEEL: 1 << 2,
    TOUCH: 1 << 3,
    IME: 1 << 4
  };
  var PPIE_Class = PP_InputEvent_Class;

  var mod_masks = {
    shiftKey:PP_InputEvent_Modifier.SHIFTKEY,
    ctrlKey:PP_InputEvent_Modifier.CONTROLKEY,
    altKey:PP_InputEvent_Modifier.ALTKEY,
    metaKey:PP_InputEvent_Modifier.METAKEY
  };

  var mod_buttons = [
    PPIE_Modifier.LEFTBUTTONDOWN,
    PPIE_Modifier.MIDDLEBUTTONDOWN,
    PPIE_Modifier.RIGHTBUTTONDOWN
  ];

  var eventclassdata = {
    MOUSE: {
      mousedown: PPIE_Type.MOUSEDOWN,
      mouseup: PPIE_Type.MOUSEUP,
      mousemove: PPIE_Type.MOUSEMOVE,
      mouseenter: PPIE_Type.MOUSEENTER,
      mouseout: PPIE_Type.MOUSELEAVE,
      contextmenu: PPIE_Type.CONTEXTMENU
    },

    KEYBOARD: {
      keydown: PPIE_Type.KEYDOWN,
      keyup: PPIE_Type.KEYUP,
      keypress: PPIE_Type.CHAR
    },

    WHEEL: {
      mousewheel: PPIE_Type.WHEEL,  // Chrome
      wheel: PPIE_Type.WHEEL        // Firefox
    }
  };


  var ResolveOrElse = function(event, key, alternative) {
    var resource = resources.resolve(event);
    if (resource === undefined) {
      return alternative;
    }
    return resource[key];
  };

  var GetEventPos = function(event) {
    // hopefully this will be reasonably cross platform
    var bcr = event.target.getBoundingClientRect();
    return {
      x: event.clientX-bcr.left,
      y: event.clientY-bcr.top
    };
  };

  var GetWheelScroll = function(e) {
    var x = ('deltaX' in e) ? e.deltaX : -e.wheelDeltaX;
    var y = ('deltaY' in e) ? e.deltaY : -e.wheelDeltaY;
    // scroll by lines, not pixels/pages
    if (e.deltaMode === 1) {
      x *= 40;
      y *= 40;
    }
    return {x: x, y: y};
  };

  var GetMovement = function(e) {
    return {
      //will be e.movementX once standardized, but for now we need to polyfill
      x: e.webkitMovementX || e.mozMovementX || 0,
      y: e.webkitMovementY || e.mozMovementY || 0
    };
  };

  var RegisterHandlers = function(instance, event_classes, filtering) {
    var resource = resources.resolve(instance);

    var makeCallback = function(event_type) {
      return function(event) {

        var modifiers = 0;
        for(var key in mod_masks) {
          if (mod_masks.hasOwnProperty(key) && event[key]) {
            modifiers |= mod_masks[key];
          }
        }
        if (typeof event.button === 'number' && event.button !== -1) {
          modifiers |= mod_buttons[event.button];
        }

        var obj_uid = resources.register("input_event", {
          // can't use type as attribute name since deplug uses that internally
          ie_type: event_type,
          pos: GetEventPos(event),
          button: event.button,
          // TODO(grosse): Make sure this actually follows the Pepper API
          time: event.timeStamp,
          modifiers: modifiers,
          movement: GetMovement(event),
          delta: GetWheelScroll(event),
          scrollByPage: (event.deltaMode === 2),
          keyCode: event.keyCode
        });

        var rval = _HandleInputEvent(instance, obj_uid);
        if (!filtering || rval) {
          // Don't prevent default on mousedown so we can get focus when clicked
          if (event_type !== PPIE_Type.MOUSEDOWN) {
            event.preventDefault();
          }
          event.stopPropagation();
        }
        return filtering && !rval;
      };
    };

    var canvas = resource.canvas;
    for(var key in eventclassdata) {
      if (eventclassdata.hasOwnProperty(key) && (event_classes & PPIE_Class[key])) {
        var data = eventclassdata[key];

        for(var key2 in data) {
          if (data.hasOwnProperty(key2)) {
            canvas.addEventListener(key2, makeCallback(data[key2]), false);
          }
        }
      }
    }
  };

  var InputEvent_RequestInputEvents = function(instance, event_classes) {
    RegisterHandlers(instance, event_classes, false);
  };

  var InputEvent_RequestFilteringInputEvents = function(instance, event_classes) {
    RegisterHandlers(instance, event_classes, true);
  };

  var InputEvent_ClearInputEventRequest = function(instance, event_classes) {
    throw "InputEvent_ClearInputEventRequest not implemented";
  };

  var InputEvent_IsInputEvent = function(resource) {
    return resources.is(resource, "input_event");
  };

  var InputEvent_GetType = function(event) {
    return ResolveOrElse(event, 'ie_type', PPIE_Type.UNDEFINED);
  };

  var InputEvent_GetTimeStamp = function(event) {
    return ResolveOrElse(event, 'time', 0.0);
  };


  var InputEvent_GetModifiers = function(event) {
    return ResolveOrElse(event, 'modifiers', 0);
  };

  registerInterface("PPB_InputEvent;1.0", [
    InputEvent_RequestInputEvents,
    InputEvent_RequestFilteringInputEvents,
    InputEvent_ClearInputEventRequest,
    InputEvent_IsInputEvent,
    InputEvent_GetType,
    InputEvent_GetTimeStamp,
    InputEvent_GetModifiers
  ]);

  var MouseInputEvent_Create = function() {
    throw "MouseInputEvent_Create not implemented";
  };

  var MouseInputEvent_IsMouseInputEvent = function(event) {
    // ppb_input_event_thunk.cc line 126
    if (!InputEvent_IsInputEvent(event)) {
      return false;
    }
    var type = resources.resolve(event).ie_type;
    return (
      type === PPIE_Type.MOUSEDOWN ||
        type === PPIE_Type.MOUSEUP ||
        type === PPIE_Type.MOUSEMOVE ||
        type === PPIE_Type.MOUSEENTER ||
        type === PPIE_Type.MOUSELEAVE ||
        type === PPIE_Type.CONTEXTMENU
    );
  };

  var MouseInputEvent_GetButton = function(event) {
    return ResolveOrElse(event, 'button', PPIE_MouseButton.NONE);
  };

  var MouseInputEvent_GetPosition = function(ptr, event) {
    var point = {x: 0, y: 0};
    if (MouseInputEvent_IsMouseInputEvent(event)) {
      point = resources.resolve(event).pos;
    }
    ppapi_glue.setPoint(point, ptr);
  };

  var MouseInputEvent_ClickCount = function(event) {
    // TODO(grosse): Find way to implement this
    return 0;
  };

  var MouseInputEvent_Movement = function(ptr, event) {
    ppapi_glue.setPoint(resources.resolve(event).movement, ptr);
  };

  registerInterface("PPB_MouseInputEvent;1.1", [
    MouseInputEvent_Create,
    MouseInputEvent_IsMouseInputEvent,
    MouseInputEvent_GetButton,
    MouseInputEvent_GetPosition,
    MouseInputEvent_ClickCount,
    MouseInputEvent_Movement
  ]);

  var WheelInputEvent_Create = function() {
    throw "WheelInputEvent_Create not implemented";
  };

  var WheelInputEvent_IsWheelInputEvent = function(event) {
    // ppb_input_event_thunk.cc line 205
    if (!InputEvent_IsInputEvent(event)) {
      return false;
    }
    var type = resources.resolve(event).ie_type;
    return (type === PPIE_Type.WHEEL);
  };

  var WheelInputEvent_GetDelta = function(ptr, event) {
    var point = {x: 0, y: 0};
    if (WheelInputEvent_IsWheelInputEvent(event)) {
      point = resources.resolve(event).delta;
    }
    ppapi_glue.setFloatPoint(point, ptr);
  };

  var WheelInputEvent_GetTicks = function(ptr, event) {};

  var WheelInputEvent_GetScrollByPage = function(event) {
    return ResolveOrElse(event, 'scrollByPage', false);
  };

  registerInterface("PPB_WheelInputEvent;1.0", [
    WheelInputEvent_Create,
    WheelInputEvent_IsWheelInputEvent,
    WheelInputEvent_GetDelta,
    WheelInputEvent_GetTicks,
    WheelInputEvent_GetScrollByPage
  ]);

  var KeyboardInputEvent_Create = function() {
    throw "KeyboardInputEvent_Create not implemented";
  };

  var KeyboardInputEvent_IsKeyboardInputEvent = function(event) {
    // ppb_input_event_thunk.cc line 262
    if (!InputEvent_IsInputEvent(event)) {
      return false;
    }
    var type = resources.resolve(event).ie_type;
    return (
      type === PPIE_Type.KEYDOWN ||
        type === PPIE_Type.KEYUP ||
        type === PPIE_Type.RAWKEYDOWN ||
        type === PPIE_Type.CHAR
    );
  };

  var KeyboardInputEvent_GetKeyCode = function(event) {
    return ResolveOrElse(event, 'keyCode', 0);
  };

  var KeyboardInputEvent_GetCharacterText = function(ptr, event) {
    // TODO(grosse): Find way to implement this
    ppapi_glue.varForJS(ptr, undefined);
  };

  registerInterface("PPB_KeyboardInputEvent;1.0", [
    KeyboardInputEvent_Create,
    KeyboardInputEvent_IsKeyboardInputEvent,
    KeyboardInputEvent_GetKeyCode,
    KeyboardInputEvent_GetCharacterText
  ]);

})();
