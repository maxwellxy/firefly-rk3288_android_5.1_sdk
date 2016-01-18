/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/******************************************************************************
 *
 * $Id: fopenCMake.c 5629 2011-06-11 03:13:08Z mcaramello $
 *
 *****************************************************************************/

#include <string.h>

#include "fopenCMake.h"
#include "pathConfigure.h"

/**
 *  @brief  Replacement for fopen that concatenates the location of the 
 *          source tree onto the filename path. 
 *          It looks in 3 locations:
 *              - in the current directory, 
 *              - then it looks in "..", 
 *              - lastly in the define UNITTEST_SOURCE_DIR which 
 *                gets defined by CMake.
 * @param filename 
 *              Filename relative to base of source directory.
 * @param prop 
 *              Second argument to fopen.
 */
FILE *fopenCMake(const char *filename, const char *prop)
{
    char path[150];
    FILE *file;

    // Look first in current directory
    file = fopen(filename, prop);
    if (file == NULL) {
        // Now look in ".."
#ifdef WIN32
        strcpy(path, "..\\");
#else
        strcpy(path, "../");
#endif
        strcat(path, filename);
        file = fopen(path, prop);
        if (file == NULL) {
            // Now look in definition by CMake
            strcpy(path, PATH_SOURCE_DIR);
            strcat(path, filename);
            file = fopen(path, prop);
        }
    }
    return file;
}


