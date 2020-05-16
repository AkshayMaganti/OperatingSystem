//create a directory in v6 file system
void mkdir(char *path_to_dir){
    //get file def for current and parent according to path
    struct curr_and_parent curr_parent = get_file_def_for_parent(path_to_dir);

    //get parent file def
    struct file_def parent_filedef = curr_parent.parent_file;

    //get current file def
    struct file_def this_file_def = curr_parent.this_file;

    //get parent's inode and make sure it exists
    struct inode parent_inode;
    if(parent_filedef.inum!=0){
        parent_inode = get_inode(parent_filedef.inum);
    }
    else{
        printf("Parent directory does not exist\n");
        return;
    }

    //if the directory doesn't already exist
    if(this_file_def.inum == 0){
        //get a free inode
        this_file_def.inum = get_a_free_inode();
        printf("Creating directory at inode #%d\n", this_file_def.inum);

        //create file defs . and ..
        struct file_def dot_file_def;
        dot_file_def.inum = this_file_def.inum;
        strcpy(dot_file_def.filename, ".");

        struct file_def parent_file_def;
        parent_file_def.inum = parent_filedef.inum;
        strcpy(parent_file_def.filename,"..");

        //create an array of . and .. and write it to a new data block
        struct file_def buff[2];
        buff[0] = dot_file_def;
        buff[1] = parent_file_def;

        int data_block_number = allocate_a_data_block();

        if(lseek(fd, data_block_number*1024, SEEK_SET)<=0){
            printf("Some unknown error occurred!");
            exit(-1);
        }
        if(write(fd, buff, 2*sizeof(struct file_def))<=0){
            printf("Some unknown error occurred!");
            exit(-1);
        }

        //create a new inode, set it's size, flags and addr[0] to the block allocated above 
        struct inode this_inode;
        this_inode.flags = 49663;
        this_inode.addr[0] = data_block_number;
        this_inode.size = 64;
        this_inode.actime = (unsigned) time(NULL);
        this_inode.modtime = (unsigned) time(NULL);

        //write inode back to disk
        write_inode(this_file_def.inum, this_inode);


        //read the file_def array from disk
        int parent_data_block = parent_inode.addr[0];
        printf("Moving to position %d ", parent_data_block);
        if(lseek(fd,get_data_block_position(parent_data_block),SEEK_SET)<=0){
            printf("Error occurred while reading parent data block ");
            exit(-1);
        }
        struct file_def temp_file_def_array[32];
        if(read(fd,&temp_file_def_array,sizeof(temp_file_def_array))<=0){
            printf("Error occurred while reading parent data block ");
            exit(-1);
        }

        //add an entry to the file_def array and write it back to disk
        int i;
        for(i=0;i<32;i++){
            printf("Traversing parent. Got inode# %d\n", temp_file_def_array[i].inum);
            if(temp_file_def_array[i].inum == 0 ){
                temp_file_def_array[i]= this_file_def;
                
                //move to the position of the parent_data_block and write the file_def array to disk
                if(lseek(fd,get_data_block_position(parent_data_block),SEEK_SET)<0){
                    printf("Error occurred while writing new file to parent data block");
                    exit(-1);
                }
                if(write(fd,&temp_file_def_array,sizeof(temp_file_def_array))<0){
                    printf("Error occurred while writing parent data block to disk");
                    exit(-1);
                }
                break;
            }
            else if(temp_file_def_array[i].inum == this_file_def.inum){
                //we dont have to do anything. 
            }
        }
    }
    else{
        printf("Directory already exists and the inum is %d!\n", this_file_def.inum);
    }
}