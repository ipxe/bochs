//
// wxmain.cc
// Main program for wxWindows.  This does not replace main.cc by any means.
// It just provides the program entry point, and calls functions in main.cc
// when it's appropriate.
//
// Note that this file does NOT include bochs.h.  In an attempt to keep the
// interface between Bochs and its gui clean and well defined, all 
// communication is done through the siminterface object.
//
// Types of events between threads:
// - async events from gui to Bochs. example: start, pause, resume, keypress
// - async events from Bochs to gui. example: log message
// - synchronous events from Bochs to gui. Bochs will block forever until the
//   gui responds to it.
//
// Sync events are what I need first.  Bochs calls siminterface function
// which creates an event and calls wx_bochs2gui_sync_event (event).  This
// function clears the synchronous event mailbox, posts the event onto 
// the GUI event queue, then blocks forever until the GUI places the response
// event in the mailbox.  Then it clears the mailbox and returns the response
// event.

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "config.h"
extern "C" {
// siminterface needs stdio.h
#include <stdio.h>
}
#include "osdep.h"
#include "gui/control.h"
#include "gui/siminterface.h"

class MyApp: public wxApp
{
virtual bool OnInit();
};

class MyFrame;

class BochsThread: public wxThread
{
  // when Bochs sends a synchronous event to the GUI thread, the response
  // is stored here.
  MyFrame *frame;

  // support response to a synchronous event.
  // FIXME: this would be cleaner and more reusable if I made a thread-safe
  // mailbox class.
  BxEvent *bochs2gui_mailbox;
  wxCriticalSection bochs2gui_mailbox_lock;

public:
  BochsThread (MyFrame *_frame) { frame = _frame; }
  virtual ExitCode Entry ();
  void OnExit ();
  // called by the siminterface code, with the pointer to the Bochs thread
  // in the thisptr arg.
  static BxEvent *SiminterfaceCallback (void *thisptr, BxEvent *event);
  BxEvent *SiminterfaceCallback2 (BxEvent *event);
  // methods to coordinate synchronous response mailbox
  void ClearSyncResponse ();
  void SendSyncResponse (BxEvent *);
  BxEvent *GetSyncResponse ();
private:
};

class MyFrame: public wxFrame
{
public:
MyFrame(const wxString& title, const wxPoint& pos, const wxSize&
size);
void OnQuit(wxCommandEvent& event);
void OnAbout(wxCommandEvent& event);
void OnStartBochs(wxCommandEvent& event);
void OnPauseResumeBochs(wxCommandEvent& event);
void OnKillBochs(wxCommandEvent& event);
void OnBochs2GuiEvent(wxCommandEvent& event);
int HandleAskParam (BxEvent *event);
int HandleAskFilename (BxEvent *event);
int StartBochsThread ();

// called from the Bochs thread's OnExit() method.
void OnBochsThreadExit ();

private:
wxCriticalSection bochs_thread_lock;
BochsThread *bochs_thread; // get the lock before accessing bochs_thread
int HandleVgaGuiButton (bx_id param);
int start_bochs_times;
wxMenu *menuConfiguration;
wxMenu *menuEdit;
wxMenu *menuSimulate;
wxMenu *menuDebug;
wxMenu *menuLog;
wxMenu *menuHelp;

DECLARE_EVENT_TABLE()
};

enum
{
ID_Quit = 1,
ID_Config_New,
ID_Config_Read,
ID_Config_Save,
ID_Edit_Disks,
ID_Edit_Boot,
ID_Edit_Vga,
ID_Edit_Memory,
ID_Edit_Sound,
ID_Edit_Network,
ID_Edit_Keyboard,
ID_Edit_Other,
ID_Simulate_Start,
ID_Simulate_PauseResume,
ID_Simulate_Stop,
ID_Simulate_Speed,
ID_Debug_ShowCpu,
ID_Debug_ShowMemory,
ID_Log_View,
ID_Log_Prefs,
ID_Log_PrefsDevice,
ID_Help_About,
ID_Bochs2Gui_Event,
};

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(ID_Quit, MyFrame::OnQuit)
EVT_MENU(ID_Help_About, MyFrame::OnAbout)
EVT_MENU(ID_Simulate_Start, MyFrame::OnStartBochs)
EVT_MENU(ID_Simulate_PauseResume, MyFrame::OnPauseResumeBochs)
EVT_MENU(ID_Simulate_Stop, MyFrame::OnKillBochs)
EVT_MENU(ID_Bochs2Gui_Event, MyFrame::OnBochs2GuiEvent)
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
  siminterface_init ();
  MyFrame *frame = new MyFrame( "Bochs Control Panel", wxPoint(50,50),
  wxSize(450,340) );
  frame->Show( TRUE );
  SetTopWindow( frame );
  return TRUE;
}

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const
wxSize& size)
: wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
  // init variables
  bochs_thread = NULL;
  start_bochs_times = 0;

  // set up the gui
  menuConfiguration = new wxMenu;
  menuConfiguration->Append( ID_Config_New, "&New Configuration" );
  menuConfiguration->Append( ID_Config_Read, "&Read Configuration" );
  menuConfiguration->Append( ID_Config_Save, "&Save Configuration" );
  menuConfiguration->AppendSeparator ();
  menuConfiguration->Append (ID_Quit, "&Quit");

  menuEdit = new wxMenu;
  menuEdit->Append( ID_Edit_Disks, "&Disks..." );
  menuEdit->Append( ID_Edit_Boot, "&Boot..." );
  menuEdit->Append( ID_Edit_Vga, "&VGA..." );
  menuEdit->Append( ID_Edit_Memory, "&Memory..." );
  menuEdit->Append( ID_Edit_Sound, "&Sound..." );
  menuEdit->Append( ID_Edit_Network, "&Network..." );
  menuEdit->Append( ID_Edit_Keyboard, "&Keyboard..." );
  menuEdit->Append( ID_Edit_Other, "&Other..." );

  menuSimulate = new wxMenu;
  menuSimulate->Append( ID_Simulate_Start, "&Start...");
  menuSimulate->Append( ID_Simulate_PauseResume, "&Pause...");
  menuSimulate->Append( ID_Simulate_Stop, "S&top...");
  menuSimulate->AppendSeparator ();
  menuSimulate->Append( ID_Simulate_Speed, "S&peed...");
  menuSimulate->Enable (ID_Simulate_PauseResume, FALSE);
  menuSimulate->Enable (ID_Simulate_Stop, FALSE);

  menuDebug = new wxMenu;
  menuDebug->Append (ID_Debug_ShowCpu, "Show &CPU");
  menuDebug->Append (ID_Debug_ShowMemory, "Show &memory");

  menuLog = new wxMenu;
  menuLog->Append (ID_Log_View, "&View");
  menuLog->Append (ID_Log_Prefs, "&Preferences...");
  menuLog->Append (ID_Log_PrefsDevice, "By &Device...");

  menuHelp = new wxMenu;
  menuHelp->Append( ID_Help_About, "&About..." );

  wxMenuBar *menuBar = new wxMenuBar;
  menuBar->Append( menuConfiguration, "&File" );
  menuBar->Append( menuEdit, "&Edit" );
  menuBar->Append( menuSimulate, "&Simulate" );
  menuBar->Append( menuDebug, "&Debug" );
  menuBar->Append( menuLog, "&Log" );
  menuBar->Append( menuHelp, "&Help" );
  SetMenuBar( menuBar );
  CreateStatusBar();
  SetStatusText( "Welcome to wxWindows!" );
}

void MyFrame::OnQuit(wxCommandEvent& event)
{
  Close( TRUE );
  OnKillBochs (event);
#if 0
  if (SIM)
	SIM->quit_sim(0);  // give bochs a chance to shut down
#endif
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
wxMessageBox( "wxWindows Control Panel for Bochs. (Very Experimental)",
"Bochs Control Panel", wxOK | wxICON_INFORMATION );
}

void MyFrame::OnStartBochs(wxCommandEvent& WXUNUSED(event))
{
  wxCriticalSectionLocker lock(bochs_thread_lock);
  if (bochs_thread != NULL) {
	wxMessageBox (
	  "Can't start Bochs simulator, because it is already running",
	  "Already Running", wxOK | wxICON_ERROR);
	return;
  }
  wxLogStatus ("Starting a Bochs simulation\n");
  start_bochs_times++;
  if (start_bochs_times>1) {
	wxMessageBox (
	"You have already started up Bochs once this session. Due to memory leaks, you may get unstable behavior.",
	"2nd time warning", wxOK | wxICON_WARNING);
  }
  bochs_thread = new BochsThread (this);
  bochs_thread->Create ();
  bochs_thread->Run ();                                                        
  wxLogDebug ("Bochs thread has started.\n");
  // set up callback for events from Bochs
  SIM->set_notify_callback (&BochsThread::SiminterfaceCallback, bochs_thread);
  // fix up menu choices
  menuSimulate->Enable (ID_Simulate_Start, FALSE);
  menuSimulate->Enable (ID_Simulate_PauseResume, TRUE);
  menuSimulate->Enable (ID_Simulate_Stop, TRUE);
  menuSimulate->SetLabel (ID_Simulate_PauseResume, "&Pause");
}

void MyFrame::OnPauseResumeBochs(wxCommandEvent& WXUNUSED(event))
{
  wxCriticalSectionLocker lock(bochs_thread_lock);
  if (bochs_thread) {
    if (bochs_thread->IsPaused ()) {
	  SetStatusText ("Resuming the Bochs simulation");
	  bochs_thread->Resume ();
	  menuSimulate->SetLabel (ID_Simulate_PauseResume, "&Pause");
	} else {
	  SetStatusText ("Pausing the Bochs simulation");
	  bochs_thread->Pause ();
	  menuSimulate->SetLabel (ID_Simulate_PauseResume, "&Resume");
	}
  }
}

void MyFrame::OnKillBochs(wxCommandEvent& WXUNUSED(event))
{
  // DON'T use a critical section here.  Delete implicitly calls
  // OnBochsThreadExit, which also tries to lock bochs_thread_lock.
  // If we grab the lock at this level, deadlock results.
  if (bochs_thread) {
	SetStatusText ("Killing the Bochs simulation");
	bochs_thread->Delete ();
	menuSimulate->Enable (ID_Simulate_Start, TRUE);
	menuSimulate->Enable (ID_Simulate_PauseResume, FALSE);
	menuSimulate->Enable (ID_Simulate_Stop, FALSE);
	menuSimulate->SetLabel (ID_Simulate_PauseResume, "&Pause");
  }
}

void
MyFrame::OnBochsThreadExit () {
  wxCriticalSectionLocker lock (bochs_thread_lock);
  bochs_thread = NULL; 
}

// This is called when the simulator needs to ask the user to choose
// a value or setting.  For example, when the user indicates that he wants
// to change the floppy disk image for drive A, an ask-param event is created
// with the parameter id set to BXP_FLOPPYA_PATH.  The simulator blocks until
// the gui has displayed a dialog and received a selection from the user.
// In the current implemention, the GUI will look up the parameter's 
// data structure using SIM->get_param() and then call the set method on the
// parameter to change the param.  The return value only needs to return
// success or failure (failure = cancelled, or not implemented).
// Returns 1 if the user chose a value and the param was modified.
// Returns 0 if the user cancelled.
// Returns -1 if the gui doesn't know how to ask for that param.
int 
MyFrame::HandleAskParam (BxEvent *event)
{
  wxASSERT (event->type == BX_SYNC_EVT_ASK_PARAM);
  bx_id param = event->u.param.id;
  switch (param) {
  case BXP_FLOPPYA_PATH:
  case BXP_FLOPPYB_PATH:
  case BXP_DISKC_PATH:
  case BXP_DISKD_PATH:
  case BXP_CDROM_PATH:
	{
	  Raise();  // bring the control panel to front so dialog shows
	  char *msg;
	  if (param==BXP_FLOPPYA_PATH || param==BXP_FLOPPYB_PATH)
	    msg = "Choose new floppy disk image file";
      else if (param==BXP_DISKC_PATH || param==BXP_DISKD_PATH)
	    msg = "Choose new hard disk image file";
      else if (param==BXP_CDROM_PATH)
	    msg = "Choose new CDROM image file";
	  else
	    msg = "Choose new image file";
	  wxFileDialog dialog(this, msg, "", "", "*.*", 0);
	  int ret = dialog.ShowModal();
	  if (ret == wxID_OK)
	  {
	    char *newpath = (char *)dialog.GetPath().c_str ();
	    if (newpath && strlen(newpath)>0) {
	      // change floppy path to this value.
	      bx_param_string_c *Opath = SIM->get_param_string (param);
	      assert (Opath != NULL);
	      wxLogDebug ("Setting floppy %c path to '%s'\n", 
		    param == BXP_FLOPPYA_PATH ? 'A' : 'B',
		    newpath);
	      Opath->set (newpath);
	      return 1;
	    }
	  }
	  return 0;
	}
  default:
	wxLogError ("HandleAskParam: parameter %d, not implemented\n", event->u.param.id);
  }
  return -1;  // could not display
}

int 
MyFrame::HandleAskFilename (BxEvent *event)
{
  Raise();  // bring the control panel to front so dialog shows
  // add option for confirm overwrite.
  wxFileDialog dialog(this, 
    event->u.askfile.prompt,
	event->u.askfile.the_default,
	"", "*.*", 0);
  int ret = dialog.ShowModal();
  if (ret == wxID_OK)
  {
	char *newpath = (char *)dialog.GetPath().c_str ();
	if (newpath && strlen(newpath)>0) {
	  strncpy (event->u.askfile.result, newpath, event->u.askfile.result_len);
	  return 1;
	}
  }
  return 0;
}

void 
MyFrame::OnBochs2GuiEvent (wxCommandEvent& event)
{
  wxLogDebug ("received a bochs event in the GUI thread\n");
  BxEvent *be = (BxEvent *) event.GetEventObject ();
  wxLogDebug ("event type = %d\n", (int) be->type);
  // all cases should return.  sync event handlers MUST send back a 
  // response.
  switch (be->type) {
  case BX_SYNC_EVT_ASK_FILENAME:
    be->retcode = HandleAskFilename (be);
    // sync must return something; just return a copy of the event.
    bochs_thread->SendSyncResponse(be);
	return;
  case BX_SYNC_EVT_ASK_PARAM:
    be->retcode = HandleAskParam (be);
    // sync must return something; just return a copy of the event.
    bochs_thread->SendSyncResponse(be);
	return;
  case BX_ASYNC_EVT_SHUTDOWN_GUI:
	wxLogDebug ("control panel is exiting\n");
    Close (TRUE);
	wxExit ();
	return;
  case BX_ASYNC_EVT_LOG_MSG:
    wxLogDebug ("log msg: level=%d, prefix='%s', msg='%s'\n",
	  be->u.logmsg.level,
	  be->u.logmsg.prefix,
	  be->u.logmsg.msg);
    return;
  default:
    wxLogDebug ("OnBochs2GuiEvent: event type %d ignored\n", (int)be->type);
	// assume it's a synchronous event and send back a response, to avoid
	// potential deadlock.
    bochs_thread->SendSyncResponse(be);
	return;
  }
  // it is critical to send a response back eventually since the Bochs thread
  // is blocking.
  wxASSERT_MSG (0, "switch stmt should have returned");
}

/////////////// bochs thread

void *
BochsThread::Entry (void)
{     
  int argc=1;
  char *argv[] = {"bochs"};
  wxLogDebug ("in BochsThread, starting at bx_continue_after_control_panel\n");
  // run all the rest of the Bochs simulator code.  This function will
  // run forever, unless a "kill_bochs_request" is issued.  The procedure
  // is as follows:
  //   - user selects "Kill Bochs" or GUI decides to kill bochs
  //   - GUI calls bochs_thread->Delete ()
  //   - Bochs continues to run until the next time it reaches SIM->periodic().
  //   - SIM->periodic() sends a synchronous tick event to the GUI, which
  //     finally calls TestDestroy() and realizes it needs to stop.  It
  //     sets the sync event return code to -1.  SIM->periodic() sets the
  //     kill_bochs_request flag in cpu #0.
  //   - cpu loop notices kill_bochs_request and returns to main.cc:
  //     bx_continue_after_control_panel(), which notices the
  //     kill_bochs_request and returns back to this Entry() function.
  //   - Entry() exits and the thread stops. Whew.
  bx_continue_after_control_panel (argc, argv);
  wxLogDebug ("in BochsThread, bx_continue_after_control_panel exited\n");
  return NULL;
}

void
BochsThread::OnExit ()
{
  // notify the MyFrame that the bochs thread has died.  I can't adjust
  // the bochs_thread directly because it's private.
  frame->OnBochsThreadExit ();
  // don't use this BochsThread's callback function anymore.
  SIM->set_notify_callback (NULL, NULL);
}

// return 1 if the user selected a value.
// return 0 if the question was displayed and the user cancelled.
// return -1 if we don't know how to display the question.
int
MyFrame::HandleVgaGuiButton (bx_id param)
{
  wxLogDebug ("HandleVgaGuiButton: button %d was pressed\n", (int)param);
  switch (param)
  {
	case BXP_FLOPPYA_PATH:
	case BXP_FLOPPYB_PATH:
	case BXP_DISKC_PATH:
	case BXP_DISKD_PATH:
	case BXP_CDROM_PATH:
	default:
	  wxLogDebug ("HandleVgaGuiButton: button %d not recognized\n", param);
	  return -1;
  }
}

// This function is declared static so that I can get a usable function
// pointer for it.  The function pointer is passed to SIM->set_notify_callback
// so that the siminterface can call this function when it needs to contact
// the gui.  It will always be called with a pointer to the BochsThread
// as the first argument.
BxEvent *
BochsThread::SiminterfaceCallback (void *thisptr, BxEvent *event)
{
  BochsThread *me = (BochsThread *)thisptr;
  // call the normal non-static method now that we know the this pointer.
  return me->SiminterfaceCallback2 (event);
}

// callback function for Bochs-generated events.  This is called from
// the Bochs thread, not the GUI thread.  So any GUI actions must be
// done after getting the gui mutex.
BxEvent *
BochsThread::SiminterfaceCallback2 (BxEvent *event)
{
  //wxLogDebug ("SiminterfaceCallback with event type=%d\n", (int)event->type);
  event->retcode = 0;  // default return code
  int async = 1;
  switch (event->type)
  {
  case BX_SYNC_EVT_ASK_PARAM:
  case BX_SYNC_EVT_ASK_FILENAME:
  case BX_SYNC_EVT_TICK:
    ClearSyncResponse ();  // must be before postevent.
    async = 0;
	break;
  }

  // tick event must be handled right here in the bochs thread.
  if (event->type == BX_SYNC_EVT_TICK) {
	if (TestDestroy ()) {
	  // tell simulator to quit
	  event->retcode = -1;
	}
	return event;
  }

  //encapsulate the bxevent in a wxwindows event
  wxCommandEvent wxevent (wxEVT_COMMAND_MENU_SELECTED, ID_Bochs2Gui_Event);
  wxevent.SetEventObject ((wxEvent *)event);
  wxLogDebug ("Sending an event to the window\n");
  wxPostEvent (frame, wxevent);
  // if it is an asynchronous event, return immediately
  if (async) return NULL;
  wxLogDebug ("SiminterfaceCallback2: synchronous event; waiting for response\n");
  // now wait forever for the GUI to post a response.
  BxEvent *response = NULL;
  while (response == NULL) {
	response = GetSyncResponse ();
	if (!response) {
	  wxLogDebug ("no sync response yet, waiting\n");
	  this->Sleep(500);
	}
  }
  wxASSERT (response != NULL);
  return response;
}

void 
BochsThread::ClearSyncResponse ()
{
  wxCriticalSectionLocker lock(bochs2gui_mailbox_lock);
  if (bochs2gui_mailbox != NULL) {
    wxLogDebug ("WARNING: ClearSyncResponse is throwing away an event that was previously in the mailbox\n");
  }
  bochs2gui_mailbox = NULL;
}

void 
BochsThread::SendSyncResponse (BxEvent *event)
{
  wxCriticalSectionLocker lock(bochs2gui_mailbox_lock);
  if (bochs2gui_mailbox != NULL) {
    wxLogDebug ("WARNING: SendSyncResponse is throwing away an event that was previously in the mailbox\n");
  }
  bochs2gui_mailbox = event;
}

BxEvent *
BochsThread::GetSyncResponse ()
{
  wxCriticalSectionLocker lock(bochs2gui_mailbox_lock);
  BxEvent *event = bochs2gui_mailbox;
  bochs2gui_mailbox = NULL;
  return event;
}
