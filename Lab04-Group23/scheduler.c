#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))

// total jobs
int numofjobs = 0;

typedef struct Job
{
    // job id is ordered by the arrival; jobs arrived first have smaller job id, always increment by 1
    int id;
    int arrival; // arrival time; safely assume the time unit has the minimal increment of 1
    int length;
    int tickets; // number of tickets for lottery scheduling
    // TODO: add any other metadata you need to track here
    int remaining;
    int jobStart;
    int completedJob;

    struct Job *next;
} Job;

// the workload list
Job *head = NULL;

void append_to(Job **head_pointer, int arrival, int length, int tickets)
{
    // TODO: create a new job and init it with proper data
    Job *new_job = malloc(sizeof(Job));
    if (new_job == NULL)
    {
        perror("malloc failled");
    }

    new_job->id = numofjobs; // setting new_job id to current numberofjobs, start from 0
    numofjobs++;             // incrementing numberofjobs
    new_job->length = length;
    new_job->arrival = arrival;
    new_job->tickets = tickets;

    new_job->remaining = length;

    new_job->jobStart = -1;
    new_job->completedJob = -1;
    new_job->next = NULL;

    // if empty list this is the first job

    if (*head_pointer == NULL)
    {
        *head_pointer = new_job;
        return;
    }

    // otherwise if the last job

    Job *current = *head_pointer;

    while (current->next != NULL)
    {
        current = current->next;
    }
    // new job to the end of the list
    current->next = new_job;
}

void read_job_config(const char *filename)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int tickets = 0;

    char *delim = ",";
    char *arrival = NULL;
    char *length = NULL;

    // TODO, error checking
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("file open error");
        exit(EXIT_FAILURE);
    }
    // TODO: if the file is empty, we should just exit with error

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);

    if (size == 0)
    {
        fprintf(stderr, "file is empty");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    rewind(fp); // putting pointer back to start before reading

    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (line[read - 1] == '\n')
            line[read - 1] = 0;
        arrival = strtok(line, delim);
        length = strtok(NULL, delim);
        tickets += 100;

        append_to(&head, atoi(arrival), atoi(length), tickets);
    }

    fclose(fp);
    if (line)
        free(line);
}

void policy_SJF()
{
    printf("Execution trace with SJF:\n");

    // TODO: implement SJF policy
    int current_time = 0;

    while (head != NULL)
    {
        if (head->arrival > current_time)
        {
            current_time = head->arrival;
        } // head is typically always starting point, if not at t=0, then change to head time

        Job *shortest = head;
        Job *beforeShortest = NULL;
        Job *temp = head;
        Job *beforeTemp = NULL;

        while (temp != NULL)
        {
            if (temp->arrival > current_time)
                break; // everything that arrives during previous runtime can now run if shortest
            if (temp->length < shortest->length)
            {
                shortest = temp;
                beforeShortest = beforeTemp;
            }
            beforeTemp = temp;
            temp = temp->next;
        }

        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", current_time, shortest->id, shortest->arrival, shortest->length);
        shortest->jobStart = current_time;
        shortest->completedJob = current_time + shortest->length;
        current_time += shortest->length;

        if (beforeShortest == NULL)
            head = shortest->next; // if shortest is the head, assign new head to next node
        else
            beforeShortest->next = shortest->next; // otherwise, remove current shortest from list as normal
    }

    printf("End of execution with SJF.\n");
}

void policy_STCF()
{
    printf("Execution trace with STCF:\n");

    // TODO: implement STCF policy
    int current_time = 0;
    int completed = 0;
    int start = 0;
    Job *running = NULL;

    while (completed < numofjobs){

        Job *shortest = NULL;
        Job *temp = head;
        while (temp != NULL){
            if (temp->arrival <= current_time && temp->remaining > 0){
                if (shortest == NULL || temp->remaining < shortest->remaining)
                    shortest = temp;
            }
            temp = temp->next;

        }


        if ( shortest != running){
            if (running != NULL)
            {
                printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", start, running->id, running->arrival, current_time - start);
            }
            running = shortest;
            start = current_time;
            if (running != NULL && running->jobStart == -1)
                running->jobStart = current_time;
        }

        if (running == NULL){
            Job *next_job = head;
            while (next_job != NULL){
                if ( next_job->remaining > 0 && next_job->arrival > current_time)
                    break;
                next_job = next_job->next;
            }

            if ( next_job == NULL)
                break;
            current_time = next_job->arrival;
            continue;
        }
        running->remaining--;
        current_time++;
        if (running->remaining == 0){
            printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", start, running->id, running->arrival, current_time - start);
            running->completedJob = current_time;
            completed++;
            running = NULL;
        }
        
        
    }

    printf("End of execution with STCF.\n");
}

void policy_RR(int slice)
{
    printf("Execution trace with RR:\n");

    // TODO: implement RR policy

    printf("End of execution with RR.\n");
}

void policy_LT(int slice)
{
    printf("Execution trace with LT:\n");

    // Leave this here, it will ensure the scheduling behavior remains deterministic
    srand(42);

    // In the following, you'll need to:
    // Figure out which active job to run first
    // Pick the job with the shortest remaining time
    // Considers jobs in order of arrival, so implicitly breaks ties by choosing the job with the lowest ID

    // To achieve consistency with the tests, you are encouraged to choose the winning ticket as follows:
    // int winning_ticket = rand() % total_tickets;
    // And pick the winning job using the linked list approach discussed in class, or equivalent

    printf("End of execution with LT.\n");
}

void policy_FIFO()
{
    printf("Execution trace with FIFO:\n");
    // TODO: implement FIFO policy
    int current_time = 0;
    Job *temp = head;

    while (temp != NULL)
    {
        if (current_time < temp->arrival)
            current_time = temp->arrival;
        temp->jobStart = current_time;

        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", current_time, temp->id, temp->arrival, temp->length);
        current_time = current_time + temp->length;
        temp->completedJob = current_time;
        temp = temp->next;
    }
    printf("End of execution with FIFO.\n");
}

void analyze_jobs(Job **jobs, const char *name)
{
    printf("Begin analyzing %s:\n", name);

    double totalResponseTime = 0;
    double totalTurnaround = 0;
    double totalWait = 0;

    for (int i = 0; i < numofjobs; i++)
    {
        int responseTime = (jobs[i]->jobStart) - (jobs[i]->arrival);
        int turnaround = (jobs[i]->completedJob) - (jobs[i]->arrival);
        int wait = turnaround - (jobs[i]->length);

        printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n", jobs[i]->id, responseTime, turnaround, wait);

        totalResponseTime += responseTime;
        totalTurnaround += turnaround;
        totalWait += wait;
    }

    printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n",
           totalResponseTime / numofjobs,
           totalTurnaround / numofjobs,
           totalWait / numofjobs);

    printf("End analyzing %s.\n", name);
}

int main(int argc, char **argv)
{

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

    Job **jobs = malloc(numofjobs * sizeof(Job *));
    for (Job *job = head; job != NULL; job = job->next)
        jobs[job->id] = job;

    if (strcmp(pname, "FIFO") == 0)
    {
        policy_FIFO();
        if (analysis == 1)
        {
            // TODO: perform analysis
        }
    }
    else if (strcmp(pname, "SJF") == 0)
    {
        // TODO
        policy_SJF();
        if (analysis == 1)
        {
            analyze_jobs(jobs, "SJF");
        }
    }
    else if (strcmp(pname, "STCF") == 0)
    {
        policy_STCF();

        if (analysis == 1)
        {
            analyze_jobs(jobs, "STCF");
        }
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