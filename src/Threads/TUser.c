
#include <TUser.h>
#include <stdio.h>
/*
void * TUser_main(void * p_arg){
    TUserArg * arg = (TUserArg *)p_arg;
    
    void * data = NULL;
    char * line = NULL;

    printf("[User %d]: start!\n", arg->id);
    
    while(1){
        printf("%s: In attesa di nuova linea ...\n", label);
        if(BQueue_removeWait(arg->in, &data) != 1){//Error on getting input
            printf("%s: an error occurred on getting string to process.\n", label);
        } else{
            if(data == NULL) {//End of inputs
                BQueue_addWait(arg->out, data);
                break; 
            }
            line = (char *)data;
            sendTokens(line, " ", arg->out);
        }
    }


    printf("%s: end\n", label);
    return (void *)NULL;
	return 0;
}
*/