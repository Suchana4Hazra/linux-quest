#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BLOCK_SIZE 4096

typedef struct {
    int block_size;
    int no_of_blocks;
    int first_free_block;
    int first_root_block;
    int root_block_size;
} super_block;

typedef struct {
    char type;      // 1 for file, 2 for directory
    char name[12];
    int first_block;
    int size;
} file_desc;

typedef struct {
    int block_id;
    char data[BLOCK_SIZE - 8]; // -8 for block_id and next_block
    int next_block;
} data_block;


// Forward declarations for the filesystem functions
data_block my_read_block(int fd, int block_id);
void my_write_block(int fd, int block_id, data_block db);
int get_free_block(int fd, super_block *sb);
int mymkfs(const char *fname, int block_size, int no_of_blocks);
int mycopyTo(const char *fname, char *myfname);
int mycopyFrom(char *myfname, const char *fname);
int myrm(char *myfname);
int mymkdir(char *mydirname);
int myrmdir(char *mydirname);
int myreadBlock(char *myfname, char *buf, int block_no);
int mystat(char *myname, char *buf);

int mymkfs(const char *fname, int block_size, int no_of_blocks) {
    int fd = open(fname, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("cannot open file");
        return -1;
    }
    
    // Initialize super block
    super_block sb;
    sb.block_size = block_size;
    sb.no_of_blocks = no_of_blocks;
    sb.first_free_block = 2; // First block is the super block, second is root dir
    sb.first_root_block = 1;
    sb.root_block_size = 0;

    // Write super block
    write(fd, &sb, sizeof(super_block));

    // Initialize root directory (empty)
    data_block root_dir;
    root_dir.block_id = 1;
    root_dir.next_block = 0; // No next block initially
    memset(root_dir.data, 0, sizeof(root_dir.data));
    my_write_block(fd, 1, root_dir);

    // Initialize free blocks list
    data_block empty_block;
    for (int i = 2; i < no_of_blocks; i++) {
        empty_block.block_id = i;
        empty_block.next_block = i + 1;
        memset(empty_block.data, 0, sizeof(empty_block.data));
        my_write_block(fd, i, empty_block);
    }
    
    // Last block points to nothing
    empty_block.block_id = no_of_blocks - 1;
    empty_block.next_block = 0;
    my_write_block(fd, no_of_blocks - 1, empty_block);

    close(fd);
    return 0;
}

data_block my_read_block(int fd, int block_id) {
    data_block db;
    lseek(fd, block_id * BLOCK_SIZE, SEEK_SET);
    read(fd, &db, sizeof(data_block));
    return db;
}

void my_write_block(int fd, int block_id, data_block db) {
    lseek(fd, block_id * BLOCK_SIZE, SEEK_SET);
    write(fd, &db, sizeof(data_block));
}

int get_free_block(int fd, super_block *sb) {
    if (sb->first_free_block == 0) {
        return -1; // No free blocks
    }

    int free_block = sb->first_free_block;
    
    // Read the free block to get the next free block
    data_block db = my_read_block(fd, free_block);
    sb->first_free_block = db.next_block;

    // Update super block
    lseek(fd, 0, SEEK_SET);
    write(fd, sb, sizeof(super_block));

    return free_block;
}

// Helper function to navigate to a directory path
int navigate_to_directory(int fd, super_block *sb, const char *path, data_block *last_dir) {
    // Start from the root directory
    int current_block = sb->first_root_block;
    data_block current_dir = my_read_block(fd, current_block);
    
    // Make a copy of the path to tokenize
    char path_copy[256];
    strncpy(path_copy, path, 255);
    path_copy[255] = '\0';
    
    // Handle root path
    if (strcmp(path, "/") == 0 || strlen(path) == 0) {
        *last_dir = current_dir;
        return current_block;
    }
    
    // Skip leading slash
    char *path_ptr = path_copy;
    if (path_ptr[0] == '/') {
        path_ptr++;
    }
    
    char *token = strtok(path_ptr, "/");
    while (token != NULL) {
        int found = 0;
        
        // Search for the directory in the current directory
        for (int i = 0; i < sizeof(current_dir.data); i += 21) {
            file_desc dir_entry;
            memcpy(&dir_entry, &current_dir.data[i], 21);
            
            // Check if this entry is a directory and matches the name
            if (dir_entry.type == 2 && strcmp(dir_entry.name, token) == 0) {
                current_block = dir_entry.first_block;
                current_dir = my_read_block(fd, current_block);
                found = 1;
                break;
            }
        }
        
        if (!found) {
            return -1; // Directory not found
        }
        
        token = strtok(NULL, "/");
    }
    
    *last_dir = current_dir;
    return current_block;
}

// Helper function to find file in a directory
int find_file_in_directory(data_block dir, const char *filename, file_desc *file_descriptor) {
    for (int i = 0; i < sizeof(dir.data); i += sizeof(file_desc)) {
        file_desc entry;
        memcpy(&entry, &dir.data[i], sizeof(file_desc));
        
        // Check if entry is valid and matches filename
        if (entry.type != 0 && strcmp(entry.name, filename) == 0) {
            *file_descriptor = entry;
            return i; // Return position in directory
        }
    }
    return -1; // File not found
}

int mycopyTo(const char *fname, char *myfname) {
    // Parse myfname into target_path and myfs_name
    char *target_path = strtok(myfname, "@");
    char *myfs_name = strtok(NULL, "@");
    
    if (target_path == NULL || myfs_name == NULL) {
        fprintf(stderr, "Invalid format. Use <target_path>@<myfs_name>\n");
        return -1;
    }
    
    // Open the source file
    int src_fd = open(fname, O_RDONLY);
    if (src_fd == -1) {
        perror("Cannot open source file");
        return -1;
    }
    
    // Open the myfs file
    int myfs_fd = open(myfs_name, O_RDWR);
    if (myfs_fd == -1) {
        perror("Cannot open myfs file");
        close(src_fd);
        return -1;
    }
    
    // Read the super block
    super_block sb;
    lseek(myfs_fd, 0, SEEK_SET);
    read(myfs_fd, &sb, sizeof(super_block));
    
    // Extract directory path and filename
    char path_copy[256];
    strncpy(path_copy, target_path, 255);
    path_copy[255] = '\0';
    
    char *last_slash = strrchr(path_copy, '/');
    char *dir_path;
    char *filename;
    
    if (last_slash == NULL) {
        // No directory, just filename in root
        dir_path = "/";
        filename = path_copy;
    } else {
        *last_slash = '\0'; // Split the path
        dir_path = path_copy;
        filename = last_slash + 1;
        
        // If dir_path is empty, we're in root
        if (strlen(dir_path) == 0) {
            dir_path = "/";
        }
    }
    
    // Navigate to the directory
    data_block dir_block;
    int dir_block_id = navigate_to_directory(myfs_fd, &sb, dir_path, &dir_block);
    if (dir_block_id == -1) {
        fprintf(stderr, "Directory not found\n");
        close(src_fd);
        close(myfs_fd);
        return -1;
    }
    
    // Check if the file already exists
    file_desc existing_file;
    if (find_file_in_directory(dir_block, filename, &existing_file) != -1) {
        // Implement overwrite logic if needed
        fprintf(stderr, "File already exists\n");
        close(src_fd);
        close(myfs_fd);
        return -1;
    }
    
    // Get the first free block for the file
    int first_block = get_free_block(myfs_fd, &sb);
    if (first_block == -1) {
        fprintf(stderr, "No free blocks available\n");
        close(src_fd);
        close(myfs_fd);
        return -1;
    }
    
    // Create file descriptor
    file_desc new_file;
    new_file.type = 1; // File type
    strncpy(new_file.name, filename, 11);
    new_file.name[11] = '\0';
    new_file.first_block = first_block;
    new_file.size = 0;
    
    // Copy data from source file to blocks
    int current_block = first_block;
    data_block block_data;
    int bytes_read;
    
    while ((bytes_read = read(src_fd, block_data.data, sizeof(block_data.data))) > 0) {
        new_file.size += bytes_read;
        block_data.block_id = current_block;
        
        // Get the next block if needed
        int next_block = 0;
        if (bytes_read == sizeof(block_data.data)) {
            next_block = get_free_block(myfs_fd, &sb);
            if (next_block == -1) {
                fprintf(stderr, "No free blocks available\n");
                // Should clean up allocated blocks here
                close(src_fd);
                close(myfs_fd);
                return -1;
            }
        }
        
        block_data.next_block = next_block;
        my_write_block(myfs_fd, current_block, block_data);
        current_block = next_block;
    }
    
    // Add file descriptor to directory
    int added = 0;
    for (int i = 0; i < sizeof(dir_block.data); i += sizeof(file_desc)) {
        file_desc entry;
        memcpy(&entry, &dir_block.data[i], sizeof(file_desc));
        
        if (entry.type == 0) { // Empty slot
            memcpy(&dir_block.data[i], &new_file, sizeof(file_desc));
            my_write_block(myfs_fd, dir_block_id, dir_block);
            added = 1;
            break;
        }
    }
    
    if (!added) {
        fprintf(stderr, "Directory is full\n");
        // Should clean up allocated blocks here
        close(src_fd);
        close(myfs_fd);
        return -1;
    }
    
    close(src_fd);
    close(myfs_fd);
    return 0;
}

int mycopyFrom(char *myfname, const char *fname) {
    // Parse myfname into source_path and myfs_name
    char *source_path = strtok(myfname, "@");
    char *myfs_name = strtok(NULL, "@");
    
    if (source_path == NULL || myfs_name == NULL) {
        fprintf(stderr, "Invalid format. Use <source_path>@<myfs_name>\n");
        return -1;
    }
    
    // Open the myfs file
    int myfs_fd = open(myfs_name, O_RDWR);
    if (myfs_fd == -1) {
        perror("Cannot open myfs file");
        return -1;
    }
    
    // Open/create the destination file
    int dest_fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (dest_fd == -1) {
        perror("Cannot open destination file");
        close(myfs_fd);
        return -1;
    }
    
    // Read the super block
    super_block sb;
    lseek(myfs_fd, 0, SEEK_SET);
    read(myfs_fd, &sb, sizeof(super_block));
    
    // Extract directory path and filename
    char path_copy[256];
    strncpy(path_copy, source_path, 255);
    path_copy[255] = '\0';
    
    char *last_slash = strrchr(path_copy, '/');
    char *dir_path;
    char *filename;
    
    if (last_slash == NULL) {
        // No directory, just filename in root
        dir_path = "/";
        filename = path_copy;
    } else {
        *last_slash = '\0'; // Split the path
        dir_path = path_copy;
        filename = last_slash + 1;
        
        // If dir_path is empty, we're in root
        if (strlen(dir_path) == 0) {
            dir_path = "/";
        }
    }
    
    // Navigate to the directory
    data_block dir_block;
    int dir_block_id = navigate_to_directory(myfs_fd, &sb, dir_path, &dir_block);
    if (dir_block_id == -1) {
        fprintf(stderr, "Directory not found\n");
        close(dest_fd);
        close(myfs_fd);
        return -1;
    }
    
    // Find the file in the directory
    file_desc file_desc;
    if (find_file_in_directory(dir_block, filename, &file_desc) == -1) {
        fprintf(stderr, "File not found\n");
        close(dest_fd);
        close(myfs_fd);
        return -1;
    }
    
    // Check if it's a file
    if (file_desc.type != 1) {
        fprintf(stderr, "Not a file\n");
        close(dest_fd);
        close(myfs_fd);
        return -1;
    }
    
    // Copy data from file blocks to destination file
    int current_block = file_desc.first_block;
    while (current_block != 0) {
        data_block block_data = my_read_block(myfs_fd, current_block);
        
        // Determine how much to write
        int bytes_to_write;
        if (block_data.next_block == 0) {
            // Last block might not be full
            bytes_to_write = file_desc.size % sizeof(block_data.data);
            if (bytes_to_write == 0 && file_desc.size > 0) {
                bytes_to_write = sizeof(block_data.data);
            }
        } else {
            bytes_to_write = sizeof(block_data.data);
        }
        
        write(dest_fd, block_data.data, bytes_to_write);
        current_block = block_data.next_block;
    }
    
    close(dest_fd);
    close(myfs_fd);
    return 0;
}

int myrm(char *myfname) {
    // Parse myfname into target_path and myfs_name
    char *target_path = strtok(myfname, "@");
    char *myfs_name = strtok(NULL, "@");
    
    if (target_path == NULL || myfs_name == NULL) {
        fprintf(stderr, "Invalid format. Use <target_path>@<myfs_name>\n");
        return -1;
    }
    
    // Open the myfs file
    int myfs_fd = open(myfs_name, O_RDWR);
    if (myfs_fd == -1) {
        perror("Cannot open myfs file");
        return -1;
    }
    
    // Read the super block
    super_block sb;
    lseek(myfs_fd, 0, SEEK_SET);
    read(myfs_fd, &sb, sizeof(super_block));
    
    // Extract directory path and filename
    char path_copy[256];
    strncpy(path_copy, target_path, 255);
    path_copy[255] = '\0';
    
    char *last_slash = strrchr(path_copy, '/');
    char *dir_path;
    char *filename;
    
    if (last_slash == NULL) {
        // No directory, just filename in root
        dir_path = "/";
        filename = path_copy;
    } else {
        *last_slash = '\0'; // Split the path
        dir_path = path_copy;
        filename = last_slash + 1;
        
        // If dir_path is empty, we're in root
        if (strlen(dir_path) == 0) {
            dir_path = "/";
        }
    }
    
    // Navigate to the directory
    data_block dir_block;
    int dir_block_id = navigate_to_directory(myfs_fd, &sb, dir_path, &dir_block);
    if (dir_block_id == -1) {
        fprintf(stderr, "Directory not found\n");
        close(myfs_fd);
        return -1;
    }
    
    // Find the file in the directory
    file_desc file_desc;
    int file_pos = find_file_in_directory(dir_block, filename, &file_desc);
    if (file_pos == -1) {
        fprintf(stderr, "File not found\n");
        close(myfs_fd);
        return -1;
    }
    
    // Check if it's a file
    if (file_desc.type != 1) {
        fprintf(stderr, "Not a file\n");
        close(myfs_fd);
        return -1;
    }
    
    // Free the file blocks
    int current_block = file_desc.first_block;
    while (current_block != 0) {
        data_block block_data = my_read_block(myfs_fd, current_block);
        int next_block = block_data.next_block;
        
        // Add block to free list
        block_data.next_block = sb.first_free_block;
        my_write_block(myfs_fd, current_block, block_data);
        sb.first_free_block = current_block;
        
        current_block = next_block;
    }
    
    // Update super block
    lseek(myfs_fd, 0, SEEK_SET);
    write(myfs_fd, &sb, sizeof(super_block));
    
    // Remove file entry from directory
    memset(&dir_block.data[file_pos], 0, sizeof(file_desc));
    my_write_block(myfs_fd, dir_block_id, dir_block);
    
    close(myfs_fd);
    return 0;
}

int mymkdir(char *mydirname) {
    // Parse mydirname into target_path and myfs_name
    char *target_path = strtok(mydirname, "@");
    char *myfs_name = strtok(NULL, "@");
    
    if (target_path == NULL || myfs_name == NULL) {
        fprintf(stderr, "Invalid format. Use <target_path>@<myfs_name>\n");
        return -1;
    }
    
    // Open the myfs file
    int myfs_fd = open(myfs_name, O_RDWR);
    if (myfs_fd == -1) {
        perror("Cannot open myfs file");
        return -1;
    }
    
    // Read the super block
    super_block sb;
    lseek(myfs_fd, 0, SEEK_SET);
    read(myfs_fd, &sb, sizeof(super_block));
    
    // Create a copy of the path for processing
    char path_copy[256];
    strncpy(path_copy, target_path, 255);
    path_copy[255] = '\0';
    
    // Remove trailing slash if present
    int path_len = strlen(path_copy);
    if (path_len > 1 && path_copy[path_len - 1] == '/') {
        path_copy[path_len - 1] = '\0';
    }
    
    // Extract parent directory path and new directory name
    char *last_slash = strrchr(path_copy, '/');
    char *parent_path;
    char *dirname;
    
    if (last_slash == NULL) {
        // No parent directory, creating in root
        parent_path = "/";
        dirname = path_copy;
    } else {
        *last_slash = '\0'; // Split the path
        parent_path = path_copy;
        dirname = last_slash + 1;
        
        // If parent_path is empty, we're in root
        if (strlen(parent_path) == 0) {
            parent_path = "/";
        }
    }
    
    // Navigate to the parent directory
    data_block parent_dir;
    int parent_dir_id = navigate_to_directory(myfs_fd, &sb, parent_path, &parent_dir);
    if (parent_dir_id == -1) {
        fprintf(stderr, "Parent directory not found\n");
        close(myfs_fd);
        return -1;
    }
    
    // Check if directory already exists
    file_desc existing_dir;
    if (find_file_in_directory(parent_dir, dirname, &existing_dir) != -1) {
        fprintf(stderr, "Directory already exists\n");
        close(myfs_fd);
        return -1;
    }
    
    // Get a free block for the new directory
    int dir_block_id = get_free_block(myfs_fd, &sb);
    if (dir_block_id == -1) {
        fprintf(stderr, "No free blocks available\n");
        close(myfs_fd);
        return -1;
    }
    
    // Initialize the new directory
    data_block new_dir;
    new_dir.block_id = dir_block_id;
    new_dir.next_block = 0;
    memset(new_dir.data, 0, sizeof(new_dir.data));
    my_write_block(myfs_fd, dir_block_id, new_dir);
    
    // Create directory entry
    file_desc dir_entry;
    dir_entry.type = 2; // Directory type
    strncpy(dir_entry.name, dirname, 11);
    dir_entry.name[11] = '\0';
    dir_entry.first_block = dir_block_id;
    dir_entry.size = 0;
    
    // Add directory entry to parent directory
    int added = 0;
    for (int i = 0; i < sizeof(parent_dir.data); i += sizeof(file_desc)) {
        file_desc entry;
        memcpy(&entry, &parent_dir.data[i], sizeof(file_desc));
        
        if (entry.type == 0) { // Empty slot
            memcpy(&parent_dir.data[i], &dir_entry, sizeof(file_desc));
            my_write_block(myfs_fd, parent_dir_id, parent_dir);
            added = 1;
            break;
        }
    }
    
    if (!added) {
        fprintf(stderr, "Parent directory is full\n");
        // Add the allocated block back to free list
        data_block tmp = my_read_block(myfs_fd, dir_block_id);
        tmp.next_block = sb.first_free_block;
        sb.first_free_block = dir_block_id;
        my_write_block(myfs_fd, dir_block_id, tmp);
        
        // Update super block
        lseek(myfs_fd, 0, SEEK_SET);
        write(myfs_fd, &sb, sizeof(super_block));
        
        close(myfs_fd);
        return -1;
    }
    
    close(myfs_fd);
    return 0;
}

int myrmdir(char *mydirname) {
    // Parse mydirname into target_path and myfs_name
    char *target_path = strtok(mydirname, "@");
    char *myfs_name = strtok(NULL, "@");
    
    if (target_path == NULL || myfs_name == NULL) {
        fprintf(stderr, "Invalid format. Use <target_path>@<myfs_name>\n");
        return -1;
    }
    
    // Open the myfs file
    int myfs_fd = open(myfs_name, O_RDWR);
    if (myfs_fd == -1) {
        perror("Cannot open myfs file");
        return -1;
    }
    
    // Read the super block
    super_block sb;
    lseek(myfs_fd, 0, SEEK_SET);
    read(myfs_fd, &sb, sizeof(super_block));
    
    // Extract parent directory path and directory name
    char path_copy[256];
    strncpy(path_copy, target_path, 255);
    path_copy[255] = '\0';
    
    // Remove trailing slash if present
    int path_len = strlen(path_copy);
    if (path_len > 1 && path_copy[path_len - 1] == '/') {
        path_copy[path_len - 1] = '\0';
    }
    
    char *last_slash = strrchr(path_copy, '/');
    char *parent_path;
    char *dirname;
    
    if (last_slash == NULL) {
        // No parent directory, removing from root
        parent_path = "/";
        dirname = path_copy;
    } else {
        *last_slash = '\0'; // Split the path
        parent_path = path_copy;
        dirname = last_slash + 1;
        
        // If parent_path is empty, we're in root
        if (strlen(parent_path) == 0) {
            parent_path = "/";
        }
    }
    
    // Navigate to the parent directory
    data_block parent_dir;
    int parent_dir_id = navigate_to_directory(myfs_fd, &sb, parent_path, &parent_dir);
    if (parent_dir_id == -1) {
        fprintf(stderr, "Parent directory not found\n");
        close(myfs_fd);
        return -1;
    }
    
    // Find the directory entry
    file_desc dir_entry;
    int dir_pos = find_file_in_directory(parent_dir, dirname, &dir_entry);
    if (dir_pos == -1) {
        fprintf(stderr, "Directory not found\n");
        close(myfs_fd);
        return -1;
    }
    
    // Check if it's a directory
    if (dir_entry.type != 2) {
        fprintf(stderr, "Not a directory\n");
        close(myfs_fd);
        return -1;
    }
    
    // Check if directory is empty
    data_block dir_block = my_read_block(myfs_fd, dir_entry.first_block);
    for (int i = 0; i < sizeof(dir_block.data); i += sizeof(file_desc)) {
        file_desc entry;
        memcpy(&entry, &dir_block.data[i], sizeof(file_desc));
        
        if (entry.type != 0) {
            fprintf(stderr, "Directory not empty\n");
            close(myfs_fd);
            return -1;
        }
    }
    
    // Free the directory block
    dir_block.next_block = sb.first_free_block;
    my_write_block(myfs_fd, dir_entry.first_block, dir_block);
    sb.first_free_block = dir_entry.first_block;
    
    // Update super block
    lseek(myfs_fd, 0, SEEK_SET);
    write(myfs_fd, &sb, sizeof(super_block));
    
    // Remove directory entry from parent directory
    memset(&parent_dir.data[dir_pos], 0, sizeof(file_desc));
    my_write_block(myfs_fd, parent_dir_id, parent_dir);
    
    close(myfs_fd);
    return 0;
}

int myreadBlock(char *myfname, char *buf, int block_no) {
    // Parse myfname into file_path and myfs_name
    char *file_path = strtok(myfname, "@");
    char *myfs_name = strtok(NULL, "@");
    
    if (file_path == NULL || myfs_name == NULL) {
        fprintf(stderr, "Invalid format. Use <file_path>@<myfs_name>\n");
        return -1;
    }
    
    // Open the myfs file
    int myfs_fd = open(myfs_name, O_RDONLY);
    if (myfs_fd == -1) {
        perror("Cannot open myfs file");
        return -1;
    }
    
    // Read the super block
    super_block sb;
    lseek(myfs_fd, 0, SEEK_SET);
    read(myfs_fd, &sb, sizeof(super_block));
    
    // Extract directory path and filename
    char path_copy[256];
    strncpy(path_copy, file_path, 255);
    path_copy[255] = '\0';
    
    char *last_slash = strrchr(path_copy, '/');
    char *dir_path;
    char *filename;
    
    if (last_slash == NULL) {
        // No directory, just filename in root
        dir_path = "/";
        filename = path_copy;
    } else {
        *last_slash = '\0'; // Split the path
        dir_path = path_copy;
        filename = last_slash + 1;
        
        // If dir_path is empty, we're in root
        if (strlen(dir_path) == 0) {
            dir_path = "/";
        }
    }
    
    // Navigate to the directory
    data_block dir_block;
    int dir_block_id = navigate_to_directory(myfs_fd, &sb, dir_path, &dir_block);
    if (dir_block_id == -1) {
        fprintf(stderr, "Directory not found\n");
        close(myfs_fd);
        return -1;
    }
    
    // Find the file in the directory
    file_desc file_desc;
    if (find_file_in_directory(dir_block, filename, &file_desc) == -1) {
        fprintf(stderr, "File not found\n");
        close(myfs_fd);
        return -1;
    }
    
    // Check if it's a file
    if (file_desc.type != 1) {
        fprintf(stderr, "Not a file\n");
        close(myfs_fd);
        return -1;
    }
    
    // Navigate to the specified block
    int current_block = file_desc.first_block;
    int current_block_idx = 0;
    
    while (current_block != 0 && current_block_idx < block_no) {
        data_block block_data = my_read_block(myfs_fd, current_block);
        current_block = block_data.next_block;
        current_block_idx++;
    }
    
    if (current_block == 0 || current_block_idx != block_no) {
        fprintf(stderr, "Block number out of range\n");
        close(myfs_fd);
        return -1;
    }
    
    // Read the requested block
    data_block block_data = my_read_block(myfs_fd, current_block);
    memcpy(buf, block_data.data, sizeof(block_data.data));
    
    close(myfs_fd);
    return 0;
}

int mywriteBlock(char *myfname, char *buf, int block_no) {
    // Parse myfname into file_path and myfs_name
    char *file_path = strtok(myfname, "@");
    char *myfs_name = strtok(NULL, "@");
    
    if (file_path == NULL || myfs_name == NULL) {
        fprintf(stderr, "Invalid format. Use <file_path>@<myfs_name>\n");
        return -1;
    }
    
    // Open the myfs file
    int myfs_fd = open(myfs_name, O_RDWR);
    if (myfs_fd == -1) {
        perror("Cannot open myfs file");
        return -1;
    }
    
    // Read the super block
    super_block sb;
    lseek(myfs_fd, 0, SEEK_SET);
    read(myfs_fd, &sb, sizeof(super_block));
    
    // Extract directory path and filename
    char path_copy[256];
    strncpy(path_copy, file_path, 255);
    path_copy[255] = '\0';
    
    char *last_slash = strrchr(path_copy, '/');
    char *dir_path;
    char *filename;
    
    if (last_slash == NULL) {
        // No directory, just filename in root
        dir_path = "/";
        filename = path_copy;
    } else {
        *last_slash = '\0'; // Split the path
        dir_path = path_copy;
        filename = last_slash + 1;
        
        // If dir_path is empty, we're in root
        if (strlen(dir_path) == 0) {
            dir_path = "/";
        }
    }
    
    // Navigate to the directory
    data_block dir_block;
    int dir_block_id = navigate_to_directory(myfs_fd, &sb, dir_path, &dir_block);
    if (dir_block_id == -1) {
        fprintf(stderr, "Directory not found\n");
        close(myfs_fd);
        return -1;
    }
    
    // Find the file in the directory
    file_desc file_desc;
    int file_pos = find_file_in_directory(dir_block, filename, &file_desc);
    if (file_pos == -1) {
        fprintf(stderr, "File not found\n");
        close(myfs_fd);
        return -1;
    }
    
    // Check if it's a file
    if (file_desc.type != 1) {
        fprintf(stderr, "Not a file\n");
        close(myfs_fd);
        return -1;
    }
    
    // Navigate to the specified block
    int current_block = file_desc.first_block;
    int current_block_idx = 0;
    
    while (current_block != 0 && current_block_idx < block_no) {
        data_block block_data = my_read_block(myfs_fd, current_block);
        current_block = block_data.next_block;
        current_block_idx++;
    }
    
    if (current_block == 0 || current_block_idx != block_no) {
        fprintf(stderr, "Block number out of range\n");
        close(myfs_fd);
        return -1;
    }
    
    // Read the current block to modify
    data_block block_data = my_read_block(myfs_fd, current_block);
    
    // Update the data but keep the block_id and next_block pointers intact
    memcpy(block_data.data, buf, sizeof(block_data.data));
    
    // Write the updated block back
    my_write_block(myfs_fd, current_block, block_data);
    
    close(myfs_fd);
    return 0;
}

int mystat(char *myname, char *buf) {
    // Parse myname into path and myfs_name
    char *path = strtok(myname, "@");
    char *myfs_name = strtok(NULL, "@");
    
    if (path == NULL || myfs_name == NULL) {
        fprintf(stderr, "Invalid format. Use <path>@<myfs_name>\n");
        return -1;
    }
    
    // Open the myfs file
    int myfs_fd = open(myfs_name, O_RDONLY);
    if (myfs_fd == -1) {
        perror("Cannot open myfs file");
        return -1;
    }
    
    // Read the super block
    super_block sb;
    lseek(myfs_fd, 0, SEEK_SET);
    read(myfs_fd, &sb, sizeof(super_block));
    
    // Handle the root directory case
    if (strcmp(path, "/") == 0) {
        // Root directory stats
        char stats[256];
        sprintf(stats, "Type: Directory\nName: /\nFirst Block: %d\nSize: %d\n", 
                sb.first_root_block, sb.root_block_size);
        strcpy(buf, stats);
        close(myfs_fd);
        return 0;
    }
    
    // Create a copy of the path for processing
    char path_copy[256];
    strncpy(path_copy, path, 255);
    path_copy[255] = '\0';
    
    // Remove trailing slash if present
    int path_len = strlen(path_copy);
    if (path_len > 1 && path_copy[path_len - 1] == '/') {
        path_copy[path_len - 1] = '\0';
    }
    
    // Extract directory path and filename/dirname
    char *last_slash = strrchr(path_copy, '/');
    char *dir_path;
    char *name;
    
    if (last_slash == NULL) {
        // No directory, just name in root
        dir_path = "/";
        name = path_copy;
    } else {
        *last_slash = '\0'; // Split the path
        dir_path = path_copy;
        name = last_slash + 1;
        
        // If dir_path is empty, we're in root
        if (strlen(dir_path) == 0) {
            dir_path = "/";
        }
    }
    
    // Navigate to the directory
    data_block dir_block;
    int dir_block_id = navigate_to_directory(myfs_fd, &sb, dir_path, &dir_block);
    if (dir_block_id == -1) {
        fprintf(stderr, "Directory not found\n");
        close(myfs_fd);
        return -1;
    }
    
    // Find the file/directory in the directory
    file_desc entry;
    int entry_pos = find_file_in_directory(dir_block, name, &entry);
    if (entry_pos == -1) {
        fprintf(stderr, "File/Directory not found\n");
        close(myfs_fd);
        return -1;
    }
    
    // Generate stats based on type
    char stats[256];
    if (entry.type == 1) {
        // It's a file
        sprintf(stats, "Type: File\nName: %s\nFirst Block: %d\nSize: %d bytes\n", 
                entry.name, entry.first_block, entry.size);
        
        // Count the number of blocks used
        int block_count = 0;
        int current_block = entry.first_block;
        
        while (current_block != 0) {
            block_count++;
            data_block block_data = my_read_block(myfs_fd, current_block);
            current_block = block_data.next_block;
        }
        
        char block_info[64];
        sprintf(block_info, "Blocks Used: %d\n", block_count);
        strcat(stats, block_info);
    } else {
        // It's a directory
        sprintf(stats, "Type: Directory\nName: %s\nFirst Block: %d\n", 
                entry.name, entry.first_block);
        
        // Count entries in the directory
        data_block dir_content = my_read_block(myfs_fd, entry.first_block);
        int entry_count = 0;
        
        for (int i = 0; i < sizeof(dir_content.data); i += sizeof(file_desc)) {
            file_desc temp_entry;
            memcpy(&temp_entry, &dir_content.data[i], sizeof(file_desc));
            if (temp_entry.type != 0) {
                entry_count++;
            }
        }
        
        char dir_info[64];
        sprintf(dir_info, "Entries: %d\n", entry_count);
        strcat(stats, dir_info);
    }
    
    // Copy the stats to the buffer
    strcpy(buf, stats);
    
    close(myfs_fd);
    return 0;
}

void print_usage() {
    printf("Usage:\n");
    printf("  ./myfs mymkfs <filename> <block_size> <number_of_blocks>\n");
    printf("  ./myfs mycopyTo <linux_filename> <myfile_name>@<myfs_filename>\n");
    printf("  ./myfs mycopyFrom <myfile_name>@<myfs_filename> <linux_filename>\n");
    printf("  ./myfs myrm <myfile_name>@<myfs_filename>\n");
    printf("  ./myfs mymkdir <mydir_name>@<myfs_filename>\n");
    printf("  ./myfs myrmdir <mydir_name>@<myfs_filename>\n");
    printf("  ./myfs myreadBlock <myfile_name>@<myfs_filename> <block_number>\n");
    printf("  ./myfs mystat <myname>@<myfs_filename>\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    // Determine which function to call based on the first argument
    if (strcmp(argv[0], "./mymkfs") == 0) {
        if (argc != 4) {
            printf("Error: mymkfs requires 3 arguments\n");
            print_usage();
            return 1;
        }
        int block_size = atoi(argv[2]);
        int no_of_blocks = atoi(argv[3]);
        return mymkfs(argv[1], block_size, no_of_blocks);
    } 
    else if (strcmp(argv[0], "./mycopyTo") == 0) {
        if (argc != 3) {
            printf("Error: mycopyTo requires 2 arguments\n");
            print_usage();
            return 1;
        }
        return mycopyTo(argv[1], argv[2]);
    } 
    else if (strcmp(argv[0], "./mycopyFrom") == 0) {
        if (argc != 3) {
            printf("Error: mycopyFrom requires 2 arguments\n");
            print_usage();
            return 1;
        }
        return mycopyFrom(argv[1], argv[2]);
    } 
    else if (strcmp(argv[0], "./myrm") == 0) {
        if (argc != 2) {
            printf("Error: myrm requires 1 argument\n");
            print_usage();
            return 1;
        }
        return myrm(argv[1]);
    } 
    else if (strcmp(argv[0], "./mymkdir") == 0) {
        if (argc != 2) {
            printf("Error: mymkdir requires 1 argument\n");
            print_usage();
            return 1;
        }
        return mymkdir(argv[1]);
    } 
    else if (strcmp(argv[0], "./myrmdir") == 0) {
        if (argc != 2) {
            printf("Error: myrmdir requires 1 argument\n");
            print_usage();
            return 1;
        }
        return myrmdir(argv[1]);
    } 
    else if (strcmp(argv[0], "./myreadBlock") == 0) {
        if (argc != 3) {
            printf("Error: myreadBlock requires 2 arguments\n");
            print_usage();
            return 1;
        }
        char buf[4096]; // Assuming maximum block size is 4096 bytes
        int block_no = atoi(argv[2]);
        int result = myreadBlock(argv[1], buf, block_no);
        
        if (result >= 0) {
            printf("Block %d read successfully. Content:\n", block_no);
            write(STDOUT_FILENO, buf, result);
            printf("\n");
        }
        return result;
    } 
    else if (strcmp(argv[0], "./mystat") == 0) {
        if (argc != 2) {
            printf("Error: mystat requires 1 argument\n");
            print_usage();
            return 1;
        }
        char buf[1024]; // Buffer for stat information
        int result = mystat(argv[1], buf);
        
        if (result >= 0) {
            printf("Metadata for %s:\n%s\n", argv[1], buf);
        }
        return result;
    } 
    else {
        printf("Unknown command: %s\n", argv[0]);
        print_usage();
        return 1;
    }

    return 0;
}
