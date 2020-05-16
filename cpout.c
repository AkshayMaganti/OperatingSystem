//method to copy file block by block
int copy_data_block(int block_num, int fd1, int remaining_file_size){
    char block[1024];
    int bytes_to_read = 1024;

    //calculate the bytes to read from this block
    if(remaining_file_size<bytes_to_read) bytes_to_read = remaining_file_size;

    if(bytes_to_read>0){
        //seek to the position of the block_num on disk
        if(lseek(fd, get_data_block_position(block_num), SEEK_SET)<=0){
            printf("Error occured while reading block #%d", block_num);
            exit(-1);
        }

        //read the bytes from the disk
        int bytes_read = read(fd, &block, bytes_to_read);
        if( bytes_read<=0 ){
            printf("Error occured while reading block #%d", block_num);
            exit(-1);
        }

        //write the read bytes to external file
        if(write(fd1, &block, bytes_read)<=0){
            printf("Error while writing block to external file.");
            exit(-1);
        }
    }
    //return the remaining file size
    return remaining_file_size-bytes_to_read;
}

//method to copy the v6 file 
void copy_data(int addr[], char *file_path, int file_size){
    //create/open the external file
    int fd1 = creat(file_path, 2);
    if(fd1<0){
        printf("Error occured while opening the external file.\n");
        return;
    }

    //write thr first 10 blocks directly to external file if the block is valid
    int i;
    for(i = 0; i<10; i++){
        int block_num = addr[i];
        if(block_num==0) break;
        file_size = copy_data_block(block_num, fd1, file_size);
    }

    //we use triple indirect addressing for the 11th block
    if(addr[10]!=0){
        //maintain 3 arrays for first indirect, second indirect and third direct blocks to load values from disk
        int first_indirect[256] = {};
        int second_indirect[256] = {};
        int third_direct[256] = {};

        //move to position of 11th block in addr and read the first indirect block
        int position = get_data_block_position(addr[10]);
        if(lseek(fd, position, SEEK_SET)<0){
            printf("Error occurred while copying the data blocks for existing file\n");
            exit(-1);
        }

        if(read(fd, &first_indirect, sizeof(first_indirect)) < 0 ){
            printf("Error occurred while copying the data blocks for existing file\n");
            exit(-1);
        }
        //iterate across the first_indirect array of blocks
        for(i = 0; i<256; i++){
            if(first_indirect[i]>0 && first_indirect[i]<sb.fsize){
                //for each valid block in first indirect, read the second indirect block numbers
                position = get_data_block_position(first_indirect[i]);
                if(lseek(fd, position, SEEK_SET)<0){
                    printf("Error occurred while copying the data blocks for existing file\n");
                    exit(-1);
                }

                if(read(fd, &second_indirect, sizeof(second_indirect)) < 0 ){
                    printf("Error occurred while copying the data blocks for existing file\n");
                    exit(-1);
                }

                //iterate across the second_indirect array of blocks
                int j;
                for(j = 0; j<256; j++){
                    if(second_indirect[j]>0 && second_indirect[j]<sb.fsize){
                        //for each valid block in second indirect, read the third direct block numbers
                        position = get_data_block_position(second_indirect[j]);
                        if(lseek(fd, position, SEEK_SET)<0){
                            printf("Error occurred while copying the data blocks for existing file data block #%d \n", second_indirect[j]);
                            exit(-1);
                        }

                        if(read(fd, &third_direct, sizeof(third_direct)) < 0 ){
                            printf("Error occurred while copying the data blocks for existing file\n");
                            exit(-1);
                        }
                        //iterate across all the blocks in the third direct array
                        int k;
                        for(k = 0; k < 256; k++){
                            //for each valid block in third direct, we copy this block to external file and update the remaining file size
                            if(third_direct[k]>0 && third_direct[k] < sb.fsize) file_size = copy_data_block(third_direct[k], fd1, file_size);
                            else break;
                        }
                    }
                    else{
                        break;
                    }
                }
            }
            else break;
        }
    }
    //close the file
    close(fd1);
}

//copy file in v6 to an external file
void cpout(char *path_in_v6, char *external_path){
    //get the file def for the file and parent from v6 disk
    struct curr_and_parent curr_parent = get_file_def_for_parent(path_in_v6);

    //check if the file exists
    struct file_def this_file = curr_parent.this_file;
    printf("Found Inode #%d\n",this_file.inum);
    if(this_file.inum==0) {
        printf("File not found\n");
        return;
    }

    //make sure that the entry is not a directory
    struct inode this_inode = get_inode(this_file.inum);
    unsigned short masked_flags = mask & this_inode.flags;

    if(masked_flags>>13==2){
        printf("File is a directory. Unable to copy\n");
        return;
    }

    //copy the addr array to external path
    copy_data(this_inode.addr, external_path, this_inode.size);
}