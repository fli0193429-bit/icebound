#include "control.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static IbTelemetry fresh(void) {return (IbTelemetry){0,0,0,0,0,12,100,0};}
static IbControl setup(IbTelemetry *t) {
    IbControl c;IbConfig cfg=ib_default_config();assert(ib_init(&c,&cfg,t)==0);return c;
}
static void advance(IbTelemetry *t,double now) {
    t->now=now;t->sample_time=now;t->command_time=now;
}
int main(void) {
    IbTelemetry t=fresh();IbControl c=setup(&t);IbPoint goal={30,0};
    IbCommand o=ib_step(&c,&t,goal,0);assert(o.mode==IB_SURVEY&&o.vx==1&&o.vy==0);
    advance(&t,1);t.x=5;ib_step(&c,&t,goal,0);assert(c.count==2);
    advance(&t,2);t.x=10;ib_step(&c,&t,goal,0);assert(c.count==3);
    advance(&t,5);t.command_time=2;o=ib_step(&c,&t,goal,0);
    assert(o.mode==IB_RETURN&&o.reason==IB_LEASE&&o.vx<0);
    advance(&t,6);o=ib_step(&c,&t,goal,0);assert(o.mode==IB_RETURN); /* no return cancellation */
    t=fresh();c=setup(&t);advance(&t,1);t.sample_time=0;
    o=ib_step(&c,&t,goal,0);assert(o.mode==IB_HOLD&&o.vx==0&&o.vy==0);
    advance(&t,2);t.x=2;o=ib_step(&c,&t,goal,0);assert(o.mode==IB_RETURN);
    t=fresh();c=setup(&t);t.x=NAN;o=ib_step(&c,&t,goal,0);assert(o.mode==IB_FAULT&&o.vx==0);
    t=fresh();o=ib_step(&c,&t,goal,0);assert(o.mode==IB_FAULT); /* fault latches */
    t=fresh();c=setup(&t);t.sample_time=1;o=ib_step(&c,&t,goal,0);assert(o.mode==IB_FAULT);
    t=fresh();c=setup(&t);advance(&t,2);ib_step(&c,&t,goal,0);t.now=1;
    o=ib_step(&c,&t,goal,0);assert(o.mode==IB_FAULT);
    t=fresh();c=setup(&t);t.energy_wh=13;o=ib_step(&c,&t,goal,0);assert(o.mode==IB_FAULT);
    t=fresh();c=setup(&t);t.energy_wh=0;o=ib_step(&c,&t,goal,0);assert(o.mode==IB_FAULT&&o.reason==IB_EXHAUSTED);
    t=fresh();c=setup(&t);t.obstacle_m=0.2;o=ib_step(&c,&t,goal,0);assert(o.mode==IB_HOLD&&o.vx==0);
    t=fresh();c=setup(&t);t.deadline_missed=1;o=ib_step(&c,&t,goal,0);assert(o.mode==IB_HOLD&&o.reason==IB_DEADLINE);
    t=fresh();c=setup(&t);t.x=50;t.energy_wh=2;o=ib_step(&c,&t,goal,0);
    assert(o.mode==IB_RETURN&&o.reason==IB_RESERVE&&o.required_wh>2);
    t=fresh();c=setup(&t);o=ib_step(&c,&t,goal,1);assert(o.mode==IB_DOCKED);
    advance(&t,1);o=ib_step(&c,&t,goal,0);assert(o.mode==IB_DOCKED&&o.vx==0);
    t=fresh();t.energy_wh=1000;c=setup(&t);c.count=IB_MAX_CRUMBS;
    for(size_t i=0;i<c.count;i++) c.crumbs[i]=(IbPoint){(double)i,0};
    t.x=260;o=ib_step(&c,&t,goal,0);
    assert(o.mode==IB_RETURN&&o.reason==IB_CAPACITY&&c.count==IB_MAX_CRUMBS);
    IbConfig cfg=ib_default_config();cfg.current_bound_mps=cfg.speed_mps;
    assert(ib_init(&c,&cfg,&t)<0);
    puts("Controller invariants passed: lease, reserve, stale, invalid, future time, clock reversal, energy, obstacle, deadline, terminal states, capacity.");
    return 0;
}
