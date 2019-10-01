#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>

#define LS_BLOCK_SIZE   1024u
#define BLOCK_SIZE      512u

char * mode(mode_t mode, char str[])
{
    int idx = 1;

    switch (mode & S_IFMT) {
        case S_IFBLK:  str[0] = 'b'; break;
        case S_IFCHR:  str[0] = 'c'; break;
        case S_IFDIR:  str[0] = 'd'; break;
        case S_IFIFO:  str[0] = '|'; break;
        case S_IFLNK:  str[0] = 'l'; break;
        case S_IFREG:  str[0] = '-'; break;
        case S_IFSOCK: str[0] = 's'; break;
        default:       str[0] = '?'; break;
    }

    str[idx++] = (mode & S_IRUSR) ? 'r' : '-';
    str[idx++] = (mode & S_IWUSR) ? 'w' : '-';
    str[idx++] = (mode & S_IXUSR) ? 'x' : '-';
    str[idx++] = (mode & S_IRGRP) ? 'r' : '-';
    str[idx++] = (mode & S_IWGRP) ? 'w' : '-';
    str[idx++] = (mode & S_IXGRP) ? 'x' : '-';
    str[idx++] = (mode & S_IROTH) ? 'r' : '-';
    str[idx++] = (mode & S_IWOTH) ? 'w' : '-';
    str[idx++] = (mode & S_IXOTH) ? 'x' : '-';
    str[idx++] = '\0';

    return str;
}

int is_slink(mode_t mode)
{
    return (mode & S_IFMT) == S_IFLNK;
}

char * owner(uid_t uid, char *str, size_t n)
{
    struct passwd pwd;
    struct passwd *pwd_result;

    if (getpwuid_r(uid, &pwd, str, n, &pwd_result) != 0)
        snprintf(str, n, "%d", uid);

    return str;
}

char * group(gid_t gid, char *str, size_t n)
{
    struct group grp;
    struct group *grp_result;

    if (getgrgid_r(gid, &grp, str, n, &grp_result) != 0)
        snprintf(str, n, "%d", gid);

    return str;
}

char * mtime(time_t time, char *str, size_t n)
{
    struct tm stime;

    localtime_r(&time, &stime);
    strftime(str, n, "%c", &stime);

    return str;
}

void walk_dir(DIR *dir, const char root[])
{
    struct dirent *entry;
    struct stat statbuf;
    char path[PATH_MAX + 1];
    char path_ln[PATH_MAX + 1];
    char *buff;
    size_t buff_size;
    size_t total = 0;

    if ((buff_size = sysconf(_SC_GETPW_R_SIZE_MAX)) == -1)
        buff_size = 16384;

    if ((buff = malloc(buff_size)) == NULL) {
        perror("ERROR (malloc)");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if ((strncmp(entry->d_name, ".", PATH_MAX) == 0) ||
           (strncmp(entry->d_name, "..", PATH_MAX) == 0))
            continue;

        snprintf(path, sizeof(path) - 1, "%s/%s", root, entry->d_name);

        if (lstat(path, &statbuf) != 0) {
            perror("ERROR (lstat)");
            continue;
        }

        printf("%s ", mode(statbuf.st_mode, buff));
        printf("%d\t", statbuf.st_nlink);
        printf("%s ", owner(statbuf.st_uid, buff, buff_size));
        printf("%s ", group(statbuf.st_gid, buff, buff_size));
        printf("%lu\t", statbuf.st_size);
        printf("%s ", mtime(statbuf.st_mtime, buff, buff_size));
        printf("%s", entry->d_name);

        if (is_slink(statbuf.st_mode))
            printf(" -> %s", realpath(path, path_ln));

        printf("\n");
        total += statbuf.st_blocks;
    }
    printf("total %u\n", total / (LS_BLOCK_SIZE / BLOCK_SIZE));

    free(buff);
}

void ls(const char path[])
{
    DIR *dir;
    char root[PATH_MAX + 1];

    if ((dir = opendir(path)) == NULL) {
        perror("ERROR (opendir)");
        return;
    }

    walk_dir(dir, realpath(path, root));
    closedir(dir);
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        ls("./");
    } else if (argc == 2) {
        ls(argv[1]);
    } else {
        int i;
        for (i = 1; i != argc; ++i) {
            printf("%s:\n", argv[i]);
            ls(argv[i]);
            printf("\n");
        }
    }
    return 0;
}
