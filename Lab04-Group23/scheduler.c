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
    int jobStart;
    int completedJob;
    int remaining;

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
    new_job->arrival = arrival;
    new_job->length = length;
    new_job->tickets = tickets;
    new_job->jobStart = -1;
    new_job->completedJob = -1;
    new_job->remaining = length;
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

    printf("End of execution with STCF.\n");
}

void policy_RR(int slice)
{
    printf("Execution trace with RR:\n");

    // TODO: implement RR policy
    int current_time = 0;
    int finished = 0;
    Job *last = NULL;

    while (finished < numofjobs)
    {

        Job *start = (last != NULL && last->next != NULL) ? last->next : head;

        Job *pick = NULL;
        Job *cur = start;
        do
        {
            if (cur->remaining > 0 && cur->arrival <= current_time)
            {
                pick = cur;
                break;
            }
            cur = (cur->next != NULL) ? cur->next : head;
        } while (cur != start);

        if (pick == NULL)
        {
            int next = INT_MAX;
            for (Job *j = head; j != NULL; j = j->next)
            {
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
        if (pick->remaining == 0)
        {
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

    while (finished < numofjobs)
    {
        int total_tickets = 0;
        for (Job *j = head; j != NULL; j = j->next)
        {
            if (j->remaining > 0 && j->arrival <= current_time)
                total_tickets += j->tickets;
        }

        if (total_tickets == 0)
        {
            int next = INT_MAX;
            for (Job *j = head; j != NULL; j = j->next)
            {
                if (j->remaining > 0 && j->arrival < next)
                    next = j->arrival;
            }
            current_time = next;
            continue;
        }

        int winning_ticket = rand() % total_tickets;
        int counter = 0;
        Job *winner = NULL;
        for (Job *j = head; j != NULL; j = j->next)
        {
            if (j->remaining > 0 && j->arrival <= current_time)
            {
                counter += j->tickets;
                if (counter > winning_ticket)
                {
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
        if (winner->remaining == 0)
        {
            winner->completedJob = current_time;
            finished++;
        }
    }

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

        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
               current_time, temp->id, temp->arrival, temp->length);
        temp->jobStart = current_time;
        temp->completedJob = current_time + temp->length;
        current_time += temp->length;
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
            analyze_jobs(jobs, "FIFO");
        }
    }
    else if (strcmp(pname, "SJF") == 0)
    {
        policy_SJF();
        if (analysis == 1)
        {
            analyze_jobs(jobs, "SJF");
        }
    }
    else if (strcmp(pname, "STCF") == 0)
    {
        // TODO
    }
    else if (strcmp(pname, "RR") == 0)
    {
        policy_RR(slice);
        if (analysis == 1)
        {
            analyze_jobs(jobs, "RR");
        }
    }
    else if (strcmp(pname, "LT") == 0)
    {
        policy_LT(slice);
        if (analysis == 1)
        {
            analyze_jobs(jobs, "LT");
        }
    }

    exit(0);
}