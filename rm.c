//removes a directory(if empty) or file from v6 file system

//removes entry of the file from the parent data block
void remove_entry_from_inode(int inum, char *filename){
    struct inode this_inode = get_inode(inum);
    int data_block = this_inode.addr[0];
    printf("Moving to position %d ", data_block);
    //reads the parent's data 
    if(lseek(fd,get_data_block_position(data_block),SEEK_SET)<=0){
        printf("Error occurred while reading parent data block ");
        exit(-1);
    }
    //struct to store 32 file definations in the parent's data block
    struct file_def temp_file_def_array[32];
    //reads the entries in parent's data block file by file till the end
    if(read(fd,&temp_file_def_array,sizeof(temp_file_def_array))<=0){
        printf("Error occurred while reading parent data block");
        exit(-1);
    }
    int i;
    int entry_removed = 0;
    for(i=0;i<32;i++){
        printf("Traversing parent. Got inode# %d\n", temp_file_def_array[i].inum);
        if(temp_file_def_array[i].inum == 0 ){
            //write updated file_def array to disk
            if(lseek(fd,get_data_block_position(data_block),SEEK_SET)<0){
                printf("Error occurred while writing new file to parent data block");
                exit(-1);
            }
            if(write(fd,&temp_file_def_array,sizeof(temp_file_def_array))<0){
                printf("Error occurred while writing parent data block to disk");
                exit(-1);
            }
            break;
        }
        // if the removed file entry is not the last entry, we move all the file entries below it upwards by one position
        if(entry_removed!=0){
            temp_file_def_array[i] = temp_file_def_array[i+1];
        }
        else{
            if(strcmp(temp_file_def_array[i].filename, filename)==0){
                //file/folder found
                temp_file_def_array[i] = temp_file_def_array[i+1];
                entry_removed = 1;
            }
        }
    }

}

void rm(char *path_in_v6){
    struct curr_and_parent curr_parent = get_file_def_for_parent(path_in_v6);
    struct file_def this_file = curr_parent.this_file;
    struct file_def parent_file = curr_parent.parent_file;
    if(parent_file.inum==0){
        printf("Parent directory not found\n");
        return;
    }
    if(this_file.inum==0) {
        printf("File not found\n");
        return;
    }

    //get the current inode to check if it is a directory and if it is empty or not
    struct inode this_inode = get_inode(this_file.inum);
    unsigned short masked_flags = mask & this_inode.flags;

    //check if the current file is a directory
    if(masked_flags>>13 == 2){
        int data_block = this_inode.addr[0];

        //read in 3 file defs from the disk
        if(lseek(fd,get_data_block_position(data_block),SEEK_SET)<=0){
            printf("Error occurred while reading parent data block ");
            exit(-1);
        }
        //struct to store 3 file definitions in the parent's data block
        struct file_def temp_file_def_array[3];

        //reads the entries in parent's data block file by file till the end
        if(read(fd,&temp_file_def_array,sizeof(temp_file_def_array))<=0){
            printf("Error occurred while reading parent data block");
            exit(-1);
        }

        //if the third entry is not empty, means the directory is not empty
        if(temp_file_def_array[2].inum!=0){
            printf("Directory not empty\n");
            return;
        }
    }

    clear_inode_free_data(this_inode);

    add_a_free_inode(this_file.inum);

    //remove current file from parent
    remove_entry_from_inode(parent_file.inum, this_file.filename);
}