#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

// Path direktori yang akan dimount
static const char *mounted_dir_path = "/home/zaa/portofolio/";

// Fungsi untuk membalik urutan string
void reverse_chars(char *str, size_t len) {
    char *start = str;
    char *end = str + len - 1;
    while (start < end) {
        char temp = *start;
        *start++ = *end;
        *end-- = temp;
    }
}

static int list_dir_contents(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *file_info) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", mounted_dir_path, path);

    DIR *dir_ptr;
    struct dirent *dir_entry;

    (void)offset;
    (void)file_info;

    dir_ptr = opendir(full_path);
    if (dir_ptr == NULL)
        return -errno;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    while ((dir_entry = readdir(dir_ptr)) != NULL) {
        struct stat stat_info;
        memset(&stat_info, 0, sizeof(stat_info));
        stat_info.st_ino = dir_entry->d_ino;
        stat_info.st_mode = dir_entry->d_type << 12;
        if (filler(buf, dir_entry->d_name, &stat_info, 0))
            break;
    }

    closedir(dir_ptr);
    return 0;
}

static int get_file_info(const char *path, struct stat *stat_buf) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", mounted_dir_path, path);
    int result = 0;

    memset(stat_buf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stat_buf->st_mode = S_IFDIR | 0755;
        stat_buf->st_nlink = 2;
    } else if (strcmp(path, "/bahaya") == 0) {
        stat_buf->st_mode = S_IFDIR | 0755;
        stat_buf->st_nlink = 2;
    } else {
        result = lstat(full_path, stat_buf);
        if (result == -1)
            return -errno;
    }
    return 0;
}

static int open_file_handler(const char *path, struct fuse_file_info *file_info) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", mounted_dir_path, path);
    int file_desc = open(full_path, file_info->flags);
    if (file_desc == -1)
        return -errno;

    file_info->fh = file_desc;
    return 0;
}

static int write_file_data(const char *path, const char *data, size_t length, off_t offset, struct fuse_file_info *file_info) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", mounted_dir_path, path);

    int file_desc = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_desc == -1) {
        return -errno;
    }

    int result;
    result = pwrite(file_desc, data, length, offset);

    if (result == -1) {
        result = -errno;
    }

    close(file_desc);
    return result;
}

static int create_file_or_dir(const char *path, mode_t mode, dev_t dev) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", mounted_dir_path, path);

    int result;
    if (S_ISREG(mode)) { // Jika file biasa
        result = open(full_path, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (result >= 0)
            close(result);
    } else if (S_ISFIFO(mode)) { // Jika FIFO (named pipe)
        result = mkfifo(full_path, mode);
    } else {
        result = mknod(full_path, mode, dev);
    }
    if (result == -1)
        return -errno;

    return 0;
}

static int read_file_data(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *file_info) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", mounted_dir_path, path);

    // Cek apakah file memiliki awalan 'test'
    const char *file_name = strrchr(path, '/');
    if (file_name == NULL) {
        file_name = path;
    } else {
        file_name++;
    }

    if (strncmp(file_name, "test", 4) == 0) {
        // Jika file memiliki awalan 'test', balik urutan kata dalam baris sebelum output
        FILE *file_ptr = fopen(full_path, "r");
        if (file_ptr == NULL) {
            return -errno;
        }

        char line[1024];
        buf[0] = '\0'; // Inisialisasi buffer menjadi string kosong
        while (fgets(line, sizeof(line), file_ptr) != NULL) {
            size_t len = strlen(line);
            if (line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }

            // Balik urutan kata dalam baris menggunakan reverse_chars
            reverse_chars(line, strlen(line));

            // Cek apakah buffer cukup besar
            if (strlen(buf) + strlen(line) + 1 > size) {
                fclose(file_ptr);
                return -ENOMEM;
            }

            strcat(buf, line);
            strcat(buf, "\n");
        }

        fclose(file_ptr);
    } else {
        int result = pread(file_info->fh, buf, size, offset);
        if (result == -1) {
            result = -errno;
        }

        return result;
    }

    return strlen(buf);
}

static int change_file_permissions(const char *path, mode_t mode) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", mounted_dir_path, path);
    int result;

    // Hanya ubah permission jika file bukan "script.sh"
    if (strstr(path, "/script.sh") != NULL) {
        return -EACCES; // memunculkan permission denied
    } else {
        result = chmod(full_path, mode);
    }

    if (result == -1)
        return -errno;
    return 0;
}

static int create_directory(const char *path, mode_t mode) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", mounted_dir_path, path);

    struct stat st;
    if (stat(full_path, &st) == 0) {
        // Direktori sudah ada
        return -EEXIST;
    }

    int res = mkdir(full_path, mode);
    if (res == -1)
        return -errno;

    return 0;
}

static int rename_file(const char *old_path, const char *new_path) {
   char original_file_path[1000], new_file_path[1000];
   snprintf(original_file_path, sizeof(original_file_path), "%s%s", mounted_dir_path, old_path);
   snprintf(new_file_path, sizeof(new_file_path), "%s%s", mounted_dir_path, new_path);

   if (strstr(new_path, "/wm") != NULL) { // Jika file dipindahkan ke folder /wm
       int source_file = open(original_file_path, O_RDONLY);
       if (source_file == -1) {
           perror("Gagal membuka file sumber");
           return -errno;
       }

       int destination_file = open(new_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
       if (destination_file == -1) {
           perror("Gagal membuka file tujuan");
           close(source_file);
           return -errno;
       }

       char image_magick_command[1000];
       snprintf(image_magick_command, sizeof(image_magick_command),
                "convert -gravity south -geometry +0+30 /proc/%d/fd/%d -fill blue -pointsize 36 -annotate +0+0 '%s' /proc/%d/fd/%d",
                getpid(), source_file, "inikaryakita.id", getpid(), destination_file);

       int result = system(image_magick_command);
       if (result == -1) {
           perror("Gagal mengeksekusi perintah ImageMagick");
       }

       close(source_file);
       close(destination_file);

       if (unlink(original_file_path) == -1) { // Hapus file asli setelah berhasil di-watermark
           perror("Gagal menghapus file sumber");
           return -errno;
       }
   } else { // Jika tidak dipindahkan ke /wm, lakukan rename biasa
       int result = rename(original_file_path, new_file_path);
       if (result == -1) {
           perror("Gagal rename file");
           return -errno;
       }
   }

   return 0; // Berhasil
}

// Membuat file simbolik (symlink)
static int create_symlink(const char *target_path, const char *link_path) {
   char full_target_path[1000], full_link_path[1000];
   snprintf(full_target_path, sizeof(full_target_path), "%s%s", mounted_dir_path, target_path);
   snprintf(full_link_path, sizeof(full_link_path), "%s%s", mounted_dir_path, link_path);

   int result = symlink(full_target_path, full_link_path);
   if (result == -1)
       return -errno;

   return 0;
}

// Menghapus file simbolik (symlink)
static int remove_symlink(const char *link_path) {
   char full_link_path[1000];
   snprintf(full_link_path, sizeof(full_link_path), "%s%s", mounted_dir_path, link_path);

   int result = unlink(full_link_path);
   if (result == -1)
       return -errno;

   return 0;
}

static struct fuse_operations myfs_operations = {
   .getattr = get_file_info,
   .readdir = list_dir_contents,
   .rename = rename_file,
   .chmod = change_file_permissions,
   .mknod = create_file_or_dir,
   .mkdir = create_directory,
   .open = open_file_handler,
   .read = read_file_data,
   .write = write_file_data,
   .symlink = create_symlink, 
   .unlink = remove_symlink, 

};

int main(int argc, char *argv[]) {
   umask(0);
   return fuse_main(argc, argv, &myfs_operations, NULL);
}
