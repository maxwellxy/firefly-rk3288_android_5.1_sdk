/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include <hardware/memtrack.h>

#define LOG_TAG "memtrack"
#include <cutils/log.h>

#include "memtrack_rk3288.h"

#define ION_DBGFS_VMALLOC    "/sys/kernel/debug/ion/heaps/vmalloc"
#define ION_DBGFS_CMA        "/sys/kernel/debug/ion/heaps/cma"

static struct memtrack_record record_templates[] = {
    {
        .flags = MEMTRACK_FLAG_SMAPS_ACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
    {
        .flags = MEMTRACK_FLAG_SMAPS_UNACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
};

static int ion_memtrack_get_memory_fs(const char* dbg_fs, pid_t pid,
                             struct memtrack_record *records,
                             size_t allocated_records)
{
    FILE *fp;
    char line[1024];

    fp = fopen(dbg_fs, "r");
    if (fp == NULL) {
        return -errno;
    }

    while (fgets(line, sizeof(line), fp)) {
        /* Format:
    	       client		   pid		   size
         ----------------------------------------------------
    	  mediaserver		   141		 520192
           surfaceflinger		   138	       25165824
           surfaceflinger		   138	      155402240
    	  mediaserver		   141	       92004352
    		rk_fb		     1	       38797312
         ----------------------------------------------------
         orphaned allocations (info is from last known client):
           surfaceflinger		   138		2359296 0 1	   
         */
        unsigned int allocated;
        int line_pid;

        int ret = sscanf(line, "%*s %u %u\n", &line_pid, &allocated);
        ALOGV("ret = %d, line_pid = %u, allocated = %u\n", ret, line_pid, allocated);
        if (ret == 2 && line_pid == pid) {
            if (allocated_records > 0) {
        	    records[0].size_in_bytes = 0;
            }
            if (allocated_records > 1) {
        	    records[1].size_in_bytes += allocated;
            }
            ALOGV("pid(%u) records[1].size_in_bytes=%u", line_pid, records[1].size_in_bytes);
        }
    }

    fclose(fp);
    return 0;
}

int ion_memtrack_get_memory(pid_t pid, int type,
                             struct memtrack_record *records,
                             size_t *num_records)
{
    size_t allocated_records = min(*num_records, ARRAY_SIZE(record_templates));

    *num_records = ARRAY_SIZE(record_templates);

    ALOGV("%s : pid(%d), type(%d), num_records(%d), allocated_records(%d)\n",
          __func__, pid, type, *num_records, allocated_records);

    /* fastpath to return the necessary number of records */
    if (allocated_records == 0) {
        return 0;
    }

    memcpy(records, record_templates,
           sizeof(struct memtrack_record) * allocated_records);

    ion_memtrack_get_memory_fs(ION_DBGFS_CMA, pid, records, allocated_records);
    ion_memtrack_get_memory_fs(ION_DBGFS_VMALLOC, pid, records, allocated_records);

    return 0;
}
