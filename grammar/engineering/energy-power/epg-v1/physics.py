"""Small SI constitutive models. Algebra only; no network or transient integrator."""
import math

def _finite(*values):
    if not all(isinstance(x,(int,float)) and not isinstance(x,bool) and math.isfinite(x) for x in values):raise ValueError('finite SI values required')
def _positive(x):
    _finite(x)
    if x<=0:raise ValueError('positive denominator required')
def ohms_voltage(current_A,resistance_ohm):
    _finite(current_A,resistance_ohm)
    if resistance_ohm<0:raise ValueError('negative resistance')
    return current_A*resistance_ohm

def pressure(force_N,area_m2):
    _finite(force_N);_positive(area_m2);return force_N/area_m2

def electrical_power(voltage_V,current_A):
    _finite(voltage_V,current_A);return voltage_V*current_A

def fluid_power(delta_pressure_Pa,flow_m3_s):
    _finite(delta_pressure_Pa,flow_m3_s);return delta_pressure_Pa*flow_m3_s

def shaft_power(torque_Nm,omega_rad_s):
    _finite(torque_Nm,omega_rad_s);return torque_Nm*omega_rad_s

def solar_power(irradiance_W_m2,area_m2,efficiency):
    _finite(irradiance_W_m2,area_m2,efficiency)
    if irradiance_W_m2<0 or area_m2<0 or not 0<=efficiency<=1:raise ValueError('invalid solar rating')
    return irradiance_W_m2*area_m2*efficiency

def capacitor_step(voltage_V,capacitance_F,current_into_A,dt_s):
    _finite(voltage_V,current_into_A,dt_s);_positive(capacitance_F)
    if dt_s<0:raise ValueError('negative time step')
    return voltage_V+current_into_A*dt_s/capacitance_F

def flywheel_step(omega_rad_s,inertia_kg_m2,net_torque_Nm,dt_s):
    _finite(omega_rad_s,net_torque_Nm,dt_s);_positive(inertia_kg_m2)
    if dt_s<0:raise ValueError('negative time step')
    return omega_rad_s+net_torque_Nm*dt_s/inertia_kg_m2

def elastic_stress(young_modulus_Pa,extension_m,original_length_m):
    _finite(young_modulus_Pa,extension_m);_positive(original_length_m)
    if young_modulus_Pa<0:raise ValueError('negative modulus')
    return young_modulus_Pa*extension_m/original_length_m

def transformer_voltage_ac(input_rms_V,primary_turns,secondary_turns):
    _finite(input_rms_V,secondary_turns);_positive(primary_turns);_positive(secondary_turns)
    return input_rms_V*secondary_turns/primary_turns
