# Icebound — Under-Ice Return Autonomy

A small, inspectable return controller for an under-ice robot research prototype.
It records breadcrumbs during exploration, estimates the energy needed to retrace
them against a bounded current, and latches a return when communication expires
or the energy reserve is threatened. Stale navigation, an obstacle or a missed
control tick commands zero thrust until valid input resumes; exploration does
not automatically resume afterward.

**Status: host simulation verified; QNX build and runtime not yet verified.**
This is a research simulation, not a deployed underwater vehicle or safety-certified
controller. The current submission is a draft and is not ready for final judging.

## Run locally

Requires a C11 compiler, make and Python 3. No third-party Python packages,
network connection, AI API key or paid service is needed.

```sh
make evidence
./build/icebound link-loss > evidence/custom.csv
```

Open `evidence/report.html` to inspect the actual recorded trajectories and
state transitions. All seven CSV traces and `summary.json` are reproducible from
the executable. Host timing is simulated at 100 ms steps and is **not** a
real-time benchmark. Scenario names: `nominal`, `link-loss`, `nav-stale`,
`low-energy`, `obstacle`, `deadline`, `current`.

Open `evidence/replay.html` for an offline playback with scenario selection,
pause, restart, a time slider and the actual state/energy values at each sample.
It uses no external images or services. Playback is host evidence only.

## QNX target

Target: **QNX SDP 8.0, x86_64 QEMU**. After installing an authorized SDP 8.0
toolchain and sourcing its environment:

```sh
make qnx
```

This invokes `qcc -Vgcc_ntox86_64`. Transfer `build/icebound-qnx` to the QNX
target using your authorized target deployment procedure, then run:

```sh
./icebound-qnx link-loss --realtime > qnx-link-loss.csv 2> qnx-link-loss.log
```

The native branch in `src/run.c` uses a private QNX channel, a side connection,
`SIGEV_PULSE_INIT`, `timer_create(CLOCK_MONOTONIC)` and `MsgReceivePulse` for
100 ms periodic ticks. It reports lateness and sends an over-period late tick
through the controller's hold path. Trace data stays in a bounded static buffer
until the timer is disarmed; there is no CSV I/O inside the periodic loop.

The physical plant is still simulated even on QNX. Real-time thread priorities,
hardware I/O, ability policies and target-specific worst-case execution timing
have not been configured or demonstrated. See [QNX validation](docs/QNX_VALIDATION.md)
for the evidence required before claiming target compatibility.

## Design

`src/control.c` is a bounded-memory deterministic controller. `src/mission.c`
provides fault injection and a simple 2D test plant. `src/run.c` supplies a fast
host runner and the conditional QNX native pulse runner. The controller accepts
telemetry and emits velocity commands; it does not call a generative model in
the control path. AI assistance was used for source code, documentation and tests.

Return energy is estimated as:

```
required Wh = breadcrumb return distance / (speed − current bound)
              × power bound / 3600 + fixed reserve
```

Defaults: speed 1 m/s, current bound 0.25 m/s, propulsion power bound 150 W,
reserve 1 Wh, navigation max age 0.3 s, command lease 2 s. The test plant uses
120 W when moving and 20 W when holding. These are synthetic, illustrative
parameters, not measured performance of a real robot. The current scenario
adds 0.2 m/s only while moving; other scenarios have zero current.

## Limitations

- Breadcrumbs assume a navigable previously traversed corridor; there is no
  sonar mapping, obstacle avoidance planner or proof of clearance.
- No sensor fusion, uncertainty propagation, vehicle dynamics, depth control,
  thruster allocation or hardware interface is implemented.
- Zero thrust does not imply station keeping. Under-ice vehicles cannot assume
  surfacing is possible; this controller never commands an ascent.
- Return reserve is only conservative while its current, power and position
  assumptions hold. Long holds consume energy and can exhaust the vehicle.
- A numeric sample can be valid yet physically wrong. Sensor plausibility and
  independent hardware shutdown remain future work.
- `DOCKED` means within 0.12 m of the simulated launch position, not a verified
  physical docking maneuver.

## Official API references

- [QNX 8.0 timer_create](https://www.qnx.com/developers/docs/8.0/com.qnx.doc.neutrino.lib_ref/topic/t/timer_create.html)
- [QNX 8.0 ChannelCreate](https://www.qnx.com/developers/docs/8.0/com.qnx.doc.neutrino.lib_ref/topic/c/channelcreate.html)
- [QNX 8.0 MsgReceivePulse](https://www.qnx.com/developers/docs/8.0/com.qnx.doc.neutrino.lib_ref/topic/m/msgreceivepulse.html)

MIT license. No QNX proprietary binaries, license files, credentials or SDK
headers are included.
