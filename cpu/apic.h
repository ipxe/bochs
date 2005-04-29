/////////////////////////////////////////////////////////////////////////
// $Id: apic.h,v 1.18 2005/04/29 18:38:35 sshwarts Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
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
//
/////////////////////////////////////////////////////////////////////////

#ifndef BX_CPU_APIC_H
#  define BX_CPU_APIC_H 1

typedef enum {
  APIC_TYPE_NONE,
  APIC_TYPE_IOAPIC,
  APIC_TYPE_LOCAL_APIC
} bx_apic_type_t;

#define APIC_BASE_ADDR    0xfee00000  // default APIC address

// todo: Pentium APIC_VERSION_ID (Pentium has 3 LVT entries)
/*
#if BX_CPU_LEVEL == 6 && BX_SUPPORT_SSE >= 2
#  define APIC_VERSION_ID 0x00050014  // P4 has 6 LVT entries
#else
#  define APIC_VERSION_ID 0x00040010  // P6 has 4 LVT entries
#endif
*/

// Currenly support only P6 Local APIC
#define APIC_VERSION_ID 0x00040010    // P6 has 4 LVT entries

#if BX_SUPPORT_APIC

class BOCHSAPI bx_generic_apic_c : public logfunctions {
protected:
  bx_address base_addr;
  Bit8u id;
#define APIC_UNKNOWN_ID 0xff
public:
  bx_generic_apic_c ();
  virtual ~bx_generic_apic_c () { }
  virtual void init ();
  virtual void reset () { }
  bx_address get_base (void) const { return base_addr; }
  void set_base (bx_address newbase);
  void set_id (Bit8u newid);
  Bit8u get_id () const { return id; }
  static void reset_all_ids ();
  virtual char *get_name();
  bx_bool is_selected (bx_address addr, Bit32u len);
  void read (Bit32u addr, void *data, unsigned len);
  virtual void read_aligned(Bit32u address, Bit32u *data, unsigned len) = 0;
  virtual void write(Bit32u address, Bit32u *value, unsigned len) = 0;
  virtual Bit32u get_delivery_bitmask (Bit8u dest, Bit8u dest_mode);
  virtual bx_bool deliver (Bit8u dest, Bit8u dest_mode, Bit8u delivery_mode, Bit8u vector, Bit8u level, Bit8u trig_mode);
  virtual bx_bool match_logical_addr (Bit8u address) = 0;
  virtual bx_apic_type_t get_type () = 0;
  int apic_bus_arbitrate(Bit32u apic_mask);
  int apic_bus_arbitrate_lowpri(Bit32u apic_mask);
  void arbitrate_and_trigger(Bit32u deliver_bitmask, Bit32u vector, Bit8u trigger_mode);
  void arbitrate_and_trigger_one(Bit32u deliver_bitmask, Bit32u vector, Bit8u trigger_mode);
};

#define BX_LOCAL_APIC_NUM	BX_SMP_PROCESSORS
#define BX_APIC_FIRST_VECTOR	0x10
#define BX_APIC_LAST_VECTOR	0xfe
#define BX_LOCAL_APIC_MAX_INTS  256

#define APIC_LEVEL_TRIGGERED	1
#define APIC_EDGE_TRIGGERED	0

class BOCHSAPI bx_local_apic_c : public bx_generic_apic_c 
{
  Bit32u task_priority;         // Task priority (TPR)
  Bit32u arb_id;                // Arbitration priority (APR)
  Bit32u log_dest;              // Logical destination (LDR)
  Bit32u dest_format;           // Destination format (DFR)
  Bit32u spurious_vector;       // Spurious interrupt vector register

  // ISR=in-service register.  When an IRR bit is cleared, the corresponding
  // bit in ISR is set.
  Bit8u isr[BX_LOCAL_APIC_MAX_INTS];
  // TMR=trigger mode register.  Cleared for edge-triggered interrupts
  // and set for level-triggered interrupts.  If set, local APIC must send
  // EOI message to all other APICs.  EOI's are not implemented.
  Bit8u tmr[BX_LOCAL_APIC_MAX_INTS];
  // IRR=interrupt request register.  When an interrupt is triggered by
  // the I/O APIC or another processor, it sets a bit in irr.  The bit is
  // cleared when the interrupt is acknowledged by the processor.
  Bit8u irr[BX_LOCAL_APIC_MAX_INTS];

#define APIC_ERR_ILLEGAL_ADDR    0x80
#define APIC_ERR_RX_ILLEGAL_VEC  0x40
#define APIC_ERR_TX_ILLEGAL_VEC  0x20
#define APIC_ERR_RX_ACCEPT_ERR   0x08
#define APIC_ERR_TX_ACCEPT_ERR   0x04
#define APIC_ERR_RX_CHECKSUM     0x02
#define APIC_ERR_TX_CHECKSUM     0x01

  Bit32u error_status;          // Error status Register (ESR)
  Bit32u icr_hi;                // Interrupt command register (ICR)
  Bit32u icr_lo;

#define APIC_LVT_ENTRIES 6
  Bit32u lvt[6];
#define APIC_LVT_TIMER   0
#define APIC_LVT_THERMAL 1
#define APIC_LVT_PERFORM 2
#define APIC_LVT_LINT0   3
#define APIC_LVT_LINT1   4
#define APIC_LVT_ERROR   5

  Bit32u timer_initial;         // Initial timer count
  Bit32u timer_current;         // current timer count

  Bit32u timer_divconf;         // Timer divide configuration register
  Bit32u timer_divide_counter, timer_divide_factor;

  // Internal timer state, not accessible from bus
  bx_bool timer_active;
  int timer_handle;
  Bit64u ticksInitial;

/* APIC delivery modes */
#define APIC_DM_FIXED	0
#define APIC_DM_LOWPRI	1
#define APIC_DM_SMI	2
/* RESERVED		3 */
#define APIC_DM_NMI	4
#define APIC_DM_INIT	5
#define APIC_DM_SIPI	6
#define APIC_DM_EXTINT	7

  BX_CPU_C *cpu;

  // corresponding BX_CPU_ID for the local APIC
  unsigned cpu_id;

public:
  bx_bool INTR;
  bx_bool bypass_irr_isr;
  bx_local_apic_c(BX_CPU_C *cpu);
  virtual ~bx_local_apic_c(void) { }
  virtual void reset ();
  virtual void init ();
  BX_CPU_C *get_cpu () { return cpu; }
  void set_id (Bit8u newid);   // redefine to set cpu->name
  virtual char *get_name();
  virtual void write (Bit32u addr, Bit32u *data, unsigned len);
  virtual void read_aligned(Bit32u address, Bit32u *data, unsigned len);
  void startup_msg (Bit32u vector);
  // on local APIC, trigger means raise the CPU's INTR line. For now
  // I also have to raise pc_system.INTR but that should be replaced
  // with the cpu-specific INTR signals.
  void trigger_irq (unsigned num, unsigned from, unsigned trigger_mode);
  void untrigger_irq (unsigned num, unsigned from, unsigned trigger_mode);
  Bit8u acknowledge_int ();  // only the local CPU should call this
  int highest_priority_int (Bit8u *array);
  void receive_EOI(Bit32u value);
  void service_local_apic ();
  void print_status ();
  virtual bx_bool match_logical_addr (Bit8u address);
  virtual bx_bool is_local_apic () const { return 1; }
  virtual bx_apic_type_t get_type () { return APIC_TYPE_LOCAL_APIC; }
  virtual Bit32u get_delivery_bitmask (Bit8u dest, Bit8u dest_mode);
  virtual bx_bool deliver (Bit8u destination, Bit8u dest_mode, Bit8u delivery_mode, Bit8u vector, Bit8u level, Bit8u trig_mode);
  Bit8u get_ppr ();
  Bit8u get_tpr ();
  void  set_tpr (Bit8u tpr);
  Bit8u get_apr ();
  Bit8u get_apr_lowpri();
  bx_bool is_focus(Bit32u vector);
  void adjust_arb_id(int winning_id);	// adjust the arbitration id after a bus arbitration
  static void periodic_smf(void *); // KPL
  void periodic(void); // KPL
  void set_divide_configuration (Bit32u value);
  void set_arb_id (int newid);
};

// For P6 and Pentium family processors the local APIC ID feild is 4 bits.
#define APIC_MAX_ID  0xf
#define APIC_ID_MASK 0xf

extern bx_generic_apic_c *apic_index[APIC_MAX_ID];
extern bx_local_apic_c *local_apic_index[BX_LOCAL_APIC_NUM];

#endif // if BX_SUPPORT_APIC

#endif
