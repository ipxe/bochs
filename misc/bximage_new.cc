/////////////////////////////////////////////////////////////////////////
// $Id$
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2013       Volker Ruppert
//  Copyright (C) 2001-2013  The Bochs Project
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
/////////////////////////////////////////////////////////////////////////

// TODO: Create empty hard disk or floppy disk images for bochs.
// Convert a hard disk image from one format (mode) to another.
// Commit a redolog file to a supported base image.

#ifdef WIN32
#  include <conio.h>
#endif
#include <ctype.h>

#include "config.h"
#include "bxcompat.h"
#include "osdep.h"
#include "bswap.h"

#define BXIMAGE
#include "iodev/hdimage/hdimage.h"
#include "iodev/hdimage/vmware3.h"
#include "iodev/hdimage/vmware4.h"
#include "iodev/hdimage/vpc-img.h"

#define BXIMAGE_MODE_NULL            0
#define BXIMAGE_MODE_CREATE_IMAGE    1
#define BXIMAGE_MODE_CONVERT_IMAGE   2
#define BXIMAGE_MODE_COMMIT_UNDOABLE 3

int  bximage_mode;
int  bx_hdimagemode;
int  bx_remove;
int  bx_interactive;
char bx_filename_1[512];
char bx_filename_2[512];

const char *EOF_ERR = "ERROR: End of input";
const char *svnid = "$Id$";
const char *divider = "========================================================================";
const char *main_menu_prompt =
"\n"
"1. Create new floppy or hard disk image\n"
"2. Convert hard disk image to other format (mode)\n"
"3. Commit 'undoable' redolog to base image\n"
"\n"
"0. Quit\n"
"\n"
"Please choose one ";

// menu data for choosing disk mode
const char *hdmode_menu = "\nWhat kind of image should I create?\nPlease type flat, sparse or growing. ";
const char *hdmode_choices[] = {"flat", "sparse", "growing" };
const int hdmode_choice_id[] = {BX_HDIMAGE_MODE_FLAT, BX_HDIMAGE_MODE_SPARSE, BX_HDIMAGE_MODE_GROWING};
int hdmode_n_choices = 3;

#if !BX_HAVE_SNPRINTF
#include <stdarg.h>
/* XXX use real snprintf */
/* if they don't have snprintf, just use sprintf */
int snprintf(char *s, size_t maxlen, const char *format, ...)
{
  va_list arg;
  int done;

  va_start(arg, format);
  done = vsprintf(s, format, arg);
  va_end(arg);

  return done;
}
#endif  /* !BX_HAVE_SNPRINTF */

#if !BX_HAVE_MKSTEMP
int bx_mkstemp(char *tpl)
{
  mktemp(tpl);
  return ::open(tpl, O_RDWR | O_CREAT | O_TRUNC
#ifdef O_BINARY
                | O_BINARY
#endif
                , S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP);
}
#endif // !BX_HAVE_MKSTEMP

void myexit(int code)
{
#ifdef WIN32
  printf("\nPress any key to continue\n");
  getch();
#endif
  exit(code);
}

// stolen from main.cc
void bx_center_print(FILE *file, const char *line, int maxwidth)
{
  int imax;
  int i;
  imax = (maxwidth - strlen(line)) >> 1;
  for (i=0; i<imax; i++) fputc(' ', file);
  fputs(line, file);
}

void print_banner()
{
  printf("%s\n", divider);
  bx_center_print(stdout, "bximage_new\n", 72);
  bx_center_print(stdout, "Disk Image Creation / Conversion and Commit Tool for Bochs\n", 72);
  bx_center_print(stdout, svnid, 72);
  printf("\n%s\n", divider);
}

// this is how we crash
void fatal(const char *c)
{
  printf("%s\n", c);
  myexit(1);
}

/* check if the argument string is present in the list -
   returns index on success, -1 on failure. */
int get_menu_index(char *arg, int n_choices, const char *choice[])
{
  int i;
  for (i=0; i<n_choices; i++) {
    if (!strcmp(choice[i], arg)) {
      // matched, return the choice number
      return i;
    }
  }
  return -1;
}

// remove leading spaces, newline junk at end.  returns pointer to
// cleaned string, which is between s0 and the null
char *clean_string(char *s0)
{
  char *s = s0;
  char *ptr;
  // find first nonblank
  while (isspace(*s))
    s++;
  // truncate string at first non-alphanumeric
  ptr = s;
  while (isprint(*ptr))
    ptr++;
  *ptr = 0;
  return s;
}

// returns 0 on success, -1 on failure.  The value goes into out.
int ask_int(const char *prompt, int min, int max, int the_default, int *out)
{
  int n = max + 1;
  char buffer[1024];
  char *clean;
  int illegal;
  while (1) {
    printf("%s", prompt);
    printf("[%d] ", the_default);
    if (!fgets(buffer, sizeof(buffer), stdin))
      return -1;
    clean = clean_string(buffer);
    if (strlen(clean) < 1) {
      // empty line, use the default
      *out = the_default;
      return 0;
    }
    illegal = (1 != sscanf(buffer, "%d", &n));
    if (illegal || n<min || n>max) {
      fprintf(stderr, "Your choice (%s) was not an integer between %d and %d.\n\n",
             clean, min, max);
    } else {
      // choice is okay
      *out = n;
      return 0;
    }
  }
}

int ask_menu(const char *prompt, int n_choices, const char *choice[], int the_default, int *out)
{
  char buffer[1024];
  char *clean;
  int i;
  *out = -1;
  while(1) {
    printf("%s", prompt);
    printf("[%s] ", choice[the_default]);
    if (!fgets(buffer, sizeof(buffer), stdin))
      return -1;
    clean = clean_string(buffer);
    if (strlen(clean) < 1) {
      // empty line, use the default
      *out = the_default;
      return 0;
    }
    for (i=0; i<n_choices; i++) {
      if (!strcmp(choice[i], clean)) {
        // matched, return the choice number
        *out = i;
        return 0;
      }
    }
    printf("Your choice (%s) did not match any of the choices:\n", clean);
    for (i=0; i<n_choices; i++) {
      if (i>0) printf(", ");
      printf("%s", choice[i]);
    }
    printf("\n");
  }
}

int ask_yn(const char *prompt, int the_default, int *out)
{
  char buffer[16];
  char *clean;
  *out = -1;
  while (1) {
    printf("%s", prompt);
    printf("[%s] ", the_default?"yes":"no");
    if (!fgets(buffer, sizeof(buffer), stdin))
      return -1;
    clean = clean_string(buffer);
    if (strlen(clean) < 1) {
      // empty line, use the default
      *out = the_default;
      return 0;
    }
    switch (tolower(clean[0])) {
      case 'y': *out=1; return 0;
      case 'n': *out=0; return 0;
    }
    fprintf(stderr, "Please type either yes or no.\n");
  }
}

int ask_string(const char *prompt, char *the_default, char *out)
{
  char buffer[1024];
  char *clean;
  printf("%s", prompt);
  printf("[%s] ", the_default);
  if (!fgets(buffer, sizeof(buffer), stdin))
    return -1;
  clean = clean_string(buffer);
  if (strlen(clean) < 1) {
    // empty line, use the default
    strcpy(out, the_default);
    return 0;
  }
  strcpy(out, clean);
  return 0;
}

device_image_t* init_image(Bit8u image_mode)
{
  device_image_t *hdimage = NULL;

  // instantiate the right class
  switch (image_mode) {

    case BX_HDIMAGE_MODE_FLAT:
      hdimage = new default_image_t();
      break;

    case BX_HDIMAGE_MODE_SPARSE:
      hdimage = new sparse_image_t();
      break;

    case BX_HDIMAGE_MODE_VMWARE3:
      hdimage = new vmware3_image_t();
      break;

    case BX_HDIMAGE_MODE_VMWARE4:
      hdimage = new vmware4_image_t();
      break;

    case BX_HDIMAGE_MODE_GROWING:
      hdimage = new growing_image_t();
      break;

    case BX_HDIMAGE_MODE_VPC:
      hdimage = new vpc_image_t();
      break;

    default:
      fatal("unsupported disk image mode");
      break;
  }
  return hdimage;
}

int create_image_file(const char *filename)
{
  int fd = open(filename, O_WRONLY | O_CREAT
#ifdef O_BINARY
                | O_BINARY
#endif
                , S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP
                );
  if (fd < 0) {
    fatal("ERROR: flat file is not writable");
  }
  return fd;
}

int create_flat_image(const char *filename, Bit64u size)
{
  char buffer[512];

  int fd = create_image_file(filename);
  memset(buffer, 0, 512);
  if (bx_write_image(fd, size, buffer, 512) != 512)
    fatal("ERROR: while writing block in flat file !");
  close(fd);

  return 0;
}

int create_sparse_image(const char *filename, Bit64u size)
{
  Bit64u numpages;
  sparse_header_t header;
  size_t sizesofar;
  int padtopagesize;

  memset(&header, 0, sizeof(header));
  header.magic = htod32(SPARSE_HEADER_MAGIC);
  header.version = htod32(SPARSE_HEADER_VERSION);

  header.pagesize = htod32((1 << 10) * 32); // Use 32 KB Pages - could be configurable
  numpages = (size / dtoh32(header.pagesize)) + 1;

  header.numpages = htod32((Bit32u)numpages);
  header.disk = htod64(size);

  if (numpages != dtoh32(header.numpages)) {
    fatal("ERROR: The disk image is too large for a sparse image!");
    // Could increase page size here.
    // But note this only happens at 128 Terabytes!
  }

  int fd = create_image_file(filename);
  if (bx_write_image(fd, 0, &header, sizeof(header)) != sizeof(header)) {
    close(fd);
    fatal("ERROR: The disk image is not complete - could not write header!");
  }

  Bit32u *pagetable = new Bit32u[dtoh32(header.numpages)];
  if (pagetable == NULL)
    fatal("ERROR: The disk image is not complete - could not create pagetable!");
  for (Bit32u i=0; i<dtoh32(header.numpages); i++)
    pagetable[i] = htod32(SPARSE_PAGE_NOT_ALLOCATED);

  if (bx_write_image(fd, sizeof(header), pagetable, 4 * dtoh32(header.numpages)) != (int)(4 * dtoh32(header.numpages))) {
    close(fd);
    fatal("ERROR: The disk image is not complete - could not write pagetable!");
  }
  delete [] pagetable;

  sizesofar = SPARSE_HEADER_SIZE + (4 * dtoh32(header.numpages));
  padtopagesize = dtoh32(header.pagesize) - (sizesofar & (dtoh32(header.pagesize) - 1));

  Bit8u *padding = new Bit8u[padtopagesize];
  memset(padding, 0, padtopagesize);
  if (padding == NULL)
    fatal("ERROR: The disk image is not complete - could not create padding!");
  if (bx_write_image(fd, sizesofar, padding, padtopagesize) != padtopagesize) {
    close(fd);
    fatal("ERROR: The disk image is not complete - could not write padding!");
  }
  delete [] padding;
  close(fd);

  return 0;
}

int create_growing_image(const char *filename, Bit64u size)
{
  redolog_t *redolog = new redolog_t;
  if (redolog->create(filename, REDOLOG_SUBTYPE_GROWING, size) < 0)
    fatal("Can't create growing mode image");
  redolog->close();
  delete redolog;

  return 0;
}

int convert_image(int newimgmode)
{
  device_image_t *source_image, *dest_image;
  Bit64u i, sc, s;
  char buffer[512], null_sector[512];
  bx_bool error = 0;

  printf("\n");
  memset(null_sector, 0, 512);
  int mode = hdimage_detect_image_mode(bx_filename_1);
  if (mode == BX_HDIMAGE_MODE_UNKNOWN)
    fatal("base disk image mode not detected");

  source_image = init_image(mode);
  if (source_image->open(bx_filename_1, O_RDONLY) < 0)
    fatal("cannot open source disk image");

  int fd = hdimage_open_file(bx_filename_2, O_RDONLY, NULL, NULL);
  if (fd >= 0) {
    close(fd);
    int confirm;
    sprintf(buffer, "\nThe disk image '%s' already exists.  Are you sure you want to replace it?\nPlease type yes or no. ", bx_filename_2);
    if (ask_yn(buffer, 0, &confirm) < 0)
      fatal(EOF_ERR);
    if (!confirm)
      fatal("ERROR: Aborted");
  }

  switch (newimgmode) {
    case BX_HDIMAGE_MODE_FLAT:
      create_flat_image(bx_filename_2, source_image->hd_size);
      break;

    case BX_HDIMAGE_MODE_GROWING:
      create_growing_image(bx_filename_2, source_image->hd_size);
      break;

    case BX_HDIMAGE_MODE_SPARSE:
      create_sparse_image(bx_filename_2, source_image->hd_size);
      break;

    default:
      fatal("image mode not implemented yet");
  }

  dest_image = init_image(newimgmode);
  if (dest_image->open(bx_filename_2) < 0)
    fatal("cannot open destination disk image");

  printf("\nConverting image file: [  0%%]");

  sc = source_image->hd_size / 512;
  s = 0;
  for (i = 0; i < source_image->hd_size; i+=512) {
    printf("\x8\x8\x8\x8\x8%3d%%]", (int)((s+1)*100/sc));
    fflush(stdout);
    if (source_image->lseek(i, SEEK_SET) < 0) {
      error = 1;
      break;
    }
    if ((source_image->read(buffer, 512) == 512) &&
        (memcmp(buffer, null_sector, 512) != 0)) {
      if (dest_image->lseek(i, SEEK_SET) < 0) {
        error = 1;
        break;
      }
      if (dest_image->write(buffer, 512) < 0) {
        error = 1;
        break;
      }
    }
    s++;
  }

  source_image->close();
  dest_image->close();
  delete dest_image;
  delete source_image;

  if (error) {
    fatal("image conversion failed");
  } else {
    printf(" Done.\n");
  }

  return 0;
}

int commit_redolog()
{
  device_image_t *base_image;
  redolog_t *redolog;
  Bit64u i, sc, s;
  Bit8u buffer[512];
  bx_bool error = 0;

  printf("\n");
  int mode = hdimage_detect_image_mode(bx_filename_1);
  if (mode == BX_HDIMAGE_MODE_UNKNOWN)
    fatal("base disk image mode not detected");

  base_image = init_image(mode);
  if (base_image->open(bx_filename_1) < 0)
    fatal("cannot open base disk image");

  redolog = new redolog_t;
  if (redolog->open(bx_filename_2, REDOLOG_SUBTYPE_UNDOABLE) < 0)
    fatal("Can't open redolog");
  if (!coherency_check(base_image, redolog))
    fatal("coherency check failed");

  printf("\nCommitting changes to base image file: [  0%%]");

  sc = base_image->hd_size / 512;
  s = 0;
  for (i = 0; i < base_image->hd_size; i+=512) {
    printf("\x8\x8\x8\x8\x8%3d%%]", (int)((s+1)*100/sc));
    fflush(stdout);
    if (redolog->lseek(i, SEEK_SET) < 0) {
      error = 1;
      break;
    }
    if (redolog->read(buffer, 512) == 512) {
      if (base_image->lseek(i, SEEK_SET) < 0) {
        error = 1;
        break;
      }
      if (base_image->write(buffer, 512) < 0) {
        error = 1;
        break;
      }
    }
    s++;
  }

  base_image->close();
  redolog->close();
  delete base_image;
  delete redolog;

  if (error) {
    fatal("redolog commit failed");
  } else {
    printf(" Done.\n");
  }

  return 0;
}

void print_usage()
{
  fprintf(stderr,
    "Usage: bximage_new [options] [filename1] [filename2]\n\n"
    "Supported options:\n"
    "  -mode=create  create new floppy or hard disk image\n"
    "  -mode=convert convert hard disk image to other format (mode)\n"
    "  -mode=commit  commit undoable redolog to base image\n"
    "  -imgmode=...  create/convert: hard disk image mode\n"
    "  -d            convert/commit: delete source file after operation\n"
    "  -q            quiet mode (don't prompt for user input)\n"
    "  --help        display this help and exit\n\n"
    "Other arguments:\n"
    "  filename1     create:  new image file\n"
    "                convert: source image file\n"
    "                commit:  base image file\n"
    "  filename2     convert: destination image file\n"
    "                commit:  redolog (journal) file\n\n");
}

int parse_cmdline(int argc, char *argv[])
{
  int arg = 1;
  int ret = 1;
  int fnargs = 0;

  bximage_mode = BXIMAGE_MODE_NULL;
  bx_hdimagemode = 0;
  bx_remove = 0;
  bx_interactive = 1;
  bx_filename_1[0] = 0;
  bx_filename_2[0] = 0;
  while ((arg < argc) && (ret == 1)) {
    // parse next arg
    if (!strcmp("--help", argv[arg]) || !strncmp("/?", argv[arg], 2)) {
      print_usage();
      ret = 0;
    }
    else if (!strncmp("-mode=", argv[arg], 6)) {
      if (!strcmp(&argv[arg][6], "create")) {
        bximage_mode = BXIMAGE_MODE_CREATE_IMAGE;
      } else if (!strcmp(&argv[arg][6], "convert")) {
        bximage_mode = BXIMAGE_MODE_CONVERT_IMAGE;
      } else if (!strcmp(&argv[arg][6], "commit")) {
        bximage_mode = BXIMAGE_MODE_COMMIT_UNDOABLE;
      } else {
        printf("Unknown bximage mode '%s'\n\n", &argv[arg][6]);
      }
    }
    else if (!strncmp("-imgmode=", argv[arg], 9)) {
      bx_hdimagemode = get_menu_index(&argv[arg][9], hdmode_n_choices, hdmode_choices);
      if (bx_hdimagemode < 0) {
        printf("Unknown image mode: %s\n\n", &argv[arg][9]);
        ret = 0;
      }
    }
    else if (!strcmp("-d", argv[arg])) {
      bx_remove = 1;
    }
    else if (!strcmp("-q", argv[arg])) {
      bx_interactive = 0;
    }
    else if (argv[arg][0] == '-') {
      printf("Unknown option: %s\n\n", argv[arg]);
      ret = 0;
    } else {
      if (fnargs == 0) {
        strcpy(bx_filename_1, argv[arg]);
      } else if (fnargs == 1) {
        strcpy(bx_filename_2, argv[arg]);
      } else {
        printf("Ignoring extra parameter: %s\n\n", argv[arg]);
      }
      fnargs++;
    }
    arg++;
  }
  if (bximage_mode == BXIMAGE_MODE_NULL) {
    bx_interactive = 1;
  } else if ((bximage_mode != BXIMAGE_MODE_CREATE_IMAGE) && (fnargs < 2)) {
    bx_interactive = 1;
  }
  return ret;
}

int main(int argc, char *argv[])
{
  char tmplogname[512];
  int imgmode = 0;

  if (!parse_cmdline(argc, argv))
    myexit(1);

  print_banner();

  if (bx_interactive) {
    if (ask_int(main_menu_prompt, 0, 3, bximage_mode, &bximage_mode) < 0)
      fatal(EOF_ERR);

    switch (bximage_mode) {
      case BXIMAGE_MODE_NULL:
        myexit(0);
        break;

      case BXIMAGE_MODE_CONVERT_IMAGE:
        if (!strlen(bx_filename_1)) {
          strcpy(bx_filename_1, "c.img");
        }
        if (ask_string("\nWhat is the name of the source image?\n", bx_filename_1, bx_filename_1) < 0)
          fatal(EOF_ERR);
        if (!strlen(bx_filename_2)) {
          strcpy(tmplogname, "c-new.img");
        } else {
          strcpy(tmplogname, bx_filename_2);
        }
        if (ask_string("\nWhat should be the name of the new image?\n", tmplogname, bx_filename_2) < 0)
          fatal(EOF_ERR);
        if (ask_menu(hdmode_menu, hdmode_n_choices, hdmode_choices, bx_hdimagemode, &bx_hdimagemode) < 0)
          fatal(EOF_ERR);
        imgmode = hdmode_choice_id[bx_hdimagemode];
        if (ask_yn("\nShould the source been removed afterwards?\n", 0, &bx_remove) < 0)
          fatal(EOF_ERR);
        break;

      case BXIMAGE_MODE_COMMIT_UNDOABLE:
        if (!strlen(bx_filename_1)) {
          strcpy(bx_filename_1, "c.img");
        }
        if (ask_string("\nWhat is the name of the base image?\n", bx_filename_1, bx_filename_1) < 0)
          fatal(EOF_ERR);
        if (!strlen(bx_filename_2)) {
          snprintf(tmplogname, 256, "%s%s", bx_filename_1, UNDOABLE_REDOLOG_EXTENSION);
        } else {
          strcpy(tmplogname, bx_filename_2);
        }
        if (ask_string("\nWhat is the redolog name?\n", tmplogname, bx_filename_2) < 0)
          fatal(EOF_ERR);
        if (ask_yn("\nShould the redolog been removed afterwards?\n", 1, &bx_remove) < 0)
          fatal(EOF_ERR);
        break;

      default:
        fatal("\nbximage_new: mode not implemented yet");
    }
  }
  if (bximage_mode == BXIMAGE_MODE_CREATE_IMAGE) {
    // TODO
  } else if (bximage_mode == BXIMAGE_MODE_CONVERT_IMAGE) {
    convert_image(imgmode);
    if (bx_remove) {
      if (unlink(bx_filename_1) != 0)
        fatal("ERROR: while removing the source image !\n");
    }
  } else if (bximage_mode == BXIMAGE_MODE_COMMIT_UNDOABLE) {
    commit_redolog();
    if (bx_remove) {
      if (unlink(bx_filename_2) != 0)
        fatal("ERROR: while removing the redolog !\n");
    }
  }

  return 0;
}
