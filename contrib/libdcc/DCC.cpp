/**
 *  Cortex-M DCC implementation
 *
 *  All rights reserved.
 *  Elliot Buller
 *  2014
 */
#include <stddef.h>
#include <string.h>

#include <interface/iCharDevice.h>
#include <leos/reg.h>
#include <leos/delay.h>

// Use DCRDR for DCC communications
typedef struct  __attribute__((__packed__)) {
  union {
    struct {
      uint8_t T2HF;   // Flag
      uint8_t T2HD;   // Data
    };
    uint16_t T2H;     // Address atomically as 16 bits
  };
  uint8_t H2TF;       // Flag
  uint8_t H2TD;       // Data
} dcc_reg_t;

#define DHCSR    ((uint32_t *)0xe000edf0)
#define DCRDR    ((uint32_t *)0xe000edf8)

// Flags
#define DBG_ATTACHED     1
#define BUSY             1
#define SET_LENGTH(x)    ((x & 0xffff) << 16)
#define GET_LENGTH(x)    ((x >> 16) & 0xffff)
#define SET_DATA(x)      ((x & 0xff) << 8)

// Acknowledge data read by clearing flag
#define ACK()            le_reg_write_u8(&reg->H2TF, 0)
#define WAIT_DATA_MS(ms) while ((le_reg_read_u8(&reg->H2TF) & 1) == 0) le_delay_ms (ms)

// Types of data streams
typedef enum {
  DCC_TYPE_TRACEMSG = 0,   // Trace msg
  DCC_TYPE_DBGMSG_ASCII,   // ASCII msg
  DCC_TYPE_DBGMSG_HEX = 1, // Hex msg
  DCC_TYPE_DBGMSG_CHAR,    // Alt ascii msg
  DCC_TYPE_RAWPIPE,        // Raw pipe (sent to DCC server)
  DCC_TYPE_MAX_ENUM,
} dcc_stream_t;


class DCC : public iCharDevice {
 private:
  bool               enabled;
  dcc_reg_t *reg;
  
 public:
  DCC (const char *name);
  virtual ~DCC (void) {}

  // iCharDevice interface
  int Write (const void *buf, int len);
  int Read (void *buf, int len);

  // iModule interface
  int OnLoad (const char *arg);
  void OnUnload (void) {}

  // Helper function
  void Write_u32 (uint32_t data);
};

DCC::DCC (const char *name)
{
  reg = (dcc_reg_t *)DCRDR;
}

void DCC::Write_u32 (uint32_t data)
{
  int len = 4;

  while (len--) {
    
    /* Wait for ack that prev data was read */
    while (le_reg_read_u8(&reg->T2HF) & BUSY);

    /* Write data and set flag */
    le_reg_write_u16 (&reg->T2H, (uint16_t)(SET_DATA(data) | BUSY));
    data >>= 8;
  }
}

int DCC::Write (const void *buf, int len)
{
  int written = 0;
  const uint8_t *dbuf = (const uint8_t *)buf;
  uint32_t data;

  // Ignore if no debugger attached
  if (!enabled)
    return len;

  // Write length and indicate message type
  Write_u32 (SET_LENGTH(len) | DCC_TYPE_RAWPIPE);

  // Write message out
  while (written < len) {

    // Combine bytes into word
    data = dbuf[0] |
      ((len > 1) ? dbuf[1] << 8 : 0) |
      ((len > 2) ? dbuf[2] << 16 : 0) |
      ((len > 3) ? dbuf[3] << 24 : 0);

    // Write data out, mark as available
    Write_u32 (data);
    
    // Increment pointers
    dbuf += 4;
    written += 4;
  }
  return len;
}

int DCC::Read (void *buf, int len)
{
  int rlen, i;
  uint8_t *dbuf = (uint8_t *)buf;

  if (!enabled)
    return -1;

  // Wait until data is available
  WAIT_DATA_MS (10);

  // Read len
  rlen = le_reg_read_u8 (&reg->H2TD) << 8;
  ACK (); WAIT_DATA_MS (1);
  rlen |= le_reg_read_u8 (&reg->H2TD);
  ACK ();

  // Read data
  for (i = 0; (i < len) && (i < rlen); i++) {

    // Wait for data
    WAIT_DATA_MS (1);

    // Save byte
    dbuf[i] = le_reg_read_u8 (&reg->H2TD);

    // Ack data read
    ACK ();
  }

  return (len < rlen) ? len : rlen;
}

int DCC::OnLoad (const char *arg)
{
  // Init DCC hardware, detect if debugger is present
  if (le_reg_read_u32 (DHCSR) & DBG_ATTACHED) {
    enabled = true;
    
    // Clear DCC register
    le_reg_write_u32 (reg, 0);
  }
  else
    enabled = false;

  return 0;
}

SW_DRIVER (DCC);
EXPORT_MODULE (DCC, "DCC", NULL, MOD_VERSION (0, 0, 1));
