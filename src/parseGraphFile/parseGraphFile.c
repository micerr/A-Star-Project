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

typedef struct{
    pthread_t tid;
    int outFd;      //descriptord of the output file
    FILE *intFd;    //descriptor of the input file
    int pos;        //number of lines to be skipped
    long inputSize; //total size of the input file
} thStruct_t;

typedef struct{
    char c;
    int num;
    int coord1;
    int coord2;
} vert_t;

typedef struct{
    char c;
    int vert1, vert2;
    int wt;
} edge_t;

long maxThread;

void parseFiles(char *name, char *path);
void parseDistanceWeight(char *name, char *path);
void parseTimeWeight(char *name, char *path);
void *readVertex(void *par);

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
    printf("\tname: %s\n\tpath: %s\n", name, path);    
    
    parseDistanceWeight(name, path);
    parseTimeWeight(name, path);

    printf("\n");
    return;
}

void parseDistanceWeight(char *name, char *path){
    char *outFilePath, prefix[4], buf[1024];
    char *coordinatesFile, *distanceFile;
    int outFd, num;
    struct stat st;
    FILE *coordinatesFd, *distanceFd;
    thStruct_t *thArray = (thStruct_t *)malloc(maxThread*sizeof(thStruct_t));
    
    //create the path of the file
    outFilePath = (char *)malloc(100 * sizeof(char));
    sprintf(outFilePath,"%s/%s-distanceWeight.txt",path,name);
    printf("\tfinal file: %s\n",outFilePath);

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

    //create the path of the file containing all distances
    distanceFile = (char *)malloc(100 * sizeof(char));
    if(distanceFile == NULL){
        printf("Error creating the path of distanceFile\n");
        exit(1);
    }
    sprintf(distanceFile,"%s/%s_distance.txt",path,prefix);
    //open the file containing all distances
    distanceFd = fopen(distanceFile, "r");
    if(distanceFd == NULL){
        printf("Error opening %s: ", distanceFile);
        perror("");
        exit(1);
    }

//START MERGING
    
    //skip first 4 bytes of the out file. They will store the number of vertex
    lseek(outFd,4,SEEK_SET);

    //skip first 6 rows of the file containing the coordinates
    for(int i=0; i<=6; i++)
        fgets(buf,1024,coordinatesFd);

    //retrieve size of the input file
    if(stat(coordinatesFile, &st) < 0){
        perror("Errore retrieving the size of the file: ");
        exit(1);
    }
    

    //create N thread to read all vertexes
    for(int i=0; i<maxThread; i++){
        thArray[i].outFd = outFd;
        thArray[i].intFd = distanceFd;
        thArray[i].pos = i;
        thArray[i].inputSize = st.st_size;
        pthread_create(&thArray[i].tid,NULL,readVertex,(void*)&thArray[i]);        
    }

    //wait their termination
    for(int i=0; i<maxThread; i++)
        pthread_join(thArray[i].tid,NULL);
    
    return;
}


void parseTimeWeight(char *name, char *path){
    char *outFilePath;
    int fd;
    
    //create the path of the file
    outFilePath = (char *)malloc(100 * sizeof(char));
    sprintf(outFilePath,"%s/%s-timeWeight.txt",path,name);
    printf("\tfinal file: %s\n",outFilePath);

    //create the final file
    fd = open(outFilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXO | S_IRWXG);
    if(fd < 0){
        printf("Error creating the file %s: ", name);
        perror("");
        exit(1);
    }

    return;
}

void *readVertex(void *par){
    thStruct_t *thS = (thStruct_t *)par;

    long offset = thS->pos;
    while(1){
        printf("1");
    }
    
    
    pthread_exit(NULL);
}