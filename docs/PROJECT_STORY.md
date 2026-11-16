# Icebound — Under-Ice Return Autonomy

## Inspiration
Under ice, "return home" has a stricter meaning. A robot cannot assume it can
surface anywhere, and a lost radio or acoustic command cannot leave its next
action to an unbounded wait. Icebound explores a small, inspectable supervisor
that can stop exploring and retrace what it already knows.

## What it does
The C controller records a bounded breadcrumb trail. It returns when its command
lease expires, when the estimated return-energy reserve is threatened, or when
the survey finishes. Stale navigation, an obstacle and a missed tick command
zero thrust; once the input becomes valid again, the return remains latched.
Invalid numeric inputs and clock reversal latch a fault.

## Implementation
A portable controller shares its logic with a QNX-specific periodic runner.
The QNX branch uses private channels and monotonic timer pulses. A simple test
plant injects seven reproducible scenarios. Raw CSV traces generate an offline
verification report, keeping the visual explanation tied to executable output.

## Validation status — draft
The host build and seven end-to-end scenarios passed, including complete
simulated return with remaining energy. The QNX-specific source is prepared,
but no QNX compile or target execution has been verified yet. The project must
not claim compliance with the competition's QNX execution requirement until
that evidence exists.

## Scope and next work
This is a simulation prototype. It does not implement sonar, sensor fusion,
physical docking or a real underwater actuator interface. Completing the QNX
toolchain setup, running on QEMU, measuring actual pulse timing and documenting
the target evidence are the next gates. AI assistance was used for development;
no generative model controls the vehicle.
