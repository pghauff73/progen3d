"""Cross-check native loaded assembly and print geometry against solver telemetry."""
import argparse
import csv
import io
import math
from pathlib import Path
import subprocess
import tempfile
from check_cartesian_printer import GRAMMAR, ROOT, angle, bounds, centre, near, read_scene, solid_volume


def verify_geometry(parts, links, s):
    assert len(links) == 28
    assert ("FeedPinion","ToothFlank","FeedWheelGear","ToothFlank") in links
    assert ("FeedWheelGear","Hub","Hob","Axle") in links
    assert len(parts["FeedPinion"]) == 19 and len(parts["FeedWheelGear"]) == 37
    tip = bounds(parts["Nozzle"][0])
    near(sum(tip[0])/2, s["X"])
    near(tip[1][0], 60+s["Z"])
    near(centre(parts["Bed"][1])[2], s["Y"])
    for axis, k, zero, h in [("X",0,-109,88+s["Z"]+s["Sag"]+s["Expansion"]), ("Y",2,-100,27)]:
        near(centre(parts[axis+"Clamp"][0])[k], s[axis])
        marker = centre(parts[axis+"Pulley"][-1])
        angle(math.atan2(marker[1]-h,marker[k]-zero), -2*math.pi*s["Command"+axis]/40)
    hob = centre(parts["Hob"][-1])
    angle(math.atan2(hob[1]-199,hob[0]+42), s["HobFeed"]/5)
    # The native stadium, sweep and sphere are faceted. Sum segment volumes before intersections.
    mesh = sum(solid_volume(vertices) for vertices in parts.get("Print",[]))
    analytic = s["Deposit"]+s["Ooze"]
    near(mesh, analytic, max(.00005,.025*analytic))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sampler",type=Path,default=ROOT/"build/form-study-sample")
    parser.add_argument("--csv",type=Path,default=ROOT/"build/printer-response-csv")
    args = parser.parse_args()
    data = subprocess.check_output([args.csv.resolve(),GRAMMAR],text=True)
    samples = {float(r["seconds"]): {k:float(v) for k,v in r.items()} for r in csv.DictReader(io.StringIO(data))}
    with tempfile.TemporaryDirectory() as folder:
        target = Path(folder)/"sample.obj"
        for t in [24,0,3,9.25,2.0625,17,25.75,28.5,9.25]:
            subprocess.run([args.sampler.resolve(),GRAMMAR,str(t),target],check=True,stdout=subprocess.DEVNULL)
            verify_geometry(*read_scene(target), samples[t])
    print("Loaded native positions, gear teeth, couplings, feed-wheel angles and emitted mesh volumes passed.")


if __name__ == "__main__":
    main()
