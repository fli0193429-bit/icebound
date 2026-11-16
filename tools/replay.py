"""Build an offline replay from existing host CSV evidence; no invented motion."""
import csv
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
names = ('nominal', 'link-loss', 'nav-stale', 'low-energy', 'obstacle', 'deadline', 'current')
data = {}
for name in names:
    with (ROOT / 'evidence' / (name + '.csv')).open() as f:
        data[name] = [{k: v if k in ('mode', 'reason') else float(v)
                       for k, v in row.items()} for row in csv.DictReader(f)]
template = (ROOT / 'tools' / 'replay.html').read_text()
(ROOT / 'evidence' / 'replay.html').write_text(template.replace('/*TRACE_DATA*/{}', json.dumps(data)))
print('Wrote offline CSV replay: evidence/replay.html')
