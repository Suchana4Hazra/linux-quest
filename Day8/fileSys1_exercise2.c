#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BLOCK_SIZE 4096
#define HEADER_SIZE 4096

typedef struct {
    int n;     // Number of blocks
    int s;     // Block size
    int ubn;   // Used block count
    int fbn;   // Free block count
    char *ub;  // Bitmap (allocated dynamically)
} block_header;

int init_File_dd(const char *fname, int bsize, int bno) {
    int fd = open(fname, O_CREAT | O_RDWR, 0700);
    if (fd == -1) {
        perror("Cannot create or open file");
        return -1;
    }

    block_header header;
    header.n = bno;
    header.s = bsize;
    header.ubn = 0;
    header.fbn = bno;
    header.ub = (char *)malloc(bno / 8);
    memset(header.ub, 0, bno / 8);  // Initialize all blocks as free

    // Write header to file
    write(fd, &header, sizeof(header) - sizeof(char *));
    write(fd, header.ub, bno / 8);

    // Expand file size
    lseek(fd, HEADER_SIZE + bsize * bno - 1, SEEK_SET);
    write(fd, "\0", 1);

    free(header.ub);
    close(fd);
    return 0;
}

int get_freeblock(const char *fname) {
    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        perror("Cannot open file");
        return -1;
    }

    block_header header;
    read(fd, &header, sizeof(header) - sizeof(char *));
    header.ub = (char *)malloc(header.n / 8);
    read(fd, header.ub, header.n / 8);

    for (int i = 0; i < header.n; i++) {
        int byte_index = i / 8;
        int bit_index = i % 8;
        if (!(header.ub[byte_index] & (1 << bit_index))) {
            header.ub[byte_index] |= (1 << bit_index);
            header.ubn++;
            header.fbn--;
            lseek(fd, 0, SEEK_SET);
            write(fd, &header, sizeof(header) - sizeof(char *));
            write(fd, header.ub, header.n / 8);
            lseek(fd, HEADER_SIZE + i * header.s, SEEK_SET);
            char *buffer = (char *)malloc(header.s);
            memset(buffer, 1, header.s);
            write(fd, buffer, header.s);
            free(buffer);
            free(header.ub);
            close(fd);
            return i;
        }
    }

    free(header.ub);
    close(fd);
    return -1;
}

int free_block(const char *fname, int bno) {
    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        perror("Cannot open file");
        return 0;
    }

    block_header header;
    read(fd, &header, sizeof(header) - sizeof(char *));
    header.ub = (char *)malloc(header.n / 8);
    read(fd, header.ub, header.n / 8);

    int byte_index = bno / 8;
    int bit_index = bno % 8;
    if (header.ub[byte_index] & (1 << bit_index)) {
        header.ub[byte_index] &= ~(1 << bit_index);
        header.ubn--;
        header.fbn++;
        lseek(fd, 0, SEEK_SET);
        write(fd, &header, sizeof(header) - sizeof(char *));
        write(fd, header.ub, header.n / 8);
        lseek(fd, HEADER_SIZE + bno * header.s, SEEK_SET);
        char *buffer = (char *)malloc(header.s);
        memset(buffer, 0, header.s);
        write(fd, buffer, header.s);
        free(buffer);
        free(header.ub);
        close(fd);
        return 1;
    }

    free(header.ub);
    close(fd);
    return 0;
}

int check_fs(const char *fname) {
    int fd = open(fname, O_RDONLY);
    if (fd == -1) {
        perror("Cannot open file");
        return 1;
    }

    block_header header;
    read(fd, &header, sizeof(header) - sizeof(char *));
    header.ub = (char *)malloc(header.n / 8);
    read(fd, header.ub, header.n / 8);

    int used_count = 0;
    for (int i = 0; i < header.n; i++) {
        int byte_index = i / 8;
        int bit_index = i % 8;
        if (header.ub[byte_index] & (1 << bit_index)) {
            used_count++;
        }
    }

    if (header.ubn + header.fbn != header.n || used_count != header.ubn) {
        free(header.ub);
        close(fd);
        return 1;
    }

    free(header.ub);
    close(fd);
    return 0;
}

int main() {
    const char *filename = "filesystem.dat";
    int choice, block_size = 512, num_blocks = 16;
    int block;
    
    while (1) {
        printf("\nFile System Manager\n");
        printf("1. Initialize File System\n");
        printf("2. Allocate Block\n");
        printf("3. Free Block\n");
        printf("4. Check File System Integrity\n");
        printf("5. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                if (init_File_dd(filename, block_size, num_blocks) == -1) {
                    printf("Failed to initialize file system.\n");
                } else {
                    printf("File system initialized successfully.\n");
                }
                break;
            case 2:
                block = get_freeblock(filename);
                if (block != -1) {
                    printf("Allocated block number: %d\n", block);
                } else {
                    printf("No free block available.\n");
                }
                break;
            case 3:
                printf("Enter block number to free: ");
                scanf("%d", &block);
                if (free_block(filename, block)) {
                    printf("Block %d successfully freed.\n", block);
                } else {
                    printf("Failed to free block %d.\n", block);
                }
                break;
            case 4:
                if (check_fs(filename) == 0) {
                    printf("File system integrity check passed.\n");
                } else {
                    printf("File system integrity check failed.\n");
                }
                break;
            case 5:
                return 0;
            default:
                printf("Invalid choice.\n");
        }
    }
}