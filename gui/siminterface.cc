/////////////////////////////////////////////////////////////////////////
// $Id: siminterface.cc,v 1.94.4.8 2003-03-28 02:10:45 slechta Exp $
/////////////////////////////////////////////////////////////////////////
//
// See siminterface.h for description of the siminterface concept.
// Basically, the siminterface is visible from both the simulator and
// the configuration user interface, and allows them to talk to each other.

#include "bochs.h"

bx_simulator_interface_c *SIM = NULL;
logfunctions *siminterface_log = NULL;
bx_list_c *root_param = NULL;
#define LOG_THIS siminterface_log->

// bx_simulator_interface just defines the interface that the Bochs simulator
// and the gui will use to talk to each other.  None of the methods of
// bx_simulator_interface are implemented; they are all virtual.  The
// bx_real_sim_c class is a child of bx_simulator_interface_c, and it
// implements all the methods.  The idea is that a gui needs to know only
// definition of bx_simulator_interface to talk to Bochs.  The gui should
// not need to include bochs.h.  
//
// I made this separation to ensure that all guis use the siminterface to do
// access bochs internals, instead of accessing things like
// bx_keyboard.s.internal_buffer[4] (or whatever) directly. -Bryce
// 

class bx_real_sim_c : public bx_simulator_interface_c {
  bxevent_handler bxevent_callback;
  void *bxevent_callback_data;
  const char *registered_ci_name;
  config_interface_callback_t ci_callback;
  void *ci_callback_data;
  int init_done;
  bx_param_c **param_registry;
  int registry_alloc_size;
  int enabled;
  // save context to jump to if we must quit unexpectedly
  jmp_buf *quit_context;
  int exit_code;
public:
  bx_real_sim_c ();
  virtual ~bx_real_sim_c ();
  virtual void set_quit_context (jmp_buf *context) { quit_context = context; }
  virtual int get_init_done () { return init_done; }
  virtual int set_init_done (int n) { init_done = n; return 0;}
  virtual void get_param_id_range (int *min, int *max) {
    *min = BXP_NULL;
    *max = BXP_THIS_IS_THE_LAST-1;
  }
  virtual int register_param (bx_id id, bx_param_c *it);
  virtual void reset_all_param ();

  // All lookups by ID will be eliminated soon.  They will not work anymore.
  virtual bx_param_c *get_param (bx_id id) {
    BX_INFO (("get_param called with id=%d. This method is no longer usable and will be eliminated soon. Replace it with a call to get_param(name).", (int)id));
    assert (0);
    return NULL;
  }
  virtual bx_param_num_c *get_param_num (bx_id id) {
    BX_INFO (("get_param_num called with id=%d. This method is no longer usable and will be eliminated soon. Replace it with a call to get_param_num(name).", (int)id));
    assert (0);
    return NULL;
  }
  virtual bx_param_string_c *get_param_string (bx_id id) {
    BX_INFO (("get_param_string called with id=%d. This method is no longer usable and will be eliminated soon. Replace it with a call to get_param_string(name).", (int)id));
    assert (0);
    return NULL;
  }
  virtual bx_param_bool_c *get_param_bool (bx_id id) {
    BX_INFO (("get_param_bool called with id=%d. This method is no longer usable and will be eliminated soon. Replace it with a call to get_param_bool(name).", (int)id));
    assert (0);
    return NULL;
  }
  virtual bx_param_enum_c *get_param_enum (bx_id id) {
    BX_INFO (("get_param_enum called with id=%d. This method is no longer usable and will be eliminated soon. Replace it with a call to get_param_enum(name).", (int)id));
    BX_ASSERT (0);
    return NULL;
  }




  virtual bx_param_c *get_param (const char *pname, bx_param_c *base=NULL);
  virtual bx_param_num_c *get_param_num (const char *pname);
  virtual bx_param_string_c *get_param_string (const char *pname);
  virtual bx_param_bool_c *get_param_bool (const char *pname);
  virtual bx_param_enum_c *get_param_enum (const char *pname);
  virtual int get_n_log_modules ();
  virtual char *get_prefix (int mod);
  virtual int get_log_action (int mod, int level);
  virtual void set_log_action (int mod, int level, int action);
  virtual char *get_action_name (int action);
  virtual int get_default_log_action (int level) {
	return logfunctions::get_default_action (level);
  }
  virtual void set_default_log_action (int level, int action) {
	logfunctions::set_default_action (level, action);
  }
  virtual const char *get_log_level_name (int level);
  virtual int get_max_log_level ();
  virtual void quit_sim (int code);
  virtual int get_exit_code () { return exit_code; }
  virtual int get_default_rc (char *path, int len);
  virtual int read_rc (char *path);
  virtual int write_rc (char *path, int overwrite);
  virtual int get_log_file (char *path, int len);
  virtual int set_log_file (char *path);
  virtual int get_log_prefix (char *prefix, int len);
  virtual int set_log_prefix (char *prefix);
  virtual int get_debugger_log_file (char *path, int len);
  virtual int set_debugger_log_file (char *path);
  virtual int get_floppy_options (int drive, bx_floppy_options *out);
  virtual int get_cdrom_options (int drive, bx_atadevice_options *out, int *device = NULL);
  virtual char *get_floppy_type_name (int type);
  virtual void set_notify_callback (bxevent_handler func, void *arg);
  virtual void get_notify_callback (bxevent_handler *func, void **arg);
  virtual BxEvent* sim_to_ci_event (BxEvent *event);
  virtual int log_msg (const char *prefix, int level, const char *msg);
  virtual int ask_param (bx_id which);
  // ask the user for a pathname
  virtual int ask_filename (char *filename, int maxlen, char *prompt, char *the_default, int flags);
  // called at a regular interval, currently by the keyboard handler.
  virtual void periodic ();
  virtual int create_disk_image (const char *filename, int sectors, bx_bool overwrite);
  virtual void refresh_ci ();
  virtual void refresh_vga () {
    // maybe need to check if something has been initialized yet?
    DEV_vga_refresh();
  }
  virtual void handle_events () {
    // maybe need to check if something has been initialized yet?
    bx_gui->handle_events ();
  }
  // find first hard drive or cdrom
  bx_param_c *get_first_atadevice (Bit32u search_type);
  bx_param_c *get_first_cdrom () {
    return get_first_atadevice (BX_ATA_DEVICE_CDROM);
  }
  bx_param_c *get_first_hd () {
    return get_first_atadevice (BX_ATA_DEVICE_DISK);
  }
#if BX_DEBUGGER
  virtual void debug_break ();
  virtual void debug_interpret_cmd (char *cmd);
  virtual char *debug_get_next_command ();
  virtual void debug_puts (const char *cmd);
#endif
  virtual void register_configuration_interface (
    const char* name, 
    config_interface_callback_t callback,
    void *userdata);
  virtual int configuration_interface(const char* name, ci_command_t command);
  virtual int begin_simulation (int argc, char *argv[]);
  virtual void set_sim_thread_func (is_sim_thread_func_t func) {}
  virtual bool is_sim_thread ();
  bool wxsel;
  virtual bool is_wx_selected () { return wxsel; }
  // provide interface to bx_gui->set_display_mode() method for config
  // interfaces to use.
  virtual void set_display_mode (disp_mode_t newmode) {
    if (bx_gui != NULL)
      bx_gui->set_display_mode (newmode);
  }
  virtual bool test_for_text_console ();
  // save/restore interface
  virtual bool save_state (const char *checkpoint_name);
  virtual bool restore_state (const char *checkpoint_name);
};

// recursive function to find parameters from the path
static
bx_param_c *find_param (const char *full_pname, const char *rest_of_pname, bx_param_c *base)
{
  const char *from = rest_of_pname;
  char component[BX_PATHNAME_LEN];
  char *to = component;
  // copy the first piece of pname into component, stopping at first separator
  // or at the end of the string
  while (*from != 0 && *from != '.') {
    *to = *from;
    to++;
    from++;
  }
  *to = 0;
  if (!component[0]) {
    BX_PANIC (("find_param: found empty component in parameter name %s", full_pname));
    // or does that mean that we're done?
  }
  if (base->get_type() != BXT_LIST) {
    BX_PANIC (("find_param: base was not a list!"));
  }
  BX_INFO (("searching for component '%s' in list '%s'", component, base->get_name()));

  // find the component in the list.
  bx_list_c *list = (bx_list_c *)base;
  bx_param_c *child = list->get_by_name (component);
  // if child not found, there is nothing else that can be done. return NULL.
  if (child == NULL) return NULL;
  if (from[0] == 0) {
    // that was the end of the path, we're done
    return child;
  }
  // continue parsing the path
  BX_ASSERT(from[0] == '.');
  from++;  // skip over the separator
  return find_param (full_pname, from, child);
}

bx_param_c *
bx_real_sim_c::get_param (const char *pname, bx_param_c *base) 
{
  if (base == NULL)
    base = root_param;
  // to access top level object, look for parameter "."
  if (pname[0] == '.' && pname[1] == 0)
    return base;
  return find_param (pname, pname, base);
}

bx_param_num_c *
bx_real_sim_c::get_param_num (const char *pname) {
  bx_param_c *generic = get_param(pname);
  if (generic==NULL) {
    BX_PANIC (("get_param_num(%s) could not find a parameter", pname));
    return NULL;
  }
  int type = generic->get_type ();
  if (type == BXT_PARAM_NUM || type == BXT_PARAM_BOOL || type == BXT_PARAM_ENUM)
    return (bx_param_num_c *)generic;
  BX_PANIC (("get_param_num(%s) could not find an integer parameter with that name", pname));
  return NULL;
}

bx_param_string_c *
bx_real_sim_c::get_param_string (const char *pname) {
  bx_param_c *generic = get_param(pname);
  if (generic==NULL) {
    BX_PANIC (("get_param_string(%s) could not find a parameter", pname));
    return NULL;
  }
  if (generic->get_type () == BXT_PARAM_STRING)
    return (bx_param_string_c *)generic;
  BX_PANIC (("get_param_string(%s) could not find an integer parameter with that name", pname));
  return NULL;
}

bx_param_bool_c *
bx_real_sim_c::get_param_bool (const char *pname) {
  bx_param_c *generic = get_param(pname);
  if (generic==NULL) {
    BX_PANIC (("get_param_bool(%s) could not find a parameter", pname));
    return NULL;
  }
  if (generic->get_type () == BXT_PARAM_BOOL)
    return (bx_param_bool_c *)generic;
  BX_PANIC (("get_param_bool(%s) could not find a bool parameter with that name", pname));
  return NULL;
}

bx_param_enum_c *
bx_real_sim_c::get_param_enum (const char *pname) {
  bx_param_c *generic = get_param(pname);
  if (generic==NULL) {
    BX_PANIC (("get_param_enum(%s) could not find a parameter", pname));
    return NULL;
  }
  if (generic->get_type () == BXT_PARAM_ENUM)
    return (bx_param_enum_c *)generic;
  BX_PANIC (("get_param_enum(%s) could not find a enum parameter with that name", pname));
  return NULL;
}

void bx_init_siminterface ()
{
  siminterface_log = new logfunctions ();
  siminterface_log->put ("CTRL");
  siminterface_log->settype(CTRLLOG);
  if (SIM == NULL) 
    SIM = new bx_real_sim_c();
  if (root_param == NULL) {
    root_param = new bx_list_c (NULL, 
	"bochs",
	"list of top level bochs parameters", 
	30);
  }
}

bx_simulator_interface_c::bx_simulator_interface_c ()
{
}

bx_real_sim_c::bx_real_sim_c ()
{
  bxevent_callback = NULL;
  bxevent_callback_data = NULL;
  ci_callback = NULL;
  ci_callback_data = NULL;
  is_sim_thread_func = NULL;
  wxsel = false;
  
  enabled = 1;
  int i;
  init_done = 0;
  registry_alloc_size = BXP_THIS_IS_THE_LAST - BXP_NULL;
  param_registry = new bx_param_c*  [registry_alloc_size];
  for (i=0; i<registry_alloc_size; i++)
    param_registry[i] = NULL;
  quit_context = NULL;
  exit_code = 0;
}

// called by constructor of bx_param_c, so that every parameter that is
// initialized gets registered.  This builds a list of all parameters
// which can be used to look them up by number (get_param).
bx_real_sim_c::~bx_real_sim_c ()
{
    if ( param_registry != NULL )
    {
        delete [] param_registry;
        param_registry = NULL;
    }
}

int
bx_real_sim_c::register_param (bx_id id, bx_param_c *it)
{
  if (id == BXP_NULL) return 0;
  BX_ASSERT (id >= BXP_NULL && id < BXP_THIS_IS_THE_LAST);
  int index = (int)id - BXP_NULL;
  if (this->param_registry[index] != NULL) {
    BX_INFO (("register_param is overwriting parameter id %d", id));
  }
  this->param_registry[index] = it;
  return 0;
}

void 
bx_real_sim_c::reset_all_param ()
{
  bx_reset_options ();
}

int 
bx_real_sim_c::get_n_log_modules ()
{
  return io->get_n_logfns ();
}

char *
bx_real_sim_c::get_prefix (int mod)
{
  logfunc_t *logfn = io->get_logfn (mod);
  return logfn->getprefix ();
}

int 
bx_real_sim_c::get_log_action (int mod, int level)
{
  logfunc_t *logfn = io->get_logfn (mod);
  return logfn->getonoff (level);
}

void 
bx_real_sim_c::set_log_action (int mod, int level, int action)
{
  // normal
  if (mod >= 0) {
	logfunc_t *logfn = io->get_logfn (mod);
	logfn->setonoff (level, action);
	return;
  }
  // if called with mod<0 loop over all
  int nmod = get_n_log_modules ();
  for (mod=0; mod<nmod; mod++)
	set_log_action (mod, level, action);
}

char *
bx_real_sim_c::get_action_name (int action)
{
  return io->getaction (action);
}

const char *
bx_real_sim_c::get_log_level_name (int level)
{
  return io->getlevel (level);
}

int 
bx_real_sim_c::get_max_log_level ()
{
  return N_LOGLEV;
}

void 
bx_real_sim_c::quit_sim (int code) {
  BX_INFO (("quit_sim called with exit code %d", code));
  exit_code = code;
  // use longjmp to quit cleanly, no matter where in the stack we are.
  //fprintf (stderr, "using longjmp() to jump directly to the quit context!\n");
  if (quit_context != NULL) {
    longjmp (*quit_context, 1);
    BX_PANIC (("in bx_real_sim_c::quit_sim, longjmp should never return"));
  }
  if (SIM->is_wx_selected ()) {
    // in wxWindows, the whole simulator is running in a separate thread.
    // our only job is to end the thread as soon as possible, NOT to shut
    // down the whole application with an exit.
    BX_CPU(0)->async_event = 1;
    BX_CPU(0)->kill_bochs_request = 1;
    // the cpu loop will exit very soon after this condition is set.
  } else {
    // just a single thread.  Use exit() to stop the application.
    if (!code)
      BX_PANIC (("Quit simulation command"));
    ::exit (exit_code);
  }
}

int
bx_real_sim_c::get_default_rc (char *path, int len)
{
  char *rc = bx_find_bochsrc ();
  if (rc == NULL) return -1;
  strncpy (path, rc, len);
  path[len-1] = 0;
  return 0;
}

int 
bx_real_sim_c::read_rc (char *rc)
{
  return bx_read_configuration (rc);
}

// return values:
//   0: written ok
//  -1: failed
//  -2: already exists, and overwrite was off
int 
bx_real_sim_c::write_rc (char *rc, int overwrite)
{
  return bx_write_configuration (rc, overwrite);
}

int 
bx_real_sim_c::get_log_file (char *path, int len)
{
  strncpy (path, bx_options.log.Ofilename->getptr (), len);
  return 0;
}

int 
bx_real_sim_c::set_log_file (char *path)
{
  bx_options.log.Ofilename->set (path);
  return 0;
}

int 
bx_real_sim_c::get_log_prefix (char *prefix, int len)
{
  strncpy (prefix, bx_options.log.Oprefix->getptr (), len);
  return 0;
}

int 
bx_real_sim_c::set_log_prefix (char *prefix)
{
  bx_options.log.Oprefix->set (prefix);
  return 0;
}

int 
bx_real_sim_c::get_debugger_log_file (char *path, int len)
{
  strncpy (path, bx_options.log.Odebugger_filename->getptr (), len);
  return 0;
}

int 
bx_real_sim_c::set_debugger_log_file (char *path)
{
  bx_options.log.Odebugger_filename->set (path);
  return 0;
}

int 
bx_real_sim_c::get_floppy_options (int drive, bx_floppy_options *out)
{
  *out = (drive==0)? bx_options.floppya : bx_options.floppyb;
  return 0;
}

int 
bx_real_sim_c::get_cdrom_options (int level, bx_atadevice_options *out, int *where)
{
  for (Bit8u channel=0; channel<BX_MAX_ATA_CHANNEL; channel++) {
    for (Bit8u device=0; device<2; device++) {
      if (bx_options.atadevice[channel][device].Otype->get() == BX_ATA_DEVICE_CDROM) {
        if (level==0) {
          *out = bx_options.atadevice[channel][device];
	  if (where != NULL) *where=(channel*2)+device;
          return 1;
          }
        else level--;
	}
      }
    }
  return 0;
}

char *bochs_start_names[] = { "quick", "load", "edit", "run" };
int n_bochs_start_names = 3;

char *floppy_type_names[] = { "none", "1.2M", "1.44M", "2.88M", "720K", "360K", "160K", "180K", "320K", NULL };
int floppy_type_n_sectors[] = { -1, 80*2*15, 80*2*18, 80*2*36, 80*2*9, 40*2*9, 40*1*8, 40*1*9, 40*2*8 };
int n_floppy_type_names = 9;

char *floppy_status_names[] = { "ejected", "inserted", NULL };
int n_floppy_status_names = 2;
char *floppy_bootdisk_names[] = { "floppy", "hard","cdrom", NULL };
int n_floppy_bootdisk_names = 3;
char *loader_os_names[] = { "none", "linux", "nullkernel", NULL };
int n_loader_os_names = 3;
char *keyboard_type_names[] = { "xt", "at", "mf", NULL };
int n_keyboard_type_names = 3;
char *atadevice_type_names[] = { "hard disk", "cdrom", NULL };
int n_atadevice_type_names = 2;
char *atadevice_status_names[] = { "ejected", "inserted", NULL };
int n_atadevice_status_names = 2;
char *atadevice_biosdetect_names[] = { "none", "auto", "cmos", NULL };
int n_atadevice_biosdetect_names = 3;
char *atadevice_translation_names[] = { "none", "lba", "large", "rechs", "auto", NULL };
int n_atadevice_translation_names = 3;

char *
bx_real_sim_c::get_floppy_type_name (int type)
{
  BX_ASSERT (type >= BX_FLOPPY_NONE && type <= BX_FLOPPY_LAST);
  type -= BX_FLOPPY_NONE;
  return floppy_type_names[type];
}

void 
bx_real_sim_c::set_notify_callback (bxevent_handler func, void *arg)
{
  bxevent_callback = func;
  bxevent_callback_data = arg;
}

void bx_real_sim_c::get_notify_callback (
    bxevent_handler *func,
    void **arg)
{
  *func = bxevent_callback;
  *arg = bxevent_callback_data;
}

BxEvent *
bx_real_sim_c::sim_to_ci_event (BxEvent *event)
{
  if (bxevent_callback == NULL) {
    BX_ERROR (("notify called, but no bxevent_callback function is registered"));
    return NULL;
  } else {
    return (*bxevent_callback)(bxevent_callback_data, event);
  }
}

// returns 0 for continue, 1 for alwayscontinue, 2 for die.
int 
bx_real_sim_c::log_msg (const char *prefix, int level, const char *msg)
{
  BxEvent be;
  be.type = BX_SYNC_EVT_LOG_ASK;
  be.u.logmsg.prefix = prefix;
  be.u.logmsg.level = level;
  be.u.logmsg.msg = msg;
  // default return value in case something goes wrong.
  be.retcode = BX_LOG_ASK_CHOICE_DIE;
  //fprintf (stderr, "calling notify.\n");
  sim_to_ci_event (&be);
  return be.retcode;
}

// Called by simulator whenever it needs the user to choose a new value
// for a registered parameter.  Create a synchronous ASK_PARAM event, 
// send it to the CI, and wait for the response.  The CI will call the
// set() method on the parameter if the user changes the value.
int 
bx_real_sim_c::ask_param (bx_id param)
{
  bx_param_c *paramptr = SIM->get_param(param);
  BX_ASSERT (paramptr != NULL);
  // create appropriate event
  BxEvent event;
  event.type = BX_SYNC_EVT_ASK_PARAM;
  event.u.param.param = paramptr;
  sim_to_ci_event (&event);
  return event.retcode;
}

int
bx_real_sim_c::ask_filename (char *filename, int maxlen, char *prompt, char *the_default, int flags)
{
  // implement using ASK_PARAM on a newly created param.  I can't use
  // ask_param because I don't intend to register this param.
  BxEvent event;
  bx_param_string_c param (NULL, "filename", prompt, the_default, maxlen);
  flags |= param.IS_FILENAME;
  param.get_options()->set (flags);
  event.type = BX_SYNC_EVT_ASK_PARAM;
  event.u.param.param = &param;
  sim_to_ci_event (&event);
  if (event.retcode >= 0)
    memcpy (filename, param.getptr(), maxlen);
  return event.retcode;
}

void
bx_real_sim_c::periodic ()
{
  // give the GUI a chance to do periodic things on the bochs thread. in 
  // particular, notice if the thread has been asked to die.
  BxEvent tick;
  tick.type = BX_SYNC_EVT_TICK;
  sim_to_ci_event (&tick);
  if (tick.retcode < 0) {
    BX_INFO (("Bochs thread has been asked to quit."));
    bx_atexit ();
    quit_sim (0);
  }
  static int refresh_counter = 0;
  if (++refresh_counter == 50) {
    // only ask the CI to refresh every 50 times periodic() is called.
    // This should obviously be configurable because system speeds and
    // user preferences vary.
    refresh_ci ();
    refresh_counter = 0;
  }
#if 0
  // watch for memory leaks.  Allocate a small block of memory, print the
  // pointer that is returned, then free.
  BxEvent *memcheck = new BxEvent ();
  BX_INFO(("memory allocation at %p", memcheck));
  delete memcheck;
#endif
}

// create a disk image file called filename, size=512 bytes * sectors.
// If overwrite is true and the file exists, returns -1 without changing it.
// Otherwise, opens up the image and starts writing.  Returns -2 if
// the image could not be opened, or -3 if there are failures during
// write, e.g. disk full.
// 
// wxWindows: This may be called from the gui thread.
int 
bx_real_sim_c::create_disk_image (
    const char *filename,
    int sectors,
    bx_bool overwrite) 
{
  FILE *fp;
  if (!overwrite) {
    // check for existence first
    fp = fopen (filename, "r");
    if (fp) {
      // yes it exists
      fclose (fp);
      return -1;
    }
  }
  fp = fopen (filename, "w");
  if (fp == NULL) {
#ifdef HAVE_PERROR
    char buffer[1024];
    sprintf (buffer, "while opening '%s' for writing", filename);
    perror (buffer);
    // not sure how to get this back into the CI
#endif
    return -2;
  }
  int sec = sectors;
  /*
   * seek to sec*512-1 and write a single character.
   * can't just do: fseek(fp, 512*sec-1, SEEK_SET)
   * because 512*sec may be too large for signed int.
   */
  while (sec > 0)
  {
    /* temp <-- min(sec, 4194303)
     * 4194303 is (int)(0x7FFFFFFF/512)
     */
    int temp = ((sec < 4194303) ? sec : 4194303);
    fseek(fp, 512*temp, SEEK_CUR);
    sec -= temp;
  }

  fseek(fp, -1, SEEK_CUR);
  if (fputc('\0', fp) == EOF)
  {
    fclose (fp);
    return -3;
  }
  fclose (fp);
  return 0;
}

void bx_real_sim_c::refresh_ci () {
  if (SIM->is_wx_selected ()) {
    // presently, only wxWindows interface uses these events
    // It's an async event, so allocate a pointer and send it.
    // The event will be freed by the recipient.
    BxEvent *event = new BxEvent ();
    event->type = BX_ASYNC_EVT_REFRESH;
    sim_to_ci_event (event);
  }
}

bx_param_c *
bx_real_sim_c::get_first_atadevice (Bit32u search_type) {
  for (int channel=0; channel<BX_MAX_ATA_CHANNEL; channel++) {
    if (!bx_options.ata[channel].Opresent->get ())
      continue;
    for (int slave=0; slave<2; slave++) {
      Bit32u present = bx_options.atadevice[channel][slave].Opresent->get ();
      Bit32u type = bx_options.atadevice[channel][slave].Otype->get ();
      if (present && (type == search_type)) {
	return bx_options.atadevice[channel][slave].Omenu;
      }
    }
  }
  return NULL;
}

#if BX_DEBUGGER

// this can be safely called from either thread.
void bx_real_sim_c::debug_break () {
  bx_debug_break ();
}

// this should only be called from the sim_thread.
void bx_real_sim_c::debug_interpret_cmd (char *cmd) {
  if (!is_sim_thread ()) {
    fprintf (stderr, "ERROR: debug_interpret_cmd called but not from sim_thread\n");
    return;
  }
  bx_dbg_interpret_line (cmd);
}

char *bx_real_sim_c::debug_get_next_command ()
{
  fprintf (stderr, "begin debug_get_next_command\n");
  BxEvent event;
  event.type = BX_SYNC_EVT_GET_DBG_COMMAND;
  BX_INFO (("asking for next debug command"));
  sim_to_ci_event (&event);
  BX_INFO (("received next debug command: '%s'", event.u.debugcmd.command));
  if (event.retcode >= 0)
    return event.u.debugcmd.command;
  return NULL;
}

void bx_real_sim_c::debug_puts (const char *text)
{
  if (SIM->is_wx_selected ()) {
    // send message to the wxWindows debugger
    BxEvent *event = new BxEvent ();
    event->type = BX_ASYNC_EVT_DBG_MSG;
    event->u.logmsg.msg = text;
    sim_to_ci_event (event);
    // the event will be freed by the recipient
  } else {
    // text mode debugger: just write to console
    fputs (text, stderr);
    delete [] (char *)text;
  }
}
#endif

void 
bx_real_sim_c::register_configuration_interface (
  const char* name, 
  config_interface_callback_t callback,
  void *userdata)
{
  ci_callback = callback;
  ci_callback_data = userdata;
  registered_ci_name = name;
}

int 
bx_real_sim_c::configuration_interface(const char *ignore, ci_command_t command)
{
  bx_param_enum_c *ci_param = SIM->get_param_enum ("display.config_interface");
  char *name = ci_param->get_choice (ci_param->get ());
  if (!ci_callback) {
    BX_PANIC (("no configuration interface was loaded"));
    return -1;
  }
  if (strcmp (name, registered_ci_name) != 0) {
    BX_PANIC (("siminterface does not support loading one configuration interface and then calling another"));
    return -1;
  }
  if (!strcmp (name, "wx")) 
    wxsel = true;
  else
    wxsel = false;
  // enter configuration mode, just while running the configuration interface
  set_display_mode (DISP_MODE_CONFIG);
  int retval = (*ci_callback)(ci_callback_data, command);
  set_display_mode (DISP_MODE_SIM);
  return retval;
}

int 
bx_real_sim_c::begin_simulation (int argc, char *argv[])
{
  return bx_begin_simulation (argc, argv);
}

bool bx_real_sim_c::is_sim_thread ()
{
  if (is_sim_thread_func == NULL) return true;
  return (*is_sim_thread_func)();
}

// check if the text console exists.  On some platforms, if Bochs is
// started from the "Start Menu" or by double clicking on it on a Mac,
// there may be nothing attached to stdin/stdout/stderr.  This function
// tests if stdin/stdout/stderr are usable and returns false if not.
bool 
bx_real_sim_c::test_for_text_console ()
{
#if BX_WITH_CARBON
  // In a Carbon application, you have a text console if you run the app from
  // the command line, but if you start it from the finder you don't.
  if(!isatty(STDIN_FILENO)) return false;
#endif
  // default: yes
  return true;
}

bool 
bx_real_sim_c::save_state (const char *checkpoint_name)
{
  BX_INFO (("save_state(%s)", checkpoint_name));
  bx_param_c *root = get_param (".");
  print_tree (root, 0);
  return true;
}

bool 
bx_real_sim_c::restore_state (const char *checkpoint_name)
{
  BX_INFO (("restore_state(%s)", checkpoint_name));
  return true;
}


/////////////////////////////////////////////////////////////////////////
// define methods of bx_param_* and family
/////////////////////////////////////////////////////////////////////////

bx_object_c::bx_object_c (bx_id id)
{
  this->id = id;
  this->type = BXT_OBJECT;
}

void
bx_object_c::set_type (bx_objtype type)
{
  this->type = type;
}

const char* bx_param_c::default_text_format = NULL;

bx_param_c::bx_param_c (bx_param_c *parent, char *name, char *description)
  : bx_object_c (BXP_NULL)
{
  set_type (BXT_PARAM);
  this->name = name;
  this->description = description;
  this->text_format = default_text_format;
  this->ask_format = NULL;
  this->runtime_param = 0;
  this->enabled = 1;
  this->parent = NULL;
  this->is_shadow = 0; // can be re-set to 1 in shadow constructors --BJS
  if (parent) {
    BX_ASSERT (parent->is_type (BXT_LIST));
    this->parent = (bx_list_c *)parent;
    this->parent->add (this);
  }
}

void bx_param_c::get_param_path (char *path_out, int maxlen)
{
  if (get_parent() == NULL || get_parent() == root_param) {
    // Start with an empty string.
    // Never print the name of the root param.
    path_out[0] = 0;
  } else {
    // build path of the parent, add a period, add path of this node
    get_parent()->get_param_path (path_out, maxlen);
    strncat (path_out, ".", maxlen);
  }
  strncat (path_out, name, maxlen);
}


bx_param_c *bx_param_c::get_by_name (const char *name)
{
  if (is_type (BXT_LIST)) {
    return ((bx_list_c *)this)->get_by_name (name);
  }
  char pname[BX_PATHNAME_LEN];
  get_param_path (pname, BX_PATHNAME_LEN);
  BX_PANIC (("get_by_name called on non-list param %s", pname));
  return NULL;
}

bx_bool bx_param_c::child_of (bx_param_c *test_ancestor)
{
  bx_param_c *param;
  for (param = this; param != NULL; param=param->get_parent ()) {
    if (param == test_ancestor) return true;
  }
  return false;
}

void bx_param_c::set_parent (bx_param_c *newparent) {
  if (parent) {
    // if this object already had a parent, the correct thing
    // to do would be to remove this object from the parent's
    // list of children.  Deleting children is currently
    // not supported.
    BX_PANIC (("bx_list_c::set_parent: changing from one parent to another is not supported (object is '%s')", get_name()));
  }
  if (newparent) {
    BX_ASSERT(newparent->is_type (BXT_LIST));
    this->parent = (bx_list_c *)newparent;
    this->parent->add (this);
  }
}

const char* bx_param_c::set_default_format (const char *f) {
  const char *old = default_text_format;
  default_text_format = f; 
  return old;
}

bx_param_num_c::bx_param_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit64s min, Bit64s max, Bit64s initial_val)
  : bx_param_c (parent, name, description)
{
  set_type (BXT_PARAM_NUM);
  this->min = min;
  this->max = max;
  this->initial_val = initial_val;
  this->val.number = initial_val;
  this->handler = NULL;
  this->base = default_base;
  // dependent_list must be initialized before the set(),
  // because set calls update_dependents().
  dependent_list = NULL;
  set (initial_val);
}

Bit32u bx_param_num_c::default_base = 10;

Bit32u bx_param_num_c::set_default_base (Bit32u val) {
  Bit32u old = default_base;
  default_base = val; 
  return old;
}

void 
bx_param_num_c::reset ()
{
  this->val.number = initial_val;
}

void 
bx_param_num_c::set_handler (param_event_handler handler)
{ 
  this->handler = handler; 
  // now that there's a handler, call set once to run the handler immediately
  //set (get ());
}

void bx_param_num_c::set_dependent_list (bx_list_c *l) {
  dependent_list = l; 
  update_dependents (dependent_list);
}

Bit64s 
bx_param_num_c::get64 ()
{
  if (handler) {
    // the handler can decide what value to return and/or do some side effect
    return (*handler)(this, 0, val.number);
  } else {
    // just return the value
    return val.number;
  }
}

void
bx_param_num_c::set (Bit64s newval, bx_bool ignore_handler)
{
  if (!ignore_handler && handler) {
    // the handler can override the new value and/or perform some side effect
    // BBD: we should really send the oldval and newval to the handler,
    // so that it can change it back if necessary.  Maybe it should be
    // changed to   val.number = (*handler)(this, 1, val.number, newval);
    val.number = newval;
    (*handler)(this, 1, newval);
  } else {
    // just set the value
    val.number = newval;
  }
  if ((val.number < min || val.number > max) && max != BX_MAX_BIT64U)
    BX_PANIC (("numerical parameter '%s' was set to %lld, which is out of range %lld to %lld", get_name (), val.number, min, max));
  if (dependent_list != NULL) update_dependents (dependent_list);
}

void bx_param_num_c::set_range (Bit64u min, Bit64u max)
{
  this->min = min;
  this->max = max;
}

void bx_param_num_c::set_initial_val (Bit64s initial_val) { 
  this->val.number = this->initial_val = initial_val;
}

void bx_param_num_c::update_dependents (bx_list_c *list)
{
  if (list) {
    int en = val.number && enabled;
    for (int i=0; i<list->get_size (); i++) {
      bx_param_c *param = list->get (i);
      if (param != this)
	param->set_enabled (en);
      // descend into lists
      if (param->is_type (BXT_LIST)) {
	BX_INFO (("update_dependents descending into list '%s'", param->get_name()));
	update_dependents ((bx_list_c *)param);
	BX_INFO (("update_dependents returned from descent into list '%s'", param->get_name()));
      }
    }
  }
}

void
bx_param_num_c::set_enabled (int en)
{
  bx_param_c::set_enabled (en);
  update_dependents (dependent_list);
}

// Signed 64 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit64s *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT64S, BX_MAX_BIT64S, *ptr_to_real_val)
{
  this->varsize = 16;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p64bit = ptr_to_real_val;
  this->is_shadow = 1;
}

// Unsigned 64 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit64u *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT64U, BX_MAX_BIT64U, *ptr_to_real_val)
{
  this->varsize = 16;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p64bit = (Bit64s*) ptr_to_real_val;
  this->is_shadow = 1;
}

// Signed 32 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit32s *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, (Bit32s)BX_MIN_BIT32S, (Bit32s)BX_MAX_BIT32S, *ptr_to_real_val)
{
  this->varsize = 16;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p32bit = ptr_to_real_val;
  this->is_shadow = 1;	
}

// Unsigned 32 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit32u *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT32U, BX_MAX_BIT32U, *ptr_to_real_val)
{
  this->varsize = 32;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p32bit = (Bit32s*) ptr_to_real_val;
  this->is_shadow = 1;
}

// Signed 16 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit16s *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT16S, BX_MAX_BIT16S, *ptr_to_real_val)
{
  this->varsize = 16;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p16bit = ptr_to_real_val;
  this->is_shadow = 1;
}

// Unsigned 16 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit16u *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT16U, BX_MAX_BIT16U, *ptr_to_real_val)
{
  this->varsize = 16;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p16bit = (Bit16s*) ptr_to_real_val;
  this->is_shadow = 1;
}

// Signed 8 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit8s *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT8S, BX_MAX_BIT8S, *ptr_to_real_val)
{
  this->varsize = 16;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p8bit = ptr_to_real_val;
  this->is_shadow = 1;
}

// Unsigned 8 bit
bx_shadow_num_c::bx_shadow_num_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit8u *ptr_to_real_val,
    Bit8u highbit,
    Bit8u lowbit)
: bx_param_num_c (parent, name, description, BX_MIN_BIT8U, BX_MAX_BIT8U, *ptr_to_real_val)
{
  this->varsize = 8;
  this->lowbit = lowbit;
  this->mask = (1 << (highbit - lowbit)) - 1;
  val.p8bit = (Bit8s*) ptr_to_real_val;
  this->is_shadow = 1;
}

Bit64s
bx_shadow_num_c::get64 () {
  Bit64u current = 0;
  switch (varsize) {
    case 8: current = *(val.p8bit);  break;
    case 16: current = *(val.p16bit);  break;
    case 32: current = *(val.p32bit);  break;
    case 64: current = *(val.p64bit);  break;
    default: BX_PANIC(("unsupported varsize %d", varsize));
  }
  current = (current >> lowbit) & mask;
  if (handler) {
    // the handler can decide what value to return and/or do some side effect
    return (*handler)(this, 0, current) & mask;
  } else {
    // just return the value
    return current;
  }
}

void
bx_shadow_num_c::set (Bit64s newval, bx_bool ignore_handler)
{
  Bit64u tmp = 0;
  if ((newval < min || newval > max) && max != BX_MAX_BIT64U)
    BX_PANIC (("numerical parameter %s was set to %lld, which is out of range %lld to %lld", get_name (), newval, min, max));
  switch (varsize) {
    case 8: 
      tmp = (*(val.p8bit) >> lowbit) & mask;
      tmp |= (newval & mask) << lowbit;
      *(val.p8bit) = tmp;
      break;
    case 16:
      tmp = (*(val.p16bit) >> lowbit) & mask;
      tmp |= (newval & mask) << lowbit;
      *(val.p16bit) = tmp;
      break;
    case 32:
      tmp = (*(val.p32bit) >> lowbit) & mask;
      tmp |= (newval & mask) << lowbit;
      *(val.p32bit) = tmp;
      break;
    case 64:
      tmp = (*(val.p64bit) >> lowbit) & mask;
      tmp |= (newval & mask) << lowbit;
      *(val.p64bit) = tmp;
      break;
    default: 
      BX_PANIC(("unsupported varsize %d", varsize));
  }
  if (!ignore_handler && handler) {
    // the handler can override the new value and/or perform some side effect
    (*handler)(this, 1, tmp);
  }
}

void 
bx_shadow_num_c::reset ()
{
  BX_PANIC (("reset not supported on bx_shadow_num_c yet"));
}

bx_param_bool_c::bx_param_bool_c (bx_param_c *parent,
    char *name,
    char *description,
    Bit64s initial_val)
  : bx_param_num_c (parent, name, description, 0, 1, initial_val)
{
  set_type (BXT_PARAM_BOOL);
  set (initial_val);
}

bx_shadow_bool_c::bx_shadow_bool_c (bx_param_c *parent,
      char *name,
      char *description,
      bx_bool *ptr_to_real_val,
      Bit8u bitnum)
  : bx_param_bool_c (parent, name, description, (Bit64s) *ptr_to_real_val)
{
  val.pbool = ptr_to_real_val;
  this->bitnum = bitnum;
  this->is_shadow = 1;
}

Bit64s
bx_shadow_bool_c::get64 () {
  if (handler) {
    // the handler can decide what value to return and/or do some side effect
    Bit64s ret = (*handler)(this, 0, (Bit64s) *(val.pbool));
    return (ret>>bitnum) & 1;
  } else {
    // just return the value
    return (*(val.pbool)) & 1;
  }
}

void
bx_shadow_bool_c::set (Bit64s newval, bx_bool ignore_handler)
{
  // only change the bitnum bit
  Bit64s tmp = (newval&1) << bitnum;
  *(val.pbool) &= ~tmp;
  *(val.pbool) |= tmp;
  if (!ignore_handler && handler) {
    // the handler can override the new value and/or perform some side effect
    (*handler)(this, 1, newval&1);
  }
}

bx_param_enum_c::bx_param_enum_c (bx_param_c *parent,
      char *name,
      char *description,
      char **choices,
      Bit64s initial_val,
      Bit64s value_base)
  : bx_param_num_c (parent, name, description, value_base, BX_MAX_BIT64S, initial_val)
{
  set_type (BXT_PARAM_ENUM);
  this->choices = choices;
  // count number of choices, set max
  char **p = choices;
  while (*p != NULL) p++;
  this->min = value_base;
  // now that the max is known, replace the BX_MAX_BIT64S sent to the parent
  // class constructor with the real max.
  this->max = value_base + (p - choices - 1);
  set (initial_val);
}

int 
bx_param_enum_c::find_by_name (const char *string)
{
  char **p;
  for (p=&choices[0]; *p; p++) {
    if (!strcmp (string, *p))
      return p-choices;
  }
  return -1;
}

bool 
bx_param_enum_c::set_by_name (const char *string, bx_bool ignore_handler)
{
  int n = find_by_name (string);
  if (n<0) return false;
  set (n, ignore_handler);
  return true;
}

bx_param_string_c::bx_param_string_c (bx_param_c *parent,
    char *name,
    char *description,
    char *initial_val,
    int maxsize)
  : bx_param_c (parent, name, description)
{
  set_type (BXT_PARAM_STRING);
  if (maxsize < 0) 
    maxsize = strlen(initial_val) + 1;
  this->val = new char[maxsize];
  this->initial_val = new char[maxsize];
  this->handler = NULL;
  this->maxsize = maxsize;
  strncpy (this->val, initial_val, maxsize);
  strncpy (this->initial_val, initial_val, maxsize);
  this->options = new bx_param_num_c (NULL,
      "stringoptions", NULL, 0, BX_MAX_BIT64S, 0);
  set (initial_val);
}

bx_param_filename_c::bx_param_filename_c (bx_param_c *parent,
    char *name,
    char *description,
    char *initial_val,
    int maxsize)
  : bx_param_string_c (parent, name, description, initial_val, maxsize)
{
  get_options()->set (IS_FILENAME);
}

bx_param_string_c::~bx_param_string_c ()
{
    if ( this->val != NULL )
    {
        delete [] this->val;
        this->val = NULL;
    }
    if ( this->initial_val != NULL )
    {
        delete [] this->initial_val;
        this->initial_val = NULL;
    }

    if ( this->options != NULL )
    {
        delete [] this->options;
        this->options = NULL;
    }
}

void 
bx_param_string_c::reset () {
  strncpy (this->val, this->initial_val, maxsize);
}

void 
bx_param_string_c::set_handler (param_string_event_handler handler)
{
  this->handler = handler; 
  // now that there's a handler, call set once to run the handler immediately
  //set (getptr ());
}

Bit32s
bx_param_string_c::get (char *buf, int len)
{
  if (options->get () & RAW_BYTES)
    memcpy (buf, val, len);
  else
    strncpy (buf, val, len);
  if (handler) {
    // the handler can choose to replace the value in val/len.  Also its
    // return value is passed back as the return value of get.
    (*handler)(this, 0, buf, len);
  }
  return 0;
}

void 
bx_param_string_c::set (char *buf, bx_bool ignore_handler)
{
  if (options->get () & RAW_BYTES)
    memcpy (val, buf, maxsize);
  else
    strncpy (val, buf, maxsize);
  if (!ignore_handler && handler) {
    // the handler can return a different char* to be copied into the value
    buf = (*handler)(this, 1, buf, -1);
  }
}

bx_bool
bx_param_string_c::equals (const char *buf)
{
  if (options->get () & RAW_BYTES)
    return (memcmp (val, buf, maxsize) == 0);
  else
    return (strncmp (val, buf, maxsize) == 0);
}

bx_list_c::bx_list_c (bx_param_c *parent, int maxsize)
  : bx_param_c (parent, "list", "")
{
  set_type (BXT_LIST);
  this->size = 0;
  this->maxsize = maxsize;
  this->list = new bx_param_c*  [maxsize];
  init ();
}

bx_list_c::bx_list_c (bx_param_c *parent, char *name, char *description, 
    int maxsize)
  : bx_param_c (parent, name, description)
{
  set_type (BXT_LIST);
  this->size = 0;
  this->maxsize = maxsize;
  this->list = new bx_param_c*  [maxsize];
  init ();
}

bx_list_c::bx_list_c (bx_param_c *parent, char *name, char *description, bx_param_c **init_list)
  : bx_param_c (parent, name, description)
{
  set_type (BXT_LIST);
  this->size = 0;
  while (init_list[this->size] != NULL)
    this->size++;
  this->maxsize = this->size;
  this->list = new bx_param_c*  [maxsize];
  for (int i=0; i<this->size; i++)
    this->list[i] = init_list[i];
  init ();
}

bx_list_c::~bx_list_c()
{
    if (this->list)
    {
        delete [] this->list;
        this->list = NULL;
    }
    if ( this->title != NULL)
    {
        delete this->title;
        this->title = NULL;
    }
    if (this->options != NULL)
    {
        delete this->options;
        this->options = NULL;
    }
    if ( this->choice != NULL )
    {
        delete this->choice;
        this->choice = NULL;
    }
}

void
bx_list_c::init ()
{
  // the title defaults to the name
  this->title = new bx_param_string_c (NULL,
      "title of list",
      "",
      get_description (), 80);
  this->options = new bx_param_num_c (NULL,
      "list_option", "", 0, BX_MAX_BIT64S,
      0);
  this->choice = new bx_param_num_c (NULL,
      "list_choice", "", 0, BX_MAX_BIT64S,
      1);
}

bx_list_c *
bx_list_c::clone ()
{
  bx_list_c *newlist = new bx_list_c (NULL, name, description, maxsize);
  for (int i=0; i<get_size (); i++)
    newlist->add (get(i));
  newlist->set_options (get_options ());
  return newlist;
}

void
bx_list_c::add (bx_param_c *param)
{
  if (this->size >= this->maxsize)
    BX_PANIC (("add param %u to bx_list_c id=%u: list capacity exceeded", param->get_id (), get_id ()));
  list[size] = param;
  size++;
}

bx_param_c *
bx_list_c::get (int index)
{
  BX_ASSERT (index >= 0 && index < size);
  return list[index];
}

bx_param_c *
bx_list_c::get_by_name (const char *name)
{
  int i, imax = get_size ();
  for (i=0; i<imax; i++) {
    bx_param_c *p = get(i);
    if (0 == strcmp (name, p->get_name ())) {
      return p;
    }
  }
  return NULL;
}

void print_tree (bx_param_c *node, int level)
{
  int i;
  for (i=0; i<level; i++)
    printf ("  ");
  if (node == NULL) {
      printf ("NULL pointer\n");
      return;
  }
  switch (node->get_type()) {
    case BXT_PARAM_NUM:
      {
	bx_param_num_c *num = (bx_param_num_c *) node;
	int base = num->get_base ();
	BX_ASSERT (base==10 || base==16);
	printf ("%s = ", node->get_name ());
	if (base==10)
	  printf ("%d  (number)\n", num->get ());
	else
	  printf ("0x%x  (number)\n", num->get ());
      }
      break;
    case BXT_PARAM_BOOL:
      printf ("%s = %s  (boolean)\n", node->get_name(), ((bx_param_bool_c*)node)->get()?"true":"false");
      break;
    case BXT_PARAM_STRING:
      printf ("%s = '%s'  (string)\n", node->get_name(), ((bx_param_string_c*)node)->getptr());
      break;
    case BXT_LIST:
      {
	printf ("%s = \n", node->get_name ());
	bx_list_c *list = (bx_list_c*)node;
        for (i=0; i < list->get_size (); i++) {
          // distinguish between real children and 'links' where the 
          // child's parent ptr does not point to this list.  For links,
          // do not descend into the object.
          bx_bool is_link = (list != list->get(i)->get_parent ());
          if (is_link) {
            char pname[BX_PATHNAME_LEN];
            list->get(i)->get_param_path (pname, sizeof(pname));
            for (int indent=0; indent<level+1; indent++) printf ("  ");
            printf ("%s --> link to %s\n", list->get(i)->get_name (), pname);
	    } else {
              print_tree (list->get(i), level+1);
	    }
          }
	break;
      }
    case BXT_PARAM_ENUM:
      {
	bx_param_enum_c *e = (bx_param_enum_c*) node;
	int val = e->get ();
	char *choice = e->get_choice(val);
	printf ("%s = '%s'  (enum)\n", node->get_name (), choice);
      }
      break;
    case BXT_PARAM:
    default:
      printf ("%s = <PRINTING TYPE %d NOT SUPPORTED>\n", node->get_name (), node->get_type ());
  }
}



/*---------------------------------------------------------------------------*/
// FIXME: put these includes in a better place
#include <sys/stat.h> // for mkdir
#include <string.h>   // strcpy, strcat, etc.
#include <stdio.h>    // fopen, fgets, etc.
#include <stdlib.h>

// the following are for fstat()
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
                                                                         
                                                                              
/*---------------------------------------------------------------------------*/
void bx_checkpoint_c::save_param_tree(bx_param_c *node, int level)
{
  // indent 1 character per level of tree depth
  for (int i=0; i<level; i++)
    fprintf (m_ascii_fp, " ");
  if (node == NULL) {
    fprintf (m_ascii_fp, "NULL pointer\n");
    return;
  }

  // dump value based upon the parameter type
  switch (node->get_type()) {
  case BXT_PARAM_NUM:
    {
      // number parameters get dumped as either hex or decimal based upon the 
      // 'base' field of the param
      bx_param_num_c *num = (bx_param_num_c *) node;
      int base = num->get_base ();
      BX_ASSERT (base==10 || base==16);
      fprintf (m_ascii_fp, "%s=", node->get_name ());
      if (base==10)
        fprintf (m_ascii_fp, "%d\n", num->get ());
      else
        fprintf (m_ascii_fp, "0x%x\n", num->get ());
      break;
    }
  case BXT_PARAM_BOOL:
    {
      // boolean get dumped as either 'true' or 'false'
      fprintf (m_ascii_fp, "%s=%s\n", node->get_name(), 
               ((bx_param_bool_c*)node)->get()?"true":"false");
      break;
    }
  case BXT_PARAM_STRING:
    {
      fprintf (m_ascii_fp, "%s=\"%s\"\n", node->get_name(), 
               ((bx_param_string_c*)node)->getptr());
      break;
    }
  case BXT_LIST:
    {
      // lists begin with '{' and end with '}'.  each element is seperated by a 
      // line break.
      fprintf (m_ascii_fp, "%s={\n", node->get_name ());
      bx_list_c *list = (bx_list_c*)node;
      for (int i=0; i < list->get_size (); i++) {
        // distinguish between real children and 'links' where the 
        // child's parent ptr does not point to this list.  For links,
        // do not descend into the object.
        bx_bool is_link = (list != list->get(i)->get_parent ());
        if (is_link) {
          char pname[BX_PATHNAME_LEN];
          list->get(i)->get_param_path (pname, sizeof(pname));
          for (int indent=0; indent<level+1; indent++) fprintf (m_ascii_fp, " ");
          fprintf (m_ascii_fp, "%s=<link>\"%s\"\n", list->get(i)->get_name (), pname);
        } else {
          save_param_tree (list->get(i), level+1);
        }
      }
      
      // print ending '}' with indenting
      for (int i=0; i<level; i++)
        fprintf (m_ascii_fp, " ");
      fprintf(m_ascii_fp, "}\n");
      break;
    }
  case BXT_PARAM_ENUM:
    {
      bx_param_enum_c *e = (bx_param_enum_c*) node;
      int val = e->get ();
      fprintf (m_ascii_fp, "%s=<enum>0x%x\n", e->get_name (), val); 
      break;
    }
  case BXT_PARAM:
    {
    default:
      fprintf (m_ascii_fp, "%s=<PRINTING TYPE %d NOT SUPPORTED>\n", 
               node->get_name (), node->get_type ());
    }
  }
}


/*---------------------------------------------------------------------------*/
#define CHKPT_PARAM_NOT_FOUND()                                               \
  BX_PANIC(("error when loading checkpoint. "                                 \
          "param \"%s\" not found in list param \"%s\".\n",                   \
          param_str, qualified_path_str));

#define CHKPT_ASSERT_INPUT_TYPE(_param_type, _chk_type)                       \
  if (_param_type != _chk_type) {                                             \
    BX_PANIC(("error when loading checkpoint. "                               \
          "bad bx_objtype found in param \"%s\" not in list param \"%s\".\n", \
          param_str, qualified_path_str));                                    \
  }
  

#define CHKPT_DEBUG 0

#if CHKPT_DEBUG
static int level=0;
#endif

/*---------------------------------------------------------------------------*/
// The following recursive function is a somewhat crude and contstained parser.
// Each level of the param_tree is processed in one instance of this function
// call, and we descend the tree in a breadth first search.    --BJS
void bx_checkpoint_c::load_param_tree(bx_param_c *param_tree_p, 
                                      char *qualified_path_str)
{
  char *param_str = NULL, *value_str = NULL;
  
  BX_ASSERT(qualified_path_str != NULL);
  
  // read each parameter on this level
  while ((param_str = read_next_param()) != NULL)
    {
      // check for end of current level first
      if (strcmp(param_str, "}") == 0)
        {
          return;
        }

      // read the value for this param
      value_str = read_next_value();
      if (value_str == NULL)
        {
          BX_PANIC(("fatal error when loading checkpoint. " \
                   "value not present with param %s.\n", param_str));
        }

      // check for bx_list_c param
      if (strcmp(value_str, "{") == 0)
        {
          load_param_list(param_tree_p, param_str, value_str, 
                          qualified_path_str);
        }
      // check for bx_param_string_c
      else if ((value_str[0] == '\"') && (value_str[strlen(value_str)-1] == '\"')) 
        {
          load_param_string(param_tree_p, param_str, value_str, 
                            qualified_path_str);
        }
      // check for boolean
      else if ((strcmp(value_str, "false") == 0) || strcmp(value_str, "true") == 0)
        {
          load_param_bool(param_tree_p, param_str, value_str, 
                          qualified_path_str);
        }
      // check for hex string
      else if ((value_str[0] == '0') && (value_str[1] == 'x'))
        {
          load_param_hex_num(param_tree_p, param_str, value_str, 
                             qualified_path_str);
        }
      // check for enum
      else if (strncmp(value_str, "<enum>", 6) == 0)
        {
          load_param_enum(param_tree_p, param_str, &(value_str[6]), 
                          qualified_path_str);
        }
      // check for link
      else if (strncmp(value_str, "<link>", 6) == 0)
        {
          // we dont need to read through this link since it is somewhere
          // else in the tree
        }
      // assume that the value is an decimal string
      else
        {
          load_param_dec_num(param_tree_p, param_str, value_str, 
                             qualified_path_str);
        }
    }

  return;
}

/*---------------------------------------------------------------------------*/
void bx_checkpoint_c::write(const char *checkpoint_name, 
                            bx_param_c *param_tree_p)
{
  char *ascii_filename = NULL;
  char *data_filename = NULL;

  // before we open new files using, make sure any old handles are closed
  if (m_ascii_fp) 
    {
      fclose(m_ascii_fp);
      m_ascii_fp = NULL;
    }
  if (m_data_fp) 
    {
      fclose(m_data_fp);
      m_data_fp = NULL;
    }

  // see if checkpoint directory exists
  struct stat mystat;
  if( stat(checkpoint_name, &mystat) >= 0)
    {
      // FIXME:
      // checkpoint file exists ... is it a directory?
      //if (!S_ISDIR(mystat.st_rdev))
      //  {
      //    BX_PANIC(("could no create \"%s\" directory.  file exists.\n",
      //             checkpoint_name));
      //  }
      //else 
      //  {
      //    // already exists ... warn user.
      //    BX_INFO(("warning: checkpoint directory \"%s\"exists.\n",
      //            checkpoint_name));
      //  }
    }
  // create the checkpoint directory
  else if (mkdir(checkpoint_name, 0755) <0)
    {
      BX_PANIC(("could not create checkpoint directory \"%s\"\n",
               checkpoint_name));
      return;
    }

  // create ascii file name
  ascii_filename = 
    (char*) malloc(strlen(checkpoint_name)+strlen("/param_tree.txt")+1);
  if (ascii_filename)
    {
      strcpy(ascii_filename, checkpoint_name);
      strcat(ascii_filename, "/param_tree.txt");
    }

  // create data file name
  data_filename = 
    (char*) malloc(strlen(checkpoint_name)+strlen("/param_data")+1);
  if (data_filename)
    {
      strcpy(data_filename, checkpoint_name);
      strcat(data_filename, "/param_data");
    }

  if (data_filename == NULL || ascii_filename == NULL)
    {
      BX_PANIC(("system could not service malloc() call\n"));
    }
  // open ascii file for writing
  else if ((m_ascii_fp = fopen(ascii_filename, "w+b")) == NULL)
    {
      BX_PANIC(("could not create/open file \"%s\"\n",
               checkpoint_name));
    }
  // open binary data file for writing
  else if ((m_data_fp = fopen(data_filename, "w+b")) == NULL)
    {
      BX_PANIC(("could not create/open file \"%s\"\n",
               checkpoint_name));
      
    }
  // if no errors and the files all opened succesfully ...
  else 
    {
      // dump the parameter tree to the ascii and data files
      save_param_tree(param_tree_p, 0);
    }

  // cleanup and return
  if (m_ascii_fp) 
    {
      fclose(m_ascii_fp);
      m_ascii_fp = NULL;
    }
  if (m_data_fp) 
    {
      fclose(m_data_fp);
      m_data_fp = NULL;
    }
  if (ascii_filename)
    free(ascii_filename);
  if (data_filename)
    free(data_filename);

  return;  
}


/*---------------------------------------------------------------------------*/
void bx_checkpoint_c::read(const char *checkpoint_name, 
                           bx_param_c *param_tree_p)
{
  char *ascii_filename = NULL;
  char *data_filename = NULL;

  // before we open new files using, make sure any old handles are closed
  if (m_ascii_fp) 
    {
      fclose(m_ascii_fp);
      m_ascii_fp = NULL;
    }
  if (m_data_fp) 
    {
      fclose(m_data_fp);
      m_data_fp = NULL;
    }

  // create ascii file name
  ascii_filename = 
    (char*) malloc(strlen(checkpoint_name)+strlen("/param_tree.txt")+1);
  if (ascii_filename)
    {
      strcpy(ascii_filename, checkpoint_name);
      strcat(ascii_filename, "/param_tree.txt");
    }

  // create data file name
  data_filename = 
    (char*) malloc(strlen(checkpoint_name)+strlen("/param_data")+1);
  if (data_filename)
    {
      strcpy(data_filename, checkpoint_name);
      strcat(data_filename, "/param_data");
    }

  // check for malloc() failures
  if (data_filename == NULL || ascii_filename == NULL)
    {
      BX_PANIC(("system could not service malloc() call\n"));
    }
  // open ascii file for reading
  else if ((m_ascii_fp = fopen(ascii_filename, "rb")) == NULL)
    {
      
      BX_PANIC(("could not create/open file \"%s\"\n for reading",
               checkpoint_name));
    }
  // open binary data file for reading
  else if ((m_data_fp = fopen(data_filename, "rb")) == NULL)
    {
      BX_PANIC(("could not create/open file \"%s\" for reading\n",
               checkpoint_name));
    }
  // if no errors and the files all opened succesfully ...
  else 
    {
      // dump the parameter tree to the ascii and data files
      read_next_param();
      read_next_value();
      // TODO: assert that curly_str is '{'
#if CHKPT_DEBUG
      level = 0; 
#endif
      load_param_tree(param_tree_p, "root");
    }

  // cleanup and return
  if (m_ascii_fp) 
    {
      fclose(m_ascii_fp);
      m_ascii_fp = NULL;
    }
  if (m_data_fp) 
    {
      fclose(m_data_fp);
      m_data_fp = NULL;
    }
  if (ascii_filename)
    free(ascii_filename);
  if (data_filename)
    free(data_filename);

  return;
}


/*---------------------------------------------------------------------------*/
bx_checkpoint_c::bx_checkpoint_c()
{
  m_ascii_fp = NULL;
  m_data_fp = NULL;

  m_line_buf_cursor = NULL;

  return;
}


/*---------------------------------------------------------------------------*/
bx_checkpoint_c::~bx_checkpoint_c()
{
  if (m_ascii_fp)
    {
      fclose(m_ascii_fp);
    }
  if (m_data_fp)
    {
      fclose(m_data_fp);
    }

  return;
}


/*---------------------------------------------------------------------------*/
void bx_checkpoint_c::dump_param_tree(bx_param_c *param_tree_p)
{ 
  if (m_ascii_fp) 
    {
      fclose(m_ascii_fp);
      m_ascii_fp = NULL;
    }
  if (m_data_fp) 
    {
      fclose(m_data_fp);
      m_data_fp = NULL;
    }

  m_ascii_fp = stdout;
  save_param_tree(param_tree_p); 
  m_ascii_fp = NULL;

  return;
};


/*---------------------------------------------------------------------------*/
// read the next parameter in the line buffer
char* bx_checkpoint_c::read_next_param()
{
  if (m_ascii_fp == NULL)
    {
      m_line_buf_cursor = NULL;
      return NULL;
    }

  char* line_str = fgets(m_line_buf, MAX_CHECKPOINT_LINE_SIZE, m_ascii_fp);

  if (line_str == NULL)
    {
      m_line_buf_cursor = NULL;
      return NULL;
    }

  int line_size = strlen(m_line_buf);

  // this variable is persistent across both for loops
  int i = 0;

  // advance pointer to first non-white space
  for (i=0; i<line_size; i++)
    {
      if ( !(isspace(m_line_buf[i])) )
        {
          line_str = &(m_line_buf[i]);
          break;
        }
    }

  // look for end of first token (white space or '=' marks end-of-token)
  for (; i<line_size; i++)
    {
      if ( (m_line_buf[i] == '=') || isspace(m_line_buf[i]) )
        {
          // end the string here!
          m_line_buf[i] = '\0';
          break;
        }
    }

  // set the cursor for right after the param string
  m_line_buf_cursor = &(m_line_buf[i+1]);

  return line_str;
}


/*---------------------------------------------------------------------------*/
// read the next parameter value in the line buffer
char* bx_checkpoint_c::read_next_value()
{
  // advance pointer to first non-white space
  char *line_str = m_line_buf_cursor;

  if (line_str == NULL)
    {
      m_line_buf_cursor = NULL;
      return NULL;
    }

  int line_size = strlen(m_line_buf_cursor);

  // this variable is persistent across both for loops
  int i = 0;

  for (i=0; i<line_size; i++)
    {
      if ( !(isspace(m_line_buf_cursor[i])) )
        {
          line_str = &(m_line_buf_cursor[i]);
          break;
        }
    }
  
  int toggle_quote = 0;

  // look for end of first token (white space or '=' marks end-of-token)
  for (; i<line_size; i++)
    {
      if (m_line_buf_cursor[i] == '\"')
        {
          toggle_quote = toggle_quote ? 0 : 1;
        }
      else if ( !toggle_quote && isspace(m_line_buf_cursor[i]) )
        {
          m_line_buf_cursor[i] = '\0';
          break;
        }
    }


  return line_str;
}


/*---------------------------------------------------------------------------*/
bx_bool 
bx_checkpoint_c::load_param_hex_num(bx_param_c *parent_p, 
                                    char *param_str, 
                                    char *value_str,
                                    char *qualified_path_str)
{
#if CHKPT_DEBUG
  for (int i=0; i<level; i++) printf(" ");
  printf("hex param = %s, value = %s\n", param_str, value_str);
#endif // #if CHKPT_DEBUG

  bx_param_c *param_p = parent_p->get_by_name(param_str);
  if (param_p == NULL)
    {
      CHKPT_PARAM_NOT_FOUND();
    }
  else
    {
      CHKPT_ASSERT_INPUT_TYPE(param_p->get_type(), BXT_PARAM_NUM);
                
      // convert the hex string to a long int
      char *str_ptr;
      long int value = strtol(value_str, &str_ptr, 0);
      if (*str_ptr != '\0')
        {
          BX_PANIC(("fatal error when loading checkpoint. " \
                   "value not valid with param %s.\n", param_str));
        }

#if CHKPT_DEBUG
      for (int i=0; i<level; i++) printf(" ");
      printf("            %s, put   = %0xlx\n", param_str, value);
#endif // #if CHKPT_DEBUG

      // actual loading of new value
      if (param_p->is_shadow_param())
        {
          ((bx_shadow_num_c*)param_p)->set(value);
        }
      else
        {
          ((bx_param_num_c*)param_p)->set(value, 1);
        }
    }
          


  return 1;
}

/*---------------------------------------------------------------------------*/
bx_bool 
bx_checkpoint_c::load_param_dec_num(bx_param_c *parent_p, 
                                    char *param_str, 
                                    char *value_str,
                                    char *qualified_path_str)
{
#if CHKPT_DEBUG
  for (int i=0; i<level; i++) printf(" ");
  printf("dec param = %s, value = %s\n", param_str, value_str);
#endif // #if CHKPT_DEBUG

  bx_param_c *param_p = parent_p->get_by_name(param_str);
  if (param_p == NULL)
    {
      CHKPT_PARAM_NOT_FOUND();
    }
  else
    {
      CHKPT_ASSERT_INPUT_TYPE(param_p->get_type(), BXT_PARAM_NUM);
          
      // convert the hex string to a long int
      char *str_ptr;
      long int value = strtol(value_str, &str_ptr, 0);
      if (*str_ptr != '\0')
        {
          BX_PANIC(("error when loading checkpoint. " \
                   "value not valid with param %s.\n", param_str));
        }

#if CHKPT_DEBUG
      for (int i=0; i<level; i++) printf(" ");
      printf("            %s, put   = %ld\n", param_str, value);
#endif // #if CHKPT_DEBUG
      
      // actual loading of new value into location pointed to by shadow
      if (param_p->is_shadow_param())
        {
          ((bx_shadow_num_c*)param_p)->set(value);
        }
      else 
        {
          ((bx_param_num_c*)param_p)->set(value, 1);
        }
      
    }
  return 1;
}

/*---------------------------------------------------------------------------*/
bx_bool 
bx_checkpoint_c::load_param_string(bx_param_c *parent_p, 
                                   char *param_str, 
                                   char *value_str,
                                   char *qualified_path_str)
{
#if CHKPT_DEBUG
  for (int i=0; i<level; i++) printf(" ");
  printf("str param = %s, value = %s\n", param_str, value_str);
#endif // #if CHKPT_DEBUG
          
  bx_param_c *param_p = parent_p->get_by_name(param_str);
  if (param_p == NULL)
    {
      BX_PANIC(("error when loading checkpoint. " \
                "param \"%s\" not found in list param \"%s\".\n", 
                param_str, qualified_path_str));
    }
  else
    {
      CHKPT_ASSERT_INPUT_TYPE(param_p->get_type(), BXT_PARAM_STRING);

      BX_ASSERT(param_p->is_shadow_param() == 0);

      // since the string contains '\"' at the beginning and the end, we
      // need to remove them.
      value_str = &(value_str[1]);
      value_str[strlen(value_str)-1] = '\0';

#if CHKPT_DEBUG
      for (int i=0; i<level; i++) printf(" ");
      printf("            %s, put   = \"%s\"\n", param_str, value_str);
#endif // #if CHKPT_DEBUG

      // actual loading of new value
      ((bx_param_string_c*)param_p)->set(value_str);
    }
          

  return 1;
}

/*---------------------------------------------------------------------------*/
bx_bool 
bx_checkpoint_c::load_param_bool(bx_param_c *parent_p, 
                                 char *param_str, 
                                 char *value_str,
                                 char *qualified_path_str)
{

#if CHKPT_DEBUG
  for (int i=0; i<level; i++) printf(" ");
  printf("boolean param = %s, value = %s\n", param_str, value_str);
#endif // #if CHKPT_DEBUG

  bx_param_c *param_p = parent_p->get_by_name(param_str);
  if (param_p == NULL)
    {
      CHKPT_PARAM_NOT_FOUND();
    }
  else
    {
      CHKPT_ASSERT_INPUT_TYPE(param_p->get_type(), BXT_PARAM_BOOL);
      
      bx_bool value;

      if (strcmp(value_str, "true") == 0)
        {
          value = 1;
        }
      else 
        {
          value = 0;
        }

      // actually set the value
      if (param_p->is_shadow_param())
        {
          ((bx_shadow_bool_c*)param_p)->set(value);
        }
      else
        {
          ((bx_param_bool_c*)param_p)->set(value, 1);
        }
    }

  return 1;
}

/*---------------------------------------------------------------------------*/
bx_bool 
bx_checkpoint_c::load_param_list(bx_param_c *parent_p, 
                                 char *param_str, 
                                 char *value_str,
                                 char *qualified_path_str)
{
  // this param is a bx_list_c, so descend param_tree and recursively
  // call load_param_tree() to process next level

#if CHKPT_DEBUG
  for (int i=0; i<level; i++) printf(" ");
  printf("list param = %s\n", param_str);
#endif // #if CHKPT_DEBUG

  // find the param in the current level of the tree
  bx_param_c *param_p = parent_p->get_by_name(param_str);
  if (param_p == NULL)
    {
      CHKPT_PARAM_NOT_FOUND();
    }
  else
    {
      CHKPT_ASSERT_INPUT_TYPE(param_p->get_type(), BXT_LIST);

      // we have found the param we are looking for.  create a new
      // qualified path string and recurse
      char *new_qual_str =  
        (char*) malloc(strlen(qualified_path_str)+
                       strlen(param_str)+1+1);
      if (new_qual_str == 0)
        {
          BX_PANIC(("fatal error when loading checkpoint. " \
                   "malloc() returned no memory in %s.%s.\n", 
                   qualified_path_str, param_str));
        }
      strcpy(new_qual_str, qualified_path_str);
      strcat(new_qual_str, ".");
      strcat(new_qual_str, param_str);

      // recurse down the tree
#if CHKPT_DEBUG
      level++;
      load_param_tree(param_p, new_qual_str);
      level--;
#else // #if CHKPT_DEBUG
      load_param_tree(param_p, new_qual_str);
#endif // #if CHKPT_DEBUG

      // free string memory
      free(new_qual_str);
    }

  return 1;
}

/*---------------------------------------------------------------------------*/
bx_bool 
bx_checkpoint_c::load_param_enum(bx_param_c *parent_p, 
                                    char *param_str, 
                                    char *value_str,
                                    char *qualified_path_str)
{
#if CHKPT_DEBUG
  for (int i=0; i<level; i++) printf(" ");
  printf("enum param = %s, value = %s\n", param_str, value_str);
#endif // #if CHKPT_DEBUG

  bx_param_c *param_p = ((bx_list_c*)parent_p)->get_by_name(param_str);
  if (param_p == NULL)
    {
      CHKPT_PARAM_NOT_FOUND();
    }
  else
    {
      CHKPT_ASSERT_INPUT_TYPE(param_p->get_type(), BXT_PARAM_ENUM);
          
      // convert the hex string to a long int
      char *str_ptr;
      long int value = strtol(value_str, &str_ptr, 0);

#if CHKPT_DEBUG
      for (int i=0; i<level; i++) printf(" ");
      printf("             %s, put   = %lx\n", param_str, value);
#endif // #if CHKPT_DEBUG

      if (*str_ptr != '\0')
        {
          BX_PANIC(("error when loading checkpoint. " \
                   "value not valid with param %s.\n", param_str));
        }
      
      // actual loading of new value into location pointed to by shadow
      if (param_p->is_shadow_param())
        {
          ((bx_shadow_num_c*)param_p)->set(value);
        }
      else 
        {
          ((bx_param_enum_c*)param_p)->set(value, 1);
        }
      
    }
  return 1;
}
