/**
 * @file Test_configFileParser.c
 * @brief Test for configFileParser
 */

#include <stdio.h>
#include <stdlib.h>
#include <configFileParser.h>

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
    g_K=-10000;
    g_KS=-1000;
    g_C=-1000;
    g_E=-1000;
    g_T=-1000;
    g_P=-1000;
    g_S=-1000;
    g_S1=-1000;
    g_S2=-1000;
    g_NP=-1000;
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
    //Check if all the values are correctly retriven
    testCaseExe(1,parseConfigFile("./configFiles/Test/config_test1.txt")==1);
    testCaseExe(2,g_K==6);
    testCaseExe(3,g_KS==3);
    testCaseExe(4,g_C==50);
    testCaseExe(5,g_E==3);
    testCaseExe(6,g_T==200);
    testCaseExe(7,g_P==100);
    testCaseExe(8,g_S==20);
    testCaseExe(9,g_S1==2);
    testCaseExe(10,g_S2=10);
    testCaseExe(11,g_NP==2);
}

static void test2(){
    //Comment check 
    testCaseExe(1,parseConfigFile("./configFiles/Test/config_test2.txt")==0);
    testCaseExe(2,g_K==6);
}

static void test3(){
    //Check if duplication of config elements is
    testCaseExe(1,parseConfigFile("./configFiles/Test/config_test3.txt")==0);
}

static void test4(){
    //Check if some config element are missing
    testCaseExe(1,parseConfigFile("./configFiles/Test/config_test4.txt")==0);
}

static void test5(){
    //Empty File
    testCaseExe(1,parseConfigFile("./configFiles/Test/config_test5.txt")==0);
}

static void test6(){
    //Wrong format
    testCaseExe(1,parseConfigFile("./configFiles/Test/config_test6.txt")==0);
}

static void test7(){
    //Wrong format
    testCaseExe(1,parseConfigFile("./configFiles/Test/config_test7.txt")==0);
}

static void test8(){
    //Wrong format
    testCaseExe(1,parseConfigFile("./configFiles/Test/config_test8.txt")==0);
}

int main() {

    runTest(test1, "test1");
    runTest(test2, "test2");
    runTest(test3, "test3");
    runTest(test4, "test4");
    runTest(test5, "test5");
    runTest(test6, "test6");
    runTest(test7, "test7");
    runTest(test8, "test8");

	return 0;
}