"""Read, calibrate and compare bonded-coupon measurements."""
from bisect import bisect_left
import csv
import math
from pathlib import Path

from .domain_inputs import scalar
from .quantities import ABSOLUTE, StudyError, parse_unit


def numeric_field(row, name, line):
    try:
        return scalar(float(row[name]), name)
    except (KeyError, TypeError, ValueError) as exc:
        raise StudyError(f"measurement line {line}: invalid {name}") from exc


def csv_rows(path, columns):
    with Path(path).open(encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if not set(columns).issubset(reader.fieldnames or []) or len(set(reader.fieldnames)) != len(reader.fieldnames):
            raise StudyError(f"measurement CSV requires columns {', '.join(columns)}")
        rows = list(enumerate(reader, 2))
    if not rows:
        raise StudyError("measurement CSV has no data rows")
    return rows


def source_label(source):
    if not source.strip():
        raise StudyError("measurements require a source label")
    return source.strip()


def compare_measurements(result, path, source):
    source = source_label(source)
    observations = result.get("observations")
    if not isinstance(observations, dict):
        raise StudyError("trace has no observations")
    prepared = {}
    rows = []
    for line, row in csv_rows(path, ("time_s", "channel", "value", "unit")):
        channel = (row["channel"] or "").strip()
        if channel not in observations:
            raise StudyError(f"measurement line {line}: unknown observation {channel}")
        if channel not in prepared:
            series = observations[channel]
            if not series["samples"]:
                raise StudyError(f"trace channel {channel} has no samples")
            times, predicted = zip(*series["samples"])
            if any(a >= b for a, b in zip(times, times[1:])):
                raise StudyError(f"trace channel {channel} has duplicate or unordered times")
            prepared[channel] = (times, predicted, parse_unit(series["unit"]))
        times, predicted, trace_unit = prepared[channel]
        time = numeric_field(row, "time_s", line)
        if not times[0] <= time <= times[-1]:
            raise StudyError(f"measurement line {line}: {time:g} s outside the trace interval")
        index = bisect_left(times, time)
        if times[index] == time:
            model = predicted[index]
        else:
            fraction = (time-times[index-1])/(times[index]-times[index-1])
            model = predicted[index-1]+fraction*(predicted[index]-predicted[index-1])
        observed_text = (row["unit"] or "").strip()
        observed_unit = parse_unit(observed_text)
        observed_kind = ABSOLUTE if observed_text == "K" else observed_unit.kind
        trace_kind = ABSOLUTE if observations[channel]["unit"] == "K" else trace_unit.kind
        if observed_unit.dimensions != trace_unit.dimensions or (
                observed_kind and trace_kind and observed_kind != trace_kind):
            raise StudyError(f"measurement line {line}: incompatible unit for {channel}")
        observed = observed_unit.to_si(numeric_field(row, "value", line))
        model_si = trace_unit.to_si(model)
        residual = model_si-observed
        item = {
            "time_s": time, "channel": channel, "unit": row["unit"],
            "observed": observed_unit.from_si(observed),
            "predicted": observed_unit.from_si(model_si),
            "residual": residual/observed_unit.scale,
            "residual_in_trace_unit": residual/trace_unit.scale,
            "trace_unit": observations[channel]["unit"],
        }
        if (row.get("standard_uncertainty") or "").strip():
            uncertainty = numeric_field(row, "standard_uncertainty", line)
            if uncertainty <= 0:
                raise StudyError(f"measurement line {line}: uncertainty must be positive")
            item["standardised_residual"] = item["residual"]/uncertainty
        rows.append(item)
    summary = {}
    for channel in prepared:
        samples = [item for item in rows if item["channel"] == channel]
        residuals = [item["residual_in_trace_unit"] for item in samples]
        summary[channel] = {
            "count": len(residuals), "unit": samples[0]["trace_unit"],
            "bias": math.fsum(residuals)/len(residuals),
            "rmse": math.sqrt(math.fsum(x*x for x in residuals)/len(residuals)),
            "max_abs_residual": max(abs(x) for x in residuals),
        }
    return {"study": result.get("study"), "measurement_source": source,
            "interpolation": "linear through recorded trace samples", "channels": summary, "rows": rows}


def fit_cohesive(path, area, source):
    """Fit the monotonic opening envelope from local separation and force."""
    import numpy as np
    from scipy.optimize import least_squares

    source = source_label(source)
    scalar(area, "area_m2", positive=True)
    rows = csv_rows(path, ("opening_m", "force_N"))
    if len(rows) < 6:
        raise StudyError("fit requires at least six loading-envelope samples")
    opening = np.array([numeric_field(row, "opening_m", line) for line, row in rows])
    forces = np.array([numeric_field(row, "force_N", line) for line, row in rows])
    if opening[0] < 0 or np.any(np.diff(opening) <= 0) or np.min(forces) < 0 or np.max(forces) <= 0:
        raise StudyError("fit requires increasing nonnegative local opening and tensile force")
    peak = int(np.argmax(forces))
    if peak < 2 or peak > len(rows)-3 or forces[-1] >= .8*forces[peak]:
        raise StudyError("fit requires distinct elastic and post-peak measurements")
    xmax, ymax = float(opening[-1]), float(forces[peak])
    normal_x, normal_y = opening/xmax, forces/ymax

    def model(parameters):
        onset, span, strength = np.exp(parameters)
        ascent = normal_x/onset
        descent = np.maximum(0, (onset+span-normal_x)/span)
        return strength*np.minimum(ascent, descent)

    fits = []
    for onset in (normal_x[peak-1], normal_x[peak], normal_x[peak+1]):
        for span in (0.25, 0.6, 1.2):
            start = np.log([max(onset, .0001), span, 1.0])
            candidate = least_squares(lambda par: model(par)-normal_y, start,
                                      bounds=(np.log([1e-6, 1e-6, .05]), np.log([3.0, 6.0, 5.0])),
                                      ftol=1e-12, xtol=1e-12, gtol=1e-12)
            if candidate.success:
                fits.append(candidate)
    if not fits:
        raise StudyError("cohesive fit did not converge")
    fit = min(fits, key=lambda result: result.cost)
    onset, span, strength = np.exp(fit.x)
    onset_m, failure_m, peak_Pa = onset*xmax, (onset+span)*xmax, strength*ymax/area
    elastic = int(sum((opening > 0) & (opening < onset_m)))
    softening = int(sum((opening > onset_m) & (opening < failure_m)))
    if elastic == 0 or softening < 2 or np.linalg.matrix_rank(fit.jac) < 3:
        raise StudyError("observations do not identify all three cohesive parameters")
    estimated_forces = model(fit.x)*ymax
    return {
        "parameter_source": source,
        "fit_input": "monotonic local opening and force",
        "area_m2": area, "onset_m": float(onset_m), "failure_m": float(failure_m),
        "stiffness_Pa_m": float(peak_Pa/onset_m), "peak_traction_Pa": float(peak_Pa),
        "fracture_energy_J_m2": float(peak_Pa*failure_m/2),
        "failure_beyond_last_sample": bool(failure_m > xmax),
        "elastic_samples": elastic, "softening_samples": softening,
        "force_rmse_N": float(np.sqrt(np.mean((estimated_forces-forces)**2))),
    }
