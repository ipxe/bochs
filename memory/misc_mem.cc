/////////////////////////////////////////////////////////////////////////
// $Id: misc_mem.cc,v 1.48 2004/08/30 21:47:24 sshwarts Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2002  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  I/O memory handlers API Copyright (C) 2003 by Frank Cornelis
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
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA




#include "iodev/iodev.h"
#define LOG_THIS BX_MEM(0)->



#if BX_PROVIDE_CPU_MEMORY
Bit32u BX_MEM_C::get_memory_in_k(void)
{
  return(BX_MEM_THIS megabytes * 1024);
}
#endif // #if BX_PROVIDE_CPU_MEMORY


#if BX_PROVIDE_CPU_MEMORY
  // BX_MEM_C constructor
BX_MEM_C::BX_MEM_C(void)
{
  char mem[6];
  snprintf(mem, 6, "MEM%d", BX_SIM_ID);
  put(mem);
  settype(MEMLOG);

  vector = NULL;
  actual_vector = NULL;
  len    = 0;
  megabytes = 0;

  memory_handlers = NULL;
}
#endif // #if BX_PROVIDE_CPU_MEMORY


#if BX_PROVIDE_CPU_MEMORY
void BX_CPP_AttrRegparmN(2)
BX_MEM_C::alloc_vector_aligned (size_t bytes, size_t alignment)
{
  if (actual_vector != NULL) {
    BX_INFO (("freeing existing memory vector"));
    delete [] actual_vector;
    actual_vector = NULL;
    vector = NULL;
  }
  Bit64u test_mask = alignment - 1;
  actual_vector = new Bit8u [bytes+test_mask];
  // round address forward to nearest multiple of alignment.  Alignment 
  // MUST BE a power of two for this to work.
  Bit64u masked = ((Bit64u)(actual_vector + test_mask)) & ~test_mask;
  vector = (Bit8u *)masked;
  // sanity check: no lost bits during pointer conversion
  BX_ASSERT (sizeof(masked) >= sizeof(vector));
  // sanity check: after realignment, everything fits in allocated space
  BX_ASSERT (vector+bytes <= actual_vector+bytes+test_mask);
  BX_INFO (("allocated memory at %p. after alignment, vector=%p", 
	actual_vector, vector));
}
#endif


#if BX_PROVIDE_CPU_MEMORY
// BX_MEM_C destructor
BX_MEM_C::~BX_MEM_C(void)
{
  if (this-> vector != NULL) {
    delete [] actual_vector;
    actual_vector = NULL;
    vector = NULL;
    delete [] memory_handlers;
    memory_handlers = NULL;
    }
  else {
    BX_DEBUG(("(%u)   memory not freed as it wasn't allocated!", BX_SIM_ID));
    }
}
#endif // #if BX_PROVIDE_CPU_MEMORY


#if BX_PROVIDE_CPU_MEMORY
  void
BX_MEM_C::init_memory(int memsize)
{
  BX_DEBUG(("Init $Id: misc_mem.cc,v 1.48 2004/08/30 21:47:24 sshwarts Exp $"));
  // you can pass 0 if memory has been allocated already through
  // the constructor, or the desired size of memory if it hasn't
  // BX_INFO(("%.2fMB", (float)(BX_MEM_THIS megabytes) ));

  if (BX_MEM_THIS vector == NULL) {
    // memory not already allocated, do now...
    alloc_vector_aligned (memsize, BX_MEM_VECTOR_ALIGN);
    BX_MEM_THIS len    = memsize;
    BX_MEM_THIS megabytes = memsize / (1024*1024);
    BX_MEM_THIS memory_handlers = new struct memory_handler_struct *[1024 * 1024];
    for (int idx = 0; idx < 1024 * 1024; idx++)
	    BX_MEM_THIS memory_handlers[idx] = NULL;
    BX_INFO(("%.2fMB", (float)(BX_MEM_THIS megabytes) ));
    }

#if BX_DEBUGGER
  if (megabytes > BX_MAX_DIRTY_PAGE_TABLE_MEGS) {
    BX_INFO(("Error: memory larger than dirty page table can handle"));
    BX_PANIC(("Error: increase BX_MAX_DIRTY_PAGE_TABLE_MEGS"));
    }
#endif

}
#endif // #if BX_PROVIDE_CPU_MEMORY


#if BX_PROVIDE_CPU_MEMORY
  void
  // Values for type :
  // 0 : System Bios
  // 1 : VGA Bios
  // 2 : Optional ROM Bios
BX_MEM_C::load_ROM(const char *path, Bit32u romaddress, Bit8u type)
{
  struct stat stat_buf;
  int fd, ret;
  unsigned long size, max_size, offset;

  if (*path == '\0') {
    if (type == 2) {
      BX_PANIC(( "ROM: Optional ROM image undefined"));
      }
    else if (type == 1) {
      BX_PANIC(( "ROM: VGA BIOS image undefined"));
      }
    else {
      BX_PANIC(( "ROM: System BIOS image undefined"));
      }
    return;
    }
  // read in ROM BIOS image file
  fd = open(path, O_RDONLY
#ifdef O_BINARY
            | O_BINARY
#endif
           );
  if (fd < 0) {
    if (type < 2) {
      BX_PANIC(( "ROM: couldn't open ROM image file '%s'.", path));
      }
    else {
      BX_ERROR(( "ROM: couldn't open ROM image file '%s'.", path));
      }
    return;
    }
  ret = fstat(fd, &stat_buf);
  if (ret) {
    if (type < 2) {
      BX_PANIC(( "ROM: couldn't stat ROM image file '%s'.", path));
      }
    else {
      BX_ERROR(( "ROM: couldn't stat ROM image file '%s'.", path));
      }
    return;
    }

  size = stat_buf.st_size;
  BX_INFO(("ROM image size is %ld ...", size));

  if (type > 0) {
    max_size = 0x10000;
  } else {
    max_size = 0x20000;
  }
  if (size > max_size) {
    BX_PANIC(("ROM: ROM image too large"));
    return;
  }
  if (type == 0) {
    if ( (romaddress + size) != 0x100000 ) {
      BX_PANIC(("ROM: System BIOS must end at 0xfffff"));
      return;
    }
  } else {
//    if ((size % 512) != 0) {
//      BX_PANIC(("ROM: ROM image size must be multiple of 512"));
//      return;
//    }
    if ((romaddress % 2048) != 0) {
      BX_PANIC(("ROM: ROM image must start at a 2k boundary"));
      return;
    }
    if ((romaddress < 0xc0000) || ((romaddress + size) > 0xf0000)) {
      BX_PANIC(("ROM: ROM address space out of range"));
      return;
    }
  }
  offset = 0;
  while (size > 0) {
    ret = read(fd, (bx_ptr_t) &BX_MEM_THIS vector[romaddress + offset], size);
    if (ret <= 0) {
      BX_PANIC(( "ROM: read failed on BIOS image: '%s'",path));
      }
    size -= ret;
    offset += ret;
    }
  close(fd);
  BX_INFO(("rom at 0x%05x/%u ('%s')",
			(unsigned) romaddress,
			(unsigned) stat_buf.st_size,
 			path));
}
#endif // #if BX_PROVIDE_CPU_MEMORY


#if ( BX_DEBUGGER || BX_DISASM || BX_GDBSTUB)
  bx_bool
BX_MEM_C::dbg_fetch_mem(Bit32u addr, unsigned len, Bit8u *buf)
{
  if ( (addr + len) > this->len ) {
    BX_INFO(("dbg_fetch_mem out of range. 0x%x > 0x%x",
      addr+len, this->len));
    return(0); // error, beyond limits of memory
    }
  for (; len>0; len--) {
    if ( (addr & 0xfffe0000) == 0x000a0000 ) {
      *buf = DEV_vga_mem_read(addr);
      }
    else {
#if BX_SUPPORT_PCI == 0
      *buf = vector[addr];
#else
      if ( bx_options.Oi440FXSupport->get () &&
          ((addr >= 0x000C0000) && (addr <= 0x000FFFFF)) ) {
        switch (DEV_pci_rd_memtype (addr)) {
          case 0x1:  // Fetch from ShadowRAM
            *buf = shadow[addr - 0xc0000];
//          BX_INFO(("Fetching from ShadowRAM %06x, len %u !", (unsigned)addr, (unsigned)len));
            break;

          case 0x0:  // Fetch from ROM
            *buf = vector[addr];
//          BX_INFO(("Fetching from ROM %06x, Data %02x ", (unsigned)addr, *buf));
            break;
          default:
            BX_PANIC(("dbg_fetch_mem: default case"));
          }
        }
      else
        *buf = vector[addr];
#endif  // #if BX_SUPPORT_PCI == 0
      }
    buf++;
    addr++;
    }
  return(1);
}
#endif

#if BX_DEBUGGER || BX_GDBSTUB
  bx_bool
BX_MEM_C::dbg_set_mem(Bit32u addr, unsigned len, Bit8u *buf)
{
  if ( (addr + len) > this->len ) {
    return(0); // error, beyond limits of memory
    }
  for (; len>0; len--) {
    if ( (addr & 0xfffe0000) == 0x000a0000 ) {
      DEV_vga_mem_write(addr, *buf);
      }
    else
      vector[addr] = *buf;
    buf++;
    addr++;
    }
  return(1);
}
#endif

  bx_bool
BX_MEM_C::dbg_crc32(unsigned long (*f)(unsigned char *buf, int len),
    Bit32u addr1, Bit32u addr2, Bit32u *crc)
{
  unsigned len;

  *crc = 0;
  if (addr1 > addr2)
    return(0);

  if (addr2 >= this->len) {
    return(0); // error, specified address past last phy mem addr
    }
  
  len = 1 + addr2 - addr1;
  *crc = f(vector + addr1, len);

  return(1);
}


  Bit8u * BX_CPP_AttrRegparmN(3)
BX_MEM_C::getHostMemAddr(BX_CPU_C *cpu, Bit32u a20Addr, unsigned op)
  // Return a host address corresponding to the guest physical memory
  // address (with A20 already applied), given that the calling
  // code will perform an 'op' operation.  This address will be
  // used for direct access to guest memory as an acceleration by
  // a few instructions, like REP {MOV, INS, OUTS, etc}.
  // Values of 'op' are { BX_READ, BX_WRITE, BX_RW }.

  // The other assumption is that the calling code _only_ accesses memory
  // directly within the page that encompasses the address requested.
{
  if ( a20Addr >= BX_MEM_THIS len )
    return(NULL); // Error, requested addr is out of bounds.
  if (op == BX_READ) {
    if ( (a20Addr > 0x9ffff) && (a20Addr < 0xc0000) )
      return(NULL); // Vetoed!  Mem mapped IO (VGA)
#if !BX_SUPPORT_PCI
    return( (Bit8u *) & vector[a20Addr] );
#else
    else if ( (a20Addr < 0xa0000) || (a20Addr > 0xfffff)
              || (!bx_options.Oi440FXSupport->get ()) )
      return( (Bit8u *) & vector[a20Addr] );
    else {
      switch (DEV_pci_rd_memtype (a20Addr)) {
        case 0x0:   // Read from ROM
          return ( (Bit8u *) & vector[a20Addr]);
        case 0x1:   // Read from ShadowRAM
          return( (Bit8u *) & shadow[a20Addr - 0xc0000]);
        default:
          BX_PANIC(("getHostMemAddr(): default case"));
          return(0);
        }
      }
#endif
    }
  else { // op == {BX_WRITE, BX_RW}
    Bit8u *retAddr;

    if ( (a20Addr < 0xa0000) || (a20Addr > 0xfffff) ) {
      retAddr = (Bit8u *) & vector[a20Addr];
      }
#if !BX_SUPPORT_PCI
    else
      return(NULL); // Vetoed!  Mem mapped IO (VGA) and ROMs
#else
    else if ( (a20Addr < 0xc0000) || (!bx_options.Oi440FXSupport->get ()) )
      return(NULL); // Vetoed!  Mem mapped IO (VGA) and ROMs
    else if (DEV_pci_wr_memtype (a20Addr) == 1) {
      // Write to ShadowRAM
      retAddr = (Bit8u *) & shadow[a20Addr - 0xc0000];
      }
    else
      return(NULL); // Vetoed!  ROMs
#endif

#if BX_SUPPORT_ICACHE
    cpu->iCache.decWriteStamp(cpu, a20Addr);
#endif

    return(retAddr);
    }
}


/*
 * One needs to provide both a read_handler and a write_handler.
 * XXX: maybe we should check for overlapping memory handlers
 */
  bx_bool 
BX_MEM_C::registerMemoryHandlers(memory_handler_t read_handler, void *read_param, 
		memory_handler_t write_handler, void *write_param, 
		unsigned long begin_addr, unsigned long end_addr)
{
	if (end_addr < begin_addr)
		return false;
	if (!read_handler)
		return false;
	if (!write_handler)
		return false;
	for (unsigned page_idx = begin_addr >> 20; page_idx <= end_addr >> 20; page_idx++) {
		struct memory_handler_struct *memory_handler = new struct memory_handler_struct;
		memory_handler->next = memory_handlers[page_idx];
		memory_handlers[page_idx] = memory_handler;
		memory_handler->read_handler = read_handler;
		memory_handler->write_handler = write_handler;
		memory_handler->read_param = read_param;
		memory_handler->write_param = write_param;
		memory_handler->begin = begin_addr;
		memory_handler->end = end_addr;
	}
	return true;
}


  bx_bool 
BX_MEM_C::unregisterMemoryHandlers(memory_handler_t read_handler, memory_handler_t write_handler, 
		unsigned long begin_addr, unsigned long end_addr)
{
	bx_bool ret = true;
	for (unsigned page_idx = begin_addr >> 20; page_idx <= end_addr >> 20; page_idx++) {
		struct memory_handler_struct *memory_handler = memory_handlers[page_idx];
		struct memory_handler_struct *prev = NULL;
		while (memory_handler && 
				memory_handler->read_handler != read_handler &&
			       	memory_handler->write_handler != write_handler && 
				memory_handler->begin != begin_addr && 
				memory_handler->end != end_addr) {
			prev = memory_handler;
			memory_handler = memory_handler->next;
		}
		if (!memory_handler) {
			ret = false; // we should have found it
			continue; // anyway, try the other pages
		}
		if (prev)
			prev->next = memory_handler->next;
		else
			memory_handlers[page_idx] = memory_handler->next;
		delete memory_handler;
	}
	return ret;
}
