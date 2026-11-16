# English demo outline — target length 2 minutes 30 seconds

Status: production outline, not a completed or uploaded video. Record the final
QNX section only after real target validation. Keep the host-simulation label
visible throughout host replay footage. Do not imply that seven test scenarios
are seven physical robots or seven independent projects.

## 0:00–0:18 — The problem

“Under ice, losing the command link does not make surfacing a safe escape.
Icebound explores a narrower question: can a small controller stop exploration
and retrace a recorded route before its return-energy reserve is threatened?”

Show the launch point and a short survey trajectory, then the title.

## 0:18–0:55 — Lost communication, recorded decisions

Open `evidence/replay.html`, choose `link-loss`, play at 1× or 4×, then pause
at the transition. The simulated last command is at 12 seconds; the 2-second
lease expires just after 14 seconds. Use the visible trace timestamp rather
than implying immediate detection.

“The controller keeps a bounded breadcrumb trail. Once the command lease
expires, return is latched. Restoring communication does not silently restart
the survey. Here it reaches the simulated recovery point at 28.1 seconds.”

## 0:55–1:20 — Different faults need different responses

Choose `nav-stale`, play through the hold, then seek into the return.

“Stale navigation commands zero thrust. When fresh input returns, the mission
returns home instead of resuming exploration. Obstacles and missed control
ticks use the same hold-and-return policy. Zero thrust is not physical station
keeping: this is a simplified plant, not a hydrodynamic model.”

## 1:20–1:50 — Real QNX proof, pending

Required footage: actual `uname -a`, successful qcc build output, native pulse
runner command, and recorded target transitions. Keep host and target output
visually separate. Do not film substitute Linux output or mock QNX headers.

Once supported by real evidence, explain that the target branch uses a private
channel and monotonic timer pulses for the control loop, with bounded trace
storage and CSV output after the timer stops. Describe only the timing actually
measured; do not claim hard-real-time guarantees from one execution.

## 1:50–2:15 — Evidence and scope

Show the seven scenario results and public source repository (URL pending).

“The current host tests cover route completion, lost communication, stale
navigation, low energy, obstacles, missed ticks and bounded current. The core
is deterministic C. AI assisted development, but no generative model decides
the velocity command. Navigation, obstacle clearance and energy assumptions
would all need validation before any real underwater deployment.”

## 2:15–2:30 — Close

“Icebound makes the return decision inspectable: the route, the trigger and the
energy estimate are recorded together. Next comes measured vehicle dynamics,
sensor integration and a validated physical recovery procedure.”

End card: repository, MIT license, QNX target version once verified. Final
English video and public upload remain pending; do not submit this outline as
the required video.
