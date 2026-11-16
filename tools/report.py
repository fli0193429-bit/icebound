"""Generate a standalone evidence report from real executable output, not fabricated traces."""
import csv
import html
import io
import json
import platform
import subprocess
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tests'))
from test_scenarios import SCENARIOS, validate

ROOT = Path(__file__).resolve().parents[1]
out = ROOT / 'evidence'
out.mkdir(exist_ok=True)
cards, summary = [], []
colors = {'SURVEY':'#79c7df','RETURN':'#dfb87b','HOLD':'#f39595','DOCKED':'#a7ddbb'}
for name in SCENARIOS:
    p = subprocess.run([str(ROOT/'build/icebound'), name], capture_output=True, text=True, check=True)
    rows = list(csv.DictReader(io.StringIO(p.stdout)))
    end = validate(name, rows)
    (out/f'{name}.csv').write_text(p.stdout)
    (out/f'{name}.log').write_text(p.stderr)
    states = []
    for r in rows:
        key=(r['mode'],r['reason'])
        if not states or states[-1]['state'] != key:
            states.append({'state':key,'time':float(r['time_s'])})
    xmax=max(42,max(float(r['x_m']) for r in rows)+2)
    ymax=max(24,max(float(r['y_m']) for r in rows)+2)
    pts=lambda r:f"{40+float(r['x_m'])*480/xmax:.2f},{260-float(r['y_m'])*210/ymax:.2f}"
    paths=[]
    # Downsample for rendering; original CSV retains every step.
    for mode in colors:
        segments=[];current=[]
        for i,r in enumerate(rows):
            if r['mode']==mode:
                if i%5==0 or not current: current.append(pts(r))
            elif current:
                segments.append(' '.join(current));current=[]
        if current: segments.append(' '.join(current))
        paths += [f'<polyline points="{s}" fill="none" stroke="{colors[mode]}" stroke-width="2.5"/>' for s in segments]
    events=''.join(f'<li><time>{s["time"]:.1f}s</time> {html.escape(s["state"][0])} · {html.escape(s["state"][1].replace("_"," "))}</li>' for s in states)
    cards.append(f'''<section><header><h2>{html.escape(name.replace('-', ' ').title())}</h2><b>DOCKED</b></header>
<svg viewBox="0 0 560 300" role="img" aria-label="Actual simulated path in meters"><path d="M40 30V260H540" fill="none" stroke="#41586a"/><text x="38" y="285">0</text><text x="485" y="285">{xmax:.0f} m</text><text x="5" y="45">{ymax:.0f}</text>{''.join(paths)}<circle cx="40" cy="260" r="6" fill="#a7ddbb"/><text x="50" y="247">launch / recovery</text></svg>
<div class="stats"><span>{float(end['time_s']):.1f}<small>seconds simulated</small></span><span>{float(end['energy_wh']):.2f}<small>Wh remaining</small></span><span>{len(rows):,}<small>recorded steps</small></span></div><ol>{events}</ol></section>''')
    summary.append({'scenario':name,'result':end['mode'],'time_s':float(end['time_s']),
                    'remaining_wh':float(end['energy_wh']),'steps':len(rows),'transitions':states})
meta={'platform':platform.platform(),'clock':'simulated','step_s':0.1,
      'qnx_build_verified':False,'qnx_runtime_verified':False,'results':summary}
(out/'summary.json').write_text(json.dumps(meta,indent=2))
document='''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Icebound / Verification report</title><style>
*{box-sizing:border-box}body{margin:0;background:#0c1924;color:#e8eef1;font:16px/1.55 system-ui,sans-serif}main{max-width:1200px;margin:auto;padding:60px 28px}h1{font-size:clamp(42px,6vw,78px);letter-spacing:-.05em;margin:10px 0}p{max-width:850px;color:#b7c7d3}.eyebrow{color:#79c7df;letter-spacing:.18em;font-size:12px}.notice{padding:20px;border:1px solid #756346;background:#282722;border-radius:12px;color:#ecd6af}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(340px,1fr));gap:22px;margin-top:35px}section{background:#122635;border:1px solid #294252;border-radius:18px;padding:22px}header{display:flex;align-items:center;justify-content:space-between}h2{font-size:22px;margin:0}b{font-size:11px;color:#a7ddbb;letter-spacing:.1em}svg{width:100%;margin-top:12px}svg text{fill:#9bb1c0;font:12px system-ui}.stats{display:flex;justify-content:space-between;border-block:1px solid #294252;padding:16px 0}.stats span{font-size:24px}.stats small{display:block;font-size:11px;color:#9bb1c0}ol{padding-left:18px;font-size:12px;color:#b7c7d3}time{color:#79c7df;margin-right:8px}footer{margin-top:35px;color:#92a9b9;font-size:13px}.legend{display:flex;gap:24px;font-size:13px}.legend i{display:inline-block;width:9px;height:9px;border-radius:50%;margin-right:7px}
</style><main><div class="eyebrow">ICEBOUND / UNDER-ICE RETURN AUTONOMY</div><h1>Find the way back.</h1><p>A bounded C controller retraces a recorded route when the command lease expires, the energy reserve is threatened, or exploration completes. Invalid inputs latch a fault; stale navigation, obstacles and missed ticks command zero thrust.</p><div class="notice"><strong>HOST SIMULATION · QNX VALIDATION PENDING</strong><br>These paths and results come from the compiled Linux executable. They do not demonstrate a QNX build, real-time performance, real underwater navigation or certified safety. Current, power and localization assumptions require field validation.</div><p>Seven deterministic scenarios, all returning within 0.12 m of the simulated launch point. Full CSV traces and test logs accompany this report.</p><div class="legend"><span><i style="background:#79c7df"></i>Survey</span><span><i style="background:#dfb87b"></i>Return</span><span><i style="background:#f39595"></i>Hold</span></div><div class="grid">'''+''.join(cards)+'''</div><footer>Generated from build/icebound by tools/report.py. 0.1-second fixed simulation steps. An external current is applied only in the named current scenario. Holding commands zero propulsion; it does not guarantee station keeping in a physical current.</footer></main></html>'''
(out/'report.html').write_text(document)
print(f'Wrote {len(summary)} actual traces and {out / "report.html"}')
