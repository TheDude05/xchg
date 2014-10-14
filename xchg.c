#define _GNU_SOURCE

#define VERSION "0.1.0"

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sysexits.h>
#include <string.h>
#include <fcntl.h>

#define RENAME_NOREPLACE    1   /* Don't overwrite target */
#define RENAME_EXCHANGE     2   /* Exchange source and dest */

int renameat2(int olddir, const char *oldname,
                int newdir, const char *newname, unsigned int flags)
{
    return syscall(SYS_renameat2, olddir, oldname, newdir, newname, flags);
}

int main(int argc, char *argv[])
{
    int c, verbose, numfiles;

    while ((c = getopt(argc, argv, "vV")) != -1) {
        switch (c) {
        case 'v':
            verbose = 1;
            break;
        case 'V':
            printf("Version: %s\n", VERSION);
            return EX_OK;
        default:
            return EX_USAGE;
        }
    }

    numfiles = argc - optind;
    if (numfiles > 2) {
        printf("Too many file arguments provided\n");
        return EX_USAGE;
    } else if (numfiles < 2) {
        printf("Must provide source and destination\n");
        return EX_USAGE;
    }

    if (verbose)
        printf("Exchanging %s with %s\n", argv[optind], argv[optind+1]);


    if (renameat2(AT_FDCWD, argv[optind],
                    AT_FDCWD, argv[optind+1], RENAME_EXCHANGE) == -1) {
        if (errno == ENOSYS) {
            // If we get here its likely because 1) the kernel does not support
            // renameat2 or 2) the underlying filesystem does not support
            // atomic exchange. We should provide more information
            printf("Cannot exchange files\n\nConfirm your kernel "
                    "supports the `renameat2` syscall *and* that "
                    "your filesystem supports atomic exchange");
            return EX_OSERR;
        } else {
            printf("Cannot exchange files. Error %d - %s\n",
                            errno, strerror(errno));
            return EX_OSERR;
        }
    }

    return EX_OK;
}
