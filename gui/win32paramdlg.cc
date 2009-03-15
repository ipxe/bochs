/////////////////////////////////////////////////////////////////////////
// $Id: win32paramdlg.cc,v 1.4 2009/03/15 16:24:54 vruppert Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2009  Volker Ruppert
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

#include "win32dialog.h"

#if BX_USE_TEXTCONFIG && defined(WIN32) && (BX_WITH_WIN32 || BX_WITH_SDL)

#include "bochs.h"
#include "textconfig.h"
#include "win32res.h"
#include "win32dialog.h"
#include "win32paramdlg.h"

#define ID_LABEL 100
#define ID_PARAM 1000
#define ID_BROWSE 2000
#define ID_UPDOWN 3000

typedef struct _dlg_list_t {
  bx_list_c *list;
  UINT dlg_list_id;
  UINT dlg_base_id;
  struct _dlg_list_t *next;
} dlg_list_t;

HFONT     DlgFont;
UINT  nextDlgID;
dlg_list_t *dlg_lists = NULL;


bx_bool registerDlgList(UINT lid, bx_list_c *list)
{
  dlg_list_t *dlg_list;
  int items;

  dlg_list = (dlg_list_t *)malloc(sizeof(dlg_list_t));
  if (dlg_list == NULL) {
    return 0;
  }

  dlg_list->list = list;
  dlg_list->dlg_list_id = lid;
  dlg_list->dlg_base_id = nextDlgID;
  items = list->get_size();
  nextDlgID += items;
  dlg_list->next = NULL;

  if (dlg_lists == NULL) {
    dlg_lists = dlg_list;
  } else {
    dlg_list_t *temp = dlg_lists;

    while (temp->next) {
      if (temp->list == dlg_list->list) {
        free(dlg_list);
        return 0;
      }
      temp = temp->next;
    }
    temp->next = dlg_list;
  }
  return dlg_list->dlg_base_id;
}

UINT findDlgListBaseID(bx_list_c *list)
{
  dlg_list_t *dlg_list;

  for (dlg_list = dlg_lists; dlg_list; dlg_list = dlg_list->next) {
    if (list == dlg_list->list) {
      return dlg_list->dlg_base_id;
    }
  }
  return 0;
}

bx_param_c *findParamFromDlgID(UINT cid)
{
  dlg_list_t *dlg_list;
  int i;

  for (dlg_list = dlg_lists; dlg_list; dlg_list = dlg_list->next) {
    if ((cid >= dlg_list->dlg_base_id) && (cid < (dlg_list->dlg_base_id + dlg_list->list->get_size()))) {
      i = cid - dlg_list->dlg_base_id;
      return dlg_list->list->get(i);
    }
    if (cid == dlg_list->dlg_list_id) {
      return dlg_list->list;
    }
  }
  return NULL;
}

UINT findDlgIDFromParam(bx_param_c *param)
{
  dlg_list_t *dlg_list;
  bx_list_c *list;
  UINT cid;
  int i;

  list = (bx_list_c*)param->get_parent();
  for (dlg_list = dlg_lists; dlg_list; dlg_list = dlg_list->next) {
    if (list == dlg_list->list) {
      cid = dlg_list->dlg_base_id;
      for (i = 0; i < list->get_size(); i++) {
        if (param == list->get(i)) {
          return (cid + i);
        }
      }
    }
  }
  return 0;
}

void cleanupDlgLists()
{
  dlg_list_t *d, *next;

  if (dlg_lists) {
    d = dlg_lists;
    while (d != NULL) {
      next = d->next;
      free(d);
      d = next;
    }
    dlg_lists = NULL;
  }
}

int AskFilename(HWND hwnd, bx_param_filename_c *param, const char *ext)
{
  OPENFILENAME ofn;
  int ret;
  DWORD errcode;
  char filename[MAX_PATH];
  const char *title;
  char errtext[80];

  param->get(filename, MAX_PATH);
  // common file dialogs don't accept raw device names
  if ((isalpha(filename[0])) && (filename[1] == ':') && (strlen(filename) == 2)) {
    filename[0] = 0;
  }
  title = param->get_label();
  if (!title) title = param->get_name();
  memset(&ofn, 0, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFile   = filename;
  ofn.nMaxFile    = MAX_PATH;
  ofn.lpstrInitialDir = bx_startup_flags.initial_dir;
  ofn.lpstrTitle = title;
  ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
  ofn.lpstrDefExt = ext;
  if (!lstrcmp(ext, "img")) {
    ofn.lpstrFilter = "Floppy image files (*.img)\0*.img\0All files (*.*)\0*.*\0";
  } else if (!lstrcmp(ext, "iso")) {
    ofn.lpstrFilter = "CD-ROM image files (*.iso)\0*.iso\0All files (*.*)\0*.*\0";
  } else if (!lstrcmp(ext, "txt")) {
    ofn.lpstrFilter = "Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
  } else {
    ofn.lpstrFilter = "All files (*.*)\0*.*\0";
  }
  if (param->get_options()->get() & bx_param_filename_c::SAVE_FILE_DIALOG) {
    ofn.Flags |= OFN_OVERWRITEPROMPT;
    ret = GetSaveFileName(&ofn);
  } else {
    ofn.Flags |= OFN_FILEMUSTEXIST;
    ret = GetOpenFileName(&ofn);
  }
  param->set(filename);
  if (ret == 0) {
    errcode = CommDlgExtendedError();
    if (errcode == 0) {
      ret = -1;
    } else {
      if (errcode == 0x3002) {
        wsprintf(errtext, "CommDlgExtendedError: invalid filename");
      } else {
        wsprintf(errtext, "CommDlgExtendedError returns 0x%04x", errcode);
      }
      MessageBox(hwnd, errtext, "Error", MB_ICONERROR);
    }
  }
  return ret;
}

void InitDlgFont(void)
{
  LOGFONT LFont;

  LFont.lfHeight         = 8;                // Default logical height of font
  LFont.lfWidth          = 0;                // Default logical average character width
  LFont.lfEscapement     = 0;                // angle of escapement
  LFont.lfOrientation    = 0;                // base-line orientation angle
  LFont.lfWeight         = FW_NORMAL;        // font weight
  LFont.lfItalic         = 0;                // italic attribute flag
  LFont.lfUnderline      = 0;                // underline attribute flag
  LFont.lfStrikeOut      = 0;                // strikeout attribute flag
  LFont.lfCharSet        = DEFAULT_CHARSET;  // character set identifier
  LFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;  // output precision
  LFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS; // clipping precision
  LFont.lfQuality        = DEFAULT_QUALITY;     // output quality
  LFont.lfPitchAndFamily = DEFAULT_PITCH;    // pitch and family
  lstrcpy(LFont.lfFaceName, "Helv");          // pointer to typeface name string
  DlgFont  = CreateFontIndirect(&LFont);
}

HWND CreateLabel(HWND hDlg, UINT cid, UINT xpos, UINT ypos, BOOL hide, const char *text)
{
  HWND Label;
  RECT r;
  int code;

  code = ID_LABEL + cid;
  r.left = xpos;
  r.top = ypos + 2;
  r.right = r.left + 78;
  r.bottom = r.top + 15;
  MapDialogRect(hDlg, &r);
  Label = CreateWindow("STATIC", text, WS_CHILD, r.left, r.top, r.right-r.left+1, r.bottom-r.top+1, hDlg, (HMENU)code, NULL, NULL);
  SendMessage(Label, WM_SETFONT, (UINT)DlgFont, TRUE);
  ShowWindow(Label, hide ? SW_HIDE : SW_SHOW);
  return Label;
}

HWND CreateGroupbox(HWND hDlg, UINT cid, UINT xpos, UINT ypos, SIZE size, BOOL hide, bx_list_c *list)
{
  HWND Groupbox;
  RECT r;
  int code;
  const char *title = NULL;

  code = ID_PARAM + cid;
  r.left = xpos;
  r.top = ypos;
  r.right = r.left + size.cx;
  r.bottom = r.top + size.cy;
  MapDialogRect(hDlg, &r);
  if (list->get_options()->get() & list->USE_BOX_TITLE) {
    title = list->get_title()->getptr();
  }
  Groupbox = CreateWindow("BUTTON", title, BS_GROUPBOX | WS_CHILD, r.left, r.top,
                          r.right-r.left+1, r.bottom-r.top+1, hDlg, (HMENU)code, NULL, NULL);
  SendMessage(Groupbox, WM_SETFONT, (UINT)DlgFont, TRUE);
  ShowWindow(Groupbox, hide ? SW_HIDE : SW_SHOW);
  return Groupbox;
}

HWND CreateTabControl(HWND hDlg, UINT cid, UINT xpos, UINT ypos, SIZE size, BOOL hide, bx_list_c *list)
{
  HWND TabControl;
  TC_ITEM tie;
  RECT r;
  int code, i;
  bx_param_c *item;
  const char *title = NULL;

  code = ID_PARAM + cid;
  r.left = xpos;
  r.top = ypos;
  r.right = r.left + size.cx;
  r.bottom = r.top + size.cy;
  MapDialogRect(hDlg, &r);
  TabControl = CreateWindow(WC_TABCONTROL, "", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                            r.left, r.top, r.right-r.left+1, r.bottom-r.top+1, hDlg, (HMENU)code, NULL, NULL); 
  for (i = 0; i < list->get_size(); i++) {
    item = list->get(i);
    if (item->get_type() == BXT_LIST) {
      tie.mask = TCIF_TEXT; 
      tie.pszText = ((bx_list_c*)item)->get_title()->getptr(); 
      TabCtrl_InsertItem(TabControl, i, &tie);
    }
  }
  SendMessage(TabControl, WM_SETFONT, (UINT)DlgFont, TRUE);
  ShowWindow(TabControl, hide ? SW_HIDE : SW_SHOW);
  return TabControl;
}


HWND CreateBrowseButton(HWND hDlg, UINT cid, UINT xpos, UINT ypos, BOOL hide)
{
  HWND Button;
  RECT r;
  int code;

  code = ID_BROWSE + cid;
  r.left = xpos + 190;
  r.top = ypos;
  r.right = r.left + 50;
  r.bottom = r.top + 14;
  MapDialogRect(hDlg, &r);
  Button = CreateWindow("BUTTON", "Browse...", WS_CHILD, r.left, r.top, r.right-r.left+1, r.bottom-r.top+1, hDlg, (HMENU)code, NULL, NULL);
  SendMessage(Button, WM_SETFONT, (UINT)DlgFont, TRUE);
  ShowWindow(Button, hide ? SW_HIDE : SW_SHOW);
  return Button;
}

HWND CreateCheckbox(HWND hDlg, UINT cid, UINT xpos, UINT ypos, BOOL hide, bx_param_bool_c *bparam)
{
  HWND Checkbox;
  RECT r;
  int code, val;

  code = ID_PARAM + cid;
  r.left = xpos + 80;
  r.top = ypos;
  r.right = r.left + 20;
  r.bottom = r.top + 14;
  MapDialogRect(hDlg, &r);
  Checkbox = CreateWindow("BUTTON", "", BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP,
                          r.left, r.top, r.right-r.left+1, r.bottom-r.top+1,
                          hDlg, (HMENU)code, NULL, NULL);
  val = bparam->get();
  SendMessage(Checkbox, BM_SETCHECK, val ? BST_CHECKED : BST_UNCHECKED, 0);
  SendMessage(Checkbox, WM_SETFONT, (UINT)DlgFont, TRUE);
  ShowWindow(Checkbox, hide ? SW_HIDE : SW_SHOW);
  return Checkbox;
}

HWND CreateInput(HWND hDlg, UINT cid, UINT xpos, UINT ypos, BOOL hide, bx_param_c *param)
{
  HWND Input, Updown;
  RECT r;
  int code, i, style;
  bx_param_num_c *nparam = NULL;
  bx_param_string_c *sparam;
  char buffer[512];
  char eachbyte[16];
  char sep_string[2];
  char *val;
  BOOL spinctrl = FALSE;

  code = ID_PARAM + cid;
  style = WS_CHILD | WS_TABSTOP;
  if (param->get_type() == BXT_PARAM_STRING) {
    sparam = (bx_param_string_c*)param;
    val = sparam->getptr();
    if (sparam->get_options()->get() & sparam->RAW_BYTES) {
      buffer[0] = 0;
      sep_string[0] = sparam->get_separator();
      sep_string[1] = 0;
      for (i = 0; i < sparam->get_maxsize(); i++) {
        wsprintf(eachbyte, "%s%02x", (i>0)?sep_string : "", (Bit8u)0xff&val[i]);
        strncat(buffer, eachbyte, sizeof(buffer));
      }
    } else {
      lstrcpyn(buffer, val, 512);
      style |= ES_AUTOHSCROLL;
    }
  } else {
    nparam = (bx_param_num_c*)param;
    if (nparam->get_base() == BASE_HEX) {
      wsprintf(buffer, "0x%x", nparam->get());
    } else {
      wsprintf(buffer, "%d", nparam->get());
      style |= ES_NUMBER;
    }
    if (nparam->get_options() & nparam->USE_SPIN_CONTROL) {
      spinctrl = TRUE;
    }
  }
  r.left = xpos + 80;
  r.top = ypos;
  r.right = r.left + 100;
  r.bottom = r.top + 14;
  MapDialogRect(hDlg, &r);
  Input = CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_NOPARENTNOTIFY, "EDIT", buffer,
                         style, r.left, r.top, r.right-r.left+1,
                         r.bottom-r.top+1, hDlg, (HMENU)code, NULL, NULL);
  if (spinctrl) {
    style = WS_CHILD | WS_BORDER | WS_VISIBLE | UDS_NOTHOUSANDS | UDS_ARROWKEYS |
            UDS_ALIGNRIGHT | UDS_SETBUDDYINT;
    Updown = CreateUpDownControl(style, 0, 0, 0, 0, hDlg, ID_UPDOWN + cid, NULL, Input,
                                 (int)nparam->get_max(), (int)nparam->get_min(), (int)nparam->get());
    ShowWindow(Updown, hide ? SW_HIDE : SW_SHOW);
  }
  SendMessage(Input, WM_SETFONT, (UINT)DlgFont, TRUE);
  ShowWindow(Input, hide ? SW_HIDE : SW_SHOW);
  return Input;
}

HWND CreateCombobox(HWND hDlg, UINT cid, UINT xpos, UINT ypos, BOOL hide, bx_param_enum_c *eparam)
{
  HWND Combo;
  RECT r;
  int code, j;
  const char *choice;

  code = ID_PARAM + cid;
  r.left = xpos + 80;
  r.top = ypos;
  r.right = r.left + 100;
  r.bottom = r.top + 14;
  MapDialogRect(hDlg, &r);
  Combo = CreateWindow("COMBOBOX", "", WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                       r.left, r.top, r.right-r.left+1, r.bottom-r.top+1, hDlg, (HMENU)code, NULL, NULL);
  j = 0;
  do {
    choice = eparam->get_choice(j);
    SendMessage(Combo, CB_ADDSTRING, 0, (LPARAM)choice);
    j++;
  } while (choice != NULL);
  SendMessage(Combo, CB_SETCURSEL, (WPARAM)(eparam->get()-eparam->get_min()), 0);
  SendMessage(Combo, WM_SETFONT, (UINT)DlgFont, TRUE);
  ShowWindow(Combo, hide ? SW_HIDE : SW_SHOW);
  return Combo;
}

SIZE CreateParamList(HWND hDlg, UINT lid, UINT xpos, UINT ypos, BOOL hide, bx_list_c *list)
{
  HWND ltext, control, browse;
  SIZE size, lsize;
  bx_param_c *param;
  bx_param_string_c *sparam;
  const char *label;
  char buffer[512];
  int options;
  UINT cid, i, items, x, y;
  BOOL ihide;

  items = list->get_size();
  options = list->get_options()->get();
  cid = registerDlgList(lid, list);
  x = xpos + 5;
  size.cx = 195;
  if (options & list->USE_TAB_WINDOW) {
    y = ypos + 15;
    size.cy = 18;
  } else {
    y = ypos + 10;
    size.cy = 13;
  }
  for (i = 0; i < items; i++) {
    param = list->get(i);
    if (!SIM->get_init_done() || (param->get_enabled() && param->get_runtime_param())) {
      ltext = NULL;
      browse = NULL;
      ihide = hide || ((i != 0) && (options & list->USE_TAB_WINDOW));
      if (param->get_type() == BXT_LIST) {
        lsize = CreateParamList(hDlg, cid, x + 4, y + 1, ihide, (bx_list_c*)param);
        if ((lsize.cx + 18) > size.cx) {
          size.cx = lsize.cx + 18;
        }
        if (!(options & list->USE_TAB_WINDOW)) {
          y += (lsize.cy + 6);
          size.cy += (lsize.cy + 6);
        } else {
          if ((lsize.cy + 24) > size.cy) {
            size.cy = lsize.cy + 24;
          }
        }
      } else {
        label = param->get_label();
        if (label == NULL) {
          label = param->get_name();
        }
        if ((options & list->SHOW_GROUP_NAME) && (param->get_group() != NULL)) {
          wsprintf(buffer, "%s %s", param->get_group(), label);
        } else {
          lstrcpyn(buffer, label, 512);
        }
        ltext = CreateLabel(hDlg, cid, x, y, hide, buffer);
        if (param->get_type() == BXT_PARAM_BOOL) {
          control = CreateCheckbox(hDlg, cid, x, y, hide, (bx_param_bool_c*)param);
        } else if (param->get_type() == BXT_PARAM_ENUM) {
          control = CreateCombobox(hDlg, cid, x, y, hide, (bx_param_enum_c*)param);
        } else if (param->get_type() == BXT_PARAM_NUM) {
          control = CreateInput(hDlg, cid, x, y, hide, param);
        } else if (param->get_type() == BXT_PARAM_STRING) {
          control = CreateInput(hDlg, cid, x, y, hide, param);
          sparam = (bx_param_string_c*)param;
          if (sparam->get_options()->get() & sparam->IS_FILENAME) {
            browse = CreateBrowseButton(hDlg, cid, x, y, hide);
            if (size.cx < 255) size.cx = 255;
          }
        }
        y += 20;
        size.cy += 20;
      }
      if (!param->get_enabled()) {
        EnableWindow(ltext, FALSE);
        EnableWindow(control, FALSE);
        if (browse) EnableWindow(browse, FALSE);
      }
    }
    cid++;
  }
  if (options & list->USE_TAB_WINDOW) {
    CreateTabControl(hDlg, lid, xpos, ypos, size, hide, list);
  } else {
    CreateGroupbox(hDlg, lid, xpos, ypos, size, hide, list);
  }
  return size;
}

void SetParamList(HWND hDlg, bx_list_c *list)
{
  bx_param_c *param;
  bx_param_num_c *nparam;
  bx_param_enum_c *eparam;
  bx_param_string_c *sparam;
  int j, val;
  const char *src;
  char buffer[512], rawbuf[512];
  UINT cid, i, items, lid;

  lid = findDlgListBaseID(list);
  items = list->get_size();
  for (i = 0; i < items; i++) {
    cid = lid + i;
    param = list->get(i);
    if (param->get_type() == BXT_LIST) {
      SetParamList(hDlg, (bx_list_c*)param);
    } else if (param->get_type() == BXT_PARAM_BOOL) {
      val = SendMessage(GetDlgItem(hDlg, ID_PARAM + cid), BM_GETCHECK, 0, 0);
      ((bx_param_bool_c*)param)->set(val == BST_CHECKED);
    } else if (param->get_type() == BXT_PARAM_ENUM) {
      val = SendMessage(GetDlgItem(hDlg, ID_PARAM + cid), CB_GETCURSEL, 0, 0);
      eparam = (bx_param_enum_c*)param;
      eparam->set(val + eparam->get_min());
    } else {
      if (SendMessage(GetDlgItem(hDlg, ID_PARAM + cid), EM_GETMODIFY, 0, 0)) {
        if (param->get_type() == BXT_PARAM_NUM) {
          nparam = (bx_param_num_c*)param;
          if (nparam->get_base() == BASE_HEX) {
            GetWindowText(GetDlgItem(hDlg, ID_PARAM + cid), buffer, 511);
            sscanf(buffer, "%x", &val);
          } else {
            val = GetDlgItemInt(hDlg, ID_PARAM + cid, NULL, FALSE);
          }
          nparam->set(val);
        } else if (param->get_type() == BXT_PARAM_STRING) {
          GetWindowText(GetDlgItem(hDlg, ID_PARAM + cid), buffer, 511);
          sparam = (bx_param_string_c*)param;
          if (sparam->get_options()->get() & sparam->RAW_BYTES) {
            src = &buffer[0];
            memset(rawbuf, 0, sparam->get_maxsize());
            for (j = 0; j < sparam->get_maxsize(); j++) {
              while (*src == sparam->get_separator())
                src++;
              if (*src == 0) break;
              if (sscanf(src, "%02x", &val)) {
                rawbuf[j] = val;
                src += 2;
              } else {
                break;
              }
            }
            sparam->set(rawbuf);
          } else {
            sparam->set(buffer);
          }
        }
      }
    }
  }
}

void ShowParamList(HWND hDlg, UINT lid, BOOL show, bx_list_c *list)
{
  UINT cid;
  int i;
  HWND Button, Updown;

  ShowWindow(GetDlgItem(hDlg, ID_PARAM + lid), show ? SW_SHOW : SW_HIDE);
  cid = findDlgListBaseID(list);
  for (i = 0; i < list->get_size(); i++) {
    if (list->get(i)->get_type() == BXT_LIST) {
      ShowParamList(hDlg, cid + i, show, (bx_list_c*)list->get(i));
    } else {
      ShowWindow(GetDlgItem(hDlg, ID_LABEL + cid + i), show ? SW_SHOW : SW_HIDE);
      ShowWindow(GetDlgItem(hDlg, ID_PARAM + cid + i), show ? SW_SHOW : SW_HIDE);
      Button = GetDlgItem(hDlg, ID_BROWSE + cid + i);
      if (Button != NULL) {
        ShowWindow(Button, show ? SW_SHOW : SW_HIDE);
      }
      Updown = GetDlgItem(hDlg, ID_UPDOWN + cid + i);
      if (Updown != NULL) {
        ShowWindow(Updown, show ? SW_SHOW : SW_HIDE);
      }
    }
  }
}

void EnableParam(HWND hDlg, bx_param_c *param, BOOL val)
{
  UINT cid;
  int i;
  bx_list_c *plist, *clist;
  HWND Button, Updown;
  Bit64u *n_disable = NULL;

  plist = (bx_list_c*)param->get_parent();
  cid = findDlgIDFromParam(param);
  if (param->get_type() == BXT_LIST) {
    clist = (bx_list_c*)param;
    for (i = 0; i < clist->get_size(); i++) {
      EnableParam(hDlg, clist->get(i), val);
    }
  } else {
    EnableWindow(GetDlgItem(hDlg, ID_LABEL + cid), val);
    EnableWindow(GetDlgItem(hDlg, ID_PARAM + cid), val);
    Button = GetDlgItem(hDlg, ID_BROWSE + cid);
    if (Button != NULL) {
      EnableWindow(Button, val);
    }
    Updown = GetDlgItem(hDlg, ID_UPDOWN + cid);
    if (Updown != NULL) {
      EnableWindow(Updown, val);
    }
  }
}

static BOOL CALLBACK ParamDlgProc(HWND Window, UINT AMessage, WPARAM wParam, LPARAM lParam)
{
  static bx_list_c *list = NULL;
  static int items = 0;
  bx_param_c *param, *dparam;
  bx_param_string_c *sparam;
  bx_list_c *deplist, *tmplist;
  int cid, code, i, j, k, val;
  RECT r, r2;
  SIZE size;
  NMHDR tcinfo;

  switch (AMessage) {
    case WM_CLOSE:
      cleanupDlgLists();
      EndDialog(Window, 0);
      break;
    case WM_INITDIALOG:
      list = (bx_list_c*)SIM->get_param((const char*)lParam);
      items = list->get_size();
      SetWindowText(Window, list->get_title()->getptr());
      nextDlgID = 1;
      size = CreateParamList(Window, 0, 6, 6, FALSE, list);
      r.left = size.cx / 2 - 50;
      r.top = size.cy + 12;
      r.right = r.left + 50;
      r.bottom = r.top + 14;
      MapDialogRect(Window, &r);
      MoveWindow(GetDlgItem(Window, IDOK), r.left, r.top, r.right-r.left+1, r.bottom-r.top+1, FALSE);
      r.left = size.cx / 2 + 10;
      r.top = size.cy + 12;
      r.right = r.left + 50;
      r.bottom = r.top + 14;
      MapDialogRect(Window, &r);
      MoveWindow(GetDlgItem(Window, IDCANCEL), r.left, r.top, r.right-r.left+1, r.bottom-r.top+1, FALSE);
      GetWindowRect(Window, &r2);
      r.left = 0;
      r.top = 0;
      r.right = size.cx + 18;
      r.bottom = size.cy + 52;
      MapDialogRect(Window, &r);
      MoveWindow(Window, r2.left, r2.top, r.right, r.bottom, TRUE);
      return TRUE;
    case WM_COMMAND:
      code = LOWORD(wParam);
      switch (code) {
        case IDCANCEL:
          cleanupDlgLists();
          EndDialog(Window, 0);
          break;
        case IDOK:
          SetParamList(Window, list);
          cleanupDlgLists();
          EndDialog(Window, 1);
          break;
        default:
          if ((code >= ID_BROWSE) && (code < (int)(ID_BROWSE + nextDlgID))) {
            i = code - ID_BROWSE;
            sparam = (bx_param_string_c *)findParamFromDlgID(i);
            if (sparam != NULL) {
              if (AskFilename(Window, (bx_param_filename_c *)sparam, "txt") > 0) {
                SetWindowText(GetDlgItem(Window, ID_PARAM + i), sparam->getptr());
                SetFocus(GetDlgItem(Window, ID_PARAM + i));
              }
            }
          } else if ((code >= ID_PARAM) && (code < (int)(ID_PARAM + nextDlgID))) {
            i = code - ID_PARAM;
            param = findParamFromDlgID(i);
            if (param != NULL) {
              if ((param->get_type() == BXT_PARAM_BOOL) ||
                  (param->get_type() == BXT_PARAM_NUM)) {
                deplist = ((bx_param_bool_c *)param)->get_dependent_list();
                if (deplist != NULL) {
                  if (param->get_type() == BXT_PARAM_BOOL) {
                    val = SendMessage(GetDlgItem(Window, code), BM_GETCHECK, 0, 0);
                  } else {
                    val = GetDlgItemInt(Window, code, NULL, FALSE);
                  }
                  for (j = 0; j < deplist->get_size(); j++) {
                    dparam = deplist->get(j);
                    if (dparam != param) {
                      EnableParam(Window, dparam, val);
                    }
                  }
                }
              }
            }
          }
      }
      break;
    case WM_NOTIFY:
      tcinfo = *(LPNMHDR)lParam;
      if (tcinfo.code == (UINT)TCN_SELCHANGE) {
        code = tcinfo.idFrom;
        j = code - ID_PARAM;
        tmplist = (bx_list_c *)findParamFromDlgID(j);
        if (tmplist != NULL) {
          k = TabCtrl_GetCurSel(GetDlgItem(Window, code));
          cid = findDlgListBaseID(tmplist);
          for (i = 0; i < tmplist->get_size(); i++) {
            ShowParamList(Window, cid + i, (i == k), (bx_list_c*)tmplist->get(i));
          }
        }
      }
      break;
    case WM_CTLCOLOREDIT:
      SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
      return (long)GetSysColorBrush(COLOR_WINDOW);
      break;
  }
  return 0;
}

int win32ParamDialog(HWND parent, const char *menu)
{
  int ret;

  InitDlgFont();
  ret = DialogBoxParam(NULL, MAKEINTRESOURCE(PARAM_DLG), parent, (DLGPROC)ParamDlgProc, (LPARAM)menu);
  DeleteObject(DlgFont);
  return ret;
}

#endif // BX_USE_TEXTCONFIG && defined(WIN32)
