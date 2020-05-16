OVERVIEW:
    We are modifying the file system which we created in the part-1 of this project.
    The new methods available in this part are:
        cpin - copies an external file into v6 file system.
        cpout - copies a file from v6 file system to an external file.
        mkdir - creates a directory and sets it's first two entries to . and .. 
        rm - deletes a file, adds the inode to inode[], removes file from parent directory, adds all data blocks to free[].

    All the commands we call are handled in util.c by the handleCommand(command) method.
About the commands available:
    The available commands are:
        - cpin externalfile /path/to/file-in-v6
        - cpout /path/to/file-in-v6 externalfile
        - mkdir /path/to/directory
        - rm /path/to/file-in-v6

    cpin    
        - this command takes two arguments.
            - externalfile
            - path to the file where the contents of external file are to be copied.
        - we check if the path is valid and if the file already exists.
            - if the path is invalid , then the program exits.
            - if the file already exists
                - then we free all the data blocks by traversing through the addr[] in the inode
                - clear the inode (except for the first bit)
            - if the file doesn't exists    
                - we get a new inode from inode[]
                - we get the parent's inode
                - make an entry of filename and its inode in the parent's data block 
        - we start reading the external file 1024 bytes at a time, get a free data block from free[], copy 1024 bytes 
          to the data block and write the data block number in the addr[] of this file's inode.
        - when we read less than 1024 bytes(that means we read the last block of the external file), we execute the loop
          for the last time.
        - we update the size of the file in the inode.
        - we update the flags to 1 00 0 000 111 111 111.
        - we write the inode

    cpout
        - this command takes two arguments.
            - path to the file where the contents of external file are to be copied.
            - externalfile
        - we check if the path is valid .
        - we get the file's inode.
        - we traverse through the addr[] , get the datablocks and copy the data block by block and write into the
          external file.
    mkdir
        - this command takes one arguments  
            - /path/to/directory/directory_name
        - we check if the directory already exists.
            - if it exists, we do nothing.
        - we get the parent's inode
        - we get a free inode from inode[] and a data block from free[].
        - we change the inode flags to 49663(1 10 0 000 111 111 111)
        - we write two entries . and .. in the data block and update addr[0] = data block number.
        - we get the parent's data block(addr[0]) from parent's inode, and write the directory's name and inode to it.
    rm
        - this commad takes one argument 
            - /path/to/file
        - we check if the file is a directory or file   
            - if it is a directory, we check if it is empty
            - if it is not empty, we don't delete it.
        - we get the file's inode and it's parent's inode.
        - we traverse through the file's addr[] and clear the data blocks and add them to free[].
        - we clear the file's inode and add it to inode[]
        - we remove the file's entry from the parent's data block.

Follow the instructions to compile and run the program in README.txt

A sample output is included in Sample_output2.txt
The in and out file are also included which were used to test cpin and cpout

Authors:

    Deepesh - FXD180002
    Akshay Maganti - AXM180074
    Harish Kaza - HXK180013
            