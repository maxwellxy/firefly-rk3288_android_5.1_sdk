/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "install.h"
#include "mincrypt/rsa.h"
#include "minui/minui.h"
#include "minzip/SysUtil.h"
#include "minzip/Zip.h"
#include "mtdutils/mounts.h"
#include "mtdutils/mtdutils.h"
#include "roots.h"
#include "verifier.h"
#include "ui.h"
#include "bootloader.h"

#ifdef USE_RADICAL_UPDATE
#include "radical_update.h"
#endif

extern RecoveryUI* ui;

#define ASSUMED_UPDATE_BINARY_NAME  "META-INF/com/google/android/update-binary"
#define PUBLIC_KEYS_FILE "/res/keys"

// Default allocation of progress bar segments to operations
static const int VERIFICATION_PROGRESS_TIME = 60;
static const float VERIFICATION_PROGRESS_FRACTION = 0.25;
static const float DEFAULT_FILES_PROGRESS_FRACTION = 0.4;
static const float DEFAULT_IMAGE_PROGRESS_FRACTION = 0.1;

extern bool bNeedClearMisc;
#ifdef USE_BOARD_ID
extern "C" int custom();
extern "C" int restore();
// static bool gIfBoardIdCustom = false;
bool gIfBoardIdCustom = false;      // 标识之后是否还要执行 board_id 定制. 在对 system_partition restore 之后置位. 
#endif

// If the package contains an update binary, extract it and run it.
// @param path
//      待安装的 ota_pkg 的路径字串. 
// @param zip
//      关联到 'path' 目标文件的 ZipArchive 实例的指针. 
// @param wipe_cache
//      传入 install 流程的 log file 的路径. 
// 
static int
try_update_binary(const char *path, ZipArchive *zip, int* wipe_cache) {
    const ZipEntry* binary_entry =
            mzFindZipEntry(zip, ASSUMED_UPDATE_BINARY_NAME);
    if (binary_entry == NULL) {
        mzCloseZipArchive(zip);
        return INSTALL_CORRUPT;
    }

    const char* binary = "/tmp/update_binary";
    unlink(binary);
    int fd = creat(binary, 0755);
    if (fd < 0) {
        mzCloseZipArchive(zip);
        LOGE("Can't make %s\n", binary);
        return INSTALL_ERROR;
    }
    bool ok = mzExtractZipEntryToFile(zip, binary_entry, fd);
    close(fd);
    mzCloseZipArchive(zip);

    if (!ok) {
        LOGE("Can't copy %s\n", ASSUMED_UPDATE_BINARY_NAME);
        return INSTALL_ERROR;
    }

    int pipefd[2];
    pipe(pipefd);

    // When executing the update binary contained in the package, the
    // arguments passed are:
    //
    //   - the version number for this interface
    //
    //   - an fd to which the program can write in order to update the
    //     progress bar.  The program can write single-line commands:
    //
    //        progress <frac> <secs>
    //            fill up the next <frac> part of of the progress bar
    //            over <secs> seconds.  If <secs> is zero, use
    //            set_progress commands to manually control the
    //            progress of this segment of the bar
    //
    //        set_progress <frac>
    //            <frac> should be between 0.0 and 1.0; sets the
    //            progress bar within the segment defined by the most
    //            recent progress command.
    //
    //        firmware <"hboot"|"radio"> <filename>
    //            arrange to install the contents of <filename> in the
    //            given partition on reboot.
    //
    //            (API v2: <filename> may start with "PACKAGE:" to
    //            indicate taking a file from the OTA package.)
    //
    //            (API v3: this command no longer exists.)
    //
    //        ui_print <string>
    //            display <string> on the screen.
    //
    //   - the name of the package zip file.
    //

    const char** args = (const char**)malloc(sizeof(char*) * 5);
    args[0] = binary;
    args[1] = EXPAND(RECOVERY_API_VERSION);   // defined in Android.mk
    char* temp = (char*)malloc(10);
    sprintf(temp, "%d", pipefd[1]);
    args[2] = temp;
    args[3] = (char*)path;
    args[4] = NULL;

    pid_t pid = fork();
    if (pid == 0) {
        umask(022);
        close(pipefd[0]);
        execv(binary, (char* const*)args);
        fprintf(stdout, "E:Can't run %s (%s)\n", binary, strerror(errno));
        _exit(-1);
    }
    close(pipefd[1]);

    *wipe_cache = 0;

    char buffer[1024];
    FILE* from_child = fdopen(pipefd[0], "r");
    while (fgets(buffer, sizeof(buffer), from_child) != NULL) {
        char* command = strtok(buffer, " \n");
        if (command == NULL) {
            continue;
        } else if (strcmp(command, "progress") == 0) {
            char* fraction_s = strtok(NULL, " \n");
            char* seconds_s = strtok(NULL, " \n");

            float fraction = strtof(fraction_s, NULL);
            int seconds = strtol(seconds_s, NULL, 10);

            ui->ShowProgress(fraction * (1-VERIFICATION_PROGRESS_FRACTION), seconds);
        } else if (strcmp(command, "set_progress") == 0) {
            char* fraction_s = strtok(NULL, " \n");
            float fraction = strtof(fraction_s, NULL);
            ui->SetProgress(fraction);
        } else if (strcmp(command, "ui_print") == 0) {
            char* str = strtok(NULL, "\n");
            if (str) {
                ui->Print("%s", str);
            } else {
                ui->Print("\n");
            }
            fflush(stdout);
        } else if (strcmp(command, "wipe_cache") == 0) {
            *wipe_cache = 1;
        } else if (strcmp(command, "clear_display") == 0) {
            ui->SetBackground(RecoveryUI::NONE);
        } else if (strcmp(command, "enable_reboot") == 0) {
            // packages can explicitly request that they want the user
            // to be able to reboot during installation (useful for
            // debugging packages that don't exit).
            ui->SetEnableReboot(true);
        } else {
            LOGE("unknown command [%s]\n", command);
        }
    }
    fclose(from_child);

    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        LOGE("Error in %s\n(Status %d)\n", path, WEXITSTATUS(status));
        return INSTALL_ERROR;
    }

    return INSTALL_SUCCESS;
}

/**
 * install_package() 的实现主体.
 * @param path
 *      待安装的 ota_pkg 的路径字串. 
 * @param install_file
 *      传入 install 流程的 log file 的路径. 
 * @param is_ru_pkg
 *      当前待安装的 ota_pkg 是否是 ru_pkg.
 */
static int
really_install_package(const char *path, int* wipe_cache, bool needs_mount, int is_ru_pkg)
{
    int ret = INSTALL_SUCCESS;
	//by mmk@rock-chips.com
	//if update loader, we hope not clear misc command.
	//default not clear misc command, let the update-script of update.zip to clear misc when no update loader.
	bNeedClearMisc = false;
    ui->SetBackground(RecoveryUI::INSTALLING_UPDATE);
    ui->Print("Finding update package...\n");
    // Give verification half the progress bar...
    ui->SetProgressType(RecoveryUI::DETERMINATE);
    ui->ShowProgress(VERIFICATION_PROGRESS_FRACTION, VERIFICATION_PROGRESS_TIME);
    LOGI("Update location: %s\n", path);

    // Map the update package into memory.
    ui->Print("Opening update package...\n");

	needs_mount = true;
    if (path && needs_mount) {
        if (path[0] == '@') {
            ensure_path_mounted(path+1);
        } else {
            ensure_path_mounted(path);
        }
    }

    MemMapping map;
    if (sysMapFile(path, &map) != 0) {
        LOGE("failed to map file\n");
        return INSTALL_CORRUPT;
    }

    int numKeys;
    Certificate* loadedKeys = load_keys(PUBLIC_KEYS_FILE, &numKeys);
    if (loadedKeys == NULL) {
        LOGE("Failed to load keys\n");
        return INSTALL_CORRUPT;
    }
    LOGI("%d key(s) loaded from %s\n", numKeys, PUBLIC_KEYS_FILE);

    ui->Print("Verifying update package...\n");

    int err;
    err = verify_file(map.addr, map.length, loadedKeys, numKeys);
    free(loadedKeys);
    LOGI("verify_file returned %d\n", err);
    if (err != VERIFY_SUCCESS) {
        LOGE("signature verification failed\n");
        sysReleaseMap(&map);
        return INSTALL_CORRUPT;
    }

    /* Try to open the package.
     */
    ZipArchive zip;
    err = mzOpenZipArchive(map.addr, map.length, &zip);
    if (err != 0) {
        LOGE("Can't open %s\n(%s)\n", path, err != -1 ? strerror(err) : "bad");
        sysReleaseMap(&map);
        return INSTALL_CORRUPT;
    }

#ifdef USE_BOARD_ID
    ensure_path_mounted("/cust");
    ensure_path_mounted("/system");
    D("to restore system_partition.");
    restore();

    gIfBoardIdCustom = true;
#endif

#ifdef USE_RADICAL_UPDATE
    
    // .KP : restore_fw_in_ota_ver : 
    // 无论安装 ru_pkg 还是 original_ota_pkg 之前, 都要 restore 可能的 backup_of_fws_in_ota_ver. 
    // 这样即便连续多次安装 ru_pkg, /radical_update/backup_of_fws_in_ota_ver 中保存的都是 fws_in_ota_ver. 
    // 才能保证, 后续若安装 ota_diff_pkg, 不会失败. 

    if ( 0 != ensure_path_mounted(RU_PARTITION_MOUNT_PATH) )
    {
        SET_ERROR_AND_JUMP("fail to mount ru_partition", ret, INSTALL_ERROR, EXIT);
    }
    if ( 0 != ensure_path_mounted(SYSTEM_PARTITION_MOUNT_PATH) )
    {
        SET_ERROR_AND_JUMP("fail to mount system_partition", ret, INSTALL_ERROR, EXIT);
    }

    if ( RadicalUpdate_isApplied() )
    {
        I("a ru_pkg is applied, to restore backup_of_fws_in_ota_ver to system_partition.");
        CHECK_FUNC_CALL( RadicalUpdate_restoreFirmwaresInOtaVer() , ret, EXIT);
    }
    else 
    {
        D("no ru_pkg is applied.");
    }
    
    if ( 0 != ensure_path_unmounted(RU_PARTITION_MOUNT_PATH) )
    {
        SET_ERROR_AND_JUMP("fail to unmount ru_partition", ret, INSTALL_ERROR, EXIT);
    }
    if ( 0 != ensure_path_unmounted(SYSTEM_PARTITION_MOUNT_PATH) )
    {
        SET_ERROR_AND_JUMP("fail to unmount system_partition", ret, INSTALL_ERROR, EXIT);
    }
#endif

    /* Verify and install the contents of the package.
     */
    ui->Print("Installing update...\n");
    ui->SetEnableReboot(false);
    ret = try_update_binary(path, &zip, wipe_cache);
    ui->SetEnableReboot(true);
    ui->Print("\n");

    sysReleaseMap(&map);

EXIT:
    return ret;
}

/**
 * 安装指定路径的 ota_pkg.
 * @param path
 *      待安装的 ota_pkg 的路径字串. 
 * @param wipe_cache
 *      用于返回后续是否要 wipe cache 分区. 
 * @param install_file
 *      传入 install 流程的 log file 的路径. 
 * @param is_ru_pkg
 *      标识 'path' 指定的 ota_pkg 是否是 ru_pkg. 
 */
int
install_package(const char* path, int* wipe_cache, const char* install_file,
                bool needs_mount, int is_ru_pkg)
{
    FILE* install_log = fopen_path(install_file, "w");
    int ret = INSTALL_SUCCESS;

    if (install_log) {
        fputs(path, install_log);
        fputc('\n', install_log);
    } else {
        LOGE("failed to open last_install: %s\n", strerror(errno));
    }

#ifdef USE_BOARD_ID
	gIfBoardIdCustom = false;
#endif

    int result;
    if (setup_install_mounts() != 0) {
        LOGE("failed to set up expected mounts for install; aborting\n");
        result = INSTALL_ERROR;
    } else {
        result = really_install_package(path, wipe_cache, needs_mount, is_ru_pkg);
    }
    if (install_log) {
        fputc(result == INSTALL_SUCCESS ? '1' : '0', install_log);
        fputc('\n', install_log);
        fclose(install_log);
    }

#ifdef USE_BOARD_ID
    if(gIfBoardIdCustom) {
    	ensure_path_mounted("/cust");
    	ensure_path_mounted("/system");
        D("to custom system_partition for board_id");
    	custom();
    }
#endif

#ifdef USE_RADICAL_UPDATE
    if ( is_ru_pkg ) {
        I("installed a ru_pkg, to apply radical_update to system_partition.");

        if ( 0 != ensure_path_mounted(RU_PARTITION_MOUNT_PATH) )
        {
            SET_ERROR_AND_JUMP("fail to mount ru_partition.", ret, INSTALL_ERROR, EXIT);
        }
        if ( 0 != ensure_path_mounted(SYSTEM_PARTITION_MOUNT_PATH) )
        {
            SET_ERROR_AND_JUMP("fail to mount system_partition.", ret, INSTALL_ERROR, EXIT);
        }

        if ( 0 != RadicalUpdate_tryToApplyRadicalUpdate() )
        {
            SET_ERROR_AND_JUMP("fail to apply radical_update.", ret, INSTALL_ERROR, EXIT);
        }
        
        D("to delete ru_pkg '%s' after being applied.", path);
        unlink(path);
        
        if ( 0 != ensure_path_unmounted(RU_PARTITION_MOUNT_PATH) )
        {
            SET_ERROR_AND_JUMP("fail to unmount ru_partition.", ret, INSTALL_ERROR, EXIT);
        }
        if ( 0 != ensure_path_unmounted(SYSTEM_PARTITION_MOUNT_PATH) )
        {
            SET_ERROR_AND_JUMP("fail to unmount system_partition.", ret, INSTALL_ERROR, EXIT);
        }
    }
    else {
        I("installed an original_ota_pkg, "
            "do not apply radical_update to system_partition, "
            "there might be incompatibility between modules in ru_ver and ota_ver.");
    }
#endif

EXIT:
    return result;
}
