/////////////////////////////////////////////////////////////////////////
// $Id: apic.cc,v 1.33 2004/10/13 20:58:15 sshwarts Exp $
/////////////////////////////////////////////////////////////////////////

#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#if BX_SUPPORT_APIC

#define LOG_THIS this->

bx_generic_apic_c *apic_index[APIC_MAX_ID];
bx_local_apic_c *local_apic_index[BX_LOCAL_APIC_NUM];

bx_generic_apic_c::bx_generic_apic_c ()
{
  id = APIC_UNKNOWN_ID;
  put("APIC?");
  settype(APICLOG);
  hwreset ();
}

bx_generic_apic_c::~bx_generic_apic_c () 
{
}

void bx_generic_apic_c::set_arb_id (int new_arb_id)
{
  // politely ignore it.  This gets sent to every APIC, regardless of its
  // type.
}

// init is called during RESET and when an INIT message is delivered.
void bx_generic_apic_c::init ()
{
}

void bx_local_apic_c::update_msr_apicbase(Bit32u newbase)
{
  Bit64u val64 = newbase;
  val64 <<= 12; /* push the APIC base address to bits 12:35 */
  val64 |= cpu->msr.apicbase & 0x0900; /* don't modify other apicbase or reserved bits */
  cpu->msr.apicbase = val64;
}

void bx_generic_apic_c::set_base (Bit32u newbase)
{
  BX_INFO(("relocate APIC id=%d to %8x", id, newbase));
  base_addr = newbase;
}

void bx_generic_apic_c::set_id (Bit8u newid) 
{
  // update apic_index
  if (id != APIC_UNKNOWN_ID) {
    BX_ASSERT (id < APIC_MAX_ID);
    if (apic_index[id] != this)
      BX_PANIC(("inconsistent APIC id table"));
    apic_index[id] = NULL;
  }
  id = newid;
  if (id != APIC_UNKNOWN_ID) {
    if (apic_index[id] != NULL)
      BX_PANIC(("duplicate APIC id assigned"));
    apic_index[id] = this;
  }
}

void bx_generic_apic_c::reset_all_ids ()
{
  for (int i=0; i<APIC_MAX_ID; i++) {
    if (apic_index[i]) 
      apic_index[i]->set_id (APIC_UNKNOWN_ID);
  }
}

char *bx_generic_apic_c::get_name ()
{
  BX_PANIC(("get_name called on bx_generic_apic_c base class"));
  return NULL;
}

bx_bool bx_generic_apic_c::is_selected (Bit32u addr, Bit32u len)
{
  if ((addr & ~0xfff) == get_base ()) {
    if ((addr & 0xf != 0) || (len != 4))
      BX_INFO(("warning: misaligned or wrong-size APIC write. addr=%08x, len=%d", addr, len));
    return true;
  }
  return false;
}

void
bx_generic_apic_c::read (Bit32u addr, void *data, unsigned len)
{
  if ((addr & ~0xf) != ((addr+len-1) & ~0xf))
    BX_PANIC(("APIC read spans 32-bit boundary"));
  Bit32u value;
  read_aligned (addr, &value, 4);
  if ((addr&3) == 0) {
    *((Bit32u *)data) = value;
    return;
  }
  // handle partial word read, independent of endian-ness.
  Bit8u bytes[4];
  bytes[0] = value & 0xff;
  bytes[1] = (value >> 8) & 0xff;
  bytes[2] = (value >> 16) & 0xff;
  bytes[3] = (value >> 24) & 0xff;
  Bit8u *p1 = bytes+(addr&3);
  Bit8u *p2 = (Bit8u *)data;
  for (unsigned i=0; i<len; i++) {
    if (bx_dbg.apic)
      BX_INFO(("apic: Copying byte %02x", (unsigned int) *p1));
    *p2++ = *p1++;
  }
}

void bx_generic_apic_c::read_aligned (Bit32u address, Bit32u *data, unsigned len)
{
  BX_PANIC(("read_aligned not implemented in base class bx_generic_apic_c"));
}

void bx_generic_apic_c::write(Bit32u address, Bit32u *value, unsigned len)
{
  BX_PANIC(("write not implemented in base class bx_generic_apic_c"));
}

void bx_generic_apic_c::startup_msg (Bit32u vector)
{
  BX_PANIC(("startup message sent to an I/O APIC"));
}

void bx_generic_apic_c::trigger_irq (unsigned num, unsigned from)
{
  BX_PANIC(("trigger_irq called on base class"));
}

void bx_generic_apic_c::untrigger_irq (unsigned num, unsigned from)
{
  BX_PANIC(("untrigger_irq called on base class"));
}

bx_bool bx_generic_apic_c::match_logical_addr (Bit8u address)
{
  BX_PANIC(("match_logical_addr called on base class"));
  return false;
}

bx_apic_type_t bx_generic_apic_c::get_type ()
{
  BX_PANIC(("get_type called on base class"));
  return APIC_TYPE_NONE;
}

/* apic_mask is the bitmask of apics allowed to arbitrate here */
int bx_generic_apic_c::apic_bus_arbitrate(Bit32u apic_mask)
{
  int winning_apr = 0, winning_id = 0, __apr, i;
  for (i = 0; i < BX_LOCAL_APIC_NUM; i++) {
    if (apic_mask & (1<<i)) {
      __apr = local_apic_index[i]->get_apr();
      if (__apr > winning_apr) {
        winning_apr = __apr;
	winning_id = i;
      }
    }
  }
  return winning_id;
}
 
/* get the CPU with the lowest arbitration ID */
int bx_generic_apic_c::apic_bus_arbitrate_lowpri(Bit32u apic_mask)
{
  // XXX initial winning_apr value, the datasheets say 15
  int winning_apr = APIC_MAX_ID, winning_id = 0 , __apr, i;
  for (i = 0; i < BX_LOCAL_APIC_NUM; i++) {
    if (apic_mask & (1<<i)) {
      __apr = local_apic_index[i]->get_apr_lowpri();
      if (__apr < winning_apr) {
	winning_apr = __apr;
	winning_id = i;
      }
    }
  }
  return winning_id;
}

void bx_generic_apic_c::arbitrate_and_trigger(Bit32u deliver_bitmask, Bit32u vector, Bit8u trigger_mode)
{
  int trigger_order[BX_LOCAL_APIC_NUM], winner, i, j;
#define TERMINATE_MAGIK	0x5a

  /* bus arbitrate ... */
  for (i = 0, j = 0; i < BX_LOCAL_APIC_NUM; i++) {
    if (deliver_bitmask & (1<<i)) {
      winner = apic_bus_arbitrate(deliver_bitmask);
      local_apic_index[winner]->adjust_arb_id(winner);
      trigger_order[j++] = winner;
    }
  }
  
  if (j < BX_LOCAL_APIC_NUM)
    trigger_order[j] = TERMINATE_MAGIK;

  i = 0;
  do {
    local_apic_index[trigger_order[i]]->trigger_irq(vector, trigger_order[i], trigger_mode);
    i++;
  } while (trigger_order[i] != TERMINATE_MAGIK);
}
 
void bx_generic_apic_c::arbitrate_and_trigger_one(Bit32u deliver_bitmask, Bit32u vector, Bit8u trigger_mode)
{
  int winner = apic_bus_arbitrate(deliver_bitmask);
  local_apic_index[winner]->adjust_arb_id(winner);
  local_apic_index[winner]->trigger_irq(vector, winner, trigger_mode);
}

Bit32u bx_generic_apic_c::get_delivery_bitmask (Bit8u dest, Bit8u dest_mode)
{
  int mask = 0;
  if (dest_mode == 0) {
    // physical 
    if (dest < APIC_MAX_ID)
      mask = 1<<dest;
    else if (dest == 0xff) {
      // physical destination 0xff means everybody. only local APICs can
      // send this.
      BX_ASSERT (get_type () == APIC_TYPE_LOCAL_APIC);
      mask = 0xff; 
    } else BX_PANIC(("bx_generic_apic_c::deliver: illegal physical destination %02x", dest));
  } else {
    // logical destination. call match_logical_addr for each APIC.
    if (dest == 0) return 0;
    for (int i=0; i<BX_LOCAL_APIC_NUM; i++) {
      if (local_apic_index[i]->match_logical_addr(dest))
        mask |= (1<<i);
    }
  }
  if (bx_dbg.apic)
    BX_INFO(("generic::get_delivery_bitmask returning 0x%04x", mask));
  return mask;
}

bx_bool bx_generic_apic_c::deliver (Bit8u dest, Bit8u dest_mode, Bit8u delivery_mode, Bit8u vector,
			    Bit8u polarity, Bit8u trig_mode)
{
  // return false if we can't deliver for any reason, so that the caller
  // knows not to clear its IRR.
  Bit32u deliver_bitmask = get_delivery_bitmask (dest, dest_mode);
  int lowest_priority = 0x100, lowest_mask = -1;
  // arbitrate by default
  int arbitrate = 1;
  int broadcast = (deliver_bitmask == BX_CPU_C::cpu_online_map);
  bx_bool once = false;
  int bit, i;

  // mask must include ONLY local APICs, or we will have problems.
  if (!deliver_bitmask) {
    BX_PANIC(("deliver failed for vector %02x: no APICs in destination bitmask", vector));
    return false;
  }
  switch (delivery_mode) {
    case APIC_DM_LOWPRI:
      /* fall through, we've already done low priority arbitration */
      arbitrate = 0;
    case APIC_DM_FIXED:
      /* once = false */
      break;
    case APIC_DM_INIT:
      // normal INIT IPI sent to processors
      for (i = 0; i < BX_LOCAL_APIC_NUM; i++) {
        if (deliver_bitmask & (1<<i))
	local_apic_index[i]->init();
      }
      // HACK! We need to do some IOAPIC init after the CPUs
      // are fired up
      apic_index[i]->init();
      return true;
    
    case APIC_DM_EXTINT:
      for (int i = 0; i < BX_LOCAL_APIC_NUM; i++)
      if (deliver_bitmask & (1<<i))
	local_apic_index[i]->bypass_irr_isr = true;
       break;
    case APIC_DM_SMI:
    case APIC_DM_NMI:
    case 3:  // reserved
      break;
    default:
      BX_PANIC(("APIC delivery mode %d not implemented", delivery_mode));
      return false;
  }
  // Fixed delivery mode
  if (bx_dbg.apic)
    BX_INFO(("delivered vector=0x%02x to bitmask=%04x", (int)vector, deliver_bitmask));

  // delivery only to one APIC
  if (once) {
    if (arbitrate)
      arbitrate_and_trigger_one(deliver_bitmask, vector, trig_mode);
    else {
      for (int i = 0; i < BX_LOCAL_APIC_NUM; i++) {
	if (deliver_bitmask & (1<<i)) {
	  local_apic_index[i]->trigger_irq(vector, i, trig_mode);
	}
	break;
      }
    }
  } else {
    if (arbitrate && !broadcast)
      arbitrate_and_trigger(deliver_bitmask, vector, trig_mode);
    else {
      for (int i = 0; i < BX_LOCAL_APIC_NUM; i++) {
	if (deliver_bitmask & (1<<i))
	  local_apic_index[i]->trigger_irq(vector, i, trig_mode);
      }
    }
  }
  return true;
}

bx_bool bx_local_apic_c::deliver (Bit8u dest, Bit8u dest_mode, Bit8u delivery_mode,
				  Bit8u vector, Bit8u polarity, Bit8u trig_mode)
{
  // In this function, implement only the behavior that is specific to
  // the local apic.  For general behavior of all apics, just send it to
  // the base class.
  Bit32u deliver_bitmask = get_delivery_bitmask (dest, dest_mode);
  int found_focus = 0;
  int broadcast = deliver_bitmask == BX_CPU_C::cpu_online_map;

  if (broadcast)
    BX_INFO(("Broadcast IPI for vector %#x delivery_mode %#x", vector, delivery_mode));

  switch (delivery_mode) {
  case APIC_DM_LOWPRI: // lowest priority of destinations
    // if we're focus processor, handle it, otherwise
    // look for the focus processor.
    dest = is_focus(vector) ? get_id() : 0;
    if (dest) break;
    for (int i = 0; i < BX_LOCAL_APIC_NUM; i++) {
      if (local_apic_index[i]->is_focus(vector) == true) {
        found_focus = 1;
        dest = i;
        break;	// stop scanning
      }
    }
    if (!found_focus) dest = apic_bus_arbitrate_lowpri(0xff);
    else return false;
    break;
  case APIC_DM_INIT:
    {
      int bit;
      int trig_mode = (icr_low >> 15) & 1;
      int level = (icr_low >> 14) & 1;
      if (level == 0 && trig_mode == 1) {
        // special mode in local apic.  See "INIT Level Deassert" in the
        // Intel Soft. Devel. Guide Vol 3, page 7-34.  This magic code
        // causes all APICs (regardless of dest address) to set their
        // arbitration ID to their APIC ID.
        BX_INFO (("INIT with Level&Deassert: synchronize arbitration IDs"));
        for (bit=0; bit<BX_LOCAL_APIC_NUM; bit++)
          local_apic_index[bit]->set_arb_id(local_apic_index[bit]->get_id());
        apic_index[bit]->set_arb_id(apic_index[bit]->get_id());	// HACK !!!
        return true;
      }
      break;	// we'll fall through to generic_deliver:case INIT
    }
  case APIC_DM_SIPI:  // Start Up (SIPI, local apic only)
    for (int bit=0; bit<BX_LOCAL_APIC_NUM; bit++) {
      if (deliver_bitmask & (1<<bit))
        local_apic_index[bit]->startup_msg(vector);
    }
    return true;
  default:
    break;
  }

   // not any special case behavior, just use generic apic code.
   return bx_generic_apic_c::deliver (dest, dest_mode, delivery_mode, vector, polarity, trig_mode);
}

bx_local_apic_c::bx_local_apic_c(BX_CPU_C *mycpu)
  : bx_generic_apic_c ()
{
  cpu = mycpu;
  hwreset ();
  INTR = 0;
}

void bx_local_apic_c::set_arb_id (int new_arb_id)
{
  BX_DEBUG (("set arbitration ID to %d", new_arb_id));
  arb_id = new_arb_id;
}

void bx_local_apic_c::hwreset () 
{
  /* same as INIT but also sets arbitration ID and APIC ID */
  init ();
  /* since id is set explicitly by the function that creates the CPU
     object, do not mess around with it */
  // id = APIC_UNKNOWN_ID;
  arb_id = id;
  BX_CPU_C::cpu_online_map |= (1 << id);
}

void bx_local_apic_c::init ()
{
  bx_generic_apic_c::init ();

  BX_INFO(("local apic in %s initializing", 
      (cpu && cpu->name) ? cpu->name : "?"));
  local_apic_index[id] = this;

  // default address for a local APIC, can be moved
  base_addr = APIC_BASE_ADDR;
  update_msr_apicbase(base_addr);
  bypass_irr_isr = false;
  err_status = 0;
  log_dest = 0;
  dest_format = 0xf;
  for (int bit=0; bit<BX_LOCAL_APIC_MAX_INTS; bit++) {
    irr[bit] = isr[bit] = tmr[bit] = 0;
  }
  icr_high = icr_low = log_dest = task_priority = 0;
  spurious_vec = 0xff;   // software disabled (bit 8)

  // KPL
  // Register a non-active timer for use when the timer is started.
  timer_handle = bx_pc_system.register_timer_ticks(this,
            BX_CPU(0)->local_apic.periodic_smf, 0, 0, 0, "lapic");
}

BX_CPU_C *bx_local_apic_c::get_cpu (Bit8u id)
{
  BX_ASSERT (id < APIC_MAX_ID);
  return cpu;
}

bx_local_apic_c::~bx_local_apic_c(void)
{
  // nothing for now
}

void bx_local_apic_c::set_id (Bit8u newid)
{
  bx_generic_apic_c::set_id (newid);
  local_apic_index[id] = this;
  sprintf (cpu->name, "CPU apicid=%02x", (Bit32u)id);
  if (id < APIC_MAX_ID) {
    char buffer[16];
    sprintf (buffer, "APIC%x", id);
    put(buffer);
    settype(CPU0LOG + id);
    sprintf (buffer, "CPU%x", id);
    cpu->put (buffer);
  } else {
    BX_INFO (("naming convention for apics requires id=0-%d only", APIC_MAX_ID));
  }
  if(BX_CPU_LEVEL<2)
    BX_INFO(( "8086" ));
  else
    BX_INFO(( "80%d86", BX_CPU_LEVEL ));
}

char *bx_local_apic_c::get_name()
{
  return cpu->name;
}

void bx_local_apic_c::set_divide_configuration (Bit32u value)
{
  BX_ASSERT (value == (value & 0x0b));
  // move bit 3 down to bit 0.
  value = ((value & 8) >> 1) | (value & 3);
  BX_ASSERT (value >= 0 && value <= 7);
  timer_divide_factor = (value==7)? 1 : (2 << value);
  BX_DEBUG(("%s: set timer divide factor to %d", cpu->name, timer_divide_factor));
}

void bx_local_apic_c::write (Bit32u addr, Bit32u *data, unsigned len)
{
  if (len != 4) {
    BX_PANIC (("local apic write with len=%d (should be 4)", len));
  }
  BX_DEBUG(("%s: write %08x to APIC address %08x", cpu->name, *data, addr));
  addr &= 0xff0;
  switch (addr) {
    case 0x20: // local APIC id
      id = ((*data)>>24) & APIC_ID_MASK;
      break;
    case 0x80: // task priority
      task_priority = *data & 0xff;
      break;
    case 0xb0: // EOI
      {
        BX_DEBUG(("%s: Wrote 0x%04x to EOI", cpu->name, *data));
        int vec = highest_priority_int (isr);
        if (vec < 0) {
          BX_INFO(("EOI written without any bit in ISR"));
        } else {
          BX_DEBUG(("%s: local apic received EOI, hopefully for vector 0x%02x", cpu->name, vec));
          isr[vec] = 0; 
          service_local_apic ();
        }
        if (bx_dbg.apic)
          print_status ();
      }
      break;
    case 0xd0: // logical destination
      log_dest = (*data >> 24) & APIC_ID_MASK;
      BX_DEBUG (("set logical destiation to %02x", log_dest));
      break;
    case 0xe0: // destination format
      dest_format = (*data >> 28) & 0xf;
      BX_DEBUG (("set destination format to %02x", dest_format));
      break;
    case 0xf0: // spurious interrupt vector
      spurious_vec = (spurious_vec & 0x0f) | (*data & 0x3f0);
      break;
    case 0x280: // error status reg
      // Here's what the IA-devguide-3 says on p.7-45:
      // The ESR is a read/write register and is reset after being written to
      // by the processor. A write to the ESR must be done just prior to
      // reading the ESR to allow the register to be updated.
      // This doesn't seem clear.  If the write clears the register, then
      // wouldn't you always read zero?  Otherwise, what does the write do?
      err_status = 0;
      break;
    case 0x300: // interrupt command reg 0-31
      {
        icr_low = *data & ~(1<<12);  // force delivery status bit = 0 (idle)
        int dest = (icr_high >> 24) & 0xff;
        int trig_mode = (icr_low >> 15) & 1;
        int level = (icr_low >> 14) & 1;
        int dest_mode = (icr_low >> 11) & 1;
        int delivery_mode = (icr_low >> 8) & 7;
        int vector = (icr_low & 0xff);
        //
        // deliver will call get_delivery_bitmask to decide who to send to.
        // This local_apic class redefines get_delivery_bitmask to 
        // implement the destination shorthand field, which doesn't exist
        // for all APICs.
        bx_bool accepted = 
           deliver (dest, dest_mode, delivery_mode, vector, level, trig_mode);
        if (!accepted)
          err_status |= APIC_ERR_TX_ACCEPT_ERR;
      }
      break;
    case 0x310: // interrupt command reg 31-63
      icr_high = *data & 0xff000000;
      break;
    case 0x320: // LVT Timer Reg
      lvt[APIC_LVT_TIMER] = *data & 0x310ff;
      break;
    case 0x330: // LVT Thermal Monitor
      lvt[APIC_LVT_THERMAL] = *data & 0x117ff;
      break;
    case 0x340: // LVT Performance Counter
      lvt[APIC_LVT_PERFORM] = *data & 0x117ff;
      break;
    case 0x350: // LVT LINT0 Reg
      lvt[APIC_LVT_LINT0] = *data & 0x1f7ff;
      break;
    case 0x360: // LVT Lint1 Reg
      lvt[APIC_LVT_LINT1] = *data & 0x1f7ff;
      break;
    case 0x370: // LVT Error Reg
      lvt[APIC_LVT_ERROR] = *data & 0x117ff;
      break;
    case 0x380: // initial count for timer
      {
      // If active before, deactive the current timer before changing it.
      if (timer_active)
        bx_pc_system.deactivate_timer(timer_handle);
      timer_initial = *data;
      if (timer_initial == 0)
        BX_PANIC(("APIC: W(init timer count): count=0"));
      // This should trigger the counter to start.  If already started,
      // restart from the new start value.
//fprintf(stderr, "APIC: W(Initial Count Register) = %u\n", *data);
      timer_current = timer_initial;
      timer_active = true;
//timer_divide_counter = 0; // KPL: delete this field.
      Bit32u timervec = lvt[APIC_LVT_TIMER];
      bx_bool continuous = (timervec & 0x20000) > 0;
      ticksInitial = bx_pc_system.getTicksTotal(); // Take a reading.
      bx_pc_system.activate_timer_ticks(timer_handle,
          Bit64u(timer_initial) * Bit64u(timer_divide_factor), continuous);
      }
      break;
    case 0x3e0: // timer divide configuration
      // only bits 3, 1, and 0 are writable
      timer_divconf = *data & 0xb;
      set_divide_configuration (timer_divconf);
      break;
    /* all read-only registers go here */
    case 0x30: // local APIC version
    case 0x90: // arbitration priority
    case 0xa0: // processor priority
    // ISRs not writable
    case 0x100: case 0x110: case 0x120: case 0x130:
    case 0x140: case 0x150: case 0x160: case 0x170:
    // TMRs not writable
    case 0x180: case 0x190: case 0x1a0: case 0x1b0:
    case 0x1c0: case 0x1d0: case 0x1e0: case 0x1f0:
    // IRRs not writable
    case 0x200: case 0x210: case 0x220: case 0x230:
    case 0x240: case 0x250: case 0x260: case 0x270:
      // current count for timer
    case 0x390:
      // all read-only registers should fall into this line
      BX_INFO(("warning: write to read-only APIC register 0x%02x", addr));
      break;
    default:
      err_status |= APIC_ERR_ILLEGAL_ADDR;
      // but for now I want to know about it in case I missed some.
      BX_PANIC(("APIC register %08x not implemented", addr));
  }
}

void bx_local_apic_c::startup_msg (Bit32u vector)
{
  if (cpu->debug_trap & 0x80000000) {
    cpu->debug_trap &= ~0x80000000;
    cpu->dword.eip = 0;
    cpu->load_seg_reg (&cpu->sregs[BX_SEG_REG_CS], vector*0x100);
    BX_INFO(("%s started up at 0x%x by APIC", cpu->name, cpu->dword.eip));
  } else {
    BX_INFO(("%s started up by APIC, but was not halted at the time", cpu->name));
  }
}

void bx_local_apic_c::read_aligned (Bit32u addr, Bit32u *data, unsigned len)
{
  if (len != 4) {
    BX_PANIC (("local apic write with len=%d (should be 4)", len));
  }
  *data = 0;  // default value for unimplemented registers
  Bit32u addr2 = addr & 0xff0;
  switch (addr2) {
  case 0x20: // local APIC id
    *data = (id) << 24; break;
  case 0x30: // local APIC version
    *data = 0x00170011; break;
  case 0x80: // task priority
    *data = task_priority & 0xff; break;
  case 0x90: // arbitration priority
    *data = get_apr (); break;
  case 0xa0: // processor priority
    *data = get_ppr (); break;
  case 0xb0: // EOI
    /*
     * Read-modify-write operations should operate without generating
     * exceptions, and are used by some operating systems to EOI.
     * The results of reads should be ignored by the OS.
     */
    break;
  case 0xd0: // logical destination
    *data = (log_dest & APIC_ID_MASK) << 24; break;
  case 0xe0: // destination format
    *data = ((dest_format & 0xf) << 24) | 0x0fffffff; break;
  case 0xf0: // spurious interrupt vector
    *data = spurious_vec; break;
  case 0x100: case 0x110:
  case 0x120: case 0x130:
  case 0x140: case 0x150:
  case 0x160: case 0x170:
    *data = isr[addr2-0x100];
    break;
  case 0x180: case 0x190:
  case 0x1a0: case 0x1b0:
  case 0x1c0: case 0x1d0:
  case 0x1e0: case 0x1f0:
    *data = tmr[addr2-0x180];
    break;
  case 0x200: case 0x210:
  case 0x220: case 0x230:
  case 0x240: case 0x250:
  case 0x260: case 0x270:
    *data = irr[addr2-0x200];
    break;
  case 0x280: // error status reg
    *data = err_status; break;
  case 0x300: // interrupt command reg 0-31
    *data = icr_low; break;
  case 0x310: // interrupt command reg 31-63
    *data = icr_high; break;
  case 0x320: // LVT Timer Reg
  case 0x330: // LVT Thermal Monitor
  case 0x340: // LVT Performance Counter
  case 0x350: // LVT LINT0 Reg
  case 0x360: // LVT Lint1 Reg
  case 0x370: // LVT Error Reg
    {
      int index = (addr2 - 0x320) >> 4;
      *data = lvt[index];
      break;
    }
  case 0x380: // initial count for timer
    *data = timer_initial;
//fprintf(stderr, "APIC: R(Initial Count Register) = %u\n", *data);
    break;
  case 0x390: // current count for timer
    {
    if (timer_active==0)
      *data = timer_current;
    else {
      Bit64u delta64;
      Bit32u delta32;
      delta64 = (bx_pc_system.time_ticks() - ticksInitial) / timer_divide_factor;
      delta32 = (Bit32u) delta64;
      if (delta32 > timer_initial)
        BX_PANIC(("APIC: R(curr timer count): delta < initial"));
      timer_current = timer_initial - delta32;
      *data = timer_current;
      }
//fprintf(stderr, "APIC: R(Current Count Register) = %u\n", *data);
    }
    break;
  case 0x3e0: // timer divide configuration
    *data = timer_divconf; break;
  default:
    BX_INFO(("APIC register %08x not implemented", addr));
  }
  if (bx_dbg.apic)
    BX_INFO(("%s: read from APIC address %08x = %08x", cpu->name, addr, *data));
}

int bx_local_apic_c::highest_priority_int (Bit8u *array)
{
  for (int i=0; i<BX_LOCAL_APIC_MAX_INTS; i++)
    if (array[i]) return i;
  return -1;
}

void bx_local_apic_c::service_local_apic ()
{
  if (bx_dbg.apic) {
    BX_INFO(("service_local_apic()"));
    print_status ();
  }
  if (INTR) return;  // INTR already up; do nothing
  // find first interrupt in irr.
  int first_irr = highest_priority_int (irr);
  int first_isr = highest_priority_int (isr);
  if (first_irr < 0) return;   // no interrupts, leave INTR=0
  if (first_isr >= 0 && first_irr >= first_isr) {
    if (bx_dbg.apic)
      BX_INFO(("local apic (%s): not delivering int%02x because int%02x is in service", cpu->name, first_irr, first_isr));
    err_status |= APIC_ERR_TX_ACCEPT_ERR;
    return;
  }
  // interrupt has appeared in irr.  raise INTR.  When the CPU
  // acknowledges, we will run highest_priority_int again and
  // return it.
  BX_DEBUG(("service_local_apic(): setting INTR=1 for vector 0x%02x", first_irr));
  INTR = 1;
  cpu->async_event = 1;
}

void bx_local_apic_c::trigger_irq (unsigned vector, unsigned from, unsigned trigger_mode)
{
  BX_DEBUG(("Local apic on %s: trigger interrupt vector=0x%x", cpu->name, vector));
  /* check for local/apic_index usage */
  BX_ASSERT(from == id);
  
  if (vector > BX_APIC_LAST_VECTOR) {
    err_status |= APIC_ERR_RX_ILLEGAL_VEC;
    BX_INFO(("bogus vector %#x,  ignoring", vector));
    return;
  }

  if (bx_dbg.apic)
    BX_INFO(("triggered vector %#02x", vector));

  if (bypass_irr_isr) {
    bypass_irr_isr = false;
    goto service_vector;
  }

  if (irr[vector] != 0) {
    err_status |= APIC_ERR_TX_ACCEPT_ERR;
    return;
  }

service_vector:
  irr[vector] = 1;
  tmr[vector] = trigger_mode;	// set for level triggered
  service_local_apic ();
}

void bx_local_apic_c::untrigger_irq (unsigned vector, unsigned from, unsigned trigger_mode)
{
  BX_DEBUG(("Local apic on %s: untrigger interrupt vector=0x%x", cpu->name, vector));
  // hardware says "no more".  clear the bit.  If the CPU hasn't yet
  // acknowledged the interrupt, it will never be serviced.
  BX_ASSERT (irr[vector] == 1);
  irr[vector] = 0;
  if (bx_dbg.apic) print_status ();
}

Bit8u bx_local_apic_c::acknowledge_int ()
{
  // CPU calls this when it is ready to service one interrupt
  if (!INTR)
    BX_PANIC(("%s: acknowledged an interrupt, but INTR=0", cpu->name));
  BX_ASSERT (INTR);
  int vector = highest_priority_int (irr);
  if (irr[vector] != 1) {
    BX_ERROR(("IRR was not 1! irr[%d]=%#x", vector, irr[vector]));
    irr[vector]=1;
  }
  BX_ASSERT (irr[vector] == 1);
  BX_DEBUG(("%s: acknowledge_int returning vector 0x%x", cpu->name, vector));
  // currently isr never gets cleared, so no point
  //BX_ASSERT (isr[vector] == 0);
  irr[vector] = 0;
  isr[vector] = 1;
  if (bx_dbg.apic) {
    BX_INFO(("Status after setting isr:"));
    print_status ();
  }
  INTR = 0;
  cpu->async_event = 1;
  service_local_apic ();  // will set INTR again if another is ready
  return vector;
}

void bx_local_apic_c::print_status ()
{
  BX_INFO(("%s local apic: status is {:", cpu->name));
  for (int vec=0; vec<BX_LOCAL_APIC_MAX_INTS; vec++) {
    if (irr[vec] || isr[vec]) {
      BX_INFO(("vec 0x%x: irr=%d, isr=%d", vec, (int)irr[vec], (int)isr[vec]));
    }
  }
  BX_INFO(("}"));
}

bx_bool bx_local_apic_c::match_logical_addr (Bit8u address) 
{
  if (dest_format != 0xf) {
    BX_PANIC(("bx_local_apic_c::match_logical_addr: cluster model addressing not implemented"));
  }
  bx_bool match = ((address & log_dest) != 0);
  BX_DEBUG (("%s: comparing MDA %02x to my LDR %02x -> %s", cpu->name,
    address, log_dest, match? "Match" : "Not a match"));
  return match;
}

Bit32u bx_local_apic_c::get_delivery_bitmask (Bit8u dest, Bit8u dest_mode)
{
  int dest_shorthand = (icr_low >> 18) & 3;
  Bit32u mask;

  switch (dest_shorthand) {
  case 0:  // no shorthand, use real destination value
    mask = bx_generic_apic_c::get_delivery_bitmask (dest, dest_mode);
    break;
  case 1:  // self
    mask = (1<<id);
    break;
  case 2:  // all including self
    mask = BX_CPU_C::cpu_online_map;
    break;
  case 3:  // all but self
    mask = BX_CPU_C::cpu_online_map & ~(1<<id);
    break;
  default:
    BX_PANIC(("Invalid desination shorthand %#x\n", dest_shorthand));
  }

  BX_DEBUG (("local::get_delivery_bitmask returning 0x%04x shorthand=%#x", mask, dest_shorthand));
  if (mask == 0)
    BX_INFO((">>WARNING<< returning a mask of 0x0, dest=%#x dest_mode=%#x", dest, dest_mode));

  return mask;
}

Bit8u bx_local_apic_c::get_ppr ()
{
  Bit32u tpr = (task_priority >> 4) & 0xf;		/* we want 7:4 */
  Bit32u isrv = (highest_priority_int(isr) >> 4) & 0xf;	/* ditto */

  if (tpr >= isrv)
    proc_priority = task_priority & 0xff;
  else
    proc_priority = isrv << 4;	/* low 4 bits of PPR have to be cleared */

  if (bx_dbg.apic)
    BX_DEBUG(("%s: get_ppr returning %#x", cpu->name, proc_priority));

  return (Bit8u) proc_priority;
}

Bit8u bx_local_apic_c::get_tpr ()
{
  return task_priority;
}

void  bx_local_apic_c::set_tpr (Bit8u priority)
{
  task_priority = priority;
}

Bit8u bx_local_apic_c::get_apr ()
{
  return arb_id;
}

Bit8u bx_local_apic_c::get_apr_lowpri()
{
  Bit32u tpr = (task_priority >> 4) & 0xf;
  Bit32u isrv = (highest_priority_int(isr) >> 4) & 0xf;
  Bit32u irrv = (highest_priority_int(irr) >> 4) & 0xf;

  if ((tpr >= irrv) && (tpr > isrv))
    arb_id = task_priority & 0xff;
  else
    arb_id = ((tpr && isrv) > irrv) ? (tpr && isrv) : irrv;

  BX_INFO(("apr = %d\n", arb_id));
  return (Bit8u)arb_id;
}

bx_bool bx_local_apic_c::is_focus(Bit32u vector)
{
  return (irr[vector] || isr[vector]) ? true : false;
}

void bx_local_apic_c::adjust_arb_id(int winning_id)
{
  int __apr, __win_apr;
  // adjust arbitration priorities
  for (int i = 0; i < BX_LOCAL_APIC_NUM; i++) {
    if (i != winning_id) {
      __apr = local_apic_index[i]->get_apr();
      if (__apr == 15) {
        __win_apr = local_apic_index[winning_id]->get_apr();
        local_apic_index[i]->set_arb_id(__win_apr+1);
      } 
      else
        local_apic_index[i]->set_arb_id(__apr+1);
    } else // the winner drops to lowest
      local_apic_index[winning_id]->set_arb_id(0);
  }
}

void bx_local_apic_c::periodic_smf(void *this_ptr)
{
  bx_local_apic_c *class_ptr = (bx_local_apic_c *) this_ptr;

  class_ptr->periodic();
}

void bx_local_apic_c::periodic(void) // KPL: changed prototype
{
  if (!timer_active) {
    BX_PANIC(("%s: bx_local_apic_c::periodic called, timer_active==0", cpu->name));
    return;
  }
  BX_DEBUG(("%s: bx_local_apic_c::periodic called", cpu->name));

  // timer reached zero since the last call to periodic.
  Bit32u timervec = lvt[APIC_LVT_TIMER];
  if (timervec & 0x20000) {
    // Periodic mode.
    // If timer is not masked, trigger interrupt.
    if ((timervec & 0x10000)==0)
      trigger_irq (timervec & 0xff, id, APIC_EDGE_TRIGGERED);
    // Reload timer values.
    timer_current = timer_initial;
    ticksInitial = bx_pc_system.getTicksTotal(); // Take a reading.
    BX_DEBUG(("%s: local apic timer (periodic) triggered int, reset counter to 0x%08x", cpu->name, timer_current));
  }
  else {
    // one-shot mode
    timer_current = 0;
    // If timer is not masked, trigger interrupt.
    if ((timervec & 0x10000)==0)
      trigger_irq (timervec & 0xff, id, APIC_EDGE_TRIGGERED);
    timer_active = false;
    BX_DEBUG (("%s: local apic timer (one-shot) triggered int", cpu->name));
    bx_pc_system.deactivate_timer(timer_handle); // Make sure.
  }
}
#endif /* if BX_SUPPORT_APIC */
