/*
 * nvidia-installer: A tool for installing NVIDIA software packages on
 * Unix and Linux systems.
 *
 * Copyright (C) 2003 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>.
 *
 *
 * sanity.c
 */

#include "nvidia-installer.h"
#include "command-list.h"
#include "user-interface.h"
#include "backup.h"
#include "misc.h"
#include "sanity.h"

#include <sys/types.h>
#include <sys/shm.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


/*
 * sanity() - perform sanity tests on an existing installation
 */

int sanity(Options *op)
{
    char *descr, *version;
    int ret;

    /* check that there's a driver installed at all */

    ret = get_installed_driver_version_and_descr(op, &version, &descr);
    
    if (!ret) {
        ui_error(op, "Unable to find any installed NVIDIA driver.  The sanity "
                 "check feature is only intended to be used with an existing "
                 "NVIDIA driver installation.");
        return FALSE;
    }

    ui_message(op, "The currently installed driver is: '%s' "
               "(version: %s).  nvidia-installer will now check "
               "that all installed files still exist.",
               descr, version);

    /* check that all the files are still where we placed them */

    if (!test_installed_files(op)) {
        ui_message(op, "The '%s' installation has been altered "
                   "since it was originally installed.  It is recommended "
                   "that you reinstall.", descr);
        return FALSE;
    }

    /* check that shared memory works */

    if (!check_sysvipc(op)) return FALSE;

    /*
     * XXX There are lots of additional tests that could be added:
     *
     * - check for any conflicting libraries
     *
     * - check that the permissions on the /dev/nvidia* files haven't
     *   been screwed up by pam
     *
     * - check that /dev/zero has appropriate permissions
     *
     * - check for possible kernel config problems (IPC, mtrr support,
     *   etc).
     */
    
    ui_message(op, "'%s' (version: %s) appears to be installed "
               "correctly.", descr, version);
    
    nvfree(descr);
    nvfree(version);

    return TRUE;
    
} /* sanity() */


/*
 * check_sysvipc() - test that shmat() and friends work
 */

int check_sysvipc(Options *op)
{
    int shmid = -1;
    int ret = FALSE;
    int size = sysconf(_SC_PAGESIZE);
    void *address = (void *) -1;

    shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
    if (shmid == -1) goto done;

    address = shmat(shmid, 0, 0);
    if (address == (void *) -1) goto done;

    ret = TRUE;

 done:

    if (shmid != -1) shmctl(shmid, IPC_RMID, 0);
    if (address != (void *) -1) shmdt(address);

    if (ret) {
        ui_log(op, "Shared memory test passed.");
    } else {
        ui_message(op, "Shared memory test failed (%s): please check that "
                   "your kernel has CONFIG_SYSVIPC enabled.", strerror(errno));
    }
    
    return ret;

} /* check_sysvipc() */
