//this mask will be used to check if the given inode is a directory/file
unsigned short mask = 24576; //0110 0000 0000 0000

//splits the path by `/` and returns a double pointer to the resultant array
char **split_path(char *input){
    char **tokens = malloc(100 * sizeof(char*));
    char *token;
    int position = 0;

    token = strtok(input, "/");

    while(token!=NULL){
        tokens[position] = token;
        position++;
        token = strtok(NULL, "/");
    }

    tokens[position] = NULL;
    return tokens;
}

//returns a struct inode after reading from disk for a given inum
struct inode get_inode(int inum){
    //read the inode from the position of inode on the disk
    struct inode this_inode;
    if(lseek(fd, get_inode_position(inum), SEEK_SET)<=0){
        printf("Error while reading the root inode.");
        exit(-1);
    }

    if(read(fd, &this_inode, sizeof(this_inode))<=0){
        printf("Error while reading the root inode.");
        exit(-1);
    }
    return this_inode;
}

//get the position in disk for a given data block
int get_data_block_position(int block_num){
    return (block_num)*1024;
}

//get file def for a given folder and file name
struct file_def get_filedef_for_entry(struct inode parent_inode, char *file_name){
    struct file_def this_file;
    struct file_def empty_file;
    empty_file.inum = 0;
    //get the data block position for the directory
    int parent_data_block_pos = get_data_block_position(parent_inode.addr[0]);
    
    printf("Traversing directory pointing to data block #%d\n", parent_inode.addr[0]);

    //read in the file_def array from the disk
    if(lseek(fd, parent_data_block_pos, SEEK_SET)<0){
        printf("Error while reading inode contents");
        exit(-1);
    }
    struct file_def temp_file_def_array[32];
    if(read(fd,&temp_file_def_array,sizeof(temp_file_def_array))<=0){
        printf("Error occurred while reading parent data block ");
        exit(-1);
    }

    //iterate over the file_def array and return as soon as we find the file.
    int i;
    for(i=0;i<32;i++){
        printf("Got inode# %d file name: %s\n", temp_file_def_array[i].inum, temp_file_def_array[i].filename);
        if(temp_file_def_array[i].inum == 0 ){
            //we reached the end, return empty file
            return empty_file;
        }
        else if(strcmp(temp_file_def_array[i].filename, file_name)==0 ){
             return temp_file_def_array[i];
        }
    }
}

//get next available free inode
int get_a_free_inode(){
    //check if we need to load inodes from the disk
    if(sb.ninode==0){
        int i = 0;
        struct inode i_temp;
        unsigned short c; 
        while(sb.ninode <= sizeof(sb.inode)/sizeof(sb.inode[0])){
            //keep reading inodes from disk
            if(lseek(fd,2048+(i*64),SEEK_SET)<=0){
                    printf("Some unknown error occurred!");
                    exit(-1);
            }
            if(read(fd,&i_temp,sizeof(struct inode))<=0){
                printf("Some unknown error occurred!");
                exit(-1);
            }

            //check if it is free
            c = i_temp.flags;
            if(c>>15 == 0)
            {
                //if free, add to inode array
                add_a_free_inode(i+1);
            }
            i++;
        }
    }

    //return the next available inode
    sb.ninode--;
    return sb.inode[sb.ninode];
}

//free a data block by adding it to the free[]
void free_data_block(int block_num){
    //get the position for this data block
    int position = get_data_block_position(block_num);

    //write zeroes everywhere
    int empty_block[256] = {0};
    if(lseek(fd, position, SEEK_SET)<0){
        printf("Error occurred while freeing the data blocks for existing file");
        exit(-1);
    }

    if(write(fd, &empty_block,1024)<0){
        printf("Error occurred while freeing the data blocks for existing file");
        exit(-1);
    }

    //add to free list
    add_a_free_block(block_num);
}

//clear data blocks occupied by this inode by traversing the free[]
struct inode clear_inode_free_data(struct inode this_inode){
    int i;
    //free the first 10 data blocks directly
    for(i = 0; i<10; i++){
        int block_num = this_inode.addr[i];
        if(block_num==0) break;
        free_data_block(block_num);
    }

    //traverse the triple indirect 11th block
    if(this_inode.addr[10]!=0){
        int first_indirect[256] = {};
        int second_indirect[256] = {};
        int third_direct[256] = {};

        //go to the position of first indirect block and read 256 values from it into the first_indirect array
        int position = get_data_block_position(this_inode.addr[10]);
        if(lseek(fd, position, SEEK_SET)<0){
            printf("Error occurred while freeing the data blocks for existing file");
            exit(-1);
        }

        if(read(fd, &first_indirect, sizeof(first_indirect)) < 0 ){
            printf("Error occurred while freeing the data blocks for existing file");
            exit(-1);
        }

        //iterate over the first indirect block
        for(i = 0; i<256; i++){
            if(first_indirect[i]>0){
                //for each valid block number, read the second_indirect array of 256 block
                position = get_data_block_position(first_indirect[i]);
                if(lseek(fd, position, SEEK_SET)<0){
                    printf("Error occurred while freeing the data blocks for existing file");
                    exit(-1);
                }

                if(read(fd, &second_indirect, sizeof(second_indirect)) < 0 ){
                    printf("Error occurred while freeing the data blocks for existing file");
                    exit(-1);
                }

                //iterate over the second indirect block
                int j;
                for(j = 0; j<256; j++){
                    if(second_indirect[j]>0){
                        //for each valid block number, read the third_direct block
                        position = get_data_block_position(second_indirect[j]);
                        if(lseek(fd, position, SEEK_SET)<0){
                            printf("Error occurred while freeing the data blocks for existing file");
                            exit(-1);
                        }

                        if(read(fd, &third_direct, sizeof(third_direct)) < 0 ){
                            printf("Error occurred while freeing the data blocks for existing file");
                            exit(-1);
                        }

                        //iterate over the third direct block
                        int k;
                        for(k = 0; k < 256; k++){
                            //for each valid data block, free it
                            if(third_direct[k]>0) free_data_block(third_direct[k]);
                            else break;
                        }
                        //free the second_indirect block num
                        free_data_block(second_indirect[j]);
                    }
                    else{
                        break;
                    }
                }
                //free the third_indirect data block
                free_data_block(first_indirect[i]);
            }
            else break;
        }

        //free the addr[10]
        free_data_block(this_inode.addr[10]);
    }
}

//add block to the addr[] for a given inode and maintain the size
struct inode add_block_to_addr(int data_block_num, struct inode temp_inode, int bytes_stored){
    int size = temp_inode.size;
    //check if the size is greater than 10kb. means large file
    if(size>=10*1024){
        int first_indirect, second_indirect, third_direct, 
        first_indirect_array[256], second_indirect_array[256], third_direct_array[256];
        int  size1 = (size - 10*1024) / 1024;

        //calculate the positions in first_indirect, second indirect and third_direct
        int pos1 = size1 / (256*256);
        int pos2 = (size1 - (pos1*256*256))/(256);
        int pos3 = (size1 - (pos1*256*256) - (pos2*256) );

        first_indirect = temp_inode.addr[10];

        //if pos3 is zero, means we need to allocate a new data block for third_direct
        if (pos3 == 0){
            third_direct = allocate_a_data_block();

            //if pos2 is zero, means we need to allocate a new block for the second_indirect
            if(pos2==0){
                second_indirect = allocate_a_data_block();
                
                //if pos2 is zero, means we need to allocate a new block for the first_direct
                if(pos1==0){
                    first_indirect = allocate_a_data_block();
                    temp_inode.addr[10] = first_indirect;
                }
                else{
                    //else get first_direct from arrd[10] and read in the first_indirect array of 256 blocks
                    first_indirect = temp_inode.addr[10];
                    if(lseek(fd, get_data_block_position(first_indirect), SEEK_SET)<0){
                        printf("Error occured while adding the block to addr array.1");
                        exit(-1);
                    }
                    if(read(fd, &first_indirect_array, sizeof(first_indirect_array))<0){
                        printf("Error occured while adding the block to addr array.2");
                        exit(-1);
                    }
                }

                //set the second indirect in first_indirect and write the updated first_direct to disk
                first_indirect_array[pos1] = second_indirect;
                if(lseek(fd,get_data_block_position(first_indirect),SEEK_SET)<0){
                    printf("Error occured while adding the block to addr array.3");
                    exit(-1);
                }
                if(write(fd,&first_indirect_array,sizeof(first_indirect_array))<0){
                    printf("Error occured while adding the block to addr array.4");
                    exit(-1);
                }
            }
            else{
                //else read in the second indirect block from the first_indirect_array
                second_indirect = first_indirect_array[pos1];
                if(lseek(fd, get_data_block_position(second_indirect), SEEK_SET)<0){
                    printf("Error occured while adding the block to addr array.5");
                    exit(-1);
                }
                if(read(fd, &second_indirect_array, sizeof(second_indirect_array))<0){
                    printf("Error occured while adding the block to addr array.6");
                    exit(-1);
                }
            }
            //set the value of third_direct into the correct position in second_indirect_array and write the updated second_indirect_array to disk
            second_indirect_array[pos2] = third_direct;
            if(lseek(fd,get_data_block_position(second_indirect),SEEK_SET)<0){
                printf("Error occured while adding the block to addr array.7");
                exit(-1);
            }
            if(write(fd,&second_indirect_array,sizeof(second_indirect_array))<0){
                printf("Error occured while adding the block to addr array.8");
                exit(-1);
            }
            
        }
        else{
            //read the third_direct block from the disk
            third_direct = second_indirect_array[pos1];
            if(lseek(fd, get_data_block_position(third_direct), SEEK_SET)<0){
                printf("Error occured while adding the block to addr array.9");
                exit(-1);
            }
            if(read(fd, &third_direct_array, sizeof(third_direct_array))<0){
                printf("Error occured while adding the block to addr array.10");
                exit(-1);
            }
        }

        //add the new data block to the third_direct and write it back to disk
        third_direct_array[pos3] = data_block_num;
        if(lseek(fd,get_data_block_position(third_direct),SEEK_SET)<0){
            printf("Error occured while adding the block to addr array.11");
            exit(-1);
        }
        if(write(fd,&third_direct_array,sizeof(third_direct_array))<0){
            printf("Error occured while adding the block to addr array.12");
            exit(-1);
        }
    }
    else{
        //calculate the position in addr[] and add the data_block_num at the position
        int pos0 = size/1024;
        temp_inode.addr[pos0] = data_block_num;
    }
    return temp_inode;
}

//get current and parent file_def for a given path in v6
struct curr_and_parent get_file_def_for_parent(char *path){
    char **tokens;
    char *file_name;
    
    //start at root
    struct inode parent_inode;
    parent_inode = get_inode(1);

    //initialize parent's file def
    struct file_def parent_filedef;
    parent_filedef.inum = 1;
    
    //initialize this file's file def
    struct file_def this_file_def;
    this_file_def.inum = 0;

    //split the path
    tokens = split_path(path);
    
    //initialize an empty file
    struct file_def empty_file;
    empty_file.inum = 0;
    
    //initialize a curr_and_parent object for returning
    struct curr_and_parent curr_parent;
    curr_parent.parent_file = empty_file;
    curr_parent.this_file = empty_file;
    
    //iterate through all the directories
    if(tokens){
        int i;
        for(i = 0; *(tokens + i); i++){
            if(*(tokens + i + 1)){
                //make sure that the current file is a directory
                unsigned short masked_flags = parent_inode.flags & mask;
                if(masked_flags>>13!=2){
                    printf("Not a directory\n");
                    return curr_parent;
                }

                //get the file def for child directory
                parent_filedef = get_filedef_for_entry(parent_inode, *(tokens + i));
                if(parent_filedef.inum == 0){
                    printf("File does not exist\n");
                    return curr_parent;
                }

                //get the inode for the child directory
                parent_inode = get_inode(parent_filedef.inum);

                printf("Traversing %s with inum %d\n", *(tokens +i), parent_filedef.inum);
            }
            else {
                //we reached the last token which is this_file
                file_name = *(tokens+i);
                
                //get the file def for the current file
                this_file_def = get_filedef_for_entry(parent_inode, file_name);
                strcpy(this_file_def.filename, file_name);
            }
        }
        printf("File name: %s\n", file_name);
    }

    //set values in curr_parent object to return
    curr_parent.this_file = this_file_def;
    curr_parent.parent_file = parent_filedef;
    return curr_parent;
}