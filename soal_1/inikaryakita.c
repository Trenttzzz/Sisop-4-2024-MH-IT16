#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>

// path clone
static const char *dirpath = "/home/zaa/portofolio/";

static int hjp_getattr(const char *path, struct stat *stbuf) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(path, "/bahaya") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        res = lstat(fpath, stbuf);
        if (res == -1)
            return -errno;
    }
    return 0;
}

static int hjp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);

    DIR *dp;
    struct dirent *de;

    (void)offset;
    (void)fi;

    dp = opendir(fpath);
    if (dp == NULL)
        return -errno;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int hjp_open(const char *path, struct fuse_file_info *fi) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int fd = open(fpath, fi->flags);
    if (fd == -1)
        return -errno;

    fi->fh = fd;
    return 0;
}

// Fungsi tambahan untuk membalik string
void reverse_string(char *str, size_t len) {
    char *start = str;
    char *end = str + len - 1;
    while (start < end) {
        char temp = *start;
        *start++ = *end;
        *end-- = temp;
    }
}

static int hjp_mknod(const char *path, mode_t mode, dev_t dev) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);

    int res;
    if (S_ISREG(mode)) { // Jika file biasa
        res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (res >= 0)
            close(res);
    } else if (S_ISFIFO(mode)) { // Jika FIFO (named pipe)
        res = mkfifo(fpath, mode);
    } else {
        res = mknod(fpath, mode, dev);
    }
    if (res == -1)
        return -errno;

    return 0;
}

static int hjp_write(const char *path, const char *data, size_t length, off_t file_offset, struct fuse_file_info *fi) {
    char full_path[1000]; 
    snprintf(full_path, sizeof(full_path), "%s%s", dirpath, path);

    int file_descriptor = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_descriptor == -1) {
        return -errno;
    }

    int result;
    result = pwrite(file_descriptor, data, length, file_offset);

    if (result == -1) {
        result = -errno;
    }

    close(file_descriptor);
    return result;
}

static int hjp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);

    // Check if the file has a 'test' prefix
    const char *filename = strrchr(path, '/');
    if (filename == NULL) {
        filename = path;
    } else {
        filename++;
    }

    if (strncmp(filename, "test", 4) == 0) {
        // If the file has a 'test' prefix, reverse the words in the lines before output
        FILE *file = fopen(fpath, "r");
        if (file == NULL) {
            return -errno;
        }

        char line[1024];
        buf[0] = '\0'; // Initialize the buffer to an empty string
        while (fgets(line, sizeof(line), file) != NULL) {
            size_t len = strlen(line);
            if (line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }

            // Reverse the words in the line using reverse_string
            reverse_string(line, strlen(line));

            // Check if the buffer is large enough
            if (strlen(buf) + strlen(line) + 1 > size) {
                fclose(file);
                return -ENOMEM;
            }

            strcat(buf, line);
            strcat(buf, "\n");
        }

        fclose(file);
    } else {
        // If the file doesn't have a 'test' prefix, output normally
        int res = pread(fi->fh, buf, size, offset);
        if (res == -1) {
            res = -errno;
        }

        return res;
    }

    return strlen(buf);
}

static int hjp_rename(const char *old_path, const char *new_path) {
    char originalFilePath[1000], newFilePath[1000];
    snprintf(originalFilePath, sizeof(originalFilePath), "%s%s", dirpath, old_path);
    snprintf(newFilePath, sizeof(newFilePath), "%s%s", dirpath, new_path);

    if (strstr(new_path, "/wm") != NULL) { // Jika file dipindahkan ke folder /wm
        int sourceFile = open(originalFilePath, O_RDONLY); 
        if (sourceFile == -1) {
            perror("Error opening source file");
            return -errno;
        }

        int destinationFile = open(newFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (destinationFile == -1) {
            perror("Error opening destination file");
            close(sourceFile);
            return -errno;
        }

        char imageMagickCommand[1000];
        snprintf(imageMagickCommand, sizeof(imageMagickCommand), 
                 "convert -gravity south -geometry +0+20 /proc/%d/fd/%d -fill white -pointsize 36 -annotate +0+0 '%s' /proc/%d/fd/%d", 
                 getpid(), sourceFile, "inikaryakita.id", getpid(), destinationFile);

        int result = system(imageMagickCommand);
        if (result == -1) {
            perror("Error executing ImageMagick command");
        }

        close(sourceFile);
        close(destinationFile);

        if (unlink(originalFilePath) == -1) { // Hapus file asli setelah berhasil di-watermark
            perror("Error deleting original file");
            return -errno;
        }
    } else { // Jika tidak dipindahkan ke /wm, lakukan rename biasa
        int result = rename(originalFilePath, newFilePath);
        if (result == -1) {
            perror("Error renaming file");
            return -errno;
        }
    }

    return 0; // Berhasil
}

static int hjp_chmod(const char *path, mode_t mode) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int res;

    // Hanya ubah permission jika file bukan "script.sh"
    if (strstr(path, "/script.sh") != NULL) {
        return -EACCES; // memunculkan permission denied
    } else {
        res = chmod(fpath, mode);
    }

    if (res == -1)
        return -errno;

    return 0;
}

static int hjp_mkdir(const char *path, mode_t mode) {
    char fpath[1000];
    sprintf(fpath, "%s%s", dirpath, path);
    int res;

    res = mkdir(fpath, mode);
    if (res == -1)
        return -errno;

    return 0;
}

static struct fuse_operations myfs_oper = {
    .getattr    = hjp_getattr,
    .readdir    = hjp_readdir,
    .open       = hjp_open,
    .read       = hjp_read,
    .write      = hjp_write,
    .rename     = hjp_rename,
    .chmod      = hjp_chmod,
    .mknod      = hjp_mknod,
    .mkdir      = hjp_mkdir,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &myfs_oper, NULL);
}
