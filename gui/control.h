/////////////////////////////////////////////////////////////////////////
// $Id: control.h,v 1.6.10.1 2002/09/12 03:38:40 bdenney Exp $
/////////////////////////////////////////////////////////////////////////
//
enum {
  BX_CI_INIT,
  BX_CI_START_MENU,
  BX_CI_START_OPTS,
  BX_CI_START_OPTS_MEM,
  BX_CI_START_OPTS_INTERFACE,
  BX_CI_START_OPTS_DISK,
  BX_CI_START_OPTS_SOUND,
  BX_CI_START_OPTS_MISC,
  BX_CI_RUNTIME,
  BX_CI_N_MENUS
};

/* special return value from bx_config_interface */
#define BX_DISABLE_CONFIG_INTERFACE 10 

int bx_config_interface (int menu);
