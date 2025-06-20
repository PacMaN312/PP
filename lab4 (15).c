#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>


sem_t sem1; // неблок
sem_t sem2; // блок

pthread_cond_t sig1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t sig2  = PTHREAD_COND_INITIALIZER;
pthread_cond_t sig21  = PTHREAD_COND_INITIALIZER;

int flag1 = 1, flag2 = 0, flag21_1 = 0, flag21_2 = 0;

pthread_t thread1;
pthread_t thread2;
pthread_t thread3;
pthread_t thread4;
pthread_t thread5;
pthread_t thread6;


struct CR2
{
    int             i1, i2;
    unsigned        u1, u2;
    long            l1, l2;
    unsigned long   ul1, ul2;
};

struct CR2 cr2 = {1, 2, 10, 20, 100, 200, 1000, 2000};


struct t_elem
{
    struct t_elem* next;
    int number;
};

struct t_elem* end_q = NULL;
struct t_elem* beg_q = NULL;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mut_sig = PTHREAD_MUTEX_INITIALIZER;

void add_elem()
{
    struct t_elem* p = (struct t_elem*)malloc(sizeof(struct t_elem));

    p->number = (end_q != NULL) ? end_q->number + 1 : 0;

    p->next = end_q;
    end_q = p;

    if (beg_q == NULL)
    {
        beg_q = p;
    }
}

void* get_elem()
{
    if (end_q == NULL)
        return NULL;

    struct t_elem* p = end_q;
    end_q = end_q->next;

    if (end_q == NULL)
    {
        beg_q = NULL;
    }

    return p;
}

void* thread_producer(void* arg)
{
    int num = *(int*)arg;

    while (1)
    {

        pthread_mutex_lock (&mut);

        if (beg_q == NULL)
            break;


        while (flag1 == 0)
        {
            pthread_cond_wait (&sig2, &mut);
        }


        add_elem();

        printf("Producer thread%d: element %d CREATED;\n",
               num,end_q->number);

        flag2 = 1;
        pthread_cond_broadcast (&sig1);

        pthread_mutex_unlock (&mut);

        if (num == 2)
        {
            pthread_mutex_lock(&mut_sig);
            while(flag21_2 == 0 || flag21_1 == 0)
            {
                pthread_cond_wait(&sig21, &mut_sig);

            }
            pthread_mutex_unlock(&mut_sig);


            printf("ul1 old before fetch_add: %lu\n", __atomic_fetch_add(&cr2.ul1, 100, __ATOMIC_SEQ_CST));
            printf("ul1 after fetch_add: %lu\n", cr2.ul1);
            printf("i1 after add: %d\n", __atomic_add_fetch(&cr2.i1, 5, __ATOMIC_SEQ_CST));
        }

    }


    pthread_cancel(thread2);
    pthread_cancel(thread3);
    pthread_cancel(thread4);
    pthread_cancel(thread5);
    pthread_cancel(thread6);

    printf("Producer thread%d  stopped !!!\n",num);

    return NULL;
}


void* thread_consumer (void* arg)
{

    int num = *(int*)arg;


    struct t_elem* curr_elem=NULL;

    while (1)
    {


        pthread_mutex_lock (&mut);

        while (flag2 == 0)
        {
            pthread_cond_wait (&sig1, &mut);
        }

        curr_elem = get_elem();

        printf("Consumer thread%d: element %d TAKEN;\n",
               num,curr_elem->number);

        flag1 = 1;
        pthread_cond_broadcast (&sig2);


        pthread_mutex_unlock (&mut);

        if( num == 5)
        {
            pthread_mutex_lock(&mut_sig);
            while(flag21_2 == 0)
            {
                pthread_cond_wait(&sig21, &mut_sig);

            }
            pthread_mutex_unlock(&mut_sig);
            printf("i2 old before fetch_and: %d\n", __atomic_fetch_and(&cr2.i2, 0xF0F0, __ATOMIC_SEQ_CST));
            printf("i2 after fetch_and: %d\n", cr2.i2);
        }
        free(curr_elem);

    }

    printf("Consumer thread%d  stopped !!!\n",num);

    return NULL;
}

void* p3 (void* arg)
{

    while(1)
    {
        printf("l1 after nand: %ld\n", __atomic_nand_fetch(&cr2.l1, 0xFF, __ATOMIC_SEQ_CST));


        if (sem_trywait(&sem1) == 0)   //синхронізація
        {
            sem_post(&sem2);
        }

        pthread_mutex_lock(&mut_sig);
        flag21_1 = 1;
        pthread_cond_signal(&sig21);
        pthread_mutex_unlock(&mut_sig);

        printf("u2 old before fetch_xor: %u\n", __atomic_fetch_xor(&cr2.u2, 0xAAAA, __ATOMIC_SEQ_CST));
        printf("u2 after fetch_xor: %u\n", cr2.u2);

    }
}

void* p6 (void* arg)
{

    printf("u1 after or: %u\n", __atomic_or_fetch(&cr2.u1, 0x0F, __ATOMIC_SEQ_CST));
    while(1)
    {


        sem_wait(&sem2); // синхронізація
        sem_post(&sem1);
        pthread_mutex_lock(&mut_sig);
        flag21_2 = 1;
        pthread_cond_broadcast(&sig21);
        pthread_mutex_unlock(&mut_sig);


        if (__atomic_compare_exchange_n(&cr2.l2, &cr2.l2, 999, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
        {
            printf("compare_exchange_n success, l2: %ld\n", cr2.l2);
        }
        else
        {
            printf("compare_exchange_n failed, l2: %ld\n", cr2.l2);
        }


    }

}


int main()
{
    sem_init(&sem1, 0, 1);
    sem_init(&sem2, 0, 0);
    int length_at_start=10;
    int i;


    for(i=0; i<length_at_start; i++)
    {
        add_elem();
    }
    printf("Start with elements from 0-th to %d-th has been created !!!\n",length_at_start-1);


    int thread1_number=1;
    int thread2_number=2;
    int thread4_number=4;
    int thread5_number=5;


    pthread_create (&thread1,NULL,&thread_producer,(void*)&thread1_number);
    pthread_create (&thread2,NULL,&thread_producer,(void*)&thread2_number);
    pthread_create (&thread4,NULL,&thread_consumer,(void*)&thread4_number);
    pthread_create (&thread5,NULL,&thread_consumer,(void*)&thread5_number);
    pthread_create (&thread3,NULL,&p3,NULL);
    pthread_create (&thread6,NULL,&p6,NULL);


    pthread_join(thread1,NULL);



    printf("All threads stopped !!!\n");

    pthread_mutex_destroy (&mut);

    pthread_cond_destroy(&sig1);
    pthread_cond_destroy(&sig2);

    sem_destroy(&sem1);
    sem_destroy(&sem2);

    return 0;
}
