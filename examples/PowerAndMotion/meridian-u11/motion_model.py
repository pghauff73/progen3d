"""Size the Meridian U11 mechanism and generate its native grammar trajectories."""
from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass, fields
import json
from math import asin, cos, degrees, hypot, pi, radians, sin, sqrt, tan
from pathlib import Path


@dataclass(frozen=True)
class Pmsm:
    pole_pairs: int
    magnet_flux_linkage_wb: float
    rotor_inertia_kg_m2: float
    phase_resistance_ohm: float
    quadrature_inductance_h: float
    bus_voltage_v: float
    current_limit_a: float
    speed_limit_rpm: float

    @property
    def torque_constant(self) -> float:
        return 1.5 * self.pole_pairs * self.magnet_flux_linkage_wb

    def electrical_state(self, torque: float, speed: float) -> tuple[float, float, float]:
        current = torque / self.torque_constant
        electrical_speed = self.pole_pairs * speed
        q_voltage = self.phase_resistance_ohm * current + electrical_speed * self.magnet_flux_linkage_wb
        d_voltage = -electrical_speed * self.quadrature_inductance_h * current
        copper_loss = 1.5 * self.phase_resistance_ohm * current**2
        return current, hypot(q_voltage, d_voltage), copper_loss


@dataclass(frozen=True)
class SocketDrive:
    gear_input_teeth: int = 18
    gear_output_teeth: int = 54
    gear_module_m: float = 0.002
    gear_pressure_angle_degrees: float = 20.0
    gear_efficiency: float = 0.96
    belt_input_teeth: int = 20
    belt_output_teeth: int = 40
    belt_pitch_m: float = 0.005
    belt_center_distance_m: float = 0.072
    belt_efficiency: float = 0.95
    belt_span_stiffness_n_m: float = 120_000.0
    belt_span_pretension_n: float = 90.0
    motor_pinion_inertia_kg_m2: float = 0.00008
    layshaft_inertia_kg_m2: float = 0.0014
    payload_mass_kg: float = 11.5
    payload_radius_of_gyration_m: float = 0.27
    socket_mass_kg: float = 2.4
    socket_radius_of_gyration_m: float = 0.065
    torsional_stiffness_nm_per_rad: float = 0.8
    torsional_damping_nm_s_per_rad: float = 0.22
    motor_drag_nm_s_per_rad: float = 0.0005
    speed_blend_rad_s: float = 0.02

    @property
    def gear_ratio(self) -> float:
        return self.gear_output_teeth / self.gear_input_teeth

    @property
    def belt_ratio(self) -> float:
        return self.belt_output_teeth / self.belt_input_teeth

    @property
    def belt_input_radius(self) -> float:
        return self.belt_input_teeth * self.belt_pitch_m / (2 * pi)

    @property
    def belt_output_radius(self) -> float:
        return self.belt_output_teeth * self.belt_pitch_m / (2 * pi)

    @property
    def load_inertia(self) -> float:
        return (self.payload_mass_kg * self.payload_radius_of_gyration_m**2
                + self.socket_mass_kg * self.socket_radius_of_gyration_m**2)

    @property
    def torque_to_span_extension(self) -> float:
        return 1 / (2 * self.belt_span_stiffness_n_m * self.belt_output_radius)


@dataclass(frozen=True)
class GuideDrive:
    screw_lead_m_per_rev: float = 0.012
    screw_efficiency: float = 0.90
    screw_inertia_kg_m2: float = 0.00017
    jaw_mass_kg: float = 1.2
    jaw_stiffness_n_m: float = 450.0
    jaw_damping_n_s_m: float = 14.0
    screw_drag_nm_s_per_rad: float = 0.0005
    speed_blend_rad_s: float = 0.02

    @property
    def screw_metres_per_radian(self) -> float:
        return self.screw_lead_m_per_rev / (2 * pi)


@dataclass(frozen=True)
class MotionCommand:
    socket_travel_rad: float = radians(60.0)
    socket_at_seconds: float = 0.0
    socket_leg_seconds: float = 2.0
    socket_return_at_seconds: float = 3.0
    jaw_travel_m: float = 0.03
    jaw_at_seconds: float = 0.5
    jaw_leg_seconds: float = 1.75
    jaw_return_at_seconds: float = 3.25


SOCKET_MOTOR = Pmsm(4, 0.045, 0.00012, 1.2, 0.0032, 48.0, 3.0, 1500.0)
SCREW_MOTOR = Pmsm(4, 0.026, 0.00009, 1.6, 0.0021, 48.0, 2.5, 900.0)
SOCKET = SocketDrive()
GUIDE = GuideDrive()
COMMAND = MotionCommand()


@dataclass(frozen=True)
class MechanismState:
    time_s: float
    socket_rad: float
    socket_rad_s: float
    socket_rad_s2: float
    socket_torque_nm: float
    socket_mechanical_power_w: float
    layshaft_rad: float
    layshaft_rad_s: float
    layshaft_rad_s2: float
    rotor_rad: float
    rotor_rad_s: float
    rotor_rad_s2: float
    rotor_torque_nm: float
    socket_iq_a: float
    socket_voltage_v: float
    socket_copper_loss_w: float
    gear_tangential_n: float
    gear_radial_n: float
    belt_travel_m: float
    belt_incremental_stretch_m: float
    belt_tension_difference_n: float
    belt_tight_n: float
    belt_slack_n: float
    belt_radial_n: float
    layshaft_bearing_load_upper_n: float
    jaw_offset_m: float
    jaw_velocity_m_s: float
    jaw_acceleration_m_s2: float
    jaw_force_n: float
    jaw_gap_m: float
    screw_rad: float
    screw_rad_s: float
    screw_rad_s2: float
    screw_torque_nm: float
    screw_iq_a: float
    screw_voltage_v: float
    screw_copper_loss_w: float


def smooth_step(phase: float) -> tuple[float, float, float, float, float]:
    """Ninth order, resting pose, velocity, acceleration, jerk and snap at both ends."""
    u = min(max(phase, 0.0), 1.0)
    g = u * (1.0 - u)
    h = 1.0 - 2.0 * u
    q = u**5 * (126.0 + u * (-420.0 + u * (540.0 + u * (-315.0 + 70.0 * u))))
    return (q, 630.0 * g**4, 2520.0 * g**3 * h,
            2520.0 * (3.0 * g**2 * h**2 - 2.0 * g**3),
            2520.0 * (6.0 * g * h**3 - 18.0 * g**2 * h))


def commanded_motion(time: float, start: float, reverse: float, duration: float,
                     travel: float) -> tuple[float, ...]:
    outward = smooth_step((time - start) / duration)
    inward = smooth_step((time - reverse) / duration)
    return tuple(travel * (up - down) / duration**order
                 for order, (up, down) in enumerate(zip(outward, inward)))


def upstream_torque(load_torque: float, load_speed: float, reduction: float, efficiency: float,
                    speed_blend: float) -> float:
    """Signed, power-dissipating reflection with a smooth low-speed transition."""
    symmetric_loss = (1 / efficiency - efficiency) / 2
    asymmetric_loss = (1 / efficiency + efficiency - 2) / 2
    direction = load_speed / hypot(load_speed, speed_blend)
    loss = symmetric_loss * abs(load_torque) * direction + asymmetric_loss * load_torque * direction**2
    return (load_torque + loss) / reduction


def evaluate(time: float) -> MechanismState:
    command, socket, guide = COMMAND, SOCKET, GUIDE
    q, omega, alpha, jerk, snap = commanded_motion(
        time, command.socket_at_seconds, command.socket_return_at_seconds,
        command.socket_leg_seconds, command.socket_travel_rad)
    inertia, damping, stiffness = (
        socket.load_inertia, socket.torsional_damping_nm_s_per_rad, socket.torsional_stiffness_nm_per_rad)
    output_torque = inertia * alpha + damping * omega + stiffness * q
    torque_rate = inertia * jerk + damping * alpha + stiffness * omega
    torque_acceleration = inertia * snap + damping * jerk + stiffness * alpha

    # The driven angle is the demanded load pose. The motor-side angle also
    # winds up the two equal, pretensioned belt spans under differential load.
    out_radius, in_radius = socket.belt_output_radius, socket.belt_input_radius
    compliance = socket.torque_to_span_extension
    belt_travel = out_radius * q + compliance * output_torque
    lay_q = belt_travel / in_radius
    lay_omega = (out_radius * omega + compliance * torque_rate) / in_radius
    lay_alpha = (out_radius * alpha + compliance * torque_acceleration) / in_radius
    rotor_q, rotor_omega, rotor_alpha = (-socket.gear_ratio * x for x in (lay_q, lay_omega, lay_alpha))

    gear_output_torque = socket.layshaft_inertia_kg_m2 * lay_alpha + upstream_torque(
        output_torque, omega, socket.belt_ratio, socket.belt_efficiency, socket.speed_blend_rad_s)
    rotor_torque = (SOCKET_MOTOR.rotor_inertia_kg_m2 + socket.motor_pinion_inertia_kg_m2) * rotor_alpha
    rotor_torque += socket.motor_drag_nm_s_per_rad * rotor_omega - upstream_torque(
        gear_output_torque, lay_omega, socket.gear_ratio, socket.gear_efficiency, socket.speed_blend_rad_s)
    socket_iq, socket_voltage, socket_loss = SOCKET_MOTOR.electrical_state(rotor_torque, rotor_omega)

    gear_tangential = abs(gear_output_torque) / (socket.gear_module_m * socket.gear_output_teeth / 2)
    gear_radial = gear_tangential * tan(radians(socket.gear_pressure_angle_degrees))
    tension_difference = output_torque / out_radius
    tight = socket.belt_span_pretension_n + abs(tension_difference) / 2
    slack = socket.belt_span_pretension_n - abs(tension_difference) / 2
    tangent_angle = asin((out_radius - in_radius) / socket.belt_center_distance_m)
    belt_radial = hypot((tight + slack) * cos(tangent_angle), (tight - slack) * sin(tangent_angle))

    x, velocity, acceleration, _, _ = commanded_motion(
        time, command.jaw_at_seconds, command.jaw_return_at_seconds,
        command.jaw_leg_seconds, command.jaw_travel_m)
    jaw_force = guide.jaw_mass_kg * acceleration + guide.jaw_damping_n_s_m * velocity + guide.jaw_stiffness_n_m * x
    pitch = guide.screw_metres_per_radian
    screw_q, screw_omega, screw_alpha = x / pitch, velocity / pitch, acceleration / pitch
    screw_torque = ((SCREW_MOTOR.rotor_inertia_kg_m2 + guide.screw_inertia_kg_m2) * screw_alpha
                    + guide.screw_drag_nm_s_per_rad * screw_omega + upstream_torque(
                        2 * jaw_force * pitch, screw_omega, 1.0, guide.screw_efficiency, guide.speed_blend_rad_s))
    screw_iq, screw_voltage, screw_loss = SCREW_MOTOR.electrical_state(screw_torque, screw_omega)

    return MechanismState(
        time, q, omega, alpha, output_torque, output_torque * omega,
        lay_q, lay_omega, lay_alpha, rotor_q, rotor_omega, rotor_alpha, rotor_torque,
        socket_iq, socket_voltage, socket_loss, gear_tangential, gear_radial, belt_travel,
        abs(compliance * output_torque), tension_difference, tight, slack, belt_radial,
        belt_radial + hypot(gear_tangential, gear_radial),
        x, velocity, acceleration, jaw_force, 2 * (command.jaw_travel_m - x),
        screw_q, screw_omega, screw_alpha, screw_torque, screw_iq, screw_voltage, screw_loss)


def check_samples(states: list[MechanismState]) -> dict:
    """Check geometry/force relationships and the analytic first two derivatives."""
    for efficiency in (SOCKET.gear_efficiency, SOCKET.belt_efficiency, GUIDE.screw_efficiency):
        for speed in (-4.0, -0.02, -0.002, 0.0, 0.002, 0.02, 4.0):
            for torque in (-1.0, 1.0):
                upstream = upstream_torque(torque, speed, 1.0, efficiency, 0.02)
                assert (upstream - torque) * speed >= 0.0

    for state in states:
        assert 0 <= state.socket_rad <= COMMAND.socket_travel_rad + 1e-12
        assert -1e-12 <= state.jaw_gap_m <= 2 * COMMAND.jaw_travel_m + 1e-12
        assert state.belt_slack_n > 0.0
        assert abs(state.socket_torque_nm - state.belt_tension_difference_n * SOCKET.belt_output_radius) < 1e-12
        assert abs(state.belt_tight_n - state.belt_slack_n - abs(state.belt_tension_difference_n)) < 1e-12
        assert abs(state.rotor_rad + SOCKET.gear_ratio * state.layshaft_rad) < 1e-12
        assert abs(state.belt_travel_m - state.layshaft_rad * SOCKET.belt_input_radius) < 1e-12
        assert abs(state.jaw_offset_m - state.screw_rad * GUIDE.screw_metres_per_radian) < 1e-12
        assert abs(state.socket_iq_a) < SOCKET_MOTOR.current_limit_a
        assert abs(state.screw_iq_a) < SCREW_MOTOR.current_limit_a
        assert state.socket_voltage_v < SOCKET_MOTOR.bus_voltage_v / sqrt(3)
        assert state.screw_voltage_v < SCREW_MOTOR.bus_voltage_v / sqrt(3)
        assert abs(state.rotor_rad_s) * 30 / pi < SOCKET_MOTOR.speed_limit_rpm
        assert abs(state.screw_rad_s) * 30 / pi < SCREW_MOTOR.speed_limit_rpm

    assert abs(evaluate(2.0).socket_rad - COMMAND.socket_travel_rad) < 1e-12
    assert abs(evaluate(2.5).socket_rad_s) < 1e-12
    assert abs(evaluate(2.25).jaw_gap_m) < 1e-12
    assert abs(evaluate(5.0).socket_rad) < 1e-12 and abs(evaluate(5.0).jaw_offset_m) < 1e-12
    assert min(state.socket_mechanical_power_w for state in states) < 0
    assert max(state.socket_mechanical_power_w for state in states) > 0

    groups = {
        "socket": ("socket_rad", "socket_rad_s", "socket_rad_s2"),
        "rotor": ("rotor_rad", "rotor_rad_s", "rotor_rad_s2"),
        "layshaft": ("layshaft_rad", "layshaft_rad_s", "layshaft_rad_s2"),
        "screw": ("screw_rad", "screw_rad_s", "screw_rad_s2"),
        "jaw": ("jaw_offset_m", "jaw_velocity_m_s", "jaw_acceleration_m_s2"),
    }
    h = 0.00001
    errors = {}
    for coordinate, (position, velocity, acceleration) in groups.items():
        velocity_error = 0.0
        acceleration_error = 0.0
        for state in states[::4]:
            before, after = evaluate(state.time_s - h), evaluate(state.time_s + h)
            finite_velocity = (getattr(after, position) - getattr(before, position)) / (2 * h)
            finite_acceleration = (getattr(after, velocity) - getattr(before, velocity)) / (2 * h)
            velocity_error = max(velocity_error, abs(finite_velocity - getattr(state, velocity)))
            acceleration_error = max(acceleration_error, abs(finite_acceleration - getattr(state, acceleration)))
        assert velocity_error < 2e-6 and acceleration_error < 2e-4, coordinate
        errors[coordinate] = {"max_velocity_error": velocity_error, "max_acceleration_error": acceleration_error}
    return {"sample_count": len(states), "finite_difference_step_s": h, "finite_difference": errors,
            "all_passed": True}


def native_grammar() -> str:
    def number(value: float) -> str:
        return f"{value:.15f}".rstrip("0").rstrip(".") or "0"

    def constant(value: float) -> str:
        return "(" + number(value) + ")"

    def emit(name: str, parameters: str, target: str, terms: list[str]) -> str:
        return f"{name}{parameters} ->\n    {target}(\n        " + "\n        ".join(f"({term})" for term in terms) + "\n    )\n"

    def window(start: float, duration: float) -> str:
        return f"clamp((t-{number(start)})/{number(duration)},0,1)"

    def expression(order: int, phase: str) -> str:
        u = phase
        g, h = f"({u}*(1-{u}))", f"(1-2*{u})"
        return (
            f"{u}^5*(126+{u}*(-420+{u}*(540+{u}*(-315+70*{u}))))",
            f"630*{g}^4",
            f"2520*{g}^3*{h}",
            f"2520*(3*{g}^2*{h}^2-2*{g}^3)",
            f"2520*(6*{g}*{h}^3-18*{g}^2*{h})",
        )[order]

    def segment(order: int, forward: str, reverse: str, travel: float, duration: float) -> str:
        return f"{constant(travel / duration**order)}*({expression(order, forward)}-({expression(order, reverse)}))"

    def reflection(torque: str, speed: str, ratio: float, efficiency: float, blend: float) -> str:
        symmetric_loss = constant((1 / efficiency - efficiency) / 2)
        asymmetric_loss = constant((1 / efficiency + efficiency - 2) / 2)
        direction = f"({speed}/sqrt({speed}^2+{constant(blend**2)}))"
        return (f"({torque}+{symmetric_loss}*abs({torque})*{direction}"
                f"+{asymmetric_loss}*{torque}*{direction}^2)/{constant(ratio)}")

    cmd, socket, guide = COMMAND, SOCKET, GUIDE
    inertia = number(socket.load_inertia)
    damping = number(socket.torsional_damping_nm_s_per_rad)
    stiffness = number(socket.torsional_stiffness_nm_per_rad)
    out_radius = number(socket.belt_output_radius)
    in_radius = number(socket.belt_input_radius)
    compliance = number(socket.torque_to_span_extension)
    pitch = number(guide.screw_metres_per_radian)
    belt_torque = reflection("torqueOut", "omega", socket.belt_ratio,
                             socket.belt_efficiency, socket.speed_blend_rad_s)
    gear_torque = reflection("gearLoad", "laySpeed", socket.gear_ratio,
                             socket.gear_efficiency, socket.speed_blend_rad_s)
    screw_torque = reflection("jawLoadTorque", "screwSpeed", 1.0,
                              guide.screw_efficiency, guide.speed_blend_rad_s)
    jaw_load = (f"{number(2 * guide.screw_metres_per_radian)}*("
                f"{number(guide.jaw_mass_kg)}*xdd+{number(guide.jaw_damping_n_s_m)}*xd"
                f"+{number(guide.jaw_stiffness_n_m)}*x)")
    header = """# Generated by motion_model.py. Length m, time s, angles radians before geometry.
# Parent-relative +Z is the common shaft axis. External spur reverses; open belt preserves.
# Continuous 60 degree command by 2 s, hold to 3 s, return by 5 s.
# Two opposed screw leads close the 60 mm total jaw gap, with compliant guides.
# Reduced-order inertia, losses, and elastic winding are evaluated at native grammar t.
"""
    return header + "\n" + "\n".join((
        emit("U11DriveMotion", "", "U11MotionWindows", [
            window(cmd.socket_at_seconds, cmd.socket_leg_seconds),
            window(cmd.socket_return_at_seconds, cmd.socket_leg_seconds),
            window(cmd.jaw_at_seconds, cmd.jaw_leg_seconds),
            window(cmd.jaw_return_at_seconds, cmd.jaw_leg_seconds),
        ]),
        emit("U11MotionWindows", "(u v a b)", "U11MotionStates",
             [segment(order, "u", "v", cmd.socket_travel_rad, cmd.socket_leg_seconds) for order in range(5)]
             + [segment(order, "a", "b", cmd.jaw_travel_m, cmd.jaw_leg_seconds) for order in range(3)]),
        emit("U11MotionStates", "(q omega alpha jerk snap x xd xdd)", "U11MotionEfforts", [
            "q", "omega", "x", "xd", "xdd",
            f"{inertia}*alpha+{damping}*omega+{stiffness}*q",
            f"{inertia}*jerk+{damping}*alpha+{stiffness}*omega",
            f"{inertia}*snap+{damping}*jerk+{stiffness}*alpha",
            "alpha", jaw_load,
        ]),
        emit("U11MotionEfforts", "(q omega x xd xdd torqueOut torqueRate torqueAccel alpha jawLoadTorque)",
             "U11MotionShafts", [
                "q", "x", "torqueOut",
                f"{out_radius}*q+{compliance}*torqueOut",
                f"({out_radius}*omega+{compliance}*torqueRate)/{in_radius}",
                f"({out_radius}*alpha+{compliance}*torqueAccel)/{in_radius}",
                f"xd/{pitch}", f"xdd/{pitch}", "jawLoadTorque", belt_torque,
             ]),
        emit("U11MotionShafts", "(q x torqueOut travel laySpeed layAcceleration screwSpeed screwAcceleration jawLoadTorque beltLoad)",
             "U11MotionCurrent", [
                "q", "x", "torqueOut", "travel", "laySpeed", "layAcceleration", "screwSpeed", "screwAcceleration",
                "jawLoadTorque", f"{number(socket.layshaft_inertia_kg_m2)}*layAcceleration+beltLoad",
             ]),
        emit("U11MotionCurrent", "(q x torqueOut travel laySpeed layAcceleration screwSpeed screwAcceleration jawLoadTorque gearLoad)",
             "U11DriveGeometry", [
                f"degrees(0-{number(socket.gear_ratio)}*travel/{in_radius})",
                f"degrees(travel/{in_radius})", "degrees(q)", f"degrees(x/{pitch})", "x", "travel",
                f"abs({compliance}*torqueOut)",
                f"(0-{number(socket.gear_ratio * (SOCKET_MOTOR.rotor_inertia_kg_m2 + socket.motor_pinion_inertia_kg_m2))}*layAcceleration"
                f"-{number(socket.gear_ratio * socket.motor_drag_nm_s_per_rad)}*laySpeed-{gear_torque})/{number(SOCKET_MOTOR.torque_constant)}",
                f"({number(SCREW_MOTOR.rotor_inertia_kg_m2 + guide.screw_inertia_kg_m2)}*screwAcceleration"
                f"+{number(guide.screw_drag_nm_s_per_rad)}*screwSpeed+{screw_torque})/{number(SCREW_MOTOR.torque_constant)}",
             ]),
    ))


def sizing_report() -> dict:
    samples = [evaluate(i / 240.0) for i in range(5 * 240 + 1)]
    validation = check_samples(samples)

    def extrema(field: str) -> dict:
        slow = min(samples, key=lambda sample: getattr(sample, field))
        fast = max(samples, key=lambda sample: getattr(sample, field))
        return {"min": getattr(slow, field), "min_at_s": slow.time_s,
                "max": getattr(fast, field), "max_at_s": fast.time_s}

    def capacity(motor: Pmsm, torque_name: str, speed_name: str,
                 current_name: str, voltage_name: str) -> dict:
        torque = max(abs(getattr(state, torque_name)) for state in samples)
        speed = max(abs(getattr(state, speed_name)) for state in samples) * 30 / pi
        current = max(abs(getattr(state, current_name)) for state in samples)
        voltage = max(abs(getattr(state, voltage_name)) for state in samples)
        rated_torque = motor.current_limit_a * motor.torque_constant
        voltage_limit = motor.bus_voltage_v / sqrt(3)
        return {
            "shaft_torque_peak_nm": torque, "shaft_torque_current_limit_nm": rated_torque,
            "shaft_torque_margin_nm": rated_torque - torque,
            "shaft_speed_peak_rpm": speed, "shaft_speed_margin_rpm": motor.speed_limit_rpm - speed,
            "peak_q_axis_current_a": current, "q_axis_current_margin_a": motor.current_limit_a - current,
            "phase_voltage_peak_v": voltage, "phase_voltage_margin_v": voltage_limit - voltage,
        }

    direct_socket_torque = max(abs(state.socket_torque_nm) for state in samples)

    chi, volume, gradient = -1.7e-4, 1.2e-6, 10.0
    vacuum_permeability = 4 * pi * 1e-7
    return {
        "model": "Meridian U11 reduced-order commanded motion and inverse dynamics",
        "units": "SI; native geometry receives degrees for its four rotary positions",
        "coordinate_sign": "Socket +Z is positive. External gear reverses rotor versus layshaft. Open belt preserves layshaft versus socket.",
        "command": asdict(COMMAND),
        "socket_motor": {**asdict(SOCKET_MOTOR), "torque_constant_nm_a": SOCKET_MOTOR.torque_constant},
        "screw_motor": {**asdict(SCREW_MOTOR), "torque_constant_nm_a": SCREW_MOTOR.torque_constant},
        "socket_transmission": {
            **asdict(SOCKET), "gear_reduction": SOCKET.gear_ratio, "belt_reduction": SOCKET.belt_ratio,
            "total_reduction": SOCKET.gear_ratio * SOCKET.belt_ratio,
            "belt_input_pitch_radius_m": SOCKET.belt_input_radius,
            "belt_output_pitch_radius_m": SOCKET.belt_output_radius,
            "load_inertia_kg_m2": SOCKET.load_inertia,
            "ideal_rotor_reflected_load_inertia_kg_m2": SOCKET.load_inertia / (SOCKET.gear_ratio * SOCKET.belt_ratio)**2,
            "ideal_rotor_reflected_layshaft_inertia_kg_m2": SOCKET.layshaft_inertia_kg_m2 / SOCKET.gear_ratio**2,
            "span_preload_extension_m": SOCKET.belt_span_pretension_n / SOCKET.belt_span_stiffness_n_m,
        },
        "guide_transmission": {
            **asdict(GUIDE), "jaw_count": 2, "nominal_max_jaw_gap_m": 2 * COMMAND.jaw_travel_m,
            "reflected_combined_jaw_inertia_kg_m2": 2 * GUIDE.jaw_mass_kg * GUIDE.screw_metres_per_radian**2,
        },
        "actuator_selection": {
            "socket": {
                "selected": "Surface PMSM, 3:1 external spur and 2:1 open synchronous belt",
                "rationale": "External spur supplies a compact reduction and reverses the layshaft. The open belt offsets the output shaft, preserves direction and transmits both motoring and braking torque.",
                "direct_drive_load_torque_lower_bound_nm": direct_socket_torque,
                "same_motor_current_limited_shaft_torque_nm": SOCKET_MOTOR.current_limit_a * SOCKET_MOTOR.torque_constant,
                "direct_drive_rationale": "This motor cannot track the specified nominal load when coupled directly. A larger direct motor would remove stages and compliance but changes packaging and capacity.",
            },
            "guide": {
                "selected": "Direct PMSM and one opposed-thread ball screw with two prismatic guides",
                "rationale": "The screw reflects load at 12 mm/rev and kinematically synchronizes both jaws. Linear bearings react off-axis loads; the screw carries the axial thrust.",
                "direct_voice_coil_rationale": "A direct voice coil is viable given 30 mm travel per jaw and adequate continuous force. Two coils would require synchronized drives and ongoing hold current against the spring load. The ball screw also needs holding or a separate brake because it can backdrive.",
                "each_jaw_stroke_m": COMMAND.jaw_travel_m,
                "each_jaw_hold_force_n": GUIDE.jaw_stiffness_n_m * COMMAND.jaw_travel_m,
                "each_jaw_peak_force_n": max(abs(state.jaw_force_n) for state in samples),
            },
            "capacity_scope": "Predicted demand for tracking the authored command under the assumed load, independently checked at 240 Hz. Margins are against assumed motor envelopes, not validation of a motor, gearbox, bearing or brake part number.",
        },
        "capacity": {
            "socket_motor": capacity(SOCKET_MOTOR, "rotor_torque_nm", "rotor_rad_s", "socket_iq_a", "socket_voltage_v"),
            "screw_motor": capacity(SCREW_MOTOR, "screw_torque_nm", "screw_rad_s", "screw_iq_a", "screw_voltage_v"),
        },

        "diamagnetic_example": {
            "volume_susceptibility_si": chi, "insert_volume_m3": volume, "prescribed_grad_b_squared_t2_m": gradient,
            "force_n": chi * volume * gradient / (2 * vacuum_permeability),
            "role": "Weak separate response towards low field; NdFeB magnet flux and coil current provide primary actuation.",
        },
        "extrema": {field.name: extrema(field.name) for field in fields(MechanismState) if field.name != "time_s"},
        "sample_poses": [asdict(evaluate(i / 4.0)) for i in range(21)],
        "validation": validation,
        "assumptions": [
            "Engineering design inputs are illustrative and are not component ratings or a measured load case.",
            "Stiffness and damping are lumped coefficients; the belt is pretensioned and remains engaged.",
            "Controlled output tracking uses inverse dynamics; the load pose is not integrated by a collision or deformation solver.",
            "Signed reflection tends to catalog efficiency in each power direction and blends close to zero speed.",
            "PMSM electrical demand uses an averaged q-axis model and ignores commutation ripple and inductive transients.",
        ],
    }


def main() -> None:
    if not __debug__:
        raise SystemExit("Run without Python -O so the sizing checks execute.")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Check that generated files match this model.")
    args = parser.parse_args()
    directory = Path(__file__).resolve().parent
    outputs = {
        "motion_profiles.p3d": native_grammar(),
        "motion-sizing.json": json.dumps(sizing_report(), indent=2) + "\n",
    }
    for name, contents in outputs.items():
        path = directory / name
        if args.check:
            if path.read_text(encoding="utf-8") != contents:
                raise SystemExit(f"{path} differs from its generated result.")
        else:
            path.write_text(contents, encoding="utf-8", newline="\n")
    print("Meridian mechanism geometry, force balance, limits and derivatives passed.")


if __name__ == "__main__":
    main()
