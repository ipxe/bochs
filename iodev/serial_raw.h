/////////////////////////////////////////////////////////////////////////
// $Id: serial_raw.h,v 1.4 2004/01/18 11:58:07 vruppert Exp $
/////////////////////////////////////////////////////////////////////////
//

#if USE_RAW_SERIAL

#ifdef __linux__
#include <linux/serial.h>
#endif

#define P_EVEN 0
#define P_ODD 1
#define C_BREAK 201

class serial_raw : public logfunctions {
  public:
    serial_raw (char *devname);
    ~serial_raw (void);
    void set_baudrate (int rate);
    void set_data_bits (int );
    void set_stop_bits (int);
    void set_parity_mode (int, int);
    void transmit (int byte);
    void send_hangup ();
    int ready_transmit ();
    int ready_receive ();
    int receive ();
};
#endif
