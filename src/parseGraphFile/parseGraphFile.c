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

typedef struct {
    char *name;
    char *path;
} par_t;

typedef struct{
    int coord1;
    int coord2;
} vert_t;

typedef struct{
    int vert1, vert2;
    short wt;
} edge_t;

long maxThread;

void parseFiles(char *name, char *path);
void *parseDistanceWeight(void *arg);
void *parseTimeWeight(void *arg);

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

        return 0;
    }



    return 0;
}

void parseFiles(char *name, char *path){
    pthread_t tid1, tid2;
    par_t par;

    par.name = name;
    par.path = path;

    printf("\tname: %s\n\tpath: %s\n", name, path);    
    
    // create 2 threads to parallelize the merging of the two files
    pthread_create(&tid1, NULL, parseDistanceWeight, (void *) &par);
    pthread_create(&tid2, NULL, parseTimeWeight, (void *) &par);
    
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    printf("\n");
    return;
}

void *parseDistanceWeight(void *arg){
    char *outFilePath, prefix[4], buf[1024];
    char *coordinatesFile, *distanceFile;
    char type;
    vert_t vert;
    edge_t edge;
    int outFd, num, totVert=0;
    FILE *coordinatesFd, *distanceFd;
    FILE *outASCII;

    //used to center all the coordinates
    long sum1 = 0, sum2 = 0;
    int mean1, mean2;       //truncates mean values

    char *name, *path;
    par_t *par = (par_t *)arg;
    name = par->name;
    path = par->path;
    
    //create the path of the file
    outFilePath = (char *)malloc(100 * sizeof(char));
    sprintf(outFilePath,"%s/%s-distanceWeight.bin",path,name);

    //create the final file
    outFd = open(outFilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXO | S_IRWXG);
    if(outFd < 0){
        printf("Error creating the file %s: ", name);
        perror("");
        exit(1);
    }
    
    char *outFilePathASCII = (char *)malloc(100 * sizeof(char));
    sprintf(outFilePathASCII,"%s/%s-distanceWeight.txt", path, name);
    outASCII = fopen(outFilePathASCII,"w+");

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

            //update the sums
            sum1 += vert.coord1;
            sum2 += vert.coord2;
            
        }
    }

    //start reading all edges
    while(fgets(buf, 1024, distanceFd) != NULL){
        sscanf(buf,"%c",&type);

        //check if an edge has been found
        if(type=='e'){
            sscanf(buf,"%c %d %d %hd", &type, &edge.vert1, &edge.vert2, &edge.wt);
            //write the new edge on the output file
            if(write(outFd, &edge, sizeof(vert_t)) < sizeof(edge_t)){
                perror("Error writing an edge on the output file: ");
                exit(1);
            }
        }
    }

    //write the total amount of nodes
    lseek(outFd,0,SEEK_SET);
    write(outFd,&totVert,sizeof(int));

    fprintf(outASCII, "%d\n", totVert);

    //compute the mean values
    mean1 = sum1 / totVert;
    mean2 = sum2 / totVert;

    printf("mean1: %d\n", mean1);
    printf("mean2: %d\n", mean2);

    //skip first 4 bytes of the binary file
    lseek(outFd, 4, SEEK_SET);

    for(int i=0; i<totVert; i++){
        //read the coordinates
        read(outFd, &vert, sizeof(vert_t));
        vert.coord1 -= mean1;
        vert.coord2 -= mean2;

        //take back the file pointer
        lseek(outFd, -sizeof(vert_t), SEEK_CUR);

        //write the centered coordinates
        write(outFd, &vert, sizeof(vert_t));

        fprintf(outASCII,"%d %d\n", vert.coord1, vert.coord2);
    }

    printf("\tfile: %s\t\tCOMPLETED\n",outFilePath);

    pthread_exit(NULL);
}

void *parseTimeWeight(void *arg){
    char *outFilePath, prefix[4], buf[1024];
    char *coordinatesFile, *timeFile;
    char type;
    vert_t vert;
    edge_t edge;
    int outFd, num, totVert=0;
    FILE *coordinatesFd, *timeFd;
    FILE *outASCII;

    //used to center all the coordinates
    long sum1 = 0, sum2 = 0;
    int mean1, mean2;       //truncates mean values

    char *name, *path;
    par_t *par = (par_t *)arg;
    name = par->name;
    path = par->path;
    
    //create the path of the file
    outFilePath = (char *)malloc(100 * sizeof(char));
    sprintf(outFilePath,"%s/%s-timeWeight.bin",path,name);

    //create the final file
    outFd = open(outFilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXO | S_IRWXG);
    if(outFd < 0){
        printf("Error creating the file %s: ", name);
        perror("");
        exit(1);
    }
    
    char *outFilePathASCII = (char *)malloc(100 * sizeof(char));
    sprintf(outFilePathASCII,"%s/%s-timeWeight.txt", path, name);
    outASCII = fopen(outFilePathASCII,"w+");

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

    //create the path of the file containing all times
    timeFile = (char *)malloc(100 * sizeof(char));
    if(timeFile == NULL){
        printf("Error creating the path of timeFile\n");
        exit(1);
    }
    sprintf(timeFile,"%s/%s_time.txt",path,prefix);
    //open the file containing all times
    timeFd = fopen(timeFile, "r");
    if(timeFd == NULL){
        printf("Error opening %s: ", timeFile);
        perror("");
        exit(1);
    }

    //skip first 4 bytes of the out file. They will store the number of vertex
    lseek(outFd,4,SEEK_SET);

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

            //update the sums
            sum1 += vert.coord1;
            sum2 += vert.coord2;
        }
    }

    //start reading all edges
    while(fgets(buf, 1024, timeFd) != NULL){
        sscanf(buf,"%c",&type);

        //check if an edge has been found
        if(type=='e'){
            sscanf(buf,"%c %d %d %hd", &type, &edge.vert1, &edge.vert2, &edge.wt);
            //write the new edge on the output file
            if(write(outFd, &edge, sizeof(vert_t)) < sizeof(edge_t)){
                perror("Error writing an edge on the output file: ");
                exit(1);
            }
        }
    }

    //write the total amount of nodes
    lseek(outFd,0,SEEK_SET);
    write(outFd,&totVert,sizeof(int));

    fprintf(outASCII, "%d\n", totVert);

    //compute the mean values
    mean1 = sum1 / totVert;
    mean2 = sum2 / totVert;

    //skip first 4 bytes of the binary file
    lseek(outFd, 4, SEEK_SET);

    for(int i=0; i<totVert; i++){
        //read the coordinates
        read(outFd, &vert, sizeof(vert_t));
        vert.coord1 -= mean1;
        vert.coord2 -= mean2;

        //take back the file pointer
        lseek(outFd, -sizeof(vert_t), SEEK_CUR);

        //write the centered coordinates
        write(outFd, &vert, sizeof(vert_t));

        fprintf(outASCII,"%d %d\n", vert.coord1, vert.coord2);
    }


    printf("\tfile: %s\t\tCOMPLETED\n",outFilePath);

    pthread_exit(NULL);
}
