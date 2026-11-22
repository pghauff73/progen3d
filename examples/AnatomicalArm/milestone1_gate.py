"""Milestone 1 reproducible validation gate for the anatomical prototype.

Run from repository root: python examples/AnatomicalArm/milestone1_gate.py
Exit status is nonzero on compilation or unit-test failures. Geometry limitations
are reported separately and never misrepresented as validated watertightness.
"""
import argparse
import json
import platform
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
PACKAGE=ROOT/"examples"/"AnatomicalArm"

def execute(label,args,timeout=300):
    started=time.perf_counter()
    try:
        completed=subprocess.run(args,cwd=ROOT,capture_output=True,text=True,
                                 timeout=timeout,check=False)
        return {"name":label,"command":args,"returncode":completed.returncode,
                "passed":completed.returncode==0,
                "elapsed_seconds":round(time.perf_counter()-started,3),
                "stdout_tail":completed.stdout[-12000:],
                "stderr_tail":completed.stderr[-12000:]}
    except subprocess.TimeoutExpired as exc:
        return {"name":label,"command":args,"returncode":None,"passed":False,
                "elapsed_seconds":round(time.perf_counter()-started,3),
                "error":"timeout after "+str(timeout)+" seconds"}
    except OSError as exc:
        return {"name":label,"command":args,"returncode":None,"passed":False,
                "elapsed_seconds":round(time.perf_counter()-started,3),
                "error":str(exc)}

def run(out,timeout=300):
    out=Path(out)
    if not out.is_absolute():out=ROOT/out
    out.mkdir(parents=True,exist_ok=True)
    checks=[
        ("compile", [sys.executable,"-m","compileall","-q",str(PACKAGE)]),
        ("unittest", [sys.executable,"-m","unittest","discover",
                       "-s",str(PACKAGE),"-p","test_*.py","-v"]),
        ("body_export", [sys.executable,str(PACKAGE/"whole_body.py"),
                         "--out",str(out/"body"),"--render-layer","skin"]),
        ("spatial_export", [sys.executable,str(PACKAGE/"spatial_body.py"),
                            "--out",str(out/"spatial"),"--render-layer","skin"]),
        ("body_cycle", [sys.executable,str(PACKAGE/"body_cycle_audit.py"),
                        "--frames","5","--out",str(out/"body-cycle.json")]),
        ("junction_sweep", [sys.executable,str(PACKAGE/"junction_sweep.py"),
                            "--steps","3","--out",str(out/"junction-sweep.json")]),
        ("spatial_geometry", [sys.executable,"-c",
                              "import sys;sys.path.insert(0, 'examples/AnatomicalArm');"
                              "from spatial_body import spatial_body;"
                              "from mesh_diagnostics import inspect_mesh;"
                              "meshes,_=spatial_body();"
                              "bad={name:inspect_mesh(*mesh) for name,mesh in meshes.items() "
                              "if not inspect_mesh(*mesh)['passed']};"
                              "assert not bad,bad"]),
        ("sleeve_animation", [sys.executable,str(PACKAGE/"coupled_animation.py"),
                              "--frames","3","--substeps","4",
                              "--out",str(out/"sleeve")]),
    ]
    results=[]
    for name,command in checks:
        result=execute(name,command,timeout)
        results.append(result)
        print(("PASS" if result["passed"] else "FAIL")+" "+name,flush=True)
    report={"schema":"progen3d.anatomical-milestone1-gate.v1",
            "timestamp_utc":datetime.now(timezone.utc).isoformat(),
            "python":sys.version,"platform":platform.platform(),
            "repository_root":str(ROOT),"passed":all(r["passed"] for r in results),
            "checks":results,
            "known_limitations":["whole-body skin is not globally watertight",
                                 "joint bridges are visual, not manifold unions",
                                 "full-body self-collision is not implemented",
                                 "XPBD cloth is a reduced-order prototype"]}
    (out/"milestone1-report.json").write_text(json.dumps(report,indent=2)+"\n")
    return report

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--out",default="build/milestone1")
    parser.add_argument("--timeout",type=int,default=300)
    args=parser.parse_args()
    if args.timeout<1:parser.error("--timeout must be positive")
    report=run(args.out,args.timeout)
    print("Milestone 1:", "PASS" if report["passed"] else "FAIL")
    raise SystemExit(0 if report["passed"] else 1)
