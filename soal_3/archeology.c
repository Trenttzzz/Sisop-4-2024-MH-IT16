#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <limits.h>

#define BUFFER_SIZE 8192
#define MAX_PATH PATH_MAX + NAME_MAX + 1

static const char *dirPath = "/home/[user]/soal3/relics";

static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char fpath[PATH_MAX];
    sprintf(fpath, "%s%s", dirPath, path);
    res = lstat(fpath, stbuf);
    if (res == -1) {
        return -errno;
    }
    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    if (strcmp(path, "/") == 0) {
        path = dirPath;
        sprintf(fpath, "%s", path);
    } else {
        snprintf(fpath, sizeof(fpath), "%s%s", dirPath, path);
    }

    int res = 0;
    DIR *dp;
    struct dirent *de;

    (void)offset;
    (void)fi;

    dp = opendir(fpath);
    if (dp == NULL) {
        return -errno;
    }

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        char buf_path[PATH_MAX];
        snprintf(buf_path, sizeof(buf_path), "%s/%s", path, de->d_name);
        res = (filler(buf, buf_path, &st, 0));
        if (res != 0) {
            break;
        }
    }

    closedir(dp);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    if (strcmp(path, "/") == 0) {
        path = dirPath;
        sprintf(fpath, "%s", path);
    } else {
        sprintf(fpath, "%s%s", dirPath, path);
    }

    int res = 0;
    int fd = open(fpath, O_RDONLY);
    if (fd == -1) {
        return -errno;
    }

    res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    }
    close(fd);
    return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    sprintf(fpath, "%s%s", dirPath, path);

    int fd;
    if (offset == 0) {
        fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } else {
        fd = open(fpath, O_WRONLY, 0644);
    }
    if (fd == -1) {
        return -errno;
    }

    int res = pwrite(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    }
    close(fd);
    return res;
}

static int xmp_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    sprintf(fpath, "%s%s", dirPath, path);

    int res = creat(fpath, mode);
    if (res == -1) {
        return -errno;
    }

    char dirname[PATH_MAX];
    sprintf(dirname, "%s", dirPath);
    int fd = open(fpath, O_RDWR);
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer));

    int file_count = 0;
    while (bytes_read > 0) {
        char filename[MAX_PATH];
        snprintf(filename, sizeof(filename), "%s/%s.%03d", dirname, path + 1, file_count);
        int fd_split = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        write(fd_split, buffer, bytes_read);
        close(fd_split);
        memset(buffer, 0, sizeof(buffer));
        bytes_read = read(fd, buffer, sizeof(buffer));
        file_count++;
    }

    close(fd);
    unlink(fpath);

    return 0;
}

static int xmp_unlink(const char *path) {
    char fpath[PATH_MAX];
    sprintf(fpath, "%s%s", dirPath, path);

    char dirname[PATH_MAX];
    sprintf(dirname, "%s", dirPath);
    DIR *dir = opendir(dirname);
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        char filename[MAX_PATH];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        snprintf(filename, sizeof(filename), "%s/%s", dirname, entry->d_name);
        if (strstr(entry->d_name, path + 1) != NULL) {
            unlink(filename);
        }
    }
    closedir(dir);

    int res = unlink(fpath);
    if (res == -1) {
        return -errno;
    }

    return 0;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .read = xmp_read,
    .write = xmp_write,
    .create = xmp_create,
    .unlink = xmp_unlink,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
