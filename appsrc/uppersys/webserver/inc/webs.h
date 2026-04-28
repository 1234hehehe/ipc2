#ifndef WEBS_H
#define WEBS_H

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <pthread.h>
#include "cjson.h"
#include "goahead.h"

/*
#define min(X,Y)  \
(__extension__  \
({  \
   typeof(X) __x=(X), __y=(Y);   \
   (__x<__y)?__x:__y;  \
}) \
)*/

void pruneSessions();
void initWebs(Webs *wp, int flags, int reuse);
void web_process_requests(Webs *wp);
//bool parseIncoming(Webs *wp);
void termWebs(Webs *wp, int reuse);

#endif
