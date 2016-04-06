#ifndef CGI_H
#define CGI_H
#define INFO os_printf

#include "httpd.h"

int cgiLed(HttpdConnData *connData);
void tplLed(HttpdConnData *connData, char *token, void **arg);
int cgiHEATER(HttpdConnData *connData);
int cgiFAN(HttpdConnData *connData);
int cgiBOILER(HttpdConnData *connData);
void tplHEATER(HttpdConnData *connData, char *token, void **arg);
void tplFAN(HttpdConnData *connData, char *token, void **arg);
void tplBOILER(HttpdConnData *connData, char *token, void **arg);
void tplIndex(HttpdConnData *connData, char *token, void **arg);
int cgiReadFlash(HttpdConnData *connData);
void tplCounter(HttpdConnData *connData, char *token, void **arg);

#endif
