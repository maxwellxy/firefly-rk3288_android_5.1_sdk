/*
 * eMMC 5.0+ Specific tools
 *
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>    /* for memset() */
#include <errno.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <ctype.h>

#include "mmc.h"
#include "mmc_cmds.h"



int do_emmc5_fw_update(int fd, __u8 *cid, __u8 *ext_csd,
                               char *emmc_fw, size_t fwsize)
{
       struct mmc_ioc_cmd idata;
       int retcode = 0;
       int ret;

       /* 1) Host send CMD6 to set MODE_CONFIG[30] = 0x01 */
       memset(&idata, 0, sizeof(idata));
       idata.opcode = MMC_SWITCH;
        idata.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
                       (30 << 16) |    /* index */
                       (1 << 8) |      /* value */
                       EXT_CSD_CMD_SET_NORMAL;
       idata.flags = MMC_RSP_R1B | MMC_CMD_AC;;
       ret = ioctl(fd, MMC_IOC_CMD, &idata);
       if (ret) {
               retcode = ret;
               printf("ioctl: MMC_SWITCH (eMMC 5.0 FW Update) %m\n");
               goto abort_update;
       }

       /* 2) send CMD25 0x0000FFFF */
       memset(&idata, 0, sizeof(idata));
       idata.write_flag = 1;
       idata.data_ptr = (__u64) ((unsigned long) emmc_fw);  /* Write this FW */
       idata.opcode = MMC_WRITE_MULTIPLE_BLOCK;
       idata.arg = 0x0000FFFF;
       idata.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;;
       ret = ioctl(fd, MMC_IOC_CMD, &idata);
       if (ret) {
               retcode = ret;
               printf("ioctl:MMC_WRITE_MULTIPLE_BLOCK (eMMC 5.0 FW Update) %m\n");
               goto abort_update;
       }

       /* 3) check FFU_STATUS[26] */
       ret = read_extcsd(fd, ext_csd);
       if (ret) {
               retcode = ret;
               fprintf(stderr, "read_extcsd error: %m\n", ret);
               goto abort_update;
       }

       switch(ext_csd[26]) {
       case 0:  break;
       case 0x10:
               fprintf(stderr, "eMMC 5.0 FFU had general error and failed (EIO).\n");
               retcode = -EIO;
               break;
       case 0x11:
               fprintf(stderr, "eMMC 5.0 FFU did not complete (EAGAIN).\n");
               retcode = -EAGAIN;
               break;
       case 0x12:
               fprintf(stderr, "eMMC 5.0 FFU download failed: checksum "
                       "mismatch or was interrupted (EINTR).\n");
               retcode = -EINTR;
               break;
       default:
               fprintf(stderr, "eMMC 5.0 FFU unknown error: %x\n", ext_csd[26]);
               retcode = -EIO;
       }

       /* 4) check NUMBER_OF_FW_SECTORS_CORRECTLY_PROGRAMMED[305-302] */
       ret = ext_csd[302] << 0 | (ext_csd[303] << 8) |
               (ext_csd[304] << 16) | (ext_csd[305] << 24);

       /* convert that gibberish to bytes */
       ret *= 512;
       if (ext_csd[61] == 1)
               ret *= 8;       /* 4K sectors */

       if (ret != fwsize) {
               fprintf(stderr, "eMMC 5.0 FFU Failed? (fwsize: %d,"
                       " PROGRAMMED_SECTORS: %d)\n", fwsize, ret);
       }

abort_update:
       /* 4) send CMD0
        * This should reset the device and force use of the new firmware.
        */
       memset(&idata, 0, sizeof(idata));
       idata.opcode = MMC_GO_IDLE_STATE;
       ret = ioctl(fd, MMC_IOC_CMD, &idata);
       if (ret) {
               retcode = ret;
               perror("ioctl:MMC_GO_IDLE_STATE (eMMC 5.0 FFU)");
       }

       return retcode;
}
