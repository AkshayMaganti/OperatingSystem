struct inode{
    unsigned short flags;
    char nlinks;
    char uid;
    char gid;
    unsigned int size;
    unsigned int addr[11];
    unsigned int actime;
    unsigned int modtime;
};