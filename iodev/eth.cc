/////////////////////////////////////////////////////////////////////////
// $Id: eth.cc,v 1.12 2002/04/18 00:59:58 bdenney Exp $
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

// eth.cc  - helper code to find and create pktmover classes

// Peter Grehan (grehan@iprg.nokia.com) coded all of this
// NE2000/ether stuff.

#include "bochs.h"
#define LOG_THIS /* not needed */

eth_locator_c *eth_locator_c::all;

//
// Each pktmover module has a static locator class that registers
// here 
//
eth_locator_c::eth_locator_c(const char *type)
{
  next = all;
  all  = this;
  this->type = type;
}

#ifdef ETH_NULL
extern class bx_null_locator_c bx_null_match;
#endif
#ifdef ETH_FBSD
extern class bx_fbsd_locator_c bx_fbsd_match;
#endif
#ifdef ETH_LINUX
extern class bx_linux_locator_c bx_linux_match;
#endif
#ifdef ETH_WIN32
extern class bx_win32_locator_c bx_win32_match;
#endif
#if HAVE_ETHERTAP
extern class bx_tap_locator_c bx_tap_match;
#endif
#if HAVE_TUNTAP
extern class bx_tuntap_locator_c bx_tuntap_match;
#endif
#ifdef ETH_TEST
extern bx_test_match;
#endif
#ifdef ETH_ARPBACK
extern class bx_arpback_locator_c bx_arpback_match;
#endif

//
// Called by ethernet chip emulations to locate and create a pktmover
// object
//
eth_pktmover_c *
eth_locator_c::create(const char *type, const char *netif,
		      const char *macaddr,
		      eth_rx_handler_t rxh, void *rxarg)
{
#ifdef eth_static_constructors
  for (eth_locator_c *p = all; p != NULL; p = p->next) {
    if (strcmp(type, p->type) == 0)
      return (p->allocate(netif, macaddr, rxh, rxarg));
  }
#else
  eth_locator_c *ptr = 0;

#ifdef ETH_ARPBACK
  {
    if (!strcmp(type, "arpback"))
      ptr = (eth_locator_c *) &bx_arpback_match;
  }
#endif
#ifdef ETH_NULL
  {
    if (!strcmp(type, "null"))
      ptr = (eth_locator_c *) &bx_null_match; 
  }
#endif
#ifdef ETH_FBSD
  {
    if (!strcmp(type, "fbsd"))    
      ptr = (eth_locator_c *) &bx_fbsd_match;
  }
#endif
#ifdef ETH_LINUX
  {
    if (!strcmp(type, "linux"))    
      ptr = (eth_locator_c *) &bx_linux_match;
  }
#endif
#if HAVE_TUNTAP
  {
    if (!strcmp(type, "tuntap"))    
      ptr = (eth_locator_c *) &bx_tuntap_match;
  }
#endif
#if HAVE_ETHERTAP
  {
    if (!strcmp(type, "tap"))    
      ptr = (eth_locator_c *) &bx_tap_match;
  }
#endif
#ifdef ETH_WIN32
  {
    if(!strcmp(type, "win32"))
      ptr = (eth_locator_c *) &bx_win32_match;
  }
#endif
#ifdef ETH_TEST
  {
    if (!strcmp(type, "test"))    
      ptr = (eth_locator_c *) &bx_test_match;
  }
#endif
  if (ptr)
    return (ptr->allocate(netif, macaddr, rxh, rxarg));
#endif

  return (NULL);
}
