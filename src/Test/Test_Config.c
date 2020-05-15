/**
 * @file Test_configFileParser.c
 * @brief Test for configFileParser
 */

#include <stdio.h>
#include <stdlib.h>
#include <Config.h>

FILE * f = NULL;
long val=-1000;
char str_aux[MAX_DIM_STR_CONF];
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

static void openFile(char * p){
    if(f != NULL) fclose(f);
    f = fopen(p, "r");
}

static void runTest(void (test_fun)(char *), char * p_title){
    tot = 0;
    err = 0; 
    pass = 0;
    setUp();
    printf("*******START TEST - %s **\n",p_title);
    test_fun(p_title);
    tot = pass  +err;
    printf("-------Test summary:\n -passed: %d/%d\n -failed: %d/%d\n",pass, tot, err, tot);
    printf("*******END TEST - %s **\n\n",p_title);
}

static void test1(){
    //Check if all the str_auxues are correctly retrieved from config file
    openFile("./configFiles/Test/config_test1.txt");
    testCaseExe(1,Config_checkFile(f)==1);
    testCaseExe(2, Config_getValue(f, "K", str_aux) == 1 && Config_parseLong(&val, str_aux) == 1 && val == 6 );
    testCaseExe(3, Config_getValue(f, "KS", str_aux) == 1 && Config_parseLong(&val, str_aux) == 1 && val == 3 );
    testCaseExe(4, Config_getValue(f, "C", str_aux) == 1 && Config_parseLong(&val, str_aux) == 1 && val == 50 );
    testCaseExe(5, Config_getValue(f, "E", str_aux) == 1 && Config_parseLong(&val, str_aux) == 1 && val == 3 );
    testCaseExe(6, Config_getValue(f, "T", str_aux) == 1 && Config_parseLong(&val, str_aux) == 1 && val == 200 );
    testCaseExe(7, Config_getValue(f, "P", str_aux) == 1 && Config_parseLong(&val, str_aux) == 1 && val == 100 );
    testCaseExe(8, Config_getValue(f, "S", str_aux) == 1 && Config_parseLong(&val, str_aux) == 1 && val == 20 );
    testCaseExe(9, Config_getValue(f, "S1", str_aux) == 1 && Config_parseLong(&val, str_aux) == 1 && val == 2 );
    testCaseExe(10, Config_getValue(f, "S2", str_aux) == 1 && Config_parseLong(&val, str_aux) == 1 && val == 10 );
    testCaseExe(11, Config_getValue(f, "NP", str_aux) == 1 && Config_parseLong(&val, str_aux) == 1 && val == 2 );
    testCaseExe(12, Config_getValue(f, "BOH!", str_aux) == 0 );

}

static void test2(){
    //Comment check 
    openFile("./configFiles/Test/config_test2.txt");
    testCaseExe(1,Config_checkFile(f)==0);
}

static void test3(){
    //Check if duplication of config elements is
    openFile("./configFiles/Test/config_test3.txt");
    testCaseExe(1,Config_checkFile(f)==0);
}

static void test4(){
    //Check if some config elements are missing
    openFile("./configFiles/Test/config_test4.txt");
    testCaseExe(1,Config_checkFile(f)==1);
    testCaseExe(2, Config_getValue(f, "KS", str_aux) == 0);
    testCaseExe(3, Config_getValue(f, "P", str_aux) == 0);
    testCaseExe(4, Config_getValue(f, "NP", str_aux) == 0);

}

static void test5(){
    //Empty File
    openFile("./configFiles/Test/config_test5.txt");
    testCaseExe(1,Config_checkFile(f)==1);
}

static void test6(){
    //Wrong format
    openFile("./configFiles/Test/config_test6.txt");
    testCaseExe(1,Config_checkFile(f)==0);
    testCaseExe(2, Config_getValue(f, "KS", str_aux) == 0);

}


int main() {
    runTest(test1, "test1");
    runTest(test2, "test2");
    runTest(test3, "test3");
    runTest(test4, "test4");
    runTest(test5, "test5");
    runTest(test6, "test6");
    fclose(f);
	return 0;
}