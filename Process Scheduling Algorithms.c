#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>


int timer = 0;
int idp = 0;
int firstlog = 0;
int schedulertype = 1;
int counter = 0;
pthread_mutex_t lock, file_lock;

typedef struct Process
{
    int waitingTime;
    int turnaroundTime;
    int P_id;
    int arrivaltime;
    int startTime;
    int endtime;
    int timeleft;
    int burstTime;
    int priority;
    bool interactive;
    int counter_effect;
    int executed_in_queue;
} Process;

void initialize_process(Process *p)
{
    p->arrivaltime = timer;
    p->startTime = -1;
    p->endtime = -1;
}

void set_process(Process *p, int id, int burst, int prio, int interact, int effect)
{
    p->P_id = id;
    p->burstTime = burst;
    p->priority = prio;
    p->interactive = (interact == 1);
    p->timeleft = burst;
    p->counter_effect = effect;
}

void log_process_to_csv(Process *p, const char *filename, bool write_header)
{
    FILE *csvFile = fopen(filename, "a");

    if (firstlog == 0)
    {
        write_header = true;
        firstlog = 1;
    }

    if (csvFile)
    {
        if (write_header)
        {
            fprintf(csvFile, "Timer,ProcessID,ArrivalTime,StartTime,EndTime,BurstTime,WaitingTime,TurnaroundTime,Priority,Interactive,DivisionType,SchedulerType\n");
        }

        fprintf(csvFile, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",timer, p->P_id, p->arrivaltime, p->startTime, p->endtime, p->burstTime,p->waitingTime, p->turnaroundTime, p->priority, p->interactive ? 1 : 0, schedulertype);

        fclose(csvFile);
    }
}

typedef struct Queue
{
    Process *p;
    int curr;
    int size;
} Queue;

struct ThreadArgs
{
        Queue *queue;
        const char *csvFileName;
};

void initialize_queue(Queue *q, int size)
{
    q->size = size;
    q->curr = -1;
    q->p = (Process *)malloc(size * sizeof(Process));
}

void add_process(Queue *q, Process *newp)
{
    int i = q->curr + 1;
    if (i < q->size) 
    {
        q->p[i] = *newp;
        q->curr = i;
    } 
    else 
    {
        printf("Limit reached\n");
    }
}

bool is_queue_nonempty(Queue *q)
{
    return q->curr >= 0;
}

bool is_queue_full(Queue *q)
{
    return q->curr == 9;
}

void remove_from_queue(Queue *q, int index)
{
    if (index < 0 || index > q->curr)
    {
        printf("Invalid index\n");
        return;
    }
    for (int i = index; i < q->curr; i++)
    {
        q->p[i] = q->p[i + 1];
    }
    q->curr--;
}

Process generator() {
    Process p;
    initialize_process(&p);

    int id = idp + 1;
    int burstT = (rand() % 100) + 1;
    int effect = (rand() % 11) - 5;
    int prio;
    int interact;

    burstT = burstT <= 1 ? 10 : (burstT <= 4 ? 9 : (burstT <= 9 ? 8 : (burstT <= 16 ? 7 : (burstT <= 25 ? 6 : (burstT <= 36 ? 5 : (burstT <= 49 ? 4 : (burstT <= 64 ? 3 : (burstT <= 81 ? 2 : 1))))))));
    prio = (rand() % 100) + 1;
    prio = prio >= 50 ? 3 : (prio >= 20 ? 2 : 1);
    interact = rand() % 2;

    if (interact == 1)
    {
        prio += 2;
    }

    set_process(&p, id, burstT, prio, interact, effect);
    idp++;

    return p;
}

void free_queue(Queue *q)
{
    free(q->p);
}

void queue_transfer(Queue *q1, Queue *q2,int index)
{
    if(q1->curr>-1)
    {
        add_process(q2,&q1->p[index]);
        remove_from_queue(q1,index);
    }
}

void prio_scheduler(Queue *standby, Queue *q1, Queue *q2)
{
    Queue temp;
    initialize_queue(&temp,400);

    while(is_queue_nonempty(q1))
    {
        queue_transfer(q1,&temp,0);
    }

    while(is_queue_nonempty(q2))
    {
        queue_transfer(q2,&temp,0);
    }

    while(is_queue_nonempty(standby))
    {
        queue_transfer(standby,&temp,0);
    }

    int max1=-1, max2=-1;

    for(int i=0;i<10 && is_queue_nonempty(&temp);i++)
    {
        max1=-1;
        max2=-1;
        int j;
        for(j=0;j<=temp.curr;j++)
        {
            if(temp.p[j].priority>max1)
            {
                max1=j;
            }
        }

        queue_transfer(&temp,q1,max1);

        if(is_queue_nonempty(&temp))
        {
            for(j=0;j<=temp.curr;j++)
            {
                if(temp.p[j].priority>max2)
                {
                    max2=j;
                }
            }

            queue_transfer(&temp,q2,max2);
        }
    }

    while(is_queue_nonempty(&temp))
    {
        queue_transfer(&temp,standby,0);
    }

    free_queue(&temp);
}

void fcfs_scheduler(Queue *standby, Queue *q1, Queue *q2)
{
    Queue temp;
    initialize_queue(&temp,400);

    while(is_queue_nonempty(q1))
    {
        queue_transfer(q1,&temp,0);
    }

    while(is_queue_nonempty(q2))
    {
        queue_transfer(q2,&temp,0);
    }

    while(is_queue_nonempty(standby))
    {
        queue_transfer(standby,&temp,0);
    }

    int min1=101,min2=101;

    for(int i=0;i<10 && is_queue_nonempty(&temp);i++)
    {
        min1=101;
        min2=101;
        int j;
        for(j=0;j<=temp.curr;j++)
        {
            if(temp.p[j].arrivaltime<min1)
            {
                min1=j;
            }
        }

        queue_transfer(&temp,q1,min1);

        if(is_queue_nonempty(&temp))
        {
            for(j=0;j<=temp.curr;j++)
            {
                if(temp.p[j].arrivaltime<min2)
                {
                    min2=j;
                }
            }

            queue_transfer(&temp,q2,min2);
        }
    }

    while(is_queue_nonempty(&temp))
    {
        queue_transfer(&temp,standby,0);
    }

    free_queue(&temp);
}

void sjf_scheduler(Queue *standby, Queue *q1, Queue *q2)
{
    Queue temp;
    initialize_queue(&temp,400);

    while(is_queue_nonempty(q1))
    {
        queue_transfer(q1,&temp,0);
    }

    while(is_queue_nonempty(q2))
    {
        queue_transfer(q2,&temp,0);
    }

    while(is_queue_nonempty(standby))
    {
        queue_transfer(standby,&temp,0);
    }

    int min1=11,min2=11;

    for(int i=0;i<10 && is_queue_nonempty(&temp);i++)
    {
        min1=11;
        min2=11;
        int j;
        for(j=0;j<=temp.curr;j++)
        {
            if(temp.p[j].burstTime<min1)
            {
                min1=j;
            }
        }

        queue_transfer(&temp,q1,min1);

        if(is_queue_nonempty(&temp))
        {
            for(j=0;j<=temp.curr;j++)
            {
                if(temp.p[j].burstTime<min2)
                {
                    min2=j;
                }
            }

            queue_transfer(&temp,q2,min2);
        }
    }

    while(is_queue_nonempty(&temp))
    {
        queue_transfer(&temp,standby,0);
    }

    free_queue(&temp);
}

void *consumer_thread(void *args)
{
    struct ThreadArgs
    {
        Queue *queue;
        const char *csvFileName;
    };

    struct ThreadArgs *threadArgs = (struct ThreadArgs *)args;

    Queue *q = threadArgs->queue;
    const char *csvFileName = threadArgs->csvFileName;


    pthread_mutex_lock(&lock);
    if (!is_queue_nonempty(q))
    {
        pthread_mutex_unlock(&lock);
    }

    Process *currentProcess = &q->p[0];
    currentProcess->timeleft--;

    counter+= currentProcess->counter_effect;

    if (currentProcess->timeleft <= 0)
    {
        currentProcess->endtime = timer;
        currentProcess->turnaroundTime = currentProcess->endtime - currentProcess->arrivaltime;
        currentProcess->waitingTime = currentProcess->turnaroundTime - currentProcess->burstTime;

        pthread_mutex_lock(&file_lock);
        log_process_to_csv(currentProcess, csvFileName, false);
        pthread_mutex_unlock(&file_lock);

        remove_from_queue(q, 0);
    }
    pthread_mutex_unlock(&lock);

    usleep(1000);

    return NULL;
}

int main()
{
    Queue q1, q2, standby;
    initialize_queue(&q1, 10);
    initialize_queue(&q2, 10);
    initialize_queue(&standby, 400);

    const char *csvFileName = "process_log.csv";
    const char *csvFileName2 = "queue_log.csv";

    struct ThreadArgs args1 = {&q1, csvFileName};
    struct ThreadArgs args2 = {&q2, csvFileName};

    FILE *csvFile2 = fopen(csvFileName2, "a");
    fprintf(csvFile2,"Time, Q1 executing, Q2 executing\n");

    while (timer < 100 || is_queue_nonempty(&standby) || is_queue_nonempty(&q1) || is_queue_nonempty(&q2))
    {
        for (int i = 0; i < 2; i++)
        {
            if (((rand() % 100) + 1 > 20) && timer <= 100)
            {
                Process p = generator();
                pthread_mutex_lock(&lock);
                add_process(&standby, &p);
                pthread_mutex_unlock(&lock); 
            }
        }

        pthread_mutex_lock(&lock);
        if (schedulertype == 0 && (timer < 100 || timer % 10 == 0))
            prio_scheduler(&standby, &q1, &q2);
        else if (schedulertype == 1 && (timer < 100 || timer % 10 == 0))
            sjf_scheduler(&standby, &q1, &q2);
        else if (schedulertype == 2 && (timer < 100 || timer % 10 == 0))
            fcfs_scheduler(&standby, &q1, &q2);
        fprintf(csvFile2, "%d,%d,%d,\n", timer, q1.p[0].P_id, q2.p[0].P_id);
        pthread_mutex_unlock(&lock);

        pthread_t t1, t2;

        if (is_queue_nonempty(&q1))
        {
            pthread_create(&t1, NULL, consumer_thread, &args1);
            pthread_join(t1, NULL);
        }

        if (is_queue_nonempty(&q2))
        {
            pthread_create(&t2, NULL, consumer_thread, &args2);
            pthread_join(t2, NULL);
        }

    timer++;

    }

    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&file_lock);

    free_queue(&q1);
    free_queue(&q2);
    free_queue(&standby);

    printf("%d",counter);
    return 0;
}
