#define _GNU_SOURCE

#define VERSION "0.2.0"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sysexits.h>
#include <string.h>
#include <fcntl.h>
#include <libgen.h>
#include <linux/fs.h>
#include <linux/limits.h>

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

int do_rename(char *src, char *dst)
{
    return renameat2(AT_FDCWD, src, AT_FDCWD, dst, RENAME_NOREPLACE);
}

int main(int argc, char *argv[])
{
    int c, verbose, numfiles;
    char *pname = NULL;
    char newname[PATH_MAX];

    while ((c = getopt(argc, argv, "vVn:")) != -1) {
        switch (c) {
        case 'v':
            verbose = 1;
            break;
        case 'n':
            pname = optarg;
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

    if (pname) {
        if (*pname == '/') {
            strncpy(newname, pname, PATH_MAX);
        } else {
            // Determine the parent directory
            char *bname = (char *) malloc(sizeof newname);
            strncpy(bname, argv[optind], PATH_MAX);
            dirname(bname);

            //TODO this code needs length/error checking
            snprintf(newname, PATH_MAX, "%s/%s", bname, pname);
        }
    }

    if (verbose)
        printf("‘%s‘ <-> ‘%s‘\n", argv[optind], argv[optind+1]);

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

    if (pname) {
        if (verbose)
            printf("‘%s‘ -> ‘%s‘\n", argv[optind], newname);

        if (do_rename(argv[optind], newname) != 0) {
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
    }

    return EX_OK;
}
