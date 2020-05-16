//struct for file definitions written to data blocks of directories
struct file_def{
    unsigned int inum;
    char filename[28];
};

//an empty inode to be written to disk
struct inode empty_inode;
int zeroes[150],i;

//superblock to be used throughout the code
struct super_block sb;

//file descriptor
int fd;

//a struct for current and parent file def
struct curr_and_parent{
    struct file_def parent_file;
    struct file_def this_file;
};