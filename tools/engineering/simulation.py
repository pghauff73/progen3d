"""Accepted-step drivers, event localisation, observations, and deterministic replay."""
from dataclasses import dataclass, field
import math

from .quantities import StudyError


@dataclass
class Trace:
    study: object
    segments: list = field(default_factory=list)
    events: list = field(default_factory=list)
    accepted: list = field(default_factory=list)

    def state(self, time, side="after"):
        if side not in ("before", "after") or not self.study.start <= time <= self.study.end:
            raise StudyError("invalid observation time or event side")
        simultaneous = [event for event in self.events if event["time_s"] == time]
        if simultaneous:
            return (simultaneous[0]["before_si"] if side == "before"
                    else simultaneous[-1]["after_si"])
        if time == self.study.start:
            return [q.value for q in self.study.initials.values()]
        low, high = 0, len(self.segments)
        while low < high:
            mid = (low + high) // 2
            if self.segments[mid][1] >= time:
                high = mid
            else:
                low = mid + 1
        if low == len(self.segments):
            raise StudyError("time is beyond the accepted trace")
        return list(map(float, self.segments[low][2](time)))

    def results(self):
        observations = {}
        for item in self.study.observations:
            rows = []
            for index in range(1_000_001):
                time = self.study.start + index * item.interval
                if time > self.study.end:
                    break
                value = self.study.environment(time, self.state(time))[item.name].value
                rows.append([time, item.unit.from_si(value)])
            else:
                raise StudyError("observation exceeds one million points")
            observations[item.name] = {"unit": item.unit_text, "samples": rows}
        return {
            "study": self.study.name, "method": self.study.method,
            "fingerprint": self.study.fingerprint, "bindings": self.study.bindings,
            "locations": self.study.locations, "observations": observations, "events": self.events,
            "accepted_steps_s": [x[0] for x in self.accepted],
            "final_state_si": dict(zip(self.study.initials, self.state(self.study.end))),
        }

    def checkpoint(self, time):
        return {"version": 1, "fingerprint": self.study.fingerprint, "time_s": time, "state_si": self.state(time)}


class RK4Stepper:
    """Fixed-step reference with cubic Hermite interpolation."""
    def __init__(self, rates, start, state, end, step):
        self.rates, self.t, self.y, self.end, self.width = rates, start, state, end, step
        self.status = "running"

    def step(self):
        t, y = self.t, self.y
        finish = min(self.end, t + self.width)
        h = finish - t
        if h <= 0:
            raise StudyError("step is below floating-point time resolution")

        def shift(slope, width):
            return [a + width * b for a, b in zip(y, slope, strict=True)]

        a = self.rates(t, y)
        b = self.rates(t + h / 2, shift(a, h / 2))
        c = self.rates(t + h / 2, shift(b, h / 2))
        d = self.rates(finish, shift(c, h))
        new = [v + h * (i + 2*j + 2*k + l) / 6
               for v, i, j, k, l in zip(y, a, b, c, d, strict=True)]
        end_rate = self.rates(finish, new)

        def interpolate(time):
            u = (time - t) / h
            return [
                (2*u**3 - 3*u*u + 1)*left + (u**3 - 2*u*u + u)*h*left_rate
                + (-2*u**3 + 3*u*u)*right + (u**3 - u*u)*h*right_rate
                for left, right, left_rate, right_rate in zip(y, new, a, end_rate, strict=True)
            ]

        self.t, self.y = finish, new
        self.interpolator = interpolate
        self.status = "finished" if finish == self.end else "running"

    def dense_output(self):
        return self.interpolator


def stepper(study, start, state):
    limit = study.step or (study.end - study.start) / (250 if study.method == "RK4" else 25)
    if study.method == "RK4":
        return RK4Stepper(study.rates, start, list(state), study.end, limit)
    try:
        from scipy.integrate import BDF, RK45
    except ImportError as exc:
        raise StudyError(f"{study.method} requires SciPy (RK4 works without it)") from exc
    driver = {"BDF": BDF, "RK45": RK45}[study.method]
    absolute = [study.tolerances[name].value if name in study.tolerances else 1e-9
                for name in study.initials]
    return driver(study.rates, start, state, study.end, max_step=limit,
                  atol=absolute, rtol=study.relative_tolerance)


def crossing(before, after, direction):
    return ((direction <= 0 and before > 0 >= after)
            or (direction >= 0 and before < 0 <= after))


def localise_event(guard, lo, hi, before):
    for _ in range(100):
        middle = lo + (hi - lo) / 2
        if middle == lo or middle == hi or hi - lo <= 1e-10:
            return middle
        value = guard(middle)
        if value == 0:
            return middle
        if (value > 0) == (before > 0):
            lo = middle
        else:
            hi = middle
    raise StudyError("could not localise event")


def simulate(study, max_steps=200_000):
    trace = Trace(study)
    engine = stepper(study, study.start, [q.value for q in study.initials.values()])
    for _ in range(max_steps):
        old_t = engine.t
        old_values = study.environment(old_t, engine.y)
        diagnostic = engine.step()
        if engine.status == "failed" or not engine.t > old_t or not all(map(math.isfinite, engine.y)):
            raise StudyError(f"solver failed near {old_t:.12g} s: {diagnostic or 'invalid step'}")
        end_t = engine.t
        end_values = study.environment(end_t, engine.y)
        dense = engine.dense_output()
        hits = []
        for event in study.events:
            if event.scheduled_s is not None:
                if old_t < event.scheduled_s <= end_t:
                    hits.append((event.scheduled_s, event))
                continue
            a, b = (event.guard.evaluate(values).value for values in (old_values, end_values))
            if crossing(a, b, event.direction):
                def evaluate_guard(time):
                    return event.guard.evaluate(study.environment(time, dense(time))).value
                root = end_t if b == 0 else localise_event(evaluate_guard, old_t, end_t, a)
                if not trace.events or not any(e["name"] == event.name and abs(e["time_s"] - root) <= 1e-10
                                               for e in trace.events[-len(study.events):]):
                    hits.append((root, event))
        reset_time = min((t for t, e in hits if e.assignment), default=None)
        reset_state = None
        indices = {name: index for index, name in enumerate(study.initials)}
        for time, event in sorted(hits, key=lambda item: (item[0], item[1].name)):
            if reset_time is not None and time > reset_time:
                break
            state = (reset_state.copy() if reset_state is not None and time == reset_time
                     else list(map(float, dense(time))))
            after = state.copy()
            if event.assignment:
                name, expression = event.assignment
                after[indices[name]] = study.registry.assign(
                    expression.evaluate(study.environment(time, state)), study.initials[name].kind).value
                reset_state = after
            trace.events.append({"name": event.name, "time_s": time, "message": event.message,
                                 "before_si": state, "after_si": after})
        if reset_time is not None:
            trace.segments.append((old_t, reset_time, dense))
            trace.accepted.append((reset_time, reset_state))
            if reset_time == study.end:
                return trace
            engine = stepper(study, reset_time, reset_state)
            continue
        trace.segments.append((old_t, end_t, dense))
        trace.accepted.append((float(end_t), list(map(float, engine.y))))
        if engine.status == "finished":
            return trace
    raise StudyError(f"integration exceeded {max_steps} accepted steps")


def replay(study, checkpoint):
    if not isinstance(checkpoint, dict) or checkpoint.get("version") != 1 or checkpoint.get("fingerprint") != study.fingerprint:
        raise StudyError("checkpoint belongs to a different model, registry, or scene revision")
    trace = simulate(study)
    try:
        state = trace.state(checkpoint["time_s"])
    except (KeyError, TypeError) as exc:
        raise StudyError("invalid checkpoint") from exc
    if state != checkpoint.get("state_si"):
        raise StudyError("checkpoint state does not match deterministic replay")
    return trace
