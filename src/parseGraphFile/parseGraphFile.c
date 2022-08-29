#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <math.h>

typedef struct {
    char *name;
    char *path;
    char *type;
} par_t;

typedef struct{
    int coord1;
    int coord2;
} vert_t;

typedef struct __attribute__((__packed__)) edge_s{
    int vert1, vert2;
    unsigned short wt;
} edge_t;

long maxThread;

void parseFiles(char *name, char *path);
void *parseFCN(void *arg);

int main(int argc, char *argv[]){

    DIR *dirHandler;
    struct dirent *dirEntry;
    struct stat dirInfo;
    char *dirEntryPath, *dirEntryName;

    long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    maxThread = number_of_processors * 2;

    //open the folder "files"
    if((dirHandler = opendir("files")) == NULL){
        perror("Error trying to open 'files' folder: ");
        exit(1);
    }
    else
        printf("'files' folder has been opened\n");

    //access each subfolder
    while( (dirEntry = readdir(dirHandler)) != NULL ){

        //save the name of the file
        if((dirEntryName = strdup(dirEntry->d_name)) == NULL){
            perror("Error saving the name of the entry: ");
            exit(1);
        }

        //create the new path
        dirEntryPath = (char *)malloc(20*sizeof(char));
        if(dirEntryPath == NULL){
            perror("Error creating the new path of the dir entry: ");
            exit(1);
        }
        sprintf(dirEntryPath, "./files/%s",dirEntryName);
        
        //retrieve information about the file
        if(stat(dirEntryPath, &dirInfo) < 0){
            printf("Error retrieving information about %s: ",dirEntryPath);
            perror("");
            exit(1);
        }

        //if the entry is a subfolder, it contains files to be parsed
        if(S_ISDIR(dirInfo.st_mode) && 
           strcmp(dirEntryName,".")!=0 && strcmp(dirEntryName,"..")!=0){
            printf("Sudirectory %s has been found\n",dirEntryName);
            parseFiles(dirEntryName, dirEntryPath);
        }

        //free memory
        free(dirEntryName);
        free(dirEntryPath);
    }

    return 0;
}

void parseFiles(char *name, char *path){
    pthread_t tid1, tid2;
    par_t parDist, parTime;
    char *dist = "distance";
    char *time = "time";

    parDist.name = name;
    parDist.path = path;
    parDist.type = dist;    
    
    parTime.name = name;
    parTime.path = path;
    parTime.type = time;

    printf("\tname: %s\n\tpath: %s\n", name, path);    
    
    // create 2 threads to parallelize the merging of the two files
    pthread_create(&tid1, NULL, parseFCN, (void *) &parDist);
    pthread_create(&tid2, NULL, parseFCN, (void *) &parTime);
    
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    printf("\n");
    return;
}

void *parseFCN(void *arg){
    char *outFilePath, prefix[4], buf[1024];
    char *coordinatesFile, *inFile;
    char type;
    vert_t vert;
    edge_t edge;
    int outFd, num, totVert=0;
    FILE *coordinatesFd, *inFd;

    char *name, *path;
    par_t *par = (par_t *)arg;
    name = par->name;
    path = par->path;
    
    //create the path of the file
    outFilePath = (char *)malloc(100 * sizeof(char));
    sprintf(outFilePath,"%s/%s-%sWeight.bin",path,name, par->type);

    //create the final file
    outFd = open(outFilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXO | S_IRWXG);
    if(outFd < 0){
        printf("Error creating the file %s: ", name);
        perror("");
        exit(1);
    }
    
    //retrieve the prefix of all input files
    sscanf(name,"%d-%s", &num, prefix);

    //create the path of the file containing all coordinates
    coordinatesFile = (char *)malloc(100 * sizeof(char));
    if(coordinatesFile == NULL){
        printf("Error creating the path of coordinatesFile\n");
        exit(1);
    }
    sprintf(coordinatesFile,"%s/%s_coordinates.txt",path,prefix);
    //open the file containing all coordinates
    coordinatesFd = fopen(coordinatesFile, "r");
    if(coordinatesFd == NULL){
        printf("Error opening %s: ", coordinatesFile);
        perror("");
        exit(1);
    }

    //create the path of the file containing all distances/times
    inFile = (char *)malloc(100 * sizeof(char));
    if(inFile == NULL){
        printf("Error creating the path of %sFile\n", par->type);
        exit(1);
    }
    sprintf(inFile,"%s/%s_%s.txt",path,prefix, par->type);
    //open the file containing all distances/times
    inFd = fopen(inFile, "r");
    if(inFd == NULL){
        printf("Error opening %s: ", inFile);
        perror("");
        exit(1);
    }

    //skip first 4 bytes of the out file. They will store the number of vertex
    lseek(outFd, 4, SEEK_SET);

    //start reading all nodes
    while(fgets(buf, 1024, coordinatesFd) != NULL){
        sscanf(buf,"%c",&type);

        //check if a vertex has been found
        if(type=='v'){
            sscanf(buf,"%c %d %d %d", &type, &num, &vert.coord1, &vert.coord2);
            //write the new vertex on the output file
            if(write(outFd, &vert, sizeof(vert_t)) < sizeof(vert_t)){
                perror("Error writing a vertex on the output file: ");
                exit(1);
            }

            //update the total number of nodes
            totVert++;
        }
    }
    
    int wt;

    //start reading all edges
    while(fgets(buf, 1024, inFd) != NULL){
        sscanf(buf,"%c",&type);
        
        //check if an edge has been found
        if(type=='a'){
            sscanf(buf,"%c %d %d %d", &type, &edge.vert1, &edge.vert2, &wt);

            if(wt >= 65535)
                edge.wt = 65535;
            else
                edge.wt = wt;
            
            //write the new edge on the output file
            if(write(outFd, &edge, sizeof(edge_t)) < sizeof(edge_t)){
                perror("(Dist) Error writing an edge on the output file: ");
                exit(1);
            }
        }
    }

    //write the total amount of nodes
    lseek(outFd,0,SEEK_SET);
    write(outFd,&totVert,sizeof(int));

    printf("\tfile: %s\t\tCOMPLETED\n",outFilePath);

    //close all files
    if(close(outFd) < 0){
        perror("(Dist) Error closing outFd: ");
        exit(1);
    }

    if(fclose(coordinatesFd) == EOF){
        perror("(Dist) Error closing coordinatesFd: ");
        exit(1);
    }

    if(fclose(inFd) == EOF){
        perror("(Dist) Error closing inFd: ");
        exit(1);
    }

    //freeing memory used for strings
    free(outFilePath);
    free(coordinatesFile);
    free(inFile);

    pthread_exit(NULL);
}
