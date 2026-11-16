#ifndef ICEBOUND_MISSION_H
#define ICEBOUND_MISSION_H
#include "control.h"
typedef enum { SC_NOMINAL,SC_LINK_LOSS,SC_NAV_STALE,SC_LOW_ENERGY,
    SC_OBSTACLE,SC_DEADLINE,SC_CURRENT } Scenario;
typedef struct {
    IbControl control;
    IbTelemetry t;
    IbCommand cmd;
    Scenario scenario;
    size_t waypoint;
    unsigned ticks;
    double step_s;
} Mission;
int mission_init(Mission *m,const char *name);
int mission_tick(Mission *m,double now,int missed);
void mission_header(void);
void mission_row(const Mission *m,double late_ms);
#endif
