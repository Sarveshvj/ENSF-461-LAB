#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define min(a,b) (((a)<(b))?(a):(b))

// total jobs
int numofjobs = 0;

typedef struct Job{
    // job id is ordered by the arrival; jobs arrived first have smaller job id, always increment by 1
    int id;
    int arrival; // arrival time; safely assume the time unit has the minimal increment of 1
    int length;
    int tickets; // number of tickets for lottery scheduling
    // TODO: add any other metadata you need to track here
    int jobStart;
    int completedJob;
    int remaining;

    struct Job* next;
} Job;

// the workload list
Job *head = NULL;


void append_to(Job **head_pointer, int arrival, int length, int tickets){
    // TODO: create a new job and init it with proper data
    Job* new_job = malloc(sizeof(Job));
        if (new_job == NULL){
            perror("malloc failled");
        }

        new_job->id = numofjobs;    // setting new_job id to current numberofjobs, start from 0
        numofjobs++;                // incrementing numberofjobs
        new_job->arrival = arrival;
        new_job->length = length;
        new_job->tickets = tickets;

        new_job->jobStart = -1;
        new_job -> completedJob = -1;
        new_job->remaining = length;
        new_job -> next = NULL;

        // if empty list this is the first job

        if (*head_pointer == NULL){
            *head_pointer = new_job;
            return;
        }

        //otherwise if the last job

        Job* current = *head_pointer;

        while (current->next != NULL){
            current = current->next;
        }
        //new job to the end of the list
        current->next = new_job;
}


void read_job_config(const char* filename)
{
    FILE *fp; 
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int tickets  = 0;

    char* delim = ",";
    char *arrival = NULL;
    char *length = NULL;

    // TODO, error checking
    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("file open error");
        exit(EXIT_FAILURE);
    }
    // TODO: if the file is empty, we should just exit with error
    
    fseek (fp, 0, SEEK_END);
    long size = ftell(fp);

    if (size == 0) {
        fprintf(stderr, "file is empty");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    rewind(fp);     // putting pointer back to start before reading

    
    while ((read = getline(&line, &len, fp)) != -1)
    {
        if( line[read-1] == '\n' )
            line[read-1] =0;
        arrival = strtok(line, delim);
        length = strtok(NULL, delim);
        tickets += 100;

        append_to(&head, atoi(arrival), atoi(length), tickets);
    }

    fclose(fp);
    if (line) free(line);
}


void policy_SJF()
{
    printf("Execution trace with SJF:\n");

    // TODO: implement SJF policy
    int current_time = 0;
    Job* currentJob = head;

    while (currentJob != NULL){
        

            


        }
        currentJob = currentJob->next;

    






    printf("End of execution with SJF.\n");

}


void policy_STCF()
{
    printf("Execution trace with STCF:\n");

    // TODO: implement STCF policy

    printf("End of execution with STCF.\n");
}


void policy_RR(int slice)
{
    printf("Execution trace with RR:\n");
	
    // TODO: implement RR policy
    int current_time = 0;
    int finished = 0;
    Job* last = NULL;    
 
    while (finished < numofjobs){
 
        Job* start = (last != NULL && last->next != NULL) ? last->next : head;
 
        Job* pick = NULL;
        Job* cur = start;
        do {
            if (cur->remaining > 0 && cur->arrival <= current_time){
                pick = cur;
                break;
            }
            cur = (cur->next != NULL) ? cur->next : head;
        } while (cur != start);
 
        if (pick == NULL){
            int next = INT_MAX;
            for (Job* j = head; j != NULL; j = j->next){
                if (j->remaining > 0 && j->arrival < next)
                    next = j->arrival;
            }
            current_time = next;
            continue;
        }
 
        int ran = min(slice, pick->remaining);  
 
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
                current_time, pick->id, pick->arrival, ran);
 
        if (pick->jobStart == -1)
            pick->jobStart = current_time;
        pick->remaining -= ran;
        current_time += ran;
        if (pick->remaining == 0){
            pick->completedJob = current_time;
            finished++;
        }
        last = pick;
    }
 
    printf("End of execution with RR.\n");
}


void policy_LT(int slice)
{
    printf("Execution trace with LT:\n");
 
    srand(42);
 
    int current_time = 0;
    int finished = 0;
 
    while (finished < numofjobs){
        int total_tickets = 0;
        for (Job* j = head; j != NULL; j = j->next){
            if (j->remaining > 0 && j->arrival <= current_time)
                total_tickets += j->tickets;
        }
 
        if (total_tickets == 0){
            int next = INT_MAX;
            for (Job* j = head; j != NULL; j = j->next){
                if (j->remaining > 0 && j->arrival < next)
                    next = j->arrival;
            }
            current_time = next;
            continue;
        }
 
        int winning_ticket = rand() % total_tickets;
        int counter = 0;
        Job* winner = NULL;
        for (Job* j = head; j != NULL; j = j->next){
            if (j->remaining > 0 && j->arrival <= current_time){
                counter += j->tickets;
                if (counter > winning_ticket){
                    winner = j;
                    break;
                }
            }
        }
 
        int ran = min(slice, winner->remaining);
 
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
                current_time, winner->id, winner->arrival, ran);
 
        if (winner->jobStart == -1)
            winner->jobStart = current_time;
        winner->remaining -= ran;
        current_time += ran;
        if (winner->remaining == 0){
            winner->completedJob = current_time;
            finished++;
        }
    }
 
    printf("End of execution with LT.\n");
}


void policy_FIFO(){
    printf("Execution trace with FIFO:\n");
    // TODO: implement FIFO policy
    int current_time = 0;
    Job* temp = head;  
    
    while (temp != NULL){
        if(current_time < temp->arrival) current_time = temp->arrival;

        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", 
                current_time, temp->id, temp->arrival, temp->length);
        current_time += temp->length;
        temp = temp->next;
    }
    printf("End of execution with FIFO.\n");
}


int main(int argc, char **argv){

    static char usage[] = "usage: %s analysis policy slice trace\n";

    int analysis;
    char *pname;
    char *tname;
    int slice;


    if (argc < 5)
    {
        fprintf(stderr, "missing variables\n");
        fprintf(stderr, usage, argv[0]);
		exit(1);
    }

    // if 0, we don't analysis the performance
    analysis = atoi(argv[1]);

    // policy name
    pname = argv[2];

    // time slice, only valid for RR
    slice = atoi(argv[3]);

    // workload trace
    tname = argv[4];

    read_job_config(tname);

    if (strcmp(pname, "FIFO") == 0){
        policy_FIFO();
        if (analysis == 1){
            // TODO: perform analysis
            
        }
    }
    else if (strcmp(pname, "SJF") == 0)
    {
        // TODO
        policy_SJF();
    }
    else if (strcmp(pname, "STCF") == 0)
    {
        // TODO
    }
    else if (strcmp(pname, "RR") == 0)
    {
        // TODO
    }
    else if (strcmp(pname, "LT") == 0)
    {
        // TODO
    }

	exit(0);
}
