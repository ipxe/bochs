/////////////////////////////////////////////////////////////////////////
// $Id: pcipnic.h,v 1.3.2.1 2004/11/05 00:56:46 slechta Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2003  Fen Systems Ltd.
//  http://www.fensystems.co.uk/
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

#include "pnic_api.h"

#if BX_USE_PCIPNIC_SMF
#  define BX_PNIC_SMF  static
#  define BX_PNIC_THIS thePNICDevice->
#  define BX_PNIC_THIS_PTR thePNICDevice
#else
#  define BX_PNIC_SMF
#  define BX_PNIC_THIS this->
#  define BX_PNIC_THIS_PTR this
#endif

#define PNIC_DATA_SIZE	4096
#define PNIC_RECV_RINGS 4

typedef struct {

  Bit32u	base_ioaddr;
  Bit8u		macaddr[6];
  Bit8u		irqEnabled;

  Bit16u	rCmd;		// Command register
  Bit16u	rStatus;	// Status register
  Bit16u	rLength;	// Length register
  Bit8u		rData[PNIC_DATA_SIZE];  // Data register array
  Bit16u	rDataCursor;

  int		recvIndex;
  int		recvQueueLength;
  Bit8u		recvRing[PNIC_RECV_RINGS][PNIC_DATA_SIZE]; // Receive buffer
  Bit16u	recvRingLength[PNIC_RECV_RINGS];

  Bit8u devfunc;
  Bit8u pci_conf[256];

} bx_pnic_t;


class bx_pcipnic_c : public bx_devmodel_c
{
public:
  bx_pcipnic_c(void);
  ~bx_pcipnic_c(void);
  virtual void   init(void);
  virtual void   reset(unsigned type);

#if BX_SAVE_RESTORE
  virtual void register_state(sr_param_c *list_p);
  virtual void before_save_state () {};
  virtual void after_restore_state () {};
#endif // #if BX_SAVE_RESTORE

private:

  bx_pnic_t s;

  static void set_irq_level(bx_bool level);

  static void pnic_timer_handler(void *);
  void pnic_timer(void);

  static Bit32u read_handler(void *this_ptr, Bit32u address, unsigned io_len);
  static void   write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len);
  static Bit32u pci_read_handler(void *this_ptr, Bit8u address, unsigned io_len);
  static void   pci_write_handler(void *this_ptr, Bit8u address, Bit32u value, unsigned io_len);
#if !BX_USE_PCIPNIC_SMF
  Bit32u read(Bit32u address, unsigned io_len);
  void   write(Bit32u address, Bit32u value, unsigned io_len);
  Bit32u pci_read(Bit8u address, unsigned io_len);
  void   pci_write(Bit8u address, Bit32u value, unsigned io_len);
#endif

  eth_pktmover_c *ethdev;
  static void exec_command(void);
  static void rx_handler(void *arg, const void *buf, unsigned len);
  BX_PNIC_SMF void rx_frame(const void *buf, unsigned io_len);

};
