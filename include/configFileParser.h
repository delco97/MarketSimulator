#ifndef	_CONFIGFILEPARSER_H
#define	_CONFIGFILEPARSER_H

extern long g_K;
extern long g_KS;
extern long g_C;
extern long g_E;
extern long g_T;
extern long g_P;
extern long g_S;
extern long g_S1;
extern long g_S2;
extern long g_NP;

int parseConfigFile(char * p_f);
void printConfiguration();

#endif	/* _CONFIGFILEPARSER_H */
