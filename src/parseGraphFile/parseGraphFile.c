#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>





int main(int argc, char *argv[]){
    //3 parameters should be passed to the program
    //  1- path of the output file (also the folder that will contain all files)
    //  2- path of the file containing the nodes
    //  3- path of the file containing all edges

    char *outFileName, *outFilePath, *nodesFilePath, *edgesFilePath;
    int outNameSize;

    if(argc < 4){
        printf("Missing parameters (%d/3): \n", argc-1);
        exit(1);
    }

    //saving the path of the outFile
    outFileName = strdup(argv[1]);      
    if(outFileName==NULL){
        printf("Error saving the path of the outFile\n");
        exit(1);
    }

    //creating a new folder for the graph
    outNameSize = strlen(outFileName);
    outFilePath = (char *) malloc( (2*outNameSize+6)*sizeof(char));
    sprintf(outFilePath,"%s/%s.txt",outFileName ,outFileName);
    printf("%s\n", outFilePath);


    //saving the path of the file containing the nodes
    nodesFilePath = strdup(argv[2]);    
    if(nodesFilePath==NULL){
        printf("Error saving the path of the file containing the nodes\n");
        exit(1);
    }
    
    //saving the path of the file containing the edges
    edgesFilePath = strdup(argv[3]);    
    if(edgesFilePath==NULL){
        printf("Error saving the path of the file containing the edges\n");
        exit(1);
    }

    int fd = open(outFilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
    if(fd < 0){
        perror("Error creating output file: ");
        exit(1);
    }

    return 0;
}












