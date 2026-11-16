#include "mission.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static const char *names[]={"nominal","link-loss","nav-stale","low-energy",
    "obstacle","deadline","current"};
static const IbPoint route[]={{20,0},{20,20},{40,20}};
int mission_init(Mission *m,const char *name) {
    memset(m,0,sizeof(*m));
    size_t i;for(i=0;i<sizeof(names)/sizeof(names[0]);i++)
        if(strcmp(names[i],name)==0) break;
    if(i==sizeof(names)/sizeof(names[0])) return -1;
    m->scenario=(Scenario)i; m->step_s=0.1;
    m->t=(IbTelemetry){0,0,0,0,0,i==SC_LOW_ENERGY?2.6:12.0,100,0};
    IbConfig cfg=ib_default_config();return ib_init(&m->control,&cfg,&m->t);
}
int mission_tick(Mission *m,double now,int missed) {
    m->t.now=now; m->t.sample_time=now; m->t.command_time=now;
    m->t.obstacle_m=100;m->t.deadline_missed=missed;
    if(m->scenario==SC_LINK_LOSS&&now>=12) m->t.command_time=12;
    if(m->scenario==SC_NAV_STALE&&now>=8&&now<12) m->t.sample_time=7.9;
    if(m->scenario==SC_OBSTACLE&&now>=8&&now<12) m->t.obstacle_m=0.4;
    if(m->scenario==SC_DEADLINE&&now>=8&&now<8.1) m->t.deadline_missed=1;
    if(m->waypoint<3&&hypot(m->t.x-route[m->waypoint].x,m->t.y-route[m->waypoint].y)<0.12)
        m->waypoint++;
    m->cmd=ib_step(&m->control,&m->t,route[m->waypoint<3?m->waypoint:2],m->waypoint==3);
    return m->cmd.mode==IB_DOCKED||m->cmd.mode==IB_FAULT;
}
void mission_header(void) {
    puts("tick,time_s,x_m,y_m,energy_wh,mode,reason,vx_mps,vy_mps,return_m,required_wh,crumbs,late_ms");
}
void mission_row(const Mission *m,double late_ms) {
    printf("%u,%.3f,%.6f,%.6f,%.6f,%s,%s,%.4f,%.4f,%.4f,%.6f,%zu,%.4f\n",
        m->ticks,m->t.now,m->t.x,m->t.y,m->t.energy_wh,
        ib_mode_name(m->cmd.mode),ib_reason_name(m->cmd.reason),
        m->cmd.vx,m->cmd.vy,m->cmd.return_m,m->cmd.required_wh,m->cmd.crumbs,late_ms);
}
