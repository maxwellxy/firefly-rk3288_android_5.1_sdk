#ifndef IPC_SOCKET_H
#define IPC_SOCKET_H

#include <stdint.h>
#include "brcm_marshall.h"
#include "lbs.h"
#include "brcm_types.h"

int brcmipc_connect(const char* path);

/** Transmit marshaled object                                                          
*   \param fd    - file descriptor for ipc socket - no handle* for easier reuse in srv & cli
*   \param pkg   - marshaled object to be transmitted                                  
*   \return      - BRCM_LBS_OK (1) if successful, else BRCM_LBS_ERROR_LBS_INVALID (-1)
********************************************************************************************/

BrcmLbs_Result brcmipc_send(OsHandle lbs, uint8_t *data, size_t datalen);

#endif

