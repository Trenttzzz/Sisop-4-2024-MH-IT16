#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>


// Helper functions for decoding

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

char* base64_decode(const char* input) {
   // implementasi decode base64
}

char* rot13_decode(const char* input) {
    char *output = strdup(input);
    for (int i = 0; output[i] != '\0'; i++) {
        if ((output[i] >= 'A' && output[i] <= 'Z')) {
            output[i] = ((output[i] - 'A' + 13) % 26) + 'A';
        } else if ((output[i] >= 'a' && output[i] <= 'z')) {
            output[i] = ((output[i] - 'a' + 13) % 26) + 'a';
        }
    }
    return output;
}

char* hex_decode(const char* input) {
    int len = strlen(input);
    char *output = (char*)malloc((len/2) + 1);
    for(int i = 0; i < len; i+=2) {
        sscanf(input + i, "%2hhx", &output[i/2]);
    }
    output[len/2] = '\0';
    return output;
}

char* reverse_decode(const char* input) {
    int len = strlen(input);
    char *output = (char*)malloc(len + 1);
    for (int i = 0; i < len; i++) {
        output[i] = input[len - 1 - i];
    }
    output[len] = '\0';
    return output;
}

static const char *root_path = "/home/zaa/sensitif";

// Helper function for logging
void log_operation(const char *status, const char *tag, const char *info) {
    FILE *log_file = fopen("/home/zaa/sensitif/logs-fuse.log", "a");
    if (log_file == NULL) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp)-1, "%d/%m/%Y-%H:%M:%S", t);
    
    fprintf(log_file, "[%s]::%s::[%s]::[%s]\n", status, timestamp, tag, info);
    fclose(log_file);
}

// Helper function for checking passwords
int check_password() {
    const char *correct_password = "12345";  // Change this password
    char input_password[100];
    printf("Enter password to access: ");
    scanf("%99s", input_password);
    if (strcmp(input_password, correct_password) == 0) {
        return 1;
    } else {
        return 0;
    }
}

// FUSE operations
static int myfs_getattr(const char *path, struct stat *stbuf) {
    int res;
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);
    res = lstat(full_path, stbuf);
    if (res == -1)
        return -errno;
    return 0;
}

static int myfs_open(const char *path, struct fuse_file_info *fi) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);
    int res = open(full_path, fi->flags);
    if (res == -1)
        return -errno;
    close(res);
    return 0;
}

static int read_file_data(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    // Ekstrak nama file tanpa path
    const char *file_name = strrchr(path, '/') + 1;

    // Baca file menggunakan pread secara default
    int result = pread(fi->fh, buf, size, offset);
    if (result == -1) {
        return -errno;
    }

    // Jika berhasil membaca, periksa prefix dan decode jika diperlukan
    if (strstr(file_name, "base64") == file_name) {
        // Decode Base64 dan pipe ke stdout
        char command[2000];
        snprintf(command, sizeof(command), "base64 -d '%s'", full_path);

        FILE *pipe = popen(command, "r");
        if (!pipe) {
            return -errno;
        }

        size_t bytes_read = fread(buf, 1, size, pipe);

        // Tulis hasil decode ke stdout
        if (write(STDOUT_FILENO, buf, bytes_read) == -1) {
            pclose(pipe);
            return -errno;
        }

        pclose(pipe);
        return bytes_read;
    } else if (strstr(file_name, "rot13") == file_name) {
        // Decode ROT13
        rot13_decode(buf);
    } else if (strstr(file_name, "hex_") == file_name) {
        // Decode Hex
        char *decoded_content = hex_decode(buf);
        strncpy(buf, decoded_content, size);
        free(decoded_content);
    } else if (strstr(file_name, "rev") == file_name) {
        // Decode Reverse
        reverse_chars(buf, strlen(buf));
    }

    return result;
}
static int list_dir_contents(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *file_info) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

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

static struct fuse_operations myfs_oper = {
    .getattr    = myfs_getattr,
    .open       = myfs_open,
    .read       = read_file_data,
    .readdir    = list_dir_contents,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &myfs_oper, NULL);
}
