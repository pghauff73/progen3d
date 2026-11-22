"""Compare anatomical-arm XPBD substep counts using real solver calls.

Writes a CSV and JSON report. Timing is wall-clock and machine-dependent.
"""
import argparse
import csv
import json
import math
import time
from pathlib import Path
from coupled_animation import CoupledArm, angle_at

def measure(substeps,frames=12,fps=30,iterations=8,contact="radial"):
    if substeps<1 or frames<2 or fps<=0:
        raise ValueError("invalid benchmark configuration")
    arm=CoupledArm(contact_mode=contact)
    rows=[]
    start=time.perf_counter()
    for frame in range(frames):
        if frame==0:
            result=arm.step(0.0,dt=1/(fps*substeps),iterations=iterations)
        else:
            for step in range(substeps):
                t=(frame-1+(step+1)/substeps)/fps
                result=arm.step(angle_at(t),dt=1/(fps*substeps),iterations=iterations)
        rows.append(result)
    elapsed=time.perf_counter()-start
    return {"substeps":substeps,"frames":frames,"fps":fps,
            "contact":contact,"elapsed_s":elapsed,
            "ms_per_frame":elapsed*1000/frames,
            "minimum_clearance_m":min(r["minimum_radial_gap_m"] for r in rows),
            "maximum_edge_strain":max(r["maximum_edge_strain"] for r in rows),
            "minimum_elbow_band_gap_m":min(r["minimum_elbow_band_gap_m"] for r in rows),
            "worst_clearance_frame":min(range(len(rows)),key=lambda i:rows[i]["minimum_radial_gap_m"]),
            "worst_clearance_ring":min(rows,key=lambda r:r["minimum_radial_gap_m"])["worst_clearance_ring"],
            "mean_edge_strain":sum(r["mean_edge_strain"] for r in rows)/len(rows),
            "passed":all(r["passed"] for r in rows)}

def benchmark(out,frames=12,fps=30,contact="radial",substeps=(1,2,4,8)):
    out=Path(out);out.mkdir(parents=True,exist_ok=True)
    results=[measure(n,frames,fps,contact=contact) for n in substeps]
    report={"schema":"progen3d.arm-substep-benchmark.v1",
            "results":results,"passed":all(r["passed"] for r in results),
            "notes":["timing depends on hardware and system load",
                     "strain is a geometric edge residual, not a tissue stress",
                     "a pass indicates sampled radial clearance only",
                     "elbow band is five rings either side of the midpoint"]}
    (out/"benchmark.json").write_text(json.dumps(report,indent=2)+"\n")
    with (out/"benchmark.csv").open("w",newline="") as f:
        writer=csv.DictWriter(f,fieldnames=list(results[0]))
        writer.writeheader();writer.writerows(results)
    return report

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--out",default="build/arm-substep-benchmark")
    parser.add_argument("--frames",type=int,default=12)
    parser.add_argument("--fps",type=int,default=30)
    parser.add_argument("--contact",choices=("radial","triangle"),default="radial")
    args=parser.parse_args()
    result=benchmark(args.out,args.frames,args.fps,args.contact)
    print(json.dumps(result,indent=2))
    raise SystemExit(0 if result["passed"] else 1)
