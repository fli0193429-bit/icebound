#include "control.h"
#include <math.h>
#include <string.h>
static double distance(IbPoint a, IbPoint b) { return hypot(a.x-b.x,a.y-b.y); }
static int finite_point(IbPoint p) { return isfinite(p.x)&&isfinite(p.y); }
IbConfig ib_default_config(void) {
    return (IbConfig){1.0,0.25,150.0,1.0,0.3,2.0,2.0};
}
static int valid_config(const IbConfig *p) {
    return isfinite(p->speed_mps)&&isfinite(p->current_bound_mps)&&
        isfinite(p->power_bound_w)&&isfinite(p->reserve_wh)&&
        isfinite(p->max_age_s)&&isfinite(p->lease_s)&&
        isfinite(p->crumb_spacing_m)&&p->speed_mps>p->current_bound_mps&&
        p->current_bound_mps>=0&&p->power_bound_w>0&&p->reserve_wh>0&&
        p->max_age_s>0&&p->lease_s>0&&p->crumb_spacing_m>=0.1;
}
static int valid_telemetry(const IbTelemetry *t) {
    return isfinite(t->now)&&isfinite(t->sample_time)&&isfinite(t->command_time)&&
        isfinite(t->x)&&isfinite(t->y)&&isfinite(t->energy_wh)&&
        isfinite(t->obstacle_m)&&t->now>=0&&t->sample_time>=0&&
        t->command_time>=0&&t->sample_time<=t->now&&t->command_time<=t->now&&
        t->energy_wh>=0&&t->obstacle_m>=0;
}
int ib_init(IbControl *c, const IbConfig *cfg, const IbTelemetry *t) {
    memset(c,0,sizeof(*c)); c->mode=IB_FAULT; c->reason=IB_INVALID;
    if(!valid_config(cfg)||!valid_telemetry(t)) return -1;
    c->cfg=*cfg; c->mode=IB_SURVEY; c->reason=IB_OK;
    c->crumbs[0]=(IbPoint){t->x,t->y}; c->count=1;
    c->last_now=t->now; c->initial_energy=t->energy_wh; return 0;
}
static double return_distance(const IbControl *c, IbPoint here) {
    double d=distance(here,c->crumbs[c->count-1]);
    for(size_t i=c->count-1;i>0;i--) d+=distance(c->crumbs[i],c->crumbs[i-1]);
    return d;
}
static void latch(IbControl *c, IbReason r) {
    if(!c->return_latched) { c->return_latched=1; c->reason=r; }
}
IbCommand ib_step(IbControl *c,const IbTelemetry *t,IbPoint goal,int finished) {
    IbCommand out={c->mode,c->reason,0,0,0,0,c->count};
    if(c->mode==IB_FAULT) return out;
    if(!valid_telemetry(t)||!finite_point(goal)||t->now<c->last_now||
       t->energy_wh>c->initial_energy+0.001) {
        c->mode=IB_FAULT; c->reason=IB_INVALID; goto done;
    }
    c->last_now=t->now;
    if(c->mode==IB_DOCKED) goto done;
    IbPoint here={t->x,t->y};
    out.return_m=return_distance(c,here);
    /* Upper-bound energy for retracing known segments, plus a fixed reserve.
       Assumes current and propulsion demand remain within configured bounds. */
    out.required_wh=out.return_m/(c->cfg.speed_mps-c->cfg.current_bound_mps)*
                    c->cfg.power_bound_w/3600.0+c->cfg.reserve_wh;
    if(t->energy_wh<=0) { c->mode=IB_FAULT;c->reason=IB_EXHAUSTED;goto done; }
    if(t->deadline_missed) {
        latch(c,IB_DEADLINE);c->mode=IB_HOLD;out.reason=IB_DEADLINE;goto hold;
    }
    if(t->now-t->sample_time>c->cfg.max_age_s) {
        latch(c,IB_STALE);c->mode=IB_HOLD;out.reason=IB_STALE;goto hold;
    }
    if(t->obstacle_m<1.0) {
        latch(c,IB_OBSTACLE);c->mode=IB_HOLD;out.reason=IB_OBSTACLE;goto hold;
    }
    if(t->now-t->command_time>c->cfg.lease_s) latch(c,IB_LEASE);
    /* One extra period's reserve avoids crossing the guard between ticks. */
    if(t->energy_wh<=out.required_wh+c->cfg.power_bound_w/3600.0) latch(c,IB_RESERVE);
    if(finished) latch(c,IB_ROUTE_DONE);
    if(!c->return_latched) {
        if(distance(here,c->crumbs[c->count-1])>=c->cfg.crumb_spacing_m||
           (distance(here,goal)<0.15&&distance(here,c->crumbs[c->count-1])>0.01)) {
            if(c->count==IB_MAX_CRUMBS) latch(c,IB_CAPACITY);
            else c->crumbs[c->count++]=here;
        }
    }
    if(c->return_latched) {
        c->mode=IB_RETURN;
        while(c->count>1&&distance(here,c->crumbs[c->count-1])<=0.12) c->count--;
        goal=c->crumbs[c->count-1];
        if(c->count==1&&distance(here,goal)<=0.12) {
            c->mode=IB_DOCKED;goto done;
        }
    } else { c->mode=IB_SURVEY;c->reason=IB_OK; }
    double d=distance(here,goal);
    if(d>0) { out.vx=c->cfg.speed_mps*(goal.x-here.x)/d;
              out.vy=c->cfg.speed_mps*(goal.y-here.y)/d; }
    goto done;
hold:
    out.mode=c->mode;out.crumbs=c->count;return out;
done:
    out.mode=c->mode;out.reason=c->reason;out.crumbs=c->count;return out;
}
const char *ib_mode_name(IbMode m) {
    static const char *names[]={"SURVEY","RETURN","HOLD","DOCKED","FAULT"};
    return m>=IB_SURVEY&&m<=IB_FAULT?names[m]:"UNKNOWN";
}
const char *ib_reason_name(IbReason r) {
    static const char *names[]={"normal","route_complete","energy_reserve",
        "command_lease_expired","navigation_stale","invalid_input",
        "deadline_missed","obstacle","breadcrumb_capacity","energy_exhausted"};
    return r>=IB_OK&&r<=IB_EXHAUSTED?names[r]:"unknown";
}
