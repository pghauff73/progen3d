"""Optional FMI 3 adapter for user-selected, trusted local FMUs."""
from pathlib import Path
import math

from .quantities import StudyError


def run_fmu(path, stop, interval, outputs, mode, require_rollback=False):
    if not math.isfinite(stop) or not math.isfinite(interval) or not 0 < interval <= stop:
        raise StudyError("FMI requires finite 0 < interval <= stop")
    if stop / interval > 1_000_000:
        raise StudyError("FMI output exceeds one million intervals")
    if not outputs or len(set(outputs)) != len(outputs) or "time" in outputs:
        raise StudyError("provide distinct scalar output names other than time")
    try:
        from fmpy import read_model_description, simulate_fmu
    except ImportError as exc:
        raise StudyError("FMI runs require the optional FMPy dependency") from exc
    filename = Path(path)
    if filename.suffix.lower() != ".fmu" or not filename.is_file():
        raise StudyError("provide a local .fmu file")
    try:
        description = read_model_description(str(filename))
        if description.fmiVersion != "3.0":
            raise StudyError("this adapter requires FMI 3.0")
        interface = (description.coSimulation if mode == "CoSimulation"
                     else description.modelExchange)
        if interface is None:
            raise StudyError(f"the FMU does not declare {mode}")
        rollback = bool(interface.canGetAndSetFMUstate)
        if require_rollback and not rollback:
            raise StudyError("FMU does not declare get/set state needed for rollback")
        variables = {x.name: x for x in description.modelVariables}
        for name in outputs:
            if (name not in variables or variables[name].shape
                    or variables[name].type not in ("Float32", "Float64")):
                raise StudyError(f"{name} is not a scalar floating-point FMU variable")
        event_mode = mode == "CoSimulation" and bool(interface.hasEventMode)
        result = simulate_fmu(str(filename), start_time=0.0, stop_time=stop,
                              output_interval=interval, fmi_type=mode,
                              output=outputs, model_description=description,
                              early_return_allowed=event_mode, use_event_mode=event_mode,
                              remote_platform=None)
    except StudyError:
        raise
    except Exception as exc:
        raise StudyError(f"FMI execution failed: {exc}") from exc
    if not len(result) or abs(float(result["time"][-1]) - stop) > max(1e-9, 1e-9*stop):
        raise StudyError("FMU stopped before the requested time")
    traces = {}
    for name in outputs:
        rows = [[float(time), float(value)] for time, value in zip(result["time"], result[name])]
        if not all(math.isfinite(value) for _, value in rows):
            raise StudyError(f"FMU output {name} contains a nonfinite value")
        traces[name] = {"unit": variables[name].unit or "FMU unit unspecified", "samples": rows}
    return {"study": description.modelName, "interface": mode,
            "declares_state_rollback": rollback, "observations": traces}


def run_scheduled_fmu(path, schedule, outputs):
    """Activate FMI partitions serially in the provided ordering."""
    from tempfile import TemporaryDirectory
    try:
        from fmpy import extract, read_model_description
        from fmpy.simulation import instantiate_fmu
    except ImportError as exc:
        raise StudyError("FMI runs require the optional FMPy dependency") from exc
    path = Path(path)
    if path.suffix.lower() != ".fmu" or not path.is_file():
        raise StudyError("provide a local .fmu file")
    if not isinstance(schedule, list) or not 1 <= len(schedule) <= 1_000_000:
        raise StudyError("schedule requires one or more explicit clock activations")
    if not outputs or len(set(outputs)) != len(outputs):
        raise StudyError("provide distinct output names for Scheduled Execution")
    try:
        description = read_model_description(str(path))
        if description.fmiVersion != "3.0" or description.scheduledExecution is None:
            raise StudyError("the FMU does not declare FMI 3 Scheduled Execution")
        variables = {x.name: x for x in description.modelVariables}
        types = ("Float32", "Float64", "Int32", "UInt32", "Int64", "UInt64")
        traces = {}
        for name in outputs:
            var = variables.get(name)
            if var is None or var.shape or var.type not in types or var.causality != "output":
                raise StudyError(f"{name} is not a supported scalar numeric output")
            traces[name] = {"unit": var.unit or "FMU unit unspecified", "samples": []}
        activations = []
        previous = 0.0
        for event in schedule:
            instant, name = event["time_s"], event["clock"]
            if type(instant) not in (int, float) or not math.isfinite(instant) or instant < previous:
                raise StudyError("clock schedule times must be nonnegative and ordered")
            clock = variables.get(name)
            if clock is None or clock.type != "Clock" or clock.causality != "input":
                raise StudyError(f"{name} is not an input clock")
            activations.append((float(instant), clock.valueReference))
            previous = instant

        with TemporaryDirectory() as directory:
            extract(str(path), directory)
            instance = instantiate_fmu(directory, description, "ScheduledExecution")
            try:
                instance.enterInitializationMode(startTime=0.0)
                instance.exitInitializationMode()
                for time, reference in activations:
                    instance.activateModelPartition(reference, time)
                    for name, trace in traces.items():
                        var = variables[name]
                        value = getattr(instance, "get" + var.type)([var.valueReference])[0]
                        if not math.isfinite(value):
                            raise StudyError(f"nonfinite scheduled output {name}")
                        trace["samples"].append([time, value])
                instance.terminate()
            finally:
                instance.freeInstance()
    except StudyError:
        raise
    except Exception as exc:
        raise StudyError(f"FMI Scheduled Execution failed: {exc}") from exc
    return {"study": description.modelName, "interface": "ScheduledExecution", "observations": traces}
