#define _GNU_SOURCE

#define VERSION "0.1.0"

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sysexits.h>
#include <string.h>
#include <fcntl.h>

//TODO Replace this with <linux/fs.h>
#define RENAME_NOREPLACE    1   /* Don't overwrite target */
#define RENAME_EXCHANGE     2   /* Exchange source and dest */

int renameat2(int olddir, const char *oldname,
                int newdir, const char *newname, unsigned int flags)
{
    return syscall(SYS_renameat2, olddir, oldname, newdir, newname, flags);
}

int do_exchange(char *src, char *dst)
{
    //TODO(ajw) What does this do to timestamps? (i.e mtime, atime, etc)
    //TODO(ajw) What about extended attributes like "immutable"?
    return renameat2(AT_FDCWD, src, AT_FDCWD, dst, RENAME_EXCHANGE);
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
        fprintf(stderr, "Too many file arguments provided\n");
        return EX_USAGE;
    } else if (numfiles < 2) {
        fprintf(stderr, "Must provide source and destination\n");
        return EX_USAGE;
    }

    if (verbose)
        printf("Exchanging %s <-> %s\n", argv[optind], argv[optind+1]);

    if (do_exchange(argv[optind], argv[optind+1]) != 0) {
        if (errno == ENOSYS) {
            // If we get here its likely because the kernel does not support
            // renameat2
            fprintf(stderr, "Cannot exchange files\n\nConfirm your kernel "
                    "supports the `renameat2` syscall and that "
                    "your filesystem supports atomic exchange\n");
            return EX_OSERR;
        } else {
            fprintf(stderr, "Cannot exchange files. Error %d - %s\n",
                            errno, strerror(errno));
            return EX_OSERR;
        }
    }

    return EX_OK;
}
