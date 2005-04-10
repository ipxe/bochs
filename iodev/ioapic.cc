/////////////////////////////////////////////////////////////////////////
// $Id: ioapic.cc,v 1.16 2005/04/10 19:42:48 sshwarts Exp $
/////////////////////////////////////////////////////////////////////////
//
#include <stdio.h>

#include "iodev.h"
#if BX_SUPPORT_APIC

class bx_ioapic_c bx_ioapic;
#define LOG_THIS  bx_ioapic.

void bx_io_redirect_entry_t::parse_value ()
{
  dest = (value >> 56) & APIC_ID_MASK;
  masked = (value >> 16) & 1;
  trig_mode = (value >> 15) & 1;
  remote_irr = (value >> 14) & 1;
  polarity = (value >> 13) & 1;
  //delivery_status = (value >> 12) & 1;
  delivery_status = 0;  // we'll change this later...
  dest_mode = (value >> 11) & 1;
  delivery_mode = (value >> 8) & 7;
  vector = value & 0xff;
}

void bx_io_redirect_entry_t::sprintf_self (char *buf)
{
  sprintf (buf, "dest=%02x, masked=%d, trig_mode=%d, remote_irr=%d, polarity=%d, delivery_status=%d, dest_mode=%d, delivery_mode=%d, vector=%02x", dest, masked, trig_mode, remote_irr, polarity, delivery_status, dest_mode, delivery_mode, vector);
}

bx_ioapic_c::bx_ioapic_c () 
  : bx_generic_apic_c ()
{
  put("IOAP");
  settype(IOAPICLOG);
}

bx_ioapic_c::~bx_ioapic_c () {}

void bx_ioapic_c::init () 
{
  bx_generic_apic_c::init ();
  BX_DEBUG(("initializing I/O APIC"));
  base_addr = 0xfec00000;
  set_id(BX_IOAPIC_DEFAULT_ID);
  ioregsel = 0;
  // all interrupts masked
  for (int i=0; i<BX_IOAPIC_NUM_PINS; i++) {
    ioredtbl[i].set_even_word (0x00010000);
    ioredtbl[i].set_odd_word  (0x00000000);
  }
  irr = 0;
}

void bx_ioapic_c::reset (unsigned type) 
{
}

void bx_ioapic_c::read_aligned(Bit32u address, Bit32u *data, unsigned len)
{
  BX_DEBUG( ("I/O APIC read_aligned addr=%08x, len=%d", address, len));
  BX_ASSERT (len == 4);
  address &= 0xff;
  if (address == 0x00) {
    // select register
    *data = ioregsel;
    return;
  } else if (address != 0x10) {
      BX_PANIC(("IOAPIC: read from unsupported address"));
  }
  // only reached when reading data register
  switch (ioregsel) {
  case 0x00:  // APIC ID, note this is 4bits, the upper 4 are reserved
    *data = ((id & APIC_ID_MASK) << 24);
    return;
  case 0x01:  // version
    *data = (((BX_IOAPIC_NUM_PINS-1) & 0xff) << 16) 
            | (BX_IOAPIC_VERSION_ID & 0x0f);
    return;
  case 0x02:
    BX_INFO(("IOAPIC: arbitration ID unsupported, returned 0"));
    *data = 0;
    return;
  default:
    int index = (ioregsel - 0x10) >> 1;
    if (index >= 0 && index < BX_IOAPIC_NUM_PINS) {
      bx_io_redirect_entry_t *entry = ioredtbl + index;
      *data = (ioregsel&1) ? entry->get_odd_word() : entry->get_even_word ();
      return;
    }
    BX_PANIC(("IOAPIC: IOREGSEL points to undefined register %02x", ioregsel));
  }
}

void bx_ioapic_c::write(Bit32u address, Bit32u *value, unsigned len)
{
  BX_DEBUG(("IOAPIC: write addr=%08x, data=%08x, len=%d", address, *value, len));
  address &= 0xff;
  if (address == 0x00)  {
    ioregsel = *value;
    return;
  } else if (address != 0x10) {
    BX_PANIC(("IOAPIC: write to unsupported address"));
  }
  // only reached when writing data register
  switch (ioregsel) {
    case 0x00: // set APIC ID
      {
	Bit8u newid = (*value >> 24) & APIC_ID_MASK;
	BX_INFO(("IOAPIC: setting id to 0x%x", newid));
	set_id (newid);
	return;
      }
    case 0x01: // version
    case 0x02: // arbitration id
      BX_INFO(("IOAPIC: could not write, IOREGSEL=0x%02x", ioregsel));
      return;
    default:
      int index = (ioregsel - 0x10) >> 1;
      if (index >= 0 && index < BX_IOAPIC_NUM_PINS) {
	bx_io_redirect_entry_t *entry = ioredtbl + index;
	if (ioregsel&1) 
	  entry->set_odd_word (*value);
	else 
	  entry->set_even_word (*value);
	char buf[1024];
	entry->sprintf_self (buf);
	BX_DEBUG(("IOAPIC: now entry[%d] is %s", index, buf));
	service_ioapic ();
	return;
      }
      BX_PANIC(("IOAPIC: IOREGSEL points to undefined register %02x", ioregsel));
  }
}

void bx_ioapic_c::raise_irq (unsigned vector, unsigned from) 
{
  BX_DEBUG(("IOAPIC: received vector %d", vector));
  if ((vector >= 0) && (vector <= BX_APIC_LAST_VECTOR)) {
    if (vector == 0) vector = 2;
    Bit32u bit = 1<<vector;
    irr |= bit;
    service_ioapic ();
  } else {
    BX_PANIC(("IOAPIC: vector %d out of range", vector));
  }
}

void bx_ioapic_c::lower_irq (unsigned num, unsigned from) 
{
  BX_DEBUG(("IOAPIC: interrupt %d went away", num));
}

void bx_ioapic_c::service_ioapic ()
{
  static unsigned int stuck = 0;
  // look in IRR and deliver any interrupts that are not masked.
  BX_DEBUG(("IOAPIC: servicing"));
  for (unsigned bit=0; bit < BX_IOAPIC_NUM_PINS; bit++) {
    if (irr & (1<<bit)) {
      bx_io_redirect_entry_t *entry = ioredtbl + bit;
      entry->parse_value();
      if (!entry->masked) {
	// clear irr bit and deliver
        BX_INFO (("dest=%02x, masked=%d, trig_mode=%d, remote_irr=%d, polarity=%d, delivery_status=%d, dest_mode=%d, delivery_mode=%d, vector=%02x", entry->dest, entry->masked, entry->trig_mode, entry->remote_irr, entry->polarity, entry->delivery_status, entry->dest_mode, entry->delivery_mode, entry->vector));
	bx_bool done = deliver (entry->dest, entry->dest_mode, entry->delivery_mode, entry->vector, entry->polarity, entry->trig_mode);
	if (done) {
	  irr &= ~(1<<bit);
	  entry->delivery_status = 0;
	  stuck = 0;
	} else {
	  entry->delivery_status = 1;
	  stuck++;
	  if (stuck > 5)
	    BX_INFO(("vector %#x stuck?\n", entry->vector));
	}
      }
    }
  }
}

#endif /* if BX_SUPPORT_APIC */
