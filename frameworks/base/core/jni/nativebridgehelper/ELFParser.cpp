#define LOG_TAG "NativeLibraryHelper"
//#define LOG_NDEBUG 0

#include <cutils/log.h>
#include <sys/stat.h>
#include "ELFParser.h"

namespace nativebridgehelper {

bool is_elf(const char* file_name) {
    struct stat statbuf;
    if (stat(file_name, &statbuf) < 0) {
        ALOGV("%s: No such file\n", file_name);
        return false;
    }

    if (!S_ISREG(statbuf.st_mode)) {
        ALOGV("%s: is not an ordinary file file\n", file_name);
        return false;
    }

    if (statbuf.st_size == 0) {
        ALOGV("%s: is an empty file\n", file_name);
        return false;
    }

    FILE* file = fopen(file_name, "rb");
    if (file == NULL) {
        ALOGV("%s: is not readable\n", file_name);
        return false;
    }

    char ident[EI_NIDENT];
    if (fread(ident, EI_NIDENT, 1, file) != 1) {
        ALOGV("%s: failed to read its magic number\n", file_name);
        fclose(file);
        return false;
    }

    if (ident[EI_MAG0] != ELFMAG0 &&
        ident[EI_MAG1] != ELFMAG1 &&
        ident[EI_MAG2] != ELFMAG2 &&
        ident[EI_MAG3] != ELFMAG3) {
        ALOGV("%s: it is not a .ELF file\n", file_name);
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

bool is_arm_elf(const char* file_name) {
    FILE* file = fopen(file_name, "rb");
    if (file == NULL) {
        ALOGV("%s: is not readable\n", file_name);
        return false;
    }

    if (fseek(file, ELF_MACHINE_OFFSET, SEEK_SET) != 0) {
        ALOGV("%s: seek failed\n", file_name);
        return false;
    }

    unsigned char machine[2];
    if (fread(machine, 2, 1, file) != 1) {
        ALOGV("%s: failed to read its machine code\n", file_name);
        fclose(file);
        return false;
    }

    // little endian
    unsigned machine_code = ((unsigned int)machine[0]) |
        (((unsigned int)machine[1]) << 8);
    if (machine_code != EM_ARM &&
        machine_code !=  EM_AARCH64 &&
        machine_code != EM_ARM184) {
        ALOGV("%s: it is not for ARM %d\n", file_name, machine_code);
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

bool is_elf_from_buffer(const char* buffer) {
    if (buffer[EI_MAG0] != ELFMAG0 &&
        buffer[EI_MAG1] != ELFMAG1 &&
        buffer[EI_MAG2] != ELFMAG2 &&
        buffer[EI_MAG3] != ELFMAG3) {
        ALOGV("it is not an .ELF file\n");
        return false;
    }
    return true;
}

bool is_arm_elf_from_buffer(const char* buffer) {
    // little endian
    unsigned machine_code = ((unsigned int)buffer[ELF_MACHINE_OFFSET]) |
        (((unsigned int)buffer[ELF_MACHINE_OFFSET + 1]) << 8);
    if (machine_code != EM_ARM &&
        machine_code !=  EM_AARCH64 &&
        machine_code != EM_ARM184) {
        return false;
    }
    ALOGV("it is for ARM %d\n", machine_code);
    return true;
}
}
