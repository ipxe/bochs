// CDROM interface for MacOSX 10+. Uses IOKit facilities
// inplace of the ioctl driver interface used by the rest
// of the Unixes.
// 
// Features:
//
// To configure and build with this cdrom interface, configure
// with: 
// 
//  ./configure --enable-cdrom --with-osxcdrom
//
// On MacOSX the CD device special file -/dev/cdrom,/dev/disk3,etc..., is
// determined dynamically by the kernel so the user should not,
// theoretically, need to set cdromd in the configuration file.  However,
// to conform to the existing boch design the user should just add any
// file path to keep the core of bochs happy example,
//
//  cdromd: dev=/dev/cdrom, status=inserted
//
// is fine, even though there is no /dev/cdrom.
//
// Much of the IOKit code was taken from two sources, "Inside MacOSX:
// Accessing Hardware From Applications", a pdf document and the mount
// source ("mount_cd9660.c") both of these are available on the 
// apple website, http://developer.apple.com
//
// The ::read_toc bochs interface is not implemented.  The reason for
// this is that at this point we do not have an application that will
// excersize this interface.  The actual reading of the toc using IOKit
// is here and is used to get the size of the data on the cd for
// ::capacity. The issue with read_toc is formatting the toc entries to
// pass back to bochs.
//
// The loggin functions are not available outside of the cdrom_interface
// class.  That is the reason for the printfs in the static functions below.
// Each cdrom_interface should inherit from cdrom_interface_base.  The methods
// that need to be implemented uniquely should be declared virtual in 
// cdrom_interface_base.  cdrom.h needs to be untangled from the rest of 
// the system for this to work painlessly.
//
// Scott Brumbaugh <scott@openosx.com>  Dec 5, 2001
//

#include "bochs.h"

#define LOG_THIS 

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dev/disk.h>
#include <errno.h>
#include <paths.h>
#include <sys/param.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOCDTypes.h>
#include <CoreFoundation/CoreFoundation.h>

// These definitions were taken from mount_cd9660.c
// There are some similar definitions in IOCDTypes.h
// however there seems to be some dissagreement in 
// the definition of CDTOC.length
struct _CDMSF {
	u_char   minute;
	u_char   second;
	u_char   frame;
};

#define MSF_TO_LBA(msf)		\
	(((((msf).minute * 60UL) + (msf).second) * 75UL) + (msf).frame - 150)

struct _CDTOC_Desc {
	u_char        session;
	u_char        ctrl_adr;  /* typed to be machine and compiler independent */
	u_char        tno;
	u_char        point;
	struct _CDMSF  address;
	u_char        zero;
	struct _CDMSF  p;
};

struct _CDTOC {
	u_short            length;  /* in native cpu endian */
	u_char             first_session;
	u_char             last_session;
	struct _CDTOC_Desc  trackdesc[1];
};

static kern_return_t FindEjectableCDMedia( io_iterator_t *mediaIterator,
					   mach_port_t *masterPort )
{
  kern_return_t     kernResult;
  CFMutableDictionaryRef     classesToMatch;
  kernResult = IOMasterPort( bootstrap_port, masterPort );
  if ( kernResult != KERN_SUCCESS )
    {
      fprintf ( stderr, "IOMasterPort returned %d\n", kernResult );
      return kernResult;
    }
  // CD media are instances of class kIOCDMediaClass.
  classesToMatch = IOServiceMatching( kIOCDMediaClass );
  if ( classesToMatch == NULL )
    fprintf ( stderr, "IOServiceMatching returned a NULL dictionary.\n" );
  else
    {
      // Each IOMedia object has a property with key kIOMediaEjectable
      // which is true if the media is indeed ejectable. So add property
      // to CFDictionary for matching.
      CFDictionarySetValue( classesToMatch,
                            CFSTR( kIOMediaEjectable ), kCFBooleanTrue );
    }
  kernResult = IOServiceGetMatchingServices( *masterPort,
                                             classesToMatch, mediaIterator );
  if ( (kernResult != KERN_SUCCESS) || (*mediaIterator == NULL) ) 
    fprintf( stderr, "No ejectable CD media found.\n kernResult = %d\n", kernResult );

  return kernResult;
}


static kern_return_t GetDeviceFilePath( io_iterator_t mediaIterator,
					char *deviceFilePath, CFIndex maxPathSize )
{
  io_object_t nextMedia;
  kern_return_t kernResult = KERN_FAILURE;
  nextMedia = IOIteratorNext( mediaIterator );
  if ( nextMedia == NULL )
    {
      *deviceFilePath = '\0';
    }
  else
    {
      CFTypeRef    deviceFilePathAsCFString;
      deviceFilePathAsCFString = IORegistryEntryCreateCFProperty(
                                                                 nextMedia, CFSTR( kIOBSDName ),
                                                                 kCFAllocatorDefault, 0 );
      *deviceFilePath = '\0';
      if ( deviceFilePathAsCFString )
        {
          size_t devPathLength = strlen( _PATH_DEV );
          strcpy( deviceFilePath, _PATH_DEV );
          if ( CFStringGetCString( (const __CFString *) deviceFilePathAsCFString,
                                   deviceFilePath + devPathLength,
                                   maxPathSize - devPathLength,
                                   kCFStringEncodingASCII ) )
            {
              // fprintf( stderr, "BSD path: %s\n", deviceFilePath );
              kernResult = KERN_SUCCESS;
            }
          CFRelease( deviceFilePathAsCFString );
        }
    }
  IOObjectRelease( nextMedia );
  return kernResult;
}


static int OpenDrive( const char *deviceFilePath )
{

  int fileDescriptor;

  fileDescriptor = open( deviceFilePath, O_RDONLY );
  if ( fileDescriptor == -1 )
    fprintf( stderr, "Error %d opening device %s.\n", errno, deviceFilePath );
  return fileDescriptor;

}

static struct _CDTOC * ReadTOC( const char * devpath ) {

  struct _CDTOC * toc_p = NULL;
  io_iterator_t iterator = 0;
  io_registry_entry_t service = 0;
  CFDictionaryRef properties = 0;
  CFDataRef data = 0;
  mach_port_t port = 0;
  char * devname;
  
  if (( devname = strrchr( devpath, '/' )) != NULL ) {
    ++devname;
  }
  else {
    devname = (char *) devpath;
  }

  if ( IOMasterPort(bootstrap_port, &port ) != KERN_SUCCESS ) {
    fprintf( stderr, "IOMasterPort failed\n" );
    goto Exit;
  }

  if ( IOServiceGetMatchingServices( port, IOBSDNameMatching( port, 0, devname ),
				     &iterator ) != KERN_SUCCESS ) {
    fprintf( stderr, "IOServiceGetMatchingServices failed\n" );
    goto Exit;
  }
  
  service = IOIteratorNext( iterator );

  IOObjectRelease( iterator );

  iterator = 0;

  while ( service && !IOObjectConformsTo( service, "IOCDMedia" )) {
    if ( IORegistryEntryGetParentIterator( service, kIOServicePlane,
					   &iterator ) != KERN_SUCCESS ) {
      fprintf( stderr, "IORegistryEntryGetParentIterator failed\n" );
      goto Exit;
    }

    IOObjectRelease( service );
    service = IOIteratorNext( iterator );
    IOObjectRelease( iterator );

  }

  if ( service == NULL ) {
    fprintf( stderr, "CD media not found\n" );
    goto Exit;
  }

  if ( IORegistryEntryCreateCFProperties( service, (__CFDictionary **) &properties,
					  kCFAllocatorDefault,
					  kNilOptions ) != KERN_SUCCESS ) {
    fprintf( stderr, "IORegistryEntryGetParentIterator failed\n" );
    goto Exit;
  }

  data = (CFDataRef) CFDictionaryGetValue( properties, CFSTR("TOC") );
  if ( data == NULL ) {
    fprintf( stderr, "CFDictionaryGetValue failed\n" );
    goto Exit;
  }
  else {

    CFRange range;
    CFIndex buflen;

    buflen = CFDataGetLength( data ) + 1;
    range = CFRangeMake( 0, buflen );
    toc_p = (struct _CDTOC *) malloc( buflen );
    if ( toc_p == NULL ) {
      fprintf( stderr, "Out of memory\n" );
      goto Exit;
    }
    else {
      CFDataGetBytes( data, range, (unsigned char *) toc_p );
    }

    /*
    fprintf( stderr, "Table of contents\n length %d first %d last %d\n",
	    toc_p->length, toc_p->first_session, toc_p->last_session );
    */

    CFRelease( properties );

  }
  

 Exit:

  if ( service ) {
    IOObjectRelease( service );
  }

  return toc_p;

}

static char CDDevicePath[ MAXPATHLEN ];

cdrom_interface::cdrom_interface(char *dev)
{
  put("CD");
  settype(CDLOG);
  fd = -1; // File descriptor not yet allocated

  if ( dev == NULL )
    path = NULL;
  else {
    path = strdup(dev);
  }
  using_file=0;
}

void
cdrom_interface::init(void) {
  BX_DEBUG(("cdrom_osx.cc,v 0.1 2001/12/5 sbrumbaugh scott@openosx.com"));
  BX_INFO(("file = '%s'",path));
}

cdrom_interface::~cdrom_interface(void)
{

  if (fd >= 0)
    close(fd);
  if (path)
    free(path);
  BX_DEBUG(("Exit"));

}

bx_bool
cdrom_interface::insert_cdrom(char *dev)
{

  mach_port_t masterPort = NULL;
  io_iterator_t mediaIterator;
  kern_return_t kernResult;

  BX_INFO(( "Insert CDROM" )); 
	
  kernResult = FindEjectableCDMedia( &mediaIterator, &masterPort );
  if ( kernResult != KERN_SUCCESS ) {
    BX_INFO (("Unable to find CDROM"));
    return false;
  }

  kernResult = GetDeviceFilePath( mediaIterator, CDDevicePath, sizeof( CDDevicePath ) );
  if ( kernResult != KERN_SUCCESS ) {
    BX_INFO (("Unable to get CDROM device file path" ));
    return false;
  }

  // Here a cdrom was found so see if we can read from it.
  // At this point a failure will result in panic.
  if ( strlen( CDDevicePath ) ) {

    fd = OpenDrive( CDDevicePath );

    if ( fd < 0 ) {
      BX_PANIC(( "Unable to open CDROM" ));
    }

    // read a test track
    char buffer[2048];

    if ( read( fd, buffer, 2048 ) == -1 ) {
      BX_PANIC(( "Unable to read from CDROM error %d", errno ));
    }

  }

  BX_INFO(( "Found CD at %s opened on descriptor %d", CDDevicePath, fd )); 
  
  return true;
  
}


void
cdrom_interface::eject_cdrom()
{

  // Logically eject the CD.  I suppose we could stick in
  // some ioctl() calls to really eject the CD as well.

  BX_INFO(( "Eject" ));
  close(fd);
  fd = -1;

}


bx_bool
cdrom_interface::read_toc(uint8* buf, int* length, bx_bool msf, int start_track)
{
  // Read CD TOC. Returns false if start track is out of bounds.

  BX_INFO(( "Read TOC - Not Implemented" ));
  return false;
}


// Find the size of the first data track on the cd.  This has produced
// the same results as the linux version on every cd I have tried, about
// 5.  The differences here seem to be that the entries in the TOC when
// retrieved from the IOKit interface appear in a reversed order when
// compared with the linux READTOCENTRY ioctl.
uint32
cdrom_interface::capacity()
{

  // Return CD-ROM capacity.  I believe you want to return
  // the number of bytes of capacity the actual media has.

  BX_INFO(( "Capacity" ));

  struct _CDTOC * toc = ReadTOC( CDDevicePath );

  if ( toc == NULL ) {
    BX_PANIC(( "capacity: Failed to read toc" ));
  }

  size_t toc_entries = ( toc->length - 2 ) / sizeof( struct _CDTOC_Desc );
  
  BX_DEBUG(( "reading %d toc entries\n", toc_entries ));

  int start_sector = -1;
  int data_track = -1;

  // Iterate through the list backward. Pick the first data track and
  // get the address of the immediately previous (or following depending
  // on how you look at it).  The difference in the sector numbers
  // is returned as the sized of the data track.
  for ( int i=toc_entries - 1; i>=0; i-- ) {

    BX_DEBUG(( "session %d ctl_adr %d tno %d point %d lba %d z %d p lba %d\n",
	     (int)toc->trackdesc[i].session,
	     (int)toc->trackdesc[i].ctrl_adr,
	     (int)toc->trackdesc[i].tno,
	     (int)toc->trackdesc[i].point,
	     MSF_TO_LBA( toc->trackdesc[i].address ),
	     (int)toc->trackdesc[i].zero,
	     MSF_TO_LBA(toc->trackdesc[i].p )));

    if ( start_sector != -1 ) {

      start_sector = MSF_TO_LBA(toc->trackdesc[i].p) - start_sector;
      break;

    }

    if (( toc->trackdesc[i].ctrl_adr >> 4) != 1 ) continue;

    if ( toc->trackdesc[i].ctrl_adr & 0x04 ) {

      data_track = toc->trackdesc[i].point;

      start_sector = MSF_TO_LBA(toc->trackdesc[i].p);

    }
      
  }  

  free( toc );

  if ( start_sector == -1 ) {
    start_sector = 0;
  }

  BX_INFO(("first data track %d data size is %d", data_track, start_sector));

  return start_sector;

}

#define CD_FRAMESIZE (2048)
#define CD_SEEK_DISTANCE kCDSectorSizeWhole

// The seeks on the CD need to be measured with kCDSectorSizeWhole which is
// defined to be 2352 or close. We then need to jump ahead 16 bytes to the
// data and read out the next 2048 as the data.
void
cdrom_interface::read_block(uint8* buf, int lba)
{
  // Read a single block from the CD

  off_t pos;
  ssize_t n;

  // This seek will leave us 16 bytes from the start of the data
  // hence the magic number.	
  pos = lseek(fd, lba*CD_SEEK_DISTANCE + 16, SEEK_SET);
  if (pos < 0) {
    BX_PANIC(("cdrom: read_block: lseek returned error."));
  }
  n = read(fd, buf, CD_FRAMESIZE);
 
  if (n != CD_FRAMESIZE) {
    BX_PANIC(("cdrom: read_block: read returned %d",
	      (int) n));
  }

  /*
  for (int i=0; i<CD_FRAMESIZE; i++) {
    printf (" %2X", buf[i] );
  }
  printf ("\n");
  */

}

