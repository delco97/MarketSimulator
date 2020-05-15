/**
 * @file Test_configFileParser.c
 * @brief Test for configFileParser
 */

#include <stdio.h>
#include <stdlib.h>
#include <TUser.h>
#include <time.h>
#include <utilities.h>
#include <unistd.h>

//Testing variables
static int tot = 0;
static int err = 0; //test cases error counter
static int pass = 0; //test cases passed counter

void testCaseExe(int id, int p_exp) {
    printf("Test %3d: ",id);
    if(p_exp){ printf("passed.\n"); pass++;}
    else {printf("failed.\n"); err++;}
}

static void setUp(){

}

static void runTest(void (test_fun)(char *), char * p_title){
    tot = 0;
    err = 0; 
    pass = 0;
    setUp();
    printf("**START TEST - %s **\n",p_title);
    test_fun(p_title);
    printf("**END TEST - %s **\n",p_title);
    tot = pass  +err;
    printf("Test summary:\n -passed: %d/%d\n -failed: %d/%d\n",pass, tot, err, tot);
}

static void test1(){
    User * u = NULL;

    testCaseExe(0, (u =  User_init(5, 20, NULL)) != NULL);
    testCaseExe(1, User_getId(u) == 1);
    testCaseExe(2, User_getProducts(u) == 5);
    testCaseExe(3, User_getShoppingTime(u) == 20);
    testCaseExe(4, User_startThread(u) == 0);
    sleep(2);
    //User should be waiting a exit signal now
    User_setState(u, USR_OUT);
    User_signalExit(u);
    sleep(1);
    testCaseExe(5, User_joinThread(u));
    testCaseExe(5, User_delete(u));

}

int main() {
    runTest(test1, "test1");
	return 0;
}