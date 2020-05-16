/*
Copy an external file to path_in_v6
external_path: /home/user/path/to/externalfile.txt
path_in_v6: /path/to/file_in_v6.txt
*/
void cpin(char *external_path,char *path_in_v6){
    struct file_def parent_filedef;
    struct file_def this_file_def;
    struct curr_and_parent curr_parent;

    //flag to maintain if the entry in parent is to be added or not
    int update_parent = 0;

    //get parent and current file defs
    curr_parent = get_file_def_for_parent(path_in_v6);
    parent_filedef = curr_parent.parent_file;
    this_file_def = curr_parent.this_file;

    if(parent_filedef.inum==0){//if parent does not exist throw error
        printf("Parent directory does not exist\n");
        return;
    }
    //get the parent's inode
    struct inode parent_inode = get_inode(parent_filedef.inum);
    
    if(this_file_def.inum != 0){//if file exists, clear it's data
        //get this file's inode
        struct inode this_inode = get_inode(this_file_def.inum);
        
        //check if this file is a directory
        unsigned short masked_flags = mask & this_inode.flags;
        if(masked_flags>>13==2){
            printf("File already exists and is a directory\n");
            return;
        }

        //if all is well, clear the inode's addr[] and free the data blocks
        clear_inode_free_data(get_inode(this_file_def.inum));
    } 
    else{
        //if file does not exist, get a new free inode
        int free_inum = get_a_free_inode();
        
        this_file_def.inum = free_inum;
        
        //set flag to update parent to add the entry in the parent's data block
        update_parent = 1;
    }

    //open the external file for reading
    int fd1 = open(external_path,'r');
    if(lseek(fd1, 0, SEEK_SET)<0){
            printf("Error occurred while reading external file\n");
            exit(-1);
    }

    //create a temp_inode to write addr[]
    struct inode temp_inode;    
    temp_inode.size = 0;
    
    //keep reading 1024 bytes of data from external file and write data and addr[] to our v6 disk
    char temp_buffer[1024] = {};
    int rs = 0;
    do
    {
        rs = read(fd1, &temp_buffer, sizeof(temp_buffer));
        int fnum = allocate_a_data_block();
        int p = get_data_block_position(fnum);
        if(lseek(fd,p,SEEK_SET)<0){
            printf("Error occurred while copying the file\n");
            exit(-1);
        }
        if(write(fd,temp_buffer,sizeof(temp_buffer))<0){
            printf("Error occurred while copying the file\n");
            exit(-1);
        }
        //printf("Wrote to block #%d\n", fnum);
        temp_inode = add_block_to_addr(fnum,temp_inode, rs);
        temp_inode.size = temp_inode.size + rs;
    }while(rs==1024);

    //set the flags for this inode
    temp_inode.flags = 33279;// 1 00 0 000 111 111 111

    //write the temp_inode to disk
    if(lseek(fd,get_inode_position(this_file_def.inum),SEEK_SET)<0){
        printf("Error occurred while writing new inode to disk");
        exit(-1);
    }
    if(write(fd,&temp_inode,sizeof(temp_inode))<0){
        printf("Error occurred while writing new inode to disk");
        exit(-1);
    }
    
    //update parent's entry only if the file doesn't already exist
    if(update_parent==1){
        //get parent's data block
        int parent_data_block = parent_inode.addr[0];

        //move to parent's data block position and read the array of file defs
        if(lseek(fd,get_data_block_position(parent_data_block),SEEK_SET)<=0){
            printf("Error occurred while reading parent data block ");
            exit(-1);
        }
        struct file_def temp_file_def_array[32];
        if(read(fd,&temp_file_def_array,sizeof(temp_file_def_array))<=0){
            printf("Error occurred while reading parent data block ");
            exit(-1);
        }

        // iterate over the file_def array till the end and add the file def to the 
        // array and write array back to disk
        int i;
        for(i=0;i<32;i++){
            printf("Traversing parent. Got inode# %d\n", temp_file_def_array[i].inum);
            if(temp_file_def_array[i].inum == 0 ){
                temp_file_def_array[i]= this_file_def;

                //write the updated file_def array back to disk
                if(lseek(fd,get_data_block_position(parent_data_block),SEEK_SET)<0){
                    printf("Error occurred while writing new file to parent data block");
                    exit(-1);
                }
                if(write(fd,&temp_file_def_array,sizeof(temp_file_def_array))<0){
                    printf("Error occurred while writing parent data block to disk");
                    exit(-1);
                }

                //we are done
                break;
            }
        }
    }

    //print the written values
    printf("Inode's num is %d size is: %d\n",this_file_def.inum, temp_inode.size);
}