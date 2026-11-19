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
    assert len(links) == 30
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


def verify_component_geometry(parts, links, x_teeth, y_teeth):
    # Three rim rings precede the tooth lands in each driven pulley. Compare the
    # angle between adjacent lands to the declared tooth pitch in the native mesh.
    for axis, teeth, horizontal in (("X", x_teeth, 0), ("Y", y_teeth, 2)):
        wheel = parts[axis + "Pulley"]
        axle = centre(wheel[0])
        land = [[sum(axis_vals) / len(axis_vals) for axis_vals in zip(*vertices)]
                for vertices in wheel[3:3 + teeth]]
        polar = [math.atan2(p[1] - axle[1], p[horizontal] - axle[horizontal]) for p in land]
        for a, b in zip(polar, polar[1:] + polar[:1]):
            near(abs(math.remainder(a - b, 2 * math.pi)), 2 * math.pi / teeth, .0015)
    for side in ("Left", "Right"):
        assert ("Z" + side + "Thrust", "Seat", "Z" + side + "Screw", "ThrustSeat") in links
        housing = parts["Z" + side + "Thrust"]
        # Two sets of 12 balls straddle the central rotating thrust washer.
        for row, y in ((housing[5:17], 46.2), (housing[17:29], 49.8)):
            assert len(row) == 12
            for ball in row:
                near(centre(ball)[1], y)
    for bearing, bore, shaft, journal in links:
        if bore != "Bore" or journal != "Journal":
            continue
        direction = {"X": 0, "Y": 2, "Z": 1}[shaft[0]]
        radial = [i for i in range(3) if i != direction]
        pieces = parts[bearing]
        origin = centre(pieces[0])
        # Each circuit contains eight loaded balls, eight returning balls and a backing track.
        for circuit in range(4):
            start = 4 + 17 * circuit
            for i, radius in enumerate([4.6] * 8 + [6.1] * 8):
                point = centre(pieces[start + i])
                near(math.hypot(*(point[j] - origin[j] for j in radial)), radius, .001)


def verify_preload_sweep(csv_tool, source, folder):
    area = math.pi * 1.75**2 / 4
    last_slip = -1
    last_deposit = math.inf
    for nip in (33, 27, 24, 20, 15):
        path = folder / "nip.p3d"
        path.write_text(source.replace("1.3 33 0.33", f"1.3 {nip} 0.33"))
        data = subprocess.check_output([csv_tool, path], text=True)
        rows = list(csv.DictReader(io.StringIO(data)))
        peak = max(float(r["Pressure"]) for r in rows)
        final = {k: float(v) for k, v in rows[-1].items()}
        assert peak <= nip * .33 / area + 1e-8
        near(final["HobFeed"] - final["Feed"], final["Slip"])
        near(area * final["Feed"], final["Deposit"] + final["Ooze"] + final["Stored"])
        if nip >= 27:
            near(final["Slip"], 0)
        else:
            near(peak, nip * .33 / area)
            assert final["Slip"] > last_slip and final["Deposit"] < last_deposit
        last_slip, last_deposit = final["Slip"], final["Deposit"]


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
            scene = read_scene(target)
            verify_geometry(*scene, samples[t])
            if t == 0:
                verify_component_geometry(*scene, 20, 20)
        source = GRAMMAR.read_text()
        alternate = Path(folder) / "teeth.p3d"
        alternate.write_text(source.replace("17 2 20", "17 2.5 16").replace("18 2 20", "18 1.6 25"))
        subprocess.run([args.sampler.resolve(), alternate, "0", target], check=True, stdout=subprocess.DEVNULL)
        verify_component_geometry(*read_scene(target), 16, 25)
        verify_preload_sweep(args.csv.resolve(), source, Path(folder))
    print("Loaded native positions, gear teeth, couplings, feed-wheel angles and emitted mesh volumes passed. Component geometry, alternate pulley pitches and feeder preload sweep passed.")


if __name__ == "__main__":
    main()
