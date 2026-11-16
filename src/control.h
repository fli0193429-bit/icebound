#ifndef ICEBOUND_CONTROL_H
#define ICEBOUND_CONTROL_H
#include <stddef.h>
#define IB_MAX_CRUMBS 256
typedef struct { double x, y; } IbPoint;
typedef enum { IB_SURVEY, IB_RETURN, IB_HOLD, IB_DOCKED, IB_FAULT } IbMode;
typedef enum { IB_OK, IB_ROUTE_DONE, IB_RESERVE, IB_LEASE, IB_STALE,
    IB_INVALID, IB_DEADLINE, IB_OBSTACLE, IB_CAPACITY, IB_EXHAUSTED } IbReason;
typedef struct {
    double now, sample_time, command_time, x, y, energy_wh;
    double obstacle_m;
    int deadline_missed;
} IbTelemetry;
typedef struct {
    double speed_mps, current_bound_mps, power_bound_w;
    double reserve_wh, max_age_s, lease_s, crumb_spacing_m;
} IbConfig;
typedef struct {
    IbConfig cfg;
    IbMode mode;
    IbReason reason;
    IbPoint crumbs[IB_MAX_CRUMBS];
    size_t count;
    int return_latched;
    double last_now, initial_energy;
} IbControl;
typedef struct {
    IbMode mode;
    IbReason reason;
    double vx, vy, return_m, required_wh;
    size_t crumbs;
} IbCommand;
IbConfig ib_default_config(void);
int ib_init(IbControl *c, const IbConfig *cfg, const IbTelemetry *t);
IbCommand ib_step(IbControl *c, const IbTelemetry *t, IbPoint survey_goal,
                  int survey_finished);
const char *ib_mode_name(IbMode m);
const char *ib_reason_name(IbReason r);
#endif
