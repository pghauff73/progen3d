"""Shared checks for scalar SI inputs in engineering domain models."""
import math

from .quantities import StudyError


def scalar(value, name, positive=False):
    if type(value) not in (float, int) or not math.isfinite(value) or (positive and value <= 0):
        raise StudyError(f"{name} must be a {'positive ' if positive else ''}finite SI scalar")
    return float(value)


def positive(data, key):
    return scalar(data[key], key, positive=True)


def nonnegative(data, key):
    value = scalar(data[key], key)
    if value < 0:
        raise StudyError(f"{key} must be nonnegative")
    return value


def required_source(data):
    source = data.get("parameter_source")
    if not isinstance(source, str) or not source.strip():
        raise StudyError("material parameters require a parameter source")
    return source.strip()
