"""Select an XPBD substep count from measured benchmark results.

Rejects invalid/failed runs, nonfinite measurements and excessive strain.
Selection uses the fastest eligible configuration, not the largest count.
"""
import argparse
import json
import math
from pathlib import Path

def select(report, max_strain=0.25, min_clearance=0.0003, max_ms_per_frame=33.333):
    limits=(max_strain,min_clearance,max_ms_per_frame)
    if any(not isinstance(v,(int,float)) or isinstance(v,bool) or not math.isfinite(v) for v in limits):
        raise ValueError("acceptance thresholds must be finite numbers")
    if max_strain<0 or min_clearance<0 or max_ms_per_frame<=0:
        raise ValueError("acceptance thresholds outside valid range")
    results=report.get("results")
    if not isinstance(results,list) or not results:
        raise ValueError("benchmark results missing")
    seen=set()
    evaluated=[]
    for item in results:
        count=item.get("substeps")
        if not isinstance(count,int) or isinstance(count,bool) or count<1 or count in seen:
            raise ValueError("invalid or duplicate substep count")
        seen.add(count)
        keys=("maximum_edge_strain","minimum_clearance_m","ms_per_frame")
        if any(not isinstance(item.get(k),(int,float)) or isinstance(item.get(k),bool)
               or not math.isfinite(item[k]) for k in keys):
            raise ValueError(f"invalid measurements for {count} substeps")
        accepted=(item.get("passed") is True and
                  item["maximum_edge_strain"]<=max_strain and
                  item["minimum_clearance_m"]>=min_clearance and
                  item["ms_per_frame"]<=max_ms_per_frame)
        evaluated.append({"substeps":count,"eligible":accepted,
                          "ms_per_frame":item["ms_per_frame"],
                          "maximum_edge_strain":item["maximum_edge_strain"],
                          "minimum_clearance_m":item["minimum_clearance_m"]})
    eligible=[r for r in evaluated if r["eligible"]]
    choice=min(eligible,key=lambda r:(r["ms_per_frame"],r["substeps"])) if eligible else None
    return {"schema":"progen3d.arm-substep-selection.v1",
            "recommended_substeps":choice["substeps"] if choice else None,
            "passed":bool(choice),
            "thresholds":{"max_edge_strain":max_strain,
                          "min_clearance_m":min_clearance,
                          "max_ms_per_frame":max_ms_per_frame},
            "evaluated":evaluated,
            "note":"Thresholds are provisional engineering screening criteria, not validated biomechanical limits."}

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("benchmark_json")
    p.add_argument("--out",default="build/arm-substep-selection.json")
    p.add_argument("--max-strain",type=float,default=0.25)
    p.add_argument("--min-clearance",type=float,default=0.0003)
    p.add_argument("--max-ms",type=float,default=33.333)
    args=p.parse_args()
    result=select(json.loads(Path(args.benchmark_json).read_text()),
                  args.max_strain,args.min_clearance,args.max_ms)
    target=Path(args.out);target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps(result,indent=2))
    raise SystemExit(0 if result["passed"] else 1)
