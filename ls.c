#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <limits.h>

int is_hidden(const char* filename) {
    return (filename[0] == '.');
}

void print_file_info(const char* filename, const struct stat* file_status) {
    char* owner_name = "Unknown";
    struct passwd* pw = getpwuid(file_status->st_uid);
    if (pw != NULL) {
        owner_name = pw->pw_name;
    }

    time_t mode_time = file_status->st_mtime;
    mode_t file_mode = file_status->st_mode;
    mode_t owner_mode = file_mode & S_IRWXU;
    mode_t group_mode = file_mode & S_IRWXG;
    mode_t other_mode = file_mode & S_IRWXO;

    char type;
    if (file_status->st_mode & S_IFDIR) {
        type = 'd';
    }
    else {
        type = '-';
    }

    char owner_str[4] = {
        (file_mode & S_IRUSR) ? 'r' : '-',
        (file_mode & S_IWUSR) ? 'w' : '-',
        (file_mode & S_IXUSR) ? 'x' : '-',
        '\0'
    };
    char group_str[4] = {
        (file_mode & S_IRGRP) ? 'r' : '-',
        (file_mode & S_IWGRP) ? 'w' : '-',
        (file_mode & S_IXGRP) ? 'x' : '-',
        '\0'
    };
    char other_str[4] = {
        (file_mode & S_IROTH) ? 'r' : '-',
        (file_mode & S_IWOTH) ? 'w' : '-',
        (file_mode & S_IXOTH) ? 'x' : '-',
        '\0'
    };

    char mod_time_str[20];
    strftime(mod_time_str, 20, "%b %d %H:%M", localtime(&mode_time));
    printf("%c%-3s %-4s %-4s %-6ld %-20s %-10ld %-20s %-20s\n",
        type,
        owner_str,
        group_str,
        other_str,
        file_status->st_nlink,
        owner_name,
        file_status->st_size,
        mod_time_str,
        filename);
}

// Сравнение для функции сортировки файлов по названию
int compare_filenames(const void* a, const void* b) {
    const struct dirent* file_a = *(const struct dirent**)a;
    const struct dirent* file_b = *(const struct dirent**)b;
    return strcmp(file_a->d_name, file_b->d_name);
}

void list_directory(const char* path, int recursive) {
    DIR* pDIR;
    struct dirent* pDirEnt;
    struct stat file_status;
    pDIR = opendir(path);

    if (pDIR == NULL) {
        fprintf(stderr, "%s %d: opendir() failed(%s)\n",
            __FILE__, __LINE__, strerror(errno));
        exit(-1);
    }

    printf("%s:\n", path); // Print the current directory path
    printf("%-4s %-4s %-4s %-8s %-18s %-9s %-25s %-10s\n",
        "U", "G", "O", "Links", "Owner", "Size", "Modification Time", "Name");

    // Collect the files and directories
    struct dirent* entries[4096];
    int entry_count = 0;
    long int total_size = 0;
    pDirEnt = readdir(pDIR);
    while (pDirEnt != NULL) {
        char file_path[PATH_MAX];
        snprintf(file_path, PATH_MAX, "%s/%s", path, pDirEnt->d_name);

        if (stat(file_path, &file_status) < 0) {
            continue;
        }

        if (!is_hidden(pDirEnt->d_name)) {
            entries[entry_count++] = pDirEnt;
            total_size += file_status.st_size;
        }

        pDirEnt = readdir(pDIR);
    }
    printf("total %ld\n", total_size);
    // Sort the files and directories by name
    qsort(entries, entry_count, sizeof(struct dirent*), compare_filenames);

    // Print the sorted files and directories in the current directory
    for (int i = 0; i < entry_count; ++i) {
        char entry_path[PATH_MAX];
        snprintf(entry_path, PATH_MAX, "%s/%s", path, entries[i]->d_name);

        if (stat(entry_path, &file_status) < 0) {
            continue;
        }

        print_file_info(entries[i]->d_name, &file_status);
    }

    if (recursive) {
        printf("\n");
        // Recursively list the contents of subdirectories
        for (int i = 0; i < entry_count; ++i) {
            char entry_path[PATH_MAX];
            snprintf(entry_path, PATH_MAX, "%s/%s", path, entries[i]->d_name);

            if (strcmp(entries[i]->d_name, ".") != 0 && strcmp(entries[i]->d_name, "..") != 0
                && entries[i]->d_type == DT_DIR) {
                list_directory(entry_path, recursive);
            }
        }
    }

    closedir(pDIR);
}

int main(int argc, char* argv[]) {
    int recursive = 0; // Default: do not display contents of subdirectories recursively

    if (argc > 1 && strcmp(argv[1], "-R") == 0) {
        recursive = 1;
    }

    list_directory(".", recursive);

    return 0;
}