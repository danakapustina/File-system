#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


const int size_of_block = SIZE_OF_BLOCK;
int filesystem_fd = -1;
const int number_of_root_block = NUMBER_OF_ROOT_BLOCK;

int init()
{
    int result = 0;
    // пытаемся открыть существующий файл с файловой системой
    filesystem_fd = open(FILESYSTEM, O_RDWR, 0666);
    if (filesystem_fd < 0)
    {
        // создаем новый
        filesystem_fd = open(FILESYSTEM, O_CREAT | O_RDWR, 0666);
        if (filesystem_fd < 0 || create_root() != 0)
        {
            result = -1;
        }
    }
    return result;
}


int create_root()
{
    int result = -1;
    inode_t *root = (inode_t *)create_block();
    if (root != NULL)
    {
        root->status = BLOCK_STATUS_FOLDER;
        root->name[0] = '\0';
        root->stat.st_mode = S_IFDIR | 0777;
        root->stat.st_nlink = 2;
        if (write_block(number_of_root_block, root) == 0)
        {
            result = 0;
        }
        destroy_block(root);
    }
    return result;
}



void *create_block()
{
    return calloc(size_of_block, sizeof(char));
}


void destroy_block(void *block)
{
    free(block);
}

int read_block(int number, void *block)
{
    int result = -1;
    if (number >= 0 && lseek(filesystem_fd, size_of_block * number, SEEK_SET) >= 0)
    {
        if (read(filesystem_fd, block, size_of_block) >= 0)
        {
            result = 0;
        }
    }
    return result;
}


int write_block(int number, void *block)
{
    int result = -1;
    if (number >= 0 && lseek(filesystem_fd, size_of_block * number, SEEK_SET) >= 0)
    {
        if (write(filesystem_fd, block, size_of_block) == size_of_block)
        {
            result = 0;
        }
    }
    return result;
}

void *get_block(int number)
{
    void *block = NULL;
    if (number >= 0)
    {
        block = create_block();
        if (block != NULL && read_block(number, block) != 0)
        {
            destroy_block(block);
            block = NULL;
        }
    }
    return block;
}

int remove_file(int number)
{
    return set_block_status(number, BLOCK_STATUS_FREE);
}

int remove_folder(int number)
{
    int result = -1;
    inode_t *folder = (inode_t *)get_block(number);
    if (folder != NULL)
    {
        int *start = (int *)folder->content;
        int *end = (int *)((void *)folder + size_of_block);
        while (start < end)
        {
            if (*start > 0)
            {
                 remove_block(*start);
            }
            start++;
        }
        destroy_block(folder);
        result = set_block_status(number, BLOCK_STATUS_FREE);
    }
    return result;
}


int create_folder(const char *name, mode_t mode)
{
    int number = search_free_block();
    if (number >= 0)
    {
        inode_t *folder = (inode_t *)create_block();
        if (folder != NULL)
        {
            int name_size = strlen(name) + 1;
            if (name_size > NODE_NAME_MAX_SIZE)
            {
                name_size = NODE_NAME_MAX_SIZE;
            }
            folder->status = BLOCK_STATUS_FOLDER;
            memcpy(folder->name, name, name_size);
            folder->stat.st_mode = S_IFDIR | mode;
            folder->stat.st_nlink = 2;
            if (write_block(number, folder) != 0)
            {
                number = -1;
            }
            destroy_block(folder);
        }
    }
    return number;
}


int create_file(const char *name, mode_t mode, dev_t dev)
{
    int number = search_free_block();
    if (number >= 0)
    {
        inode_t *file = (inode_t *)create_block();
        if (file != NULL)
        {
            int name_size = strlen(name) + 1;
            if (name_size > NODE_NAME_MAX_SIZE)
            {
                name_size = NODE_NAME_MAX_SIZE;
            }
            file->status = BLOCK_STATUS_FILE;
            memcpy(file->name, name, name_size);
            file->stat.st_mode = S_IFREG | mode;
            file->stat.st_rdev = dev;
            file->stat.st_nlink = 1;
            if (write_block(number, file) != 0)
            {
                number = -1;
            }
            destroy_block(file);
        }
    }
    return number;
}
char *create_empty_name()
{
    return (char *)calloc(NODE_NAME_MAX_SIZE, sizeof(char));
}


void destroy_name(char *name)
{
    free(name);
}
