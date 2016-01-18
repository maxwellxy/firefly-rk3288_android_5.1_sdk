
#ifndef __INV_USTORE_IO_H__
#define __INV_USTORE_IO_H__

#include "mltypes.h"

/* Only ustore_manager.c is allowed to use these functions. */

inv_error_t inv_ustore_open(void);
inv_error_t inv_ustore_close(void);


inv_error_t inv_uload_open(void);
inv_error_t inv_uload_close(void);

inv_error_t inv_ustoreload_set_max_len(int);
void inv_ustoreload_reset_len(void);


#endif // __INV_USTORE_IO_H__
