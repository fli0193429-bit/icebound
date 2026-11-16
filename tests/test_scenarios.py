import csv
import io
import math
import subprocess

SCENARIOS = ('nominal', 'link-loss', 'nav-stale', 'low-energy', 'obstacle', 'deadline', 'current')

def run(name):
    p = subprocess.run(['./build/icebound', name], capture_output=True, text=True, check=True)
    assert 'clock=simulated' in p.stderr
    return list(csv.DictReader(io.StringIO(p.stdout)))

def validate(name, rows):
    assert rows[-1]['mode'] == 'DOCKED', (name, rows[-1])
    assert math.hypot(float(rows[-1]['x_m']), float(rows[-1]['y_m'])) <= .12
    assert float(rows[-1]['energy_wh']) > 0
    latched = False
    for row in rows:
        mode = row['mode']
        if mode in ('RETURN', 'HOLD'): latched = True
        if latched: assert mode != 'SURVEY', (name, row)
        if mode in ('HOLD', 'DOCKED', 'FAULT'):
            assert float(row['vx_mps']) == float(row['vy_mps']) == 0
        assert int(row['crumbs']) <= 256
        assert math.hypot(float(row['vx_mps']), float(row['vy_mps'])) <= 1.0001
    expected = {'link-loss': 'command_lease_expired', 'nav-stale': 'navigation_stale',
                'low-energy': 'energy_reserve', 'obstacle': 'obstacle', 'deadline': 'deadline_missed'}
    if name in expected: assert any(r['reason'] == expected[name] for r in rows)
    if name in ('nav-stale', 'obstacle', 'deadline'):
        assert any(r['mode'] == 'HOLD' for r in rows)
    return rows[-1]

if __name__ == '__main__':
    for name in SCENARIOS:
        end = validate(name, run(name))
        print(f"PASS {name:12} docked at {end['time_s']}s / {end['energy_wh']} Wh remaining")
    for args in (['nonsense'], ['nominal', '--realtime']):
        p = subprocess.run(['./build/icebound', *args], capture_output=True)
        assert p.returncode == 2
