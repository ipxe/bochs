/*
 *  plex86: run multiple x86 operating systems concurrently
 *  Copyright (C) 1999-2003 Kevin P. Lawton
 *
 *  host-linux.c: Linux specific VM host driver functionality
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include "plex86.h"
#define IN_HOST_SPACE
#include "monitor.h"

#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/wrapper.h>
#include <linux/version.h>
#include <asm/irq.h>


#ifndef VERSION_CODE
#  define VERSION_CODE(vers,rel,seq) ( ((vers)<<16) | ((rel)<<8) | (seq) )
#endif


#if LINUX_VERSION_CODE < VERSION_CODE(2,4,20)
/* I use get_user_pages() to find and pin physical pages of memory
 * underlying the guest physical memory malloc()'d from user space.
 * This became an exported symbol available for kernel modules
 * as of 2.4.20.  You will have to recode some functions for
 * lesser kernels.
 */
#  error "Currently, you need Linux kernel 2.4.20 or above."
#endif


#if LINUX_VERSION_CODE >= VERSION_CODE(2,1,0)
#  include <asm/uaccess.h>
#endif

#include <asm/io.h>



/************************************************************************/
/* Compatibility macros & convenience functions for older kernels       */
/************************************************************************/

#ifndef EXPORT_NO_SYMBOLS
#  define EXPORT_NO_SYMBOLS register_symtab(NULL)
#endif

#if LINUX_VERSION_CODE >= VERSION_CODE(2,1,29)
#  define proc_register_dynamic proc_register
#endif

#if LINUX_VERSION_CODE < VERSION_CODE(2,2,0)
#define NEED_RESCHED need_resched
#else
#define NEED_RESCHED current->need_resched
#endif

#if LINUX_VERSION_CODE < VERSION_CODE(2,1,0)
  static inline unsigned long
copy_from_user(void *to, const void *from, unsigned long n)
{
  int i;
  if ( (i = verify_area(VERIFY_READ, from, n)) != 0 )
    return i;
  memcpy_fromfs(to, from, n);
  return 0;
}
  static inline unsigned long
copy_to_user(void *to, const void *from, unsigned long n)
{
  int i;
  if ( (i = verify_area(VERIFY_WRITE, to, n)) != 0 )
    return i;
  memcpy_tofs(to, from, n);
  return 0;
}
#endif

#if LINUX_VERSION_CODE >= VERSION_CODE(2,1,18) && !defined(THIS_MODULE)
/* Starting with version 2.1.18, the __this_module symbol is present,
 * but the THIS_MODULE #define was introduced much later ...
 */
#define THIS_MODULE (&__this_module)
#endif


/************************************************************************/
/* Declarations                                                         */
/************************************************************************/

/* Use dynamic major number allocation. (Set non-zero for static allocation) */
#define PLEX86_MAJOR 0
static int plex_major = PLEX86_MAJOR;
#if LINUX_VERSION_CODE >= VERSION_CODE(2,1,18)
MODULE_PARM(plex_major, "i");
MODULE_PARM_DESC(plex_major, "major number (default " __MODULE_STRING(PLEX86_MAJOR) ")");
#endif

/* The kernel segment base. */
#if LINUX_VERSION_CODE < VERSION_CODE(2,1,0)
#  define KERNEL_OFFSET 0xc0000000
#else
#  define KERNEL_OFFSET 0x00000000
#endif


/* File operations. */
static int plex86_ioctl(struct inode *, struct file *, unsigned int,
                        unsigned long);
static int plex86_open(struct inode *, struct file *);

#if LINUX_VERSION_CODE >= VERSION_CODE(2,1,31)
static int plex86_release(struct inode *, struct file *);
#else
static void plex86_release(struct inode *, struct file *);
#endif

#if LINUX_VERSION_CODE >= VERSION_CODE(2,1,0)
static int plex86_mmap(struct file * file, struct vm_area_struct * vma);
#else
static int plex86_mmap(struct inode * inode, struct file * file,
                       struct vm_area_struct * vma);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,9)
/* New License scheme. */
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL"); /* Close enough.  Keeps kernel from complaining. */
#endif
#endif



/************************************************************************/
/* Structures / Variables                                               */
/************************************************************************/

static int      retrieve_monitor_pages(void);
static unsigned retrievePhyPages(Bit32u *page, int max_pages, void *addr,
                                 unsigned size);



static struct file_operations plex86_fops = {
#if LINUX_VERSION_CODE >= VERSION_CODE(2,4,0)
  owner:    THIS_MODULE,
#endif
  mmap:     plex86_mmap,
  ioctl:    plex86_ioctl,
  open:     plex86_open,
  release:  plex86_release,
  };


#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
devfs_handle_t my_devfs_entry;
#endif

/* For the /proc/driver/plex86 entry. */
#if LINUX_VERSION_CODE >= VERSION_CODE(2,4,0) /* XXX - How far back? */
int plex86_read_procmem(char *, char **, off_t, int);
#else
int plex86_read_procmem(char *, char **, off_t, int, int);
#endif

#if LINUX_VERSION_CODE < VERSION_CODE(2,3,25)
static struct proc_dir_entry plex86_proc_entry = {
  0,                  /* dynamic inode */
  6, "driver/plex86",     /* len, name */
  S_IFREG | S_IRUGO,  /* mode */
  1, 0, 0,
  0,
  NULL,
  &plex86_read_procmem,  /* read function */
  };
#endif

#if CONFIG_X86_PAE
#  error "CONFIG_X86_PAE defined for this kernel, but unhandled in plex86"
#endif

/************************************************************************/
/* Main kernel module code                                              */
/************************************************************************/

  int
init_module(void)
{
  int err;

  /* Clear uninitialised structures. */
  memset(&monitor_pages, 0, sizeof(monitor_pages));

  /* Register the device with the kernel. */
  err = register_chrdev(plex_major, "plex86", &plex86_fops);
  if (err < 0) {
    printk(KERN_WARNING "plex86: can't get major %d\n", plex_major);
    return(err);
    }
  /* If this was a dynamic allocation, save the major for
   * the release code
   */
  if(!plex_major)
    plex_major = err;

  /* Register the /proc entry. */
#ifdef CONFIG_PROC_FS
#if LINUX_VERSION_CODE >= VERSION_CODE(2,3,25)
  if (!create_proc_info_entry("driver/plex86", 0, NULL, plex86_read_procmem))
    printk(KERN_ERR "plex86: registering /proc/driver/plex86 failed\n");
#else
  proc_register_dynamic(&proc_root, &plex86_proc_entry);
#endif
#endif

  /* Register /dev/misc/plex86 with devfs. */
#ifdef CONFIG_DEVFS_FS
  my_devfs_entry = devfs_register(NULL, "misc/plex86", 
                                  DEVFS_FL_DEFAULT, 
                                  plex_major, 0 /* minor mode*/, 
                                  S_IFCHR | 0666, &plex86_fops,
                                  NULL /* "info" */);
  if (!my_devfs_entry)
    printk(KERN_ERR "plex86: registering misc/plex86 devfs entry failed\n");
#endif

  /* Retrieve the monitor physical pages. */
  if ( !retrieve_monitor_pages() ) {
    printk(KERN_ERR "plex86: retrieve_monitor_pages returned error\n");
    err = -EINVAL;
    goto fail_retrieve_pages;
    }

  /* Kernel independent code to be run when kernel module is loaded. */
  if ( !genericModuleInit() ) {
    printk(KERN_ERR "plex86: genericModuleInit returned error\n");
    err = -EINVAL;
    goto fail_cpu_capabilities;
    }

  /* Success. */
  EXPORT_NO_SYMBOLS;
  return(0);

fail_cpu_capabilities:
fail_retrieve_pages:
  /* Unregister /proc entry. */
#ifdef CONFIG_PROC_FS
#if LINUX_VERSION_CODE >= VERSION_CODE(2,3,25)
  remove_proc_entry("driver/plex86", NULL);
#else
  proc_unregister(&proc_root, plex86_proc_entry.low_ino);
#endif
#endif

  /* Unregister device. */
  unregister_chrdev(plex_major, "plex86");
  return err;
}

  void
cleanup_module(void)
{
  /* Unregister device. */
  unregister_chrdev(plex_major, "plex86");

  /* Unregister /proc entry. */
#ifdef CONFIG_PROC_FS
#if LINUX_VERSION_CODE >= VERSION_CODE(2,3,25)
  remove_proc_entry("driver/plex86", NULL);
#else
  proc_unregister(&proc_root, plex86_proc_entry.low_ino);
#endif
#endif

#ifdef CONFIG_DEVFS_FS
  devfs_unregister(my_devfs_entry);
#endif
}



/************************************************************************/
/* Open / Release a VM                                                  */
/************************************************************************/

  int
plex86_open(struct inode *inode, struct file *filp)
{
  vm_t *vm;
#if LINUX_VERSION_CODE < VERSION_CODE(2,4,0)
  MOD_INC_USE_COUNT;
#endif

  /* Allocate a VM structure. */
  if ( (vm = hostAllocZeroedMem(sizeof(vm_t))) == NULL )
    return -ENOMEM;
  filp->private_data = vm;
  
  /* Kernel independent device open code. */
  genericDeviceOpen(vm);

  return(0);
}


#if LINUX_VERSION_CODE >= VERSION_CODE(2,1,31)
  int
#else
  void
#endif
plex86_release(struct inode *inode, struct file *filp)
{
  vm_t *vm = (vm_t *)filp->private_data;
  filp->private_data = NULL;

  /* Free the virtual memory. */
  unreserveGuestPhyPages(vm);
  unallocVmPages( vm );

  /* Free the VM structure. */
  memset( vm, 0, sizeof(*vm) );
  vfree( vm );

#if LINUX_VERSION_CODE < VERSION_CODE(2,4,0)
  MOD_DEC_USE_COUNT;
#endif

#if LINUX_VERSION_CODE >= VERSION_CODE(2,1,31)
  return(0);
#endif
}


  int
plex86_ioctl(struct inode *inode, struct file *filp,
             unsigned int cmd, unsigned long arg)
{
  vm_t *vm = (vm_t *)filp->private_data;
  int ret;

  /* Call non host-specific ioctl() code which calls back to this
   * module only when it needs host-specific features.
   */
  ret = ioctlGeneric(vm, inode, filp, cmd, arg);

  /* Convert from plex86 errno codes to host-specific errno codes.  Not
   * very exciting.
   */
  if ( ret < 0 )
    ret = - hostConvertPlex86Errno(- ret);
  return( ret );
}


  int
#if LINUX_VERSION_CODE >= VERSION_CODE(2,1,0)
plex86_mmap(struct file * file, struct vm_area_struct * vma)
#else
plex86_mmap(struct inode * inode, struct file * file, struct vm_area_struct * vma)
#endif
{
  vm_t *vm = (vm_t *)file->private_data;
  int firstpage, pagesN;
#if LINUX_VERSION_CODE >= VERSION_CODE(2,1,0)
  void *inode = NULL; /* Not used; for consistency of passing args. */
#endif
  int ret;

  /* Private mappings make no sense ... */
  if ( !(vma->vm_flags & VM_SHARED) ) {
    printk(KERN_WARNING "plex86: private mapping\n");
    return -EINVAL;
    }

#if LINUX_VERSION_CODE < VERSION_CODE(2,3,25)
  /* To simplify things, allow only page-aligned offsets */
  if ( vma->vm_offset & (PAGE_SIZE - 1) ) {
    printk(KERN_WARNING "plex86: unaligned offset %08lx\n", vma->vm_offset);
    return -EINVAL;
    }
#endif


#if LINUX_VERSION_CODE >= VERSION_CODE(2,3,25)
  firstpage = vma->vm_pgoff;
#else
  firstpage = vma->vm_offset >> PAGE_SHIFT;
#endif
  pagesN  = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;

  ret = genericMMap(vm, inode, file, vma, firstpage, pagesN);
  return( - hostConvertPlex86Errno(ret) );
}


  int
hostMMap(vm_t *vm, void *iV, void *fV, void *vmaV, 
         unsigned pagesN, Bit32u *pagesArray)
{
#if LINUX_VERSION_CODE >= VERSION_CODE(2,1,0)
  void *inode = NULL;
#else
  struct inode * inode = (struct inode *) iV;
#endif
  struct file * file = (struct file *) fV;
  struct vm_area_struct * vma = (struct vm_area_struct *) vmaV;
  unsigned i;

  UNUSED(file);

  /* Note: this function returns Plex86Errno style errors, since
   * it reports to the hostOS-independent logic.
   */

  /* Sanity check. */
#if LINUX_VERSION_CODE >= VERSION_CODE(2,3,25)
  if ( ((vma->vm_end - vma->vm_start) >> PAGE_SHIFT) > pagesN ) {
    printk(KERN_WARNING "plex86: mmap sanity checks failed.\n");
    return Plex86ErrnoEINVAL;
    }
#else
  if ( (vma->vm_end - vma->vm_start) > (pagesN << PAGE_SHIFT) ) {
    printk(KERN_WARNING "plex86: mmap sanity checks failed.\n");
    return Plex86ErrnoEINVAL;
    }
#endif

  for (i = 0; i < pagesN; i++) {
    if ( remap_page_range(vma->vm_start + (i << PAGE_SHIFT),
                          pagesArray[i] << 12,
                          PAGE_SIZE,
                          vma->vm_page_prot) )
      /* xxx What about fixing partial remaps? */
      return Plex86ErrnoEAGAIN;
    }

#if LINUX_VERSION_CODE < VERSION_CODE(2,1,0)
  /* Enter our inode into the VMA; no need to change the default ops. */
  vma->vm_inode = inode;
  if (!inode->i_count)
    inode->i_count++;
#else
  UNUSED(inode);
#endif

  return 0; /* OK. */
}



/************************************************************************/
/* Status reporting:  /proc code                                        */
/************************************************************************/

  int
plex86_read_procmem(char *buf, char **start, off_t offset,
#if LINUX_VERSION_CODE >= VERSION_CODE(2,4,0)
                    int len
#else
                    int len, int unused
#endif
                    )
{
  unsigned i;
  len = 0;
  len += sprintf(buf, "monitor-->host interrupt reflection counts\n");
  for (i=0; i<256; i++) {
    if (intRedirCount[i])
      len += sprintf(buf+len, "  0x%2x:%10u\n", i, intRedirCount[i]);
    }
  return(len);
}


  int
retrieve_monitor_pages(void)
{
  /* 
   * Retrieve start address and size of this module.
   *
   * Note that with old kernels, we cannot access the module info (size),
   * hence we rely on the fact that Linux lets at least one page of 
   * virtual address space unused after the end of the module.
   */
#ifdef THIS_MODULE
  Bit32u   driverStartAddr = (Bit32u) THIS_MODULE;
  unsigned size            = THIS_MODULE->size;
#else
  Bit32u   driverStartAddr = (Bit32u) &mod_use_count_;
  unsigned size            = 0;  /* Actual size determined below */
#endif
  Bit32u   driverStartAddrPageAligned = driverStartAddr & ~0xfff;

  int    nPages;

  if (driverStartAddr != driverStartAddrPageAligned) {
    /* Pretend this kernel module starts at the beginning of the page. */
    /* If size is known, we have to add the extra offset from the beginning
     * of the page.
     */
    if (size)
      size += (driverStartAddr & 0xfff);
    }

  nPages = retrievePhyPages(monitor_pages.page, PLEX86_MAX_MONITOR_PAGES,
                            (void *) driverStartAddrPageAligned, size);
  if (nPages == 0) {
    printk(KERN_ERR "plex86: retrieve_monitor_pages: retrieve returned error.\n");
    return( 0 ); /* Error. */
    }
  printk(KERN_WARNING "plex86: %u monitor pages located\n", nPages);

  monitor_pages.startOffset            = driverStartAddr;
  monitor_pages.startOffsetPageAligned = driverStartAddrPageAligned;
  monitor_pages.n_pages                = nPages;
  return( 1 ); /* OK. */
}


  unsigned
retrievePhyPages(Bit32u *page, int max_pages, void *addr_v, unsigned size)
{
  /*  
   * Grrr.  There doesn't seem to be an exported mechanism to retrieve
   * the physical pages underlying a vmalloc()'ed area.  We do it the
   * hard way ... 
   */
  pageEntry_t *host_pgd;
  Bit32u host_cr3;
  Bit32u addr; // start_addr;
  unsigned n_pages;
  int i;

  addr = (Bit32u) addr_v;
  if ( addr & 0xfff ) {
    printk(KERN_ERR "plex86: retrievePhyPages: not page aligned!\n");
    return 0;
    }

  if (!addr) {
    printk(KERN_ERR "plex86: retrievePhyPages: addr NULL!\n");
    return 0;
    }

  if (size == 0) {
    /* Size unknown.  Determine by cycling through page tables until
     * we find one which is not present.  We will assume that means
     * the end of the data structure.  Set the number of pages to
     * cycle through, to one more than the maximum requested.  This
     * way we'll look through enough pages.
     */
    n_pages = max_pages + 1;
    }
  else {
    n_pages = BytesToPages(size);
    if ( n_pages > max_pages ) {
      printk(KERN_ERR "plex86: retrievePhyPages: n=%u > max=%u\n",
         n_pages, max_pages);
      return 0;
      }
    }

  asm volatile ("movl %%cr3, %0" : "=r" (host_cr3));
  host_pgd = (pageEntry_t *)(phys_to_virt(host_cr3 & ~0xfff));

  for (i = 0; i < n_pages; i++) {
    Bit32u laddr;
    unsigned long lpage;
    pgd_t *pgdPtr; pmd_t *pmdPtr; pte_t *ptePtr;
    pgd_t  pgdVal; pmd_t  pmdVal; pte_t  pteVal;

    laddr = KERNEL_OFFSET + ((Bit32u) addr);

    lpage = VMALLOC_VMADDR(laddr);

    /* About to traverse the page tables.  We need to lock others
     * out of them briefly.  Newer Linux versions can do a fine-grained
     * lock on the page tables themselves.  Older ones have to do
     * a "big kernel lock".
     */
#if LINUX_VERSION_CODE >= VERSION_CODE(2,3,10)
    spin_lock(&init_mm.page_table_lock);
#else
    lock_kernel(); /* Big kernel lock. */
#endif
    pgdPtr = pgd_offset(&init_mm, lpage);
    pmdPtr = pmd_offset(pgdPtr, lpage);
    ptePtr = pte_offset(pmdPtr, lpage);

    pgdVal = *pgdPtr;
    pmdVal = *pmdPtr;
    pteVal = *ptePtr;

#if LINUX_VERSION_CODE >= VERSION_CODE(2,3,10)
    spin_unlock(&init_mm.page_table_lock);
#else
    unlock_kernel(); /* Big kernel unlock. */
#endif

    if ( !(pgdVal.pgd & 1) ||
         !(pmdVal.pmd & 1) ||
         !(pteVal.pte_low & 1) ) {
      if (size == 0)
        return i; /* Report number of pages until area ended. */
      printk(KERN_ERR "plex86: retrievePhyPages: "
                      "PDE.P==0: i=%u, n=%u laddr=0x%x\n", i, n_pages, laddr);
      return 0; /* Error, ran into unmapped page in memory range. */
      }

    /* Abort if our page list is too small. */
    if (i >= max_pages) {
      printk(KERN_WARNING "plex86: page list is too small!\n");
      printk(KERN_WARNING "plex86: n_pages=%u, max_pages=%u\n",
           n_pages, max_pages);
      return 0;
      }
    /* Get physical page address for this virtual page address. */
    page[i] = pte_val(pteVal) >> 12;
    /* Increment to the next virtual page address. */
    addr += 4096;
    }
  return(n_pages);
}



/************************************************************************
 * The requisite host-specific functions.  An implementation of each of
 * these functions needs to be offered for each host-XYZ.c file.
 ************************************************************************/

  void
hostReservePhyPages(vm_t *vm, Bit32u *hostPhyPages, unsigned nPages)
{
  unsigned p;

  /*
   * As we want to map these pages to user space, we need to mark
   * them as 'reserved' pages by setting the PG_reserved bit.
   *
   * This has the effect that:
   *  - remap_page_range accepts them as candidates for remapping
   *  - the swapper does *not* try to swap these pages out, even
   *    after they are mapped to user space
   */

  for (p = 0; p < nPages; p++)
#if LINUX_VERSION_CODE >= VERSION_CODE(2,4,0)
    set_bit(PG_reserved, &((mem_map + hostPhyPages[p])->flags));
#else
    mem_map_reserve(hostPhyPages[p]);
#endif
}

  void
hostUnreservePhyPages(vm_t *vm, Bit32u *hostPhyPages, unsigned nPages)
{
  unsigned p;

  /* Remove the PG_reserved flags before returning the pages. */
  for (p = 0; p < nPages; p++)
#if LINUX_VERSION_CODE >= VERSION_CODE(2,4,0)
    clear_bit(PG_reserved, &((mem_map + hostPhyPages[p])->flags));
#else
    mem_map_unreserve(hostPhyPages[p]);
#endif
}

  unsigned
hostIdle(void)
{
  if (NEED_RESCHED)
    schedule();

  /* return !current_got_fatal_signal(); */
  return( ! signal_pending(current) );
}

  void *
hostAllocZeroedMem(unsigned long size)
{
  void *ptr;

  ptr = vmalloc(size);
  if ( ((Bit32u) ptr) & 0x00000fff )
    return( 0 ); /* Error. */

  /* Zero pages.  This also demand maps the pages in, which we need
   * since we'll cycle through all the pages to get the physical
   * address mappings.
   */
  mon_memzero(ptr, size);
  return( ptr );
}

  void
hostFreeMem(void *ptr)
{
  vfree(ptr);
}

  void *
hostAllocZeroedPage(void)
{
  return( (void *) get_zeroed_page(GFP_KERNEL) );
}

  void
hostFreePage(void *ptr)
{
  free_page( (Bit32u)ptr );
}


  unsigned
hostGetAllocedMemPhyPages(Bit32u *page, int max_pages, void *ptr, unsigned size)
{
  return( retrievePhyPages(page, max_pages, ptr, size) );
}

  Bit32u
hostGetAllocedPagePhyPage(void *ptr)
{
  if (!ptr) return 0;
  /* return MAP_NR(ptr); */
  return(__pa(ptr) >> PAGE_SHIFT);
}

  void
hostPrint(char *fmt, ...)
{
#warning "Fix hostPrint"
#if 0
  va_list args;
  int ret;
  unsigned char buffer[256];

  va_start(args, fmt);
  ret = mon_vsnprintf(buffer, 256, fmt, args);
  if (ret == -1) {
    printk(KERN_ERR "plex86: hostPrint: vsnprintf returns error.\n");
    }
  else {
    printk(KERN_WARNING "plex86: %s\n", buffer);
    }
#endif
}


  int
hostConvertPlex86Errno(unsigned ret)
{
  switch (ret) {
    case 0: return(0);
    case Plex86ErrnoEBUSY:  return(EBUSY);
    case Plex86ErrnoENOMEM: return(ENOMEM);
    case Plex86ErrnoEFAULT: return(EFAULT);
    case Plex86ErrnoEINVAL: return(EINVAL);
    case Plex86ErrnoEACCES: return(EACCES);
    case Plex86ErrnoEAGAIN: return(EAGAIN);
    default:
      printk(KERN_ERR "plex86: ioctlAllocVPhys: case %u\n", ret);
      return(EINVAL);
    }
}


  Bit32u
hostKernelOffset(void)
{
  return( KERNEL_OFFSET );
}

  unsigned
hostMMapCheck(void *i, void *f)
{
  struct inode *inode;
  struct file *filp;

  inode = (struct inode *) i;
  filp  = (struct file *) f;

#if LINUX_VERSION_CODE >= VERSION_CODE(2,3,99)
  /* Not sure when this changed.  If you know, email us. */
  if (inode->i_data.i_mmap != NULL)
#else
  if (inode->i_mmap != NULL)
#endif
    return 1;
  return 0;
}

  void
hostModuleCountReset(vm_t *vm, void *inode, void *filp)
{
#if LINUX_VERSION_CODE < VERSION_CODE(2,4,0)
  while (MOD_IN_USE) {
    MOD_DEC_USE_COUNT;
    }
    
  MOD_INC_USE_COUNT; /* bump back to 1 so release can decrement */
#endif
}

  unsigned long
hostCopyFromUser(void *to, void *from, unsigned long len)
{
  return( copy_from_user(to, from, len) );
}

  unsigned long
hostCopyToUser(void *to, void *from, unsigned long len)
{
  return( copy_to_user(to, from, len) );
}

  unsigned
hostGetAndPinUserPages(vm_t *vm, Bit32u *pagePhyAddrList, void *userPtr,
                       unsigned nPages)
{
  int    ret;
  unsigned p;
  struct page **linuxKernelPageList;

  linuxKernelPageList = (struct page **) vm->pages.hostStructPagePtr;
  ret = get_user_pages(current,
                       current->mm,
                       (unsigned long) userPtr,
                       nPages,
                       1, /* 'write': intent to write. */
                       0, /* 'force': ? */
                       linuxKernelPageList,
                       NULL /* struct vm_area_struct *[] */
                       );
  if (ret != nPages) {
    printk(KERN_ERR "plex86: hostGetAndPinUserPages: failed.\n");
    return(0); /* Error. */
    }

  /* Now that we have a list of "struct page *", one for each physical
   * page of memory of the user space process's requested area, we can
   * calculate the physical page address by simple pointer arithmetic
   * based on "mem_map".
   */
  for (p=0; p<nPages; p++) {
    pagePhyAddrList[p] = linuxKernelPageList[p] - mem_map;
    }
  
  return(1); /* OK. */
}

  void
hostReleasePinnedUserPages(vm_t *vm, Bit32u *pageAddrList, unsigned nPages)
{
#if 0
  /* Here is some sample code from Linux 2.4.18, mm/memory.c:__free_pte() */
	struct page *page = pte_page(pte);
  if ((!VALID_PAGE(page)) || PageReserved(page))
    return;
  if (pte_dirty(pte))
    set_page_dirty(page);
  free_page_and_swap_cache(page);
#endif

	struct page *page;
  unsigned p;

static unsigned iteration = 0;
printk(KERN_WARNING "plex86: Release called %u.\n", iteration++);
  for (p=0; p<nPages; p++) {
    page = (struct page *) vm->pages.hostStructPagePtr[p];
    if (1) /* If dirty. */
      set_page_dirty(page);
    put_page(page);
    }
}
