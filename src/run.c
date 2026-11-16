#define _POSIX_C_SOURCE 200809L
#define _QNX_SOURCE 1
#include "mission.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef __QNXNTO__
#include <signal.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <errno.h>
#define TICK_CODE _PULSE_CODE_MINAVAIL
static double monotonic(void) {
    struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t)<0) return -1;
    return (double)t.tv_sec+(double)t.tv_nsec/1e9;
}
#endif
typedef struct { IbTelemetry t; IbCommand cmd; unsigned ticks; double late_ms; } Trace;
static Trace traces[6000];
int main(int argc,char **argv) {
    const char *scenario=argc>1?argv[1]:"nominal";
    int realtime=argc>2&&strcmp(argv[2],"--realtime")==0;
    if(argc>3||(argc>2&&!realtime)) {fputs("Usage: icebound [scenario] [--realtime]\n",stderr);return 2;}
    Mission m;if(mission_init(&m,scenario)<0) {fputs("Unknown scenario\n",stderr);return 2;}
#ifndef __QNXNTO__
    if(realtime) {fputs("--realtime requires QNX; host mode is simulated time only.\n",stderr);return 2;}
    fputs("platform=host; clock=simulated; QNX timing NOT validated\n",stderr);
#else
    int chid=-1,coid=-1;timer_t timer;int timer_ready=0;double start=0;
    if(realtime) {
        chid=ChannelCreate(_NTO_CHF_PRIVATE);
        if(chid<0) {perror("ChannelCreate");return 2;}
        coid=ConnectAttach(ND_LOCAL_NODE,0,chid,_NTO_SIDE_CHANNEL,0);
        if(coid<0) {perror("ConnectAttach");ChannelDestroy(chid);return 2;}
        struct sigevent ev;SIGEV_PULSE_INIT(&ev,coid,SIGEV_PULSE_PRIO_INHERIT,TICK_CODE,0);
        if(timer_create(CLOCK_MONOTONIC,&ev,&timer)<0) {perror("timer_create");goto cleanup_error;}
        timer_ready=1;
        struct itimerspec spec={{0,100000000L},{0,100000000L}};
        start=monotonic();if(start<0||timer_settime(timer,0,&spec,NULL)<0) {perror("timer_settime");goto cleanup_error;}
    }
    fprintf(stderr,"platform=QNX; clock=%s; plant=simulation\n",realtime?"monotonic-pulse":"simulated");
#endif
    int result=1;size_t recorded=0;
    for(m.ticks=0;m.ticks<6000;m.ticks++) {
        double now=m.ticks*m.step_s,late_ms=0;int missed=0;
#ifdef __QNXNTO__
        if(realtime) {
            struct _pulse pulse;int r;
            do {r=MsgReceivePulse(chid,&pulse,sizeof(pulse),NULL);} while(r<0&&errno==EINTR);
            if(r<0) {perror("MsgReceivePulse");break;}
            if(pulse.code!=TICK_CODE) {fputs("unexpected pulse\n",stderr);break;}
            double elapsed=monotonic()-start;
            now=fmax(0,elapsed-m.step_s);
            late_ms=fmax(0,(now-m.ticks*m.step_s)*1000);
            missed=late_ms>100;
        }
#endif
        int done=mission_tick(&m,now,missed);
        traces[recorded++]=(Trace){m.t,m.cmd,m.ticks,late_ms};
        if(done) {result=m.cmd.mode==IB_DOCKED?0:1;break;}
        /* Simple test plant, not a hydrodynamic model. A moving-current scenario
           stays within the configured 0.25 m/s bound and stops with thrust. */
        double moving=hypot(m.cmd.vx,m.cmd.vy)>0;
        double current=m.scenario==SC_CURRENT&&moving?0.2:0;
        m.t.x+=(m.cmd.vx+current)*m.step_s;m.t.y+=m.cmd.vy*m.step_s;
        m.t.energy_wh-= (moving?120.0:20.0)*m.step_s/3600;
        if(m.t.energy_wh<0)m.t.energy_wh=0;
    }
#ifdef __QNXNTO__
    if(timer_ready)timer_delete(timer);
    if(coid>=0)ConnectDetach(coid);
    if(chid>=0)ChannelDestroy(chid);
#endif
    /* No CSV writes in the periodic loop. Flush bounded in-memory trace only
       after the timer is disarmed, so disk/network I/O cannot block a tick. */
    mission_header();
    for(size_t i=0;i<recorded;i++) {
        m.t=traces[i].t;m.cmd=traces[i].cmd;m.ticks=traces[i].ticks;
        mission_row(&m,traces[i].late_ms);
    }
    return result;
#ifdef __QNXNTO__
cleanup_error:
    if(timer_ready)timer_delete(timer);
    if(coid>=0)ConnectDetach(coid);
    if(chid>=0)ChannelDestroy(chid);
    return 2;
#endif
}
