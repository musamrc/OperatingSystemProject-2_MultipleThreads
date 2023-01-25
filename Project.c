#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int line_count = 0;
int line_count_temp;
char *input_file;
char **writeLineArray;
pthread_mutex_t writeMutex;
pthread_mutex_t upperReplaceMutex;
pthread_mutex_t writeToFileMutex;
pthread_mutex_t printTerminalMutex;
pthread_mutex_t readArrayMutex;
pthread_mutex_t readDoneArrayMutex;
int currentWriteLine = 0; // GetLineNumber'da kullanmak için int degeri
int **replaceWorksOn; //0=not processed 1=working 2=done
int **upperWorksOn;   //0=not processed 1=working 2=done
int **upper_replace_WorkDone;//0=not read yet 1=written into writeLineArray 3=replaced or upper done 5=replaced oand upper done 6=written into text file


//Gets the information of the line to read for its ReadThread
void GetLineNumber()
{
    pthread_mutex_lock(&writeMutex);
    // Self-destruct if all lines in txt file have been read.
    if (currentWriteLine == line_count)
    {
    	pthread_mutex_unlock(&writeMutex);
        pthread_exit(NULL);
    }
    //We open the file and save what is written on the desired line.  START
    FILE *fp = fopen(input_file, "r");
    if (fp == NULL)
    {
        return; // Dosya açylamady
    }
    char temp[300]; 
    int i = 0;
    while (fgets(temp, 300, fp) != NULL)
    {
    	long pos = ftell(fp);
        	fseek(fp, pos - 1, SEEK_SET);
			char c = fgetc(fp);
        if (i == currentWriteLine)
        {
       		if(currentWriteLine+1 == line_count){
			 int x = strlen(temp);
			 temp[x-1] = c;
			 if(temp[x] == '\n')
			 temp[x] = '\"';
			int j;
			 for(j = x+1 ;j<300;j++){
			 		temp[j] = '\0';
				 }
			 }
			 else if(currentWriteLine+1 != line_count){
			int x = strlen(temp);
			 temp[x-1] = '\"';		 
			 }
            fclose(fp);
            break;
        }

        i++;
    }
    //We open the file and save what is written on the desired line.  END
    
            //Print the information
        	pthread_mutex_lock(&printTerminalMutex);
	        printf("Read_%d read the line %d which is \"%s\n",pthread_self(), currentWriteLine + 1,temp);
			pthread_mutex_unlock(&printTerminalMutex);

	currentWriteLine++;
    pthread_mutex_unlock(&writeMutex);
    read_lines_from_file((currentWriteLine - 1 ));
}

//The file reads line by line and saves the lines in an array
void read_lines_from_file(int lineNumber)
{
    FILE *fp = fopen(input_file, "r");
    if (fp == NULL)
    {
        return; // Dosya acilamadi
    }

    char line[300];
    int i = 0;
    while (fgets(line, 300, fp) != NULL)
    {
        // Dynamically allocates space for the row
        if (i == lineNumber)
        {
pthread_mutex_lock(&readArrayMutex);
            writeLineArray[i] = malloc(strlen(line)+1);
            strcpy(writeLineArray[i], line);
            upper_replace_WorkDone[i] = 1;
            fclose(fp);
pthread_mutex_unlock(&readArrayMutex);
            GetLineNumber();
        }
        i++;
    }
}
//Sends UpperThread's to UpperandReplaceGetWorkNumber Method
void UpperThreadFunction(){
	UpperandReplaceGetWorkNumber('u');
}
// Sends Replace Thread's to UpperandReplaceGetWorkNumber Method
void ReplaceThreadFunction(){
	UpperandReplaceGetWorkNumber('r');
}

//Gives tasks Upper and Replace Threads one by one
void UpperandReplaceGetWorkNumber(char threadType){

	pthread_mutex_lock(&upperReplaceMutex);

	int a = 0;
	//Checks the entire array starting at line 0
	while (a<line_count_temp){
	pthread_mutex_lock(&readDoneArrayMutex);
		if(upper_replace_WorkDone[a] == 1){ //If upper and replace did not operate on the current line
			if(threadType == 'u'){  //If the thread is Upper Threat
				upperWorksOn[a] = 1;
				int x = upper_replace_WorkDone[a];
                upper_replace_WorkDone[a] = x + 2;
				pthread_mutex_unlock(&upperReplaceMutex);
				pthread_mutex_unlock(&readDoneArrayMutex);
				ChangeToUpper(a);
			}
			if(threadType == 'r'){ //If the thread is Replace Threat
				replaceWorksOn[a] = 1;
				int x = upper_replace_WorkDone[a];
                upper_replace_WorkDone[a] = x + 2;
				pthread_mutex_unlock(&upperReplaceMutex);
				pthread_mutex_unlock(&readDoneArrayMutex);
				ReplaceSpace(a);
			}
		}
		if(upper_replace_WorkDone[a] == 3){  // If one of the upper or replace threads performed an action on the current line
			if(threadType == 'u'){  //If the thread is Upper Threat
				if(upperWorksOn[a] == 0 && replaceWorksOn[a] == 2){  //Checks if replace thread finished its job
				upperWorksOn[a] = 1;
				int x = upper_replace_WorkDone[a];
                upper_replace_WorkDone[a] = x + 2;   
				pthread_mutex_unlock(&upperReplaceMutex);
				pthread_mutex_unlock(&readDoneArrayMutex);
				ChangeToUpper(a);

				}
			}
			if(threadType == 'r'){  //If the thread is Replace Threat
				if(replaceWorksOn[a] == 0 && upperWorksOn[a] == 2){  //Checks if upper thread finished its job
					replaceWorksOn[a] = 1;
					int x = upper_replace_WorkDone[a];
                    upper_replace_WorkDone[a] = x + 2;
                    pthread_mutex_unlock(&readDoneArrayMutex);
					pthread_mutex_unlock(&upperReplaceMutex);
					ReplaceSpace(a);

				}
			}
		}
		a++;
		pthread_mutex_unlock(&readDoneArrayMutex);
	}
			
	a= 0;
	// self-destruct if all lines were edited by upper and replace threads
	while(a < line_count_temp){
	pthread_mutex_lock(&readDoneArrayMutex);
		if(upper_replace_WorkDone[a] == 5){
			a++;
			if(a = line_count){	
			pthread_mutex_unlock(&readDoneArrayMutex);
			pthread_mutex_unlock(&upperReplaceMutex);
			pthread_exit(NULL);
			}
		}
		else if (upper_replace_WorkDone[a] != 5){
		pthread_mutex_unlock(&readDoneArrayMutex);
			a= line_count +1 ;
		}
	}
	pthread_mutex_unlock(&upperReplaceMutex);
	
	//Re-queue if there's nothing to do and all rows haven't been edited yet
	if(threadType == 'r'){
		ReplaceThreadFunction();
	}
	if(threadType == 'u'){
		UpperThreadFunction();
	}
	
}

void ChangeToUpper(int a){
	int i ,length;
	char *temp;
	
pthread_mutex_lock(&readArrayMutex);
            //This part is for use it on printf. START
            length = strlen(writeLineArray[a]);
            temp = malloc(length +1);
            strcpy(temp, writeLineArray[a]);
            if(temp[length-1] == '\n'){
        	temp[length-1] = '\0';
	        }
	         else if(temp[length] != '\n' && a+1 == line_count){
	           	temp[length] = '\0';
	        }
	        //This part is for use it on printf. END
	// Loop through each element of the array and convert lowercase letters to uppercase
    for (i = 0; writeLineArray[a][i] != '\0'; i++) {
        if (islower(writeLineArray[a][i])) {  //if lowercase
            writeLineArray[a][i] = toupper(writeLineArray[a][i]);  // change to uppercase
        }
    }
    //If last character is \n remove it
    if(writeLineArray[a][i-1] == '\n'){
    	writeLineArray[a][i-1] = '\0';
	}
	 //If last character is \n remove it
	if(writeLineArray[a][i] == '\n')
		writeLineArray[a][i] = '\0';
	
	  	pthread_mutex_lock(&printTerminalMutex);
        printf("Upper_%d read index %d and converted \"%s\" to \"%s\"\n",pthread_self(),a + 1,temp,writeLineArray[a]);
        pthread_mutex_unlock(&printTerminalMutex);
pthread_mutex_unlock(&readArrayMutex);
	  	

    upperWorksOn[a] = 2;
    UpperThreadFunction();
}
void ReplaceSpace(int a){
	int i ,length;
	char *temp;
pthread_mutex_lock(&readArrayMutex);

            //This part is for use it on printf. START
            length = strlen(writeLineArray[a]);
            temp = malloc(length +1);
            strcpy(temp, writeLineArray[a]);
            if(temp[length-1] == '\n'){
        	temp[length-1] = '\0';
	        }
	         else if(temp[length] != '\n' && a+1 == line_count){
	           	temp[length] = '\0';
	        }
	        //This part is for use it on printf. END
            //Loop through each element of the array 
for (i = 0;writeLineArray[a][i] != '\0'; i++) {
        if (writeLineArray[a][i] == ' ') {  // If there is space
            writeLineArray[a][i] = '_';  // change it to _
        }
    }
	 //If last character is \n remove it
    if(writeLineArray[a][i-1] == '\n'){
    	writeLineArray[a][i-1] = '\0';
	}
	 //If last character is \n remove it
	if(writeLineArray[a][i] == '\n')
		writeLineArray[a][i] = '\0';
	
	
		  	pthread_mutex_lock(&printTerminalMutex);
         printf("Replace_%d read index %d and converted \"%s\" to \"%s\"\n",pthread_self(),a + 1,temp,writeLineArray[a]);
         pthread_mutex_unlock(&printTerminalMutex);
pthread_mutex_unlock(&readArrayMutex);
		  	

	replaceWorksOn[a] = 2;
    ReplaceThreadFunction();
}

//Write threads get a sequence number to understand which line they are processing.
void GetWriteLineNumber(){

	pthread_mutex_lock(&writeToFileMutex);
	int i = 0;
	//If all lines have been written, kill the write threads.
	while(i<line_count){
	pthread_mutex_lock(&readDoneArrayMutex);
		if(upper_replace_WorkDone[i] == 6){
		pthread_mutex_unlock(&readDoneArrayMutex);
			i++;
			if(i == line_count){
				pthread_mutex_unlock(&writeToFileMutex);
				pthread_exit(NULL);
			}
		}
		else{
		pthread_mutex_unlock(&readDoneArrayMutex);
			i = line_count + 1;
		}
	}

i=0;
	while (i<line_count){
	pthread_mutex_lock(&readDoneArrayMutex);
		if(upper_replace_WorkDone[i] == 5 || upper_replace_WorkDone[i] == 6){
		pthread_mutex_unlock(&readDoneArrayMutex);
			i++;
		}
		else{
		pthread_mutex_unlock(&readDoneArrayMutex);
			i=0;	
		}
	}
	i=0;
	pthread_mutex_lock(&readDoneArrayMutex);
	//Sends the thread to the writeThradJob method with the information of which line to read.
	while(i<line_count){
		if(upper_replace_WorkDone[i] == 5){
		pthread_mutex_unlock(&readDoneArrayMutex);
			WriteThreadJob(i);		
		}
		i++;	
	}
	pthread_mutex_unlock(&readDoneArrayMutex);
	pthread_mutex_unlock(&writeToFileMutex);
	GetWriteLineNumber();
}

void WriteThreadJob(int index){
pthread_mutex_lock(&readDoneArrayMutex);
	upper_replace_WorkDone[index] = 6; // Means that this line is all done. (It doesn't matter if we write it at the end or at the beginning of the method because we use mutex)
	pthread_mutex_unlock(&readDoneArrayMutex);
	FILE *dosya;
	dosya = fopen(input_file, "r+");
	int i,totalLetter = 0;
	pthread_mutex_lock(&readArrayMutex);
	//Calculates how many characters we need to go forward to reach the desired line
	for(i=0 ; i<index ; i++){
		totalLetter += strlen(writeLineArray[i]);
		totalLetter++;
	}
	//If we are not going to write text on the first line
    if(index > 0){
    fseek(dosya, totalLetter, SEEK_SET);
    fprintf(dosya, "\n%s", writeLineArray[index]);
	}
	//If we are going to write text on the first line
    else{
    fseek(dosya, totalLetter, SEEK_SET);
    if(writeLineArray[index][totalLetter-1] == '\n')
    writeLineArray[index][totalLetter] = '\0';
    fprintf(dosya, "%s \n", writeLineArray[index]);
	}
    fclose(dosya);
    
    	pthread_mutex_lock(&printTerminalMutex);    	  	
        printf("Writer_%d write line %d back which is \"%s\"\n",pthread_self(), index+1 ,writeLineArray[index]);
        pthread_mutex_unlock(&printTerminalMutex);
        pthread_mutex_unlock(&readArrayMutex);
	  	
	  	
    pthread_mutex_unlock(&writeToFileMutex);
    GetWriteLineNumber();
}
int main(int argc, char *argv[])
{    
    int values[4];
    int i;
        // Takes Inputs from shell command line and store them
    for (i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)
        {
            input_file = argv[i + 1];
        }
        else if (strcmp(argv[i], "-n") == 0)
        {
            for (int j = 0; j < 4; j++)
            {
                values[j] = atoi(argv[i + j + 1]);
            }
        }
    }
    

    FILE *file = fopen(input_file, "r");
    // Gives error if file didn't open
    if (file == NULL)
    {
        printf("Dosya %s açylamady!\n", input_file);
        return 1;
    }
    char line[1024];
    //Calculates how many lines the text file consists of
    while (fgets(line, sizeof(line), file) != NULL)
    {
        line_count++;      
    }
    line_count_temp = line_count;
    fclose(file);
    
    
    writeLineArray = malloc(line_count * sizeof(char *));        //We are opening as much space as the number of rows in the Array.
    replaceWorksOn = malloc(line_count * sizeof(int *));         //We are opening as much space as the number of rows in the Array.
    upperWorksOn = malloc(line_count * sizeof(int *));           //We are opening as much space as the number of rows in the Array.
    upper_replace_WorkDone = malloc(line_count * sizeof(int *)); //We are opening as much space as the number of rows in the Array.
    
    //Sets UpperWorksOn and ReplaceWorksOn and upper_replace_workdone arrays to 0
    for (i = 0; i < line_count ; i++) {
        replaceWorksOn[i] = 0;
    }
    for (i = 0; i < line_count ; i++) {
        upperWorksOn[i] = 0;
    }
    for (i = 0; i < line_count ; i++) {
        upper_replace_WorkDone[i] = 0;
    }

    //It creates an array for threads as many as the number of inputs.
    pthread_t readThreads[values[0]];
    pthread_t upperThreads[values[1]];
    pthread_t replaceThreads[values[2]];
    pthread_t writeThreads[values[3]];

    //Initialize the mutexes
    pthread_mutex_init(&writeMutex, NULL);
    pthread_mutex_init(&upperReplaceMutex, NULL);
    pthread_mutex_init(&writeToFileMutex, NULL);
    pthread_mutex_init(&printTerminalMutex, NULL);
    pthread_mutex_init(&readArrayMutex, NULL);
    pthread_mutex_init(&readDoneArrayMutex, NULL);
    
    // Creating Read,Upper,Replace,Write Threads
    int rc;
    long t;
    for (t = 0; t < values[0]; t++)
    {
        rc = pthread_create(&readThreads[t], NULL, GetLineNumber, NULL);
        if (rc)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
        

    for (t = 0; t < values[1]; t++)
    {
        rc = pthread_create(&upperThreads[t], NULL, UpperThreadFunction, NULL);
        if (rc)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for (t = 0; t < values[2]; t++)
    {
        rc = pthread_create(&replaceThreads[t], NULL, ReplaceThreadFunction, NULL);
        if (rc)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for (t = 0; t < values[3]; t++)
    {
        rc = pthread_create(&writeThreads[t], NULL, GetWriteLineNumber, NULL); 
        if (rc)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
    
    void *status;
    //Join all the threads
     for (t = 0; t < values[0]; t++)
  {
      pthread_join(readThreads[t], &status);
  }
   for (t = 0; t < values[0]; t++)
  {
      pthread_join(upperThreads[t], &status);
  }
   for (t = 0; t < values[0]; t++)
  {
      pthread_join(replaceThreads[t], &status);
  }
   for (t = 0; t < values[0]; t++)
  {
      pthread_join(writeThreads[t], &status);
  }


    pthread_exit(NULL);

}

