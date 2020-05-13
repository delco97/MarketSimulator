/**
 * @file Test_SQueue.c
 * @brief Used to test SQueue funcions.
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <SQueue.h>
#include <pthread.h>

//Testing variables
static int testId = 0;
static int err = 0; //test cases error counter
static int pass = 0; //test cases passed counter

void setupTest(){
    testId = 0;
    err = 0; //test cases error counter
    pass = 0; //test cases passed counter
}

void testCaseExe(int p_exp);

void testCaseExe(int p_exp) {
    printf("Test %3d: ",testId);
    if(p_exp){ printf("passed.\n"); pass++;}
    else {printf("failed.\n"); err++;}
    testId++;
}

void toString_int(char *p_buf, size_t p_s, void * p_data) {
    snprintf(p_buf, p_s, "%d", *(int*)p_data);
}

void freeNodeData_int(void * data){
    free(data);
}

void test_SingleThread(){
    int tot=0;

    SQueue * q = NULL;
    void * aux = NULL;
    int * x;

    setupTest();
    
    printf("**START TEST - test_SingleThread**\n");
    
    testCaseExe((q = SQueue_init(5, freeNodeData_int))!= NULL );
    testCaseExe(SQueue_isEmpty(q)==1);
    testCaseExe(SQueue_isFull(q)==0);
    testCaseExe(SQueue_dim(q)==0);
    testCaseExe(SQueue_pop(q,&aux)==-2);
    SQueue_print(q, toString_int);
    
    testCaseExe(SQueue_pop(q, &aux) == -2); 
    x = malloc(sizeof(int)); *x = 1; 
    testCaseExe(SQueue_push(q, x) == 1);
    testCaseExe(SQueue_pop(q, &aux) == 1); 
    testCaseExe(*((int *)aux) == 1);free(aux);
    testCaseExe(SQueue_isEmpty(q)==1);
    testCaseExe(SQueue_isFull(q)==0);
    x = malloc(sizeof(int)); *x = 1; 
    testCaseExe(SQueue_push(q, x) == 1);
    testCaseExe(SQueue_dim(q)==1);    
    x = malloc(sizeof(int)); *x = 2; 
    testCaseExe(SQueue_push(q, x) == 1);
    x = malloc(sizeof(int)); *x = 3; 
    testCaseExe(SQueue_push(q, x) == 1);    
    x = malloc(sizeof(int)); *x = 4; 
    testCaseExe(SQueue_push(q, x) == 1);    
    x = malloc(sizeof(int)); *x = 5; 
    testCaseExe(SQueue_push(q, x) == 1);    
    x = malloc(sizeof(int)); *x = 6; 
    testCaseExe(SQueue_push(q, x) == -2); free(x);      
    testCaseExe(SQueue_dim(q)==5);         
    SQueue_print(q, toString_int);
    
    testCaseExe(SQueue_pop(q, &aux) == 1);
    testCaseExe(*((int *)aux) == 1);free(aux);
    testCaseExe(SQueue_pop(q, &aux) == 1);
    testCaseExe(*((int *)aux) == 2);free(aux);
    testCaseExe(SQueue_pop(q, &aux) == 1);
    testCaseExe(*((int *)aux) == 3);free(aux);    
    testCaseExe(SQueue_pop(q, &aux) == 1);
    testCaseExe(*((int *)aux) == 4);free(aux);     
    testCaseExe(SQueue_pop(q, &aux) == 1);
    testCaseExe(*((int *)aux) == 5);free(aux);
    testCaseExe(SQueue_pop(q, &aux) == -2);
    testCaseExe(SQueue_dim(q)==0); 
    SQueue_deleteQueue(q);

    printf("**END TEST - test_SingleThread**\n");
    
    tot = pass + err;
    printf("Test summary:\n -passed: %d/%d\n -failed: %d/%d\n",pass, tot, err, tot);
}

void * Producer(void * p_arg){
    int c=1;
    int * x = NULL;
    SQueue * q = (SQueue *)p_arg;
    char label[5] = "P";

    while(c <= 10){
        x = malloc(sizeof(int));
        *x = c;
        printf("[%s] Wait to send ...\n", label);
        SQueue_pushWait(q, x);
        printf("[%s] sent %d\n", label, *x);
        sleep(1);
        c++;
    }
    return NULL;
}

void * Consumer(void * p_arg){
    void * x = NULL;
    SQueue * q = (SQueue *)p_arg;
    char label[5] = "C";
    while(1){
        printf("[%s] Wait to consume ...\n", label);
        SQueue_popWait(q, &x);
        printf("[%s] Consumed: %d\n", label, *(int *)x);
        if(*(int *)x == 10){
            free(x);
            break;
        }
        sleep(2);
        free(x);
    }
    return NULL;
}

void test_MultiThread(){
    int tot=0;

    SQueue * q = NULL;
    pthread_t th_producer, th_consumer;

    setupTest();
    
    printf("**START TEST - test_MultiThread**\n");
    q = SQueue_init(-1, freeNodeData_int);
    pthread_create(&th_producer, NULL, Producer, q);
    pthread_create(&th_consumer, NULL, Consumer, q);    
    pthread_join(th_producer, NULL);
    pthread_join(th_consumer, NULL);
    printf("**END TEST - test_MultiThread**\n");
    
    tot = pass + err;
    printf("Test summary:\n -passed: %d/%d\n -failed: %d/%d\n",pass, tot, err, tot);
}

int main() {
    test_SingleThread();
    test_MultiThread();
    return 0;
}
