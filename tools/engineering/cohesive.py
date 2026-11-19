"""Bilinear mode-I traction with irreversible maximum-opening history."""
from dataclasses import dataclass

from .domain_inputs import positive, required_source, scalar
from .quantities import StudyError


@dataclass(frozen=True)
class CohesiveLaw:
    stiffness: float
    strength: float
    fracture_energy: float
    source: str

    @classmethod
    def from_input(cls, data):
        law = cls(
            positive(data, "stiffness_Pa_m"),
            positive(data, "peak_traction_Pa"),
            positive(data, "fracture_energy_J_m2"),
            required_source(data),
        )
        if not 0 < law.onset < law.failure:
            raise StudyError("fracture energy must place failure opening above damage onset")
        return law

    @property
    def onset(self):
        return self.strength / self.stiffness

    @property
    def failure(self):
        return 2 * self.fracture_energy / self.strength

    def damage(self, maximum):
        if maximum <= self.onset:
            return 0.0
        if maximum >= self.failure:
            return 1.0
        # Written as 1 - K(secant)/K(initial) to keep unloading and envelope consistent.
        return self.failure * (maximum - self.onset) / (maximum * (self.failure - self.onset))

    def dissipated_energy(self, maximum):
        if maximum <= self.onset:
            return 0.0
        # Envelope work less stored recoverable energy at the maximum opening.
        progress = min(1.0, (maximum - self.onset) / (self.failure - self.onset))
        return self.fracture_energy * progress

    def maximum_opening(self, maximum_drive, compliance):
        onset_drive = self.onset + compliance * self.strength
        if maximum_drive <= onset_drive:
            return max(0.0, maximum_drive / (1 + compliance * self.stiffness))
        tangent = 1 - compliance * self.strength / (self.failure - self.onset)
        if tangent <= 0:
            raise StudyError("cohesive softening has snap-back at this series compliance")
        if maximum_drive < self.failure:
            return self.onset + (maximum_drive - onset_drive) / tangent
        return maximum_drive


def cohesive_opening(data):
    law = CohesiveLaw.from_input(data)
    rows = data["history"]
    if not isinstance(rows, list) or not rows:
        raise StudyError("cohesive opening requires a separation history")
    maximum, previous = 0.0, -1.0
    damage, traction = [], []
    for sample in rows:
        time = scalar(sample["time_s"], "time_s")
        opening = scalar(sample["opening_m"], "opening_m")
        if opening < 0 or time < 0 or time <= previous:
            raise StudyError("mode-I history requires increasing time and nonnegative opening")
        previous = time
        maximum = max(maximum, opening)
        value = law.damage(maximum)
        damage.append([time, value])
        traction.append([time, (1-value)*law.stiffness*opening])
    return {
        "onset_opening_m": law.onset, "failure_opening_m": law.failure,
        "parameter_source": law.source, "observations": {
            "damage": {"unit": "1", "samples": damage},
            "traction": {"unit": "Pa", "samples": traction},
        }
    }
