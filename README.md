# Laporan Praktikum

## Soal 1
### Langkah - Langkah
1. Pada soal kita disuruh untuk membuat sebuah program bahasa C menggunakan fuse, pertama kita disuruh untuk menjelajah web [IniKaryaKita](https://www.inikaryakita.id/) dan mencari file project untuk nomor 1 dan mendownloadnya
   , kemudian kita disuruh untuk membuat program yang dimana didalam nya berisi logika buatan kita sendiri untuk fungsi memindahkan suatu file.
2. yang kita buat pertama adalah fungsi untuk membuat folder (**mkdir**) untuk bisa membuat folder  `wm` pada direktori fuse kita nanti. berikut adalah fungsinya:
  ```C
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
  ```

3. kemudian kita disuruh untuk membuat logika fungsi untuk memindahkan file, lalu sekalian memberinya watermark. disini saya menggunakan `system` dan `imageMagick` untuk mengedit gambar dan memberi watermark pada gambarnya
   dengan menambahkan
    ```C
    snprintf(image_magick_command, sizeof(image_magick_command),
                    "convert -gravity south -geometry +0+30 /proc/%d/fd/%d -fill blue -pointsize 36 -annotate +0+0 '%s' /proc/%d/fd/%d",
                    getpid(), source_file, "inikaryakita.id", getpid(), destination_file);
    ```

   pada fungsi untuk memindahkan file kita bisa memindahkan file sekaligus memberi watermark pada gambar, dan juga tak lupa kita menambahakan sebuah if condition yang dimana dia hanya bakal memberi watermark apabila
   nama dari direktori tujuan adalah `wm` jika tidak maka hanya pindahkan tanpa memberi watermark.
   berikut adalah fungsi nya:
    ```C
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
    ```
4. selanjutnya kita disuruh untuk membuat logika saat membaca suatu file. jika file tersebut memiliki prefix `test` maka tampilakan reverse / kebalikannya, jika ya maka tampilkan tanpa reverse.
   berikut adalah implementasi logika dalam fungsi `read_file_data`:
    ```C
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
    ```
    bada fungsi diatas saya membuat fungsi `reverse_chars` yang mana fungsinya adalah untuk membalik isi dari sebuah file, berikut adalah isi dari fungsi tersebut:
      ```C
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
      ```
5. Kemudian terakhir kita disuruh untuk membuat logika dimana jika file tersebut bernama `script.sh` maka jangan izinkan untuk diganti perizinannya dengan `chmod` teteapi selain file dengan nama `script.sh` bisa
   diganti permission nya. disini kita bakal mengedit logika dari fungsi chmod itu sendiri, tinggal menambahkan if condition sesuai dengan kriteria diatas yang sudah kita sebutkan tadi.
   berikut adalah impelementasi kode nya:
    ```C
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
    ```
    dapat dilihat bahwa saya hanya menambahkan if condition saja.
7. kemudian untuk fungsi sisanya seperti `getattr`, `getaddir`, `mknod`, `open`, `read`, `write` sama seperti biasanya. berikut adalah kode full nya:
    ```C
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
    
    };
    
    int main(int argc, char *argv[]) {
       umask(0);
       return fuse_main(argc, argv, &myfs_operations, NULL);
    }
    ```
8. Berikut adalah hasil dari kode saya ketika saya memindahkan gambar kedalam folder `wm`
   ![image](https://github.com/Trenttzzz/Sisop-4-2024-MH-IT16/assets/141043792/58a018bd-3259-44c9-8ff6-95e64c23960e)
   ![image](https://github.com/Trenttzzz/Sisop-4-2024-MH-IT16/assets/141043792/9eeb644a-7852-4267-b372-b81537ffcf8a)

9. berikut adalah ketika kita mencoba untuk membacar file dengan nama prefix `test`
    ![image](https://github.com/Trenttzzz/Sisop-4-2024-MH-IT16/assets/141043792/0a18be22-ecd4-4a5a-9758-09e7d7ae6e05)

10. lalu terakhir kita coba cek apakah chmod bisa digunakan saat mencoba mengganti permission dari `script.sh`
    ![image](https://github.com/Trenttzzz/Sisop-4-2024-MH-IT16/assets/141043792/c4a3be33-6415-42a3-a425-7ac741d77de8)

    ketika mencoba untuk mengubah permission denied.

## Soal 2
### Langkah - Langkah

## Soal 3
### Langkah - Langkah
### Dependensi

#### Penambahan Library dan konstanta
   ```c
   #define FUSE_USE_VERSION 31
   #include <fuse.h>
   #include <stdio.h>
   #include <string.h>
   #include <stdlib.h>
   #include <unistd.h>
   #include <dirent.h>
   #include <errno.h>
   #include <fcntl.h>
   #include <stddef.h>
   #include <assert.h>
   #include <sys/stat.h>
   #include <time.h>

   #define MAX_BUFFER 1028
   #define MAX_SPLIT  10000
   static const char *dirpath = "/home/winter/Documents/ITS/SISOP/Modul4/soal_3/relics";
   ```
   - MAX_BUFFER: Ukuran buffer maksimum.
   - MAX_SPLIT: Ukuran maksimum setiap bagian file.
   - dirpath: Jalur ke direktori tempat bagian file disimpan.

#### Fungsi - fungsi

1. arc_getattr
   ```c
   static int arc_getattr(const char *path, struct stat *stbuf)
   {
    memset(stbuf, 0, sizeof(struct stat));

    // Mengatur atribut waktu dan kepemilikan
    stbuf->st_uid   = getuid();
    stbuf->st_gid   = getgid();
    stbuf->st_atime = time(NULL);
    stbuf->st_mtime = time(NULL);

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    // Mendapatkan jalur lengkap item
    char fpath[MAX_BUFFER];
    sprintf(fpath, "%s%s", dirpath, path);

    // Mengatur atribut file default
    stbuf->st_mode = S_IFREG | 0644;
    stbuf->st_nlink = 1;
    stbuf->st_size = 0;

    // Menghitung total ukuran semua bagian
    char ppath[MAX_BUFFER + 4];
    FILE *fd;
    int i = 0;

    while (1) {
        sprintf(ppath, "%s.%03d", fpath, i++);
        fd = fopen(ppath, "rb");
        if (!fd) break;
        fseek(fd, 0L, SEEK_END);
        stbuf->st_size += ftell(fd);
        fclose(fd);
    }

    if (i == 1) return -errno;
    return 0;
   }
    ```
   *Penjelasan*
      Fungsi arc_getattr bertanggung jawab untuk mengambil atribut dari file atau direktori yang diminta. Fungsi ini  menginisialisasi struktur stat yang diberikan dengan nol, kemudian mengatur ID pengguna, ID grup, dan waktu akses serta modifikasi saat ini. Jika jalur yang diminta adalah root ("/"), fungsi mengatur atribut direktori dengan izin 0755 dan tautan (link) sebanyak 2. Jika bukan root, fungsi membangun jalur lengkap file atau direktori, mengatur atribut file default, dan menghitung ukuran total semua bagian dari file tersebut dengan membuka setiap bagian satu per satu hingga tidak ada lagi bagian yang ditemukan.

2. arc_readdir

   ```c
   static int arc_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
   {
    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    char fpath[MAX_BUFFER];
    sprintf(fpath, "%s%s", dirpath, path);

    DIR *dp = opendir(fpath);
    if (!dp) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        if (strstr(de->d_name, ".000") == NULL) continue;

        char cut[MAX_BUFFER];
        strcpy(cut, de->d_name);
        cut[strlen(cut) - 4] = '\lu\0';

        if (filler(buf, cut, &st, 0)) break;
    }

    closedir(dp);
    return 0;
   }
   ```
*Penjelasan*
      Fungsi arc_readdir bertanggung jawab untuk membaca isi dari sebuah direktori. Fungsi ini mengisi buffer dengan entri direktori default . dan ... Kemudian, fungsi membuka direktori yang diminta dan membaca setiap entri di dalamnya. Jika nama entri berakhiran .000, fungsi memotong bagian akhiran tersebut dan menambahkan nama yang dipotong ke buffer. Setelah semua entri dibaca, fungsi menutup direktori dan mengembalikan hasilnya.

   3. arc_read
      ```c
      static int arc_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
      {
    (void) fi;

    char fpath[MAX_BUFFER];
    sprintf(fpath, "%s%s", dirpath, path);

    char ppath[MAX_BUFFER + 4];
    FILE *fd;
    int i = 0;
    size_t size_read, total_read = 0;

    while (size > 0) {
        sprintf(ppath, "%s.%03d", fpath, i++);
        fd = fopen(ppath, "rb");
        if (!fd) break;

        fseek(fd, 0L, SEEK_END);
        size_t size_part = ftell(fd);
        fseek(fd, 0L, SEEK_SET);

        if (offset >= size_part) {
            offset -= size_part;
            fclose(fd);
            continue;
        }

        fseek(fd, offset, SEEK_SET);
        size_read = fread(buf, 1, size, fd);
        fclose(fd);

        buf += size_read;
        size -= size_read;
        total_read += size_read;
        offset = 0;
    }

    return total_read;
      }
      ```
*Penjelasan*
      Fungsi arc_read bertanggung jawab untuk membaca data dari file yang dipecah menjadi beberapa bagian. Fungsi ini membangun jalur lengkap dari file yang diminta dan membaca setiap bagian satu per satu hingga ukuran yang diminta terpenuhi atau tidak ada lagi bagian yang tersisa. Jika offset lebih besar dari ukuran bagian saat ini, fungsi menguranginya dengan ukuran bagian tersebut dan melanjutkan ke bagian berikutnya. Data yang dibaca disimpan dalam buffer dan ukuran serta offset diperbarui sesuai dengan jumlah data yang dibaca.

4. arc_write
   ```c
   static int arc_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
   {
    (void) fi;

    printf("init write: %s\n", path);

    char fpath[MAX_BUFFER];
    sprintf(fpath, "%s%s", dirpath, path);

    char ppath[MAX_BUFFER + 4];
    FILE *fd;

    int pcurrent = offset / MAX_SPLIT;
    size_t poffset = offset % MAX_SPLIT;
    size_t total_write = 0;

    while (size > 0) {
        sprintf(ppath, "%s.%03d", fpath, pcurrent++);
        fd = fopen(ppath, "r+b");
        if (!fd) {
            fd = fopen(ppath, "wb");
            if (!fd) return -errno;
        }

        printf("write: %s\n", ppath);

        fseek(fd, poffset, SEEK_SET);
        size_t size_write;
        if (size > (MAX_SPLIT - poffset))
             size_write = MAX_SPLIT - poffset;
        else size_write = size;

        fwrite(buf, 1, size_write, fd);
        fclose(fd);

        buf += size_write;
        size -= size_write;
        total_write += size_write;

        poffset = 0;
    }
    return total_write;
      }
   ```
*Penjelasan*
      Fungsi arc_write bertanggung jawab untuk menulis data ke file yang dipecah menjadi beberapa bagian. Fungsi ini membangun jalur lengkap dari file yang diminta dan menulis data ke setiap bagian satu per satu hingga ukuran yang diminta terpenuhi. Jika bagian yang dimaksud belum ada, fungsi membuatnya. Data yang ditulis disimpan dalam buffer dan ukuran serta offset diperbarui sesuai dengan jumlah data yang ditulis. Offset awal dihitung berdasarkan ukuran maksimum setiap bagian (MAX_SPLIT).

   5. arc_unlink
      ```c
      static int arc_unlink(const char *path)
      {
    char fpath[MAX_BUFFER];
    char ppath[MAX_BUFFER + 4];
    sprintf(fpath, "%s%s", dirpath, path);

    int pcurrent = 0;    
    while (1) {
        printf("unlink: %s\n", ppath);

        sprintf(ppath, "%s.%03d", fpath, pcurrent++);
        int res = unlink(ppath);
        if (res == -1) {
            if (errno == ENOENT) break;
            return -errno;
        }
    }
    return 0;
      }
      ```
*Penjelasan*
      Fungsi arc_unlink bertanggung jawab untuk menghapus file yang dipecah menjadi beberapa bagian. Fungsi ini membangun jalur lengkap dari file yang diminta dan menghapus setiap bagian satu per satu hingga tidak ada lagi bagian yang tersisa atau terjadi kesalahan. Jika kesalahan yang terjadi adalah karena file tidak ditemukan (ENOENT), maka fungsi berhenti menghapus dan mengembalikan hasilnya.

   6. arc_create
      ```c
      static int arc_create(const char *path, mode_t mode, struct fuse_file_info *fi)
      {
    (void) fi;

    char fpath[MAX_BUFFER];
    sprintf(fpath, "%s%s.000", dirpath, path);

    printf("create: %s\n", path);

    int res = creat(fpath, mode);
    if (res == -1) return -errno;

    close(res);
    return 0;
      }
      ```
*Penjelasan*
      Fungsi arc_create bertanggung jawab untuk membuat file baru. Fungsi ini membangun jalur lengkap dari file yang diminta dan membuat bagian pertama (.000). Jika terjadi kesalahan saat pembuatan, fungsi mengembalikan kesalahan tersebut.

   7. arc_utimens
      ```c
      static int arc_utimens(const char *path, const struct timespec ts[2])
      {
    char fpath[MAX_BUFFER];
    sprintf(fpath, "%s%s", dirpath, path);

    char ppath[MAX_BUFFER + 4];
    int pcurrent = 0;

    while (1) {
        sprintf(ppath, "%s.%03d", fpath, pcurrent++);
        int res = utimensat(AT_FDCWD, ppath, ts, 0);
        if (res == -1) {
            if (errno == ENOENT) break;
            return -errno;
        }
    }

    return 0;
      }
      ```
*Penjelasan*
         Fungsi arc_utimens bertanggung jawab untuk mengatur waktu akses dan modifikasi dari file yang dipecah menjadi beberapa bagian. Fungsi ini membangun jalur lengkap dari file yang diminta dan mengatur waktu atribut untuk setiap bagiannya. Jika bagian yang dimaksud tidak ditemukan (ENOENT), maka fungsi berhenti mengatur waktu dan mengembalikan hasilnya.

#### Struktur fuse_operations
   ```c
   static struct fuse_operations arc_oper = {
    .getattr    = arc_getattr,
    .readdir    = arc_readdir,
    .read       = arc_read,
    .write      = arc_write,
    .unlink     = arc_unlink,
    .create     = arc_create,
    .utimens    = arc_utimens,
};
   ```
*Penjelasan*
      Struktur fuse_operations mendefinisikan fungsi-fungsi yang diimplementasikan untuk operasi FUSE. Setiap fungsi dikaitkan dengan operasi FUSE yang sesuai.

#### Fungsi Main
   ```c
   int main(int argc, char *argv[])
{
    umask(0);
    return fuse_main(argc, argv, &arc_oper, NULL);
}
   ```
*Penjelasan*
Fungsi main mengatur umask menjadi 0 untuk memastikan izin file tidak dibatasi oleh nilai umask yang ada. Fungsi ini kemudian memanggil fuse_main dengan argumen argc dan argv yang diteruskan dari baris perintah serta struktur fuse_operations yang telah didefinisikan sebelumnya. Fungsi ini memulai sistem file FUSE.











      
