/////////////////////////////////////////////////////////////////////////
// $Id: crc32.h,v 1.4 2004/09/05 10:30:18 vruppert Exp $
/////////////////////////////////////////////////////////////////////////
//
/* CRC-32 calculator
 * Adapted from http://www.createwindow.org/programming/crc32/
 */

#ifndef _CRC_32_H_
#define _CRC_32_H_

#include "../config.h"

class CRC_Generator {
private:
  Bit32u crc32_table[256];
  Bit32u reflect(Bit32u ref, Bit8u ch);
public:
  void init(void);
  CRC_Generator();
  Bit32u get_CRC(Bit8u * buf, Bit32u buflen);
};

#endif //_CRC_32_H_

