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

// Path direktori yang akan dimount
static const char *mounted_directory_path = "/home/zaa/portofolio/";

static int get_file_attributes(const char *path, struct stat *stat_buffer) {
    char full_path[1000];
    sprintf(full_path, "%s%s", mounted_directory_path, path);
    int result = 0;

    memset(stat_buffer, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stat_buffer->st_mode = S_IFDIR | 0755;
        stat_buffer->st_nlink = 2;
    } else if (strcmp(path, "/bahaya") == 0) {
        stat_buffer->st_mode = S_IFDIR | 0755;
        stat_buffer->st_nlink = 2;
    } else {
        result = lstat(full_path, stat_buffer);
        if (result == -1)
            return -errno;
    }
    return 0;
}

static int read_directory_contents(const char *path, void *buffer, fuse_fill_dir_t filler,
                                   off_t offset, struct fuse_file_info *file_info)
{
    char full_path[1000];
    sprintf(full_path, "%s%s", mounted_directory_path, path);

    DIR *directory_pointer;
    struct dirent *directory_entry;

    (void)offset;
    (void)file_info;

    directory_pointer = opendir(full_path);
    if (directory_pointer == NULL)
        return -errno;

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    while ((directory_entry = readdir(directory_pointer)) != NULL) {
        struct stat stat_info;
        memset(&stat_info, 0, sizeof(stat_info));
        stat_info.st_ino = directory_entry->d_ino;
        stat_info.st_mode = directory_entry->d_type << 12;
        if (filler(buffer, directory_entry->d_name, &stat_info, 0))
            break;
    }

    closedir(directory_pointer);
    return 0;
}

static int open_file(const char *path, struct fuse_file_info *file_info) {
    char full_path[1000];
    sprintf(full_path, "%s%s", mounted_directory_path, path);
    int file_descriptor = open(full_path, file_info->flags);
    if (file_descriptor == -1)
        return -errno;

    file_info->fh = file_descriptor;
    return 0;
}

// Fungsi untuk membalik urutan string
void reverse_string(char *str, size_t len) {
    char *start = str;
    char *end = str + len - 1;
    while (start < end) {
        char temp = *start;
        *start++ = *end;
        *end-- = temp;
    }
}

static int create_file_or_directory(const char *path, mode_t mode, dev_t dev) {
    char full_path[1000];
    sprintf(full_path, "%s%s", mounted_directory_path, path);

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

static int write_file(const char *path, const char *data, size_t length, off_t file_offset, struct fuse_file_info *file_info) {
    char full_path[1000]; 
    snprintf(full_path, sizeof(full_path), "%s%s", mounted_directory_path, path);

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

static int read_file(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *file_info) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", mounted_directory_path, path);

    // Check if the file has a 'test' prefix
    const char *filename = strrchr(path, '/');
    if (filename == NULL) {
        filename = path;
    } else {
        filename++;
    }

    if (strncmp(filename, "test", 4) == 0) {
        // If the file has a 'test' prefix, reverse the words in the lines before output
        FILE *file_pointer = fopen(full_path, "r");
        if (file_pointer == NULL) {
            return -errno;
        }

        char line[1024];
        buffer[0] = '\0'; // Initialize the buffer to an empty string
        while (fgets(line, sizeof(line), file_pointer) != NULL) {
            size_t len = strlen(line);
            if (line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }

            // Reverse the words in the line using reverse_string
            reverse_string(line, strlen(line));

            // Check if the buffer is large enough
            if (strlen(buffer) + strlen(line) + 1 > size) {
                fclose(file_pointer);
                return -ENOMEM;
            }

            strcat(buffer, line);
            strcat(buffer, "\n");
        }

        fclose(file_pointer);
    } else {
        // If the file doesn't have a 'test' prefix, output normally
        int result = pread(file_info->fh, buffer, size, offset);
        if (result == -1) {
            result = -errno;
        }

        return result;
    }

    return strlen(buffer);
}

static int rename_file(const char *old_path, const char *new_path) {
    char original_file_path[1000], new_file_path[1000];
    snprintf(original_file_path, sizeof(original_file_path), "%s%s", mounted_directory_path, old_path);
    snprintf(new_file_path, sizeof(new_file_path), "%s%s", mounted_directory_path, new_path);

    if (strstr(new_path, "/wm") != NULL) { // Jika file dipindahkan ke folder /wm
        int source_file = open(original_file_path, O_RDONLY); 
        if (source_file == -1) {
            perror("Error opening source file");
            return -errno;
        }

        int destination_file = open(new_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (destination_file == -1) {
           perror("Error opening destination file");
           close(source_file);
           return -errno;
       }

       char image_magick_command[1000];
       snprintf(image_magick_command, sizeof(image_magick_command), 
                "convert -gravity south -geometry +0+20 /proc/%d/fd/%d -fill white -pointsize 36 -annotate +0+0 '%s' /proc/%d/fd/%d", 
                getpid(), source_file, "inikaryakita.id", getpid(), destination_file);

       int result = system(image_magick_command);
       if (result == -1) {
           perror("Error executing ImageMagick command");
       }

       close(source_file);
       close(destination_file);

       if (unlink(original_file_path) == -1) { // Hapus file asli setelah berhasil di-watermark
           perror("Error deleting original file");
           return -errno;
       }
   } else { // Jika tidak dipindahkan ke /wm, lakukan rename biasa
       int result = rename(original_file_path, new_file_path);
       if (result == -1) {
           perror("Error renaming file");
           return -errno;
       }
   }

   return 0; // Berhasil
}

static int change_file_permissions(const char *path, mode_t mode) {
   char full_path[1000];
   sprintf(full_path, "%s%s", mounted_directory_path, path);
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
   sprintf(full_path, "%s%s", mounted_directory_path, path);
   int result;

   result = mkdir(full_path, mode);
   if (result == -1)
       return -errno;

   return 0;
}

static struct fuse_operations myfs_operations = {
   .getattr    = get_file_attributes,
   .readdir    = read_directory_contents,
   .open       = open_file,
   .read       = read_file,
   .write      = write_file,
   .rename     = rename_file,
   .chmod      = change_file_permissions,
   .mknod      = create_file_or_directory,
   .mkdir      = create_directory,
};

int main(int argc, char *argv[]) {
   umask(0);
   return fuse_main(argc, argv, &myfs_operations, NULL);
}
