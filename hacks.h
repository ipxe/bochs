// temporary!

enum {
  GUI_BUTTON_FLOPPYA = 1000,
  GUI_BUTTON_FLOPPYB
};

// defined in wxmain.cc for now
int gui_button_pressed (int which);

///////////////////////////////////////////////////////////////////
// event structure for communication between simulator and gui
///////////////////////////////////////////////////////////////////

typedef enum {
  BX_EVT_KEY_PRESS,         // vga gui -> simulator
  BX_EVT_KEY_RELEASE,       // vga gui -> simulator
  BX_EVT_MOUSE,             // vga gui -> simulator
  BX_EVT_GET_PARAM,         // cpanel -> simulator -> cpanel
  BX_EVT_SET_PARAM,         // cpanel -> simulator
  BX_EVT_LOG_MSG,           // simulator -> cpanel
  BX_EVT_VALUE_CHANGED,     // simulator -> cpanel
  BX_EVT_ASK_PARAM,         // simulator -> cpanel -> simulator
} BxEventType;

typedef union {
  Bit32s val;
  char *charptr;
} AnyParamVal;

typedef struct {
  BxEventType type;
  union {
    struct {
      // type is BX_EVT_KEY_PRESS or BX_EVT_KEY_RELEASE
      Bit16u scancode;
    } key;
    struct {
      // type is BX_EVT_MOUSE
      Bit16u x, y;
      Bit8u buttons;
    } mouse;
    struct {
      // type is BX_EVT_GET_PARAM, BX_EVT_SET_PARAM, BX_EVT_ASK
      bx_id id;         // id number of parameter
      AnyParamVal val;
    } param;
    struct {
      // type is BX_EVT_LOG_MSG
      Bit8u level;
      char *prefix;
      char *msg;
    } logmsg;
  } u;
} BxEvent;

