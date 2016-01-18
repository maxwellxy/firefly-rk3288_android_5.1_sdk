#ifndef _ELFPARSER_H_
#define _ELFPARSER_H_

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace nativebridgehelper {

#define EI_NIDENT   16   /* Size of e_ident[] */

/* Fields in e_ident[]
 */
#define EI_MAG0     0    /* File identification byte 0 index */
#define ELFMAG0     0x7F /* Magic number byte 0 */
#define EI_MAG1     1    /* File identification byte 1 index */
#define ELFMAG1     'E'  /* Magic number byte 1 */
#define EI_MAG2     2    /* File identification byte 2 index */
#define ELFMAG2     'L'  /* Magic number byte 2 */
#define EI_MAG3     3    /* File identification byte 3 index */
#define ELFMAG3     'F'  /* Magic number byte 3 */

/* according to implementations of ELF Header
 *     unsigned char e_ident[16];        // ELF "magic number"
 *     unsigned char e_type[2];          // Identifies object file type
 *     unsigned char e_machine[2];       // Specifies required architecture
 */
#define ELF_MACHINE_OFFSET  18

/* Values for e_machine, which identifies the architecture.  These numbers
 * are officially assigned by registry@sco.com.  See below for a list of
 * ad-hoc numbers used during initial development.
 * Please always sync them.
 */
#define EM_386        3 /* Intel 80386 */
#define EM_486        6 /* Intel 80486 *//* Reserved for future use */
#define EM_860        7 /* Intel 80860 */
#define EM_960       19 /* Intel 80960 */
#define EM_ARM       40 /* ARM */
#define EM_IA_64     50 /* Intel IA-64 Processor */
#define EM_8051     165 /* Intel 8051 and variants */
#define EM_L1OM     180 /* Intel L1OM */
#define EM_K1OM     181 /* Intel K1OM */
#define EM_INTEL182 182 /* Reserved by Intel */
#define EM_AARCH64  183 /* ARM 64-bit architecture */
#define EM_ARM184   184 /* Reserved by ARM */
#define EM_INTEL205 205 /* Reserved by Intel */
#define EM_INTEL206 206 /* Reserved by Intel */
#define EM_INTEL207 207 /* Reserved by Intel */
#define EM_INTEL208 208 /* Reserved by Intel */
#define EM_INTEL209 209 /* Reserved by Intel */

    bool is_elf(const char* file_name);
    bool is_arm_elf(const char* file_name);
    bool is_elf_from_buffer(const char* buffer);
    bool is_arm_elf_from_buffer(const char* buffer);
}
#endif
