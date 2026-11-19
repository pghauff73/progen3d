"""Reference domain models with explicit SI inputs for engineering studies."""
import math

from .quantities import StudyError


from .bonded_coupon import bonded_coupon
from .coupon_geometry import validate_coupon_geometry
from .cohesive import cohesive_opening
from .domain_inputs import positive, scalar


def thermal_network(data):
    import numpy as np
    from scipy.integrate import solve_ivp
    from scipy.sparse import coo_matrix

    duration = positive(data, "duration_s")
    nodes = data["nodes"]
    if not isinstance(nodes, list) or not nodes or len(nodes) > 10_000:
        raise StudyError("thermal network needs 1–10000 nodes")
    initial = np.array([scalar(node["initial_K"], "initial_K") for node in nodes])
    capacity = np.array([positive(node, "capacity_J_K") for node in nodes])
    if min(initial) < 0:
        raise StudyError("negative absolute temperature")
    count = len(nodes)
    row, column, value = [], [], []
    source = np.zeros(count)

    def stamp(i, j, contribution):
        row.append(i)
        column.append(j)
        value.append(contribution / capacity[i])

    def index(key):
        if type(key) is not int or not 0 <= key < count:
            raise StudyError(f"invalid thermal node index {key}")
        return key

    for edge in data.get("links", []):
        i, j = map(index, [edge["a"], edge["b"]])
        if i == j:
            raise StudyError("a thermal link cannot join a node to itself")
        conductance = positive(edge, "conductance_W_K")
        for a, b in ((i, j), (j, i)):
            stamp(a, a, -conductance)
            stamp(a, b, conductance)

    boundaries = []
    for port in data.get("reservoirs", []):
        i = index(port["node"])
        ambient = scalar(port["temperature_K"], "reservoir temperature_K")
        if ambient < 0:
            raise StudyError("negative reservoir temperature")
        g = positive(port, "conductance_W_K")
        stamp(i, i, -g)
        source[i] += g * ambient / capacity[i]
        boundaries.append((i, g, ambient))

    if "coolant" in data:
        coolant = data["coolant"]
        node = index(coolant["node"])
        inlet = scalar(coolant["inlet_K"], "coolant inlet_K")
        if inlet < 0:
            raise StudyError("negative inlet temperature")
        g = positive(coolant, "mass_flow_kg_s") * positive(coolant, "specific_heat_J_kg_K")
        stamp(node, node, -g)
        source[node] += g * inlet / capacity[node]
        boundaries.append((node, g, inlet))

    # The last state accumulates inward heat for the energy ledger.
    energy_source = 0.0
    for i, g, ambient in boundaries:
        row.append(count); column.append(i); value.append(-g)
        energy_source += g * ambient
    matrix = coo_matrix((value, (row, column)), shape=(count + 1, count + 1)).tocsc()
    forcing = np.append(source, energy_source)
    solved = solve_ivp(lambda t, state: matrix @ state + forcing, (0, duration),
                       np.append(initial, 0.0), method="BDF", jac=matrix,
                       rtol=2e-11, atol=1e-10, dense_output=True)
    if not solved.success:
        raise StudyError(solved.message)
    end = solved.y[:-1, -1]
    if min(end) < 0:
        raise StudyError("thermal solver produced a negative temperature")
    internal_change = float(capacity @ (end - initial))
    input_energy = float(solved.y[-1, -1])
    samples = solved.sol(np.linspace(0, duration, 101))
    return {"final_K": end.tolist(), "energy_change_J": internal_change,
            "integrated_inward_heat_J": input_energy,
            "energy_residual_J": internal_change - input_energy,
            "observations": {
                f"node_{i}": {"unit": "K", "samples": [[duration*j/100, float(y)] for j, y in enumerate(profile)]}
                for i, profile in enumerate(samples[:-1])
            }}


def wall_conduction(data):
    # Cell-centred finite volumes. Boundary resistance is half a cell.
    count = data["cells"]
    if type(count) is not int or not 2 <= count <= 1000:
        raise StudyError("conduction requires 2–1000 cells")
    width = positive(data, "length_m") / count
    area = positive(data, "area_m2")
    conductance = positive(data, "conductivity_W_m_K") * area / width
    cell_capacity = positive(data, "volumetric_capacity_J_m3_K") * area * width
    initial = data["initial_K"]
    if not isinstance(initial, list):
        initial = [initial] * count
    if len(initial) != count:
        raise StudyError("initial_K needs one value per conduction cell")
    result = thermal_network({
        "duration_s": data["duration_s"],
        "nodes": [{"initial_K": value, "capacity_J_K": cell_capacity} for value in initial],
        "links": [{"a": i, "b": i+1, "conductance_W_K": conductance} for i in range(count-1)],
        "reservoirs": [{"node": 0, "temperature_K": data["left_K"], "conductance_W_K": 2*conductance},
                       {"node": count-1, "temperature_K": data["right_K"], "conductance_W_K": 2*conductance}],
    })
    result["position_m"] = [(i + .5) * width for i in range(count)]
    return result


def expansion(data):
    length = positive(data, "length_m")
    members = data["members"]
    if not isinstance(members, list) or not members:
        raise StudyError("an expansion study needs at least one member")
    free_strains, stiffnesses, moduli = [], [], []
    for member in members:
        modulus = positive(member, "young_modulus_Pa")
        area = positive(member, "area_m2")
        free_strains.append(scalar(member["alpha_per_K"], "alpha_per_K") * scalar(member["delta_K"], "delta_K"))
        stiffnesses.append(modulus * area)
        moduli.append(modulus)
    if "prescribed_strain" in data:
        strain = scalar(data["prescribed_strain"], "prescribed_strain")
    else:
        strain = sum(k*e for k, e in zip(stiffnesses, free_strains)) / sum(stiffnesses)
    return {"elongation_m": strain*length, "strain": strain,
            "stress_Pa": [E*(strain-free) for E, free in zip(moduli, free_strains)],
            "reaction_N": sum(k*(strain-free) for k, free in zip(stiffnesses, free_strains))}


def laminar_pipe(data):
    diameter = positive(data, "diameter_m")
    viscosity = positive(data, "dynamic_viscosity_Pa_s")
    flow = scalar(data["volume_flow_m3_s"], "volume_flow_m3_s")
    velocity = 4*flow / (math.pi*diameter**2)
    reynolds = positive(data, "density_kg_m3") * abs(velocity) * diameter / viscosity
    if reynolds >= 2000:
        raise StudyError("laminar_pipe requires Reynolds number below 2000")
    resistance = 128*viscosity*positive(data, "length_m") / (math.pi*diameter**4)
    return {"pressure_drop_Pa": resistance*flow, "reynolds": reynolds, "resistance_Pa_s_m3": resistance}


def resistor_circuit(data):
    """One dynamic voltage with an index-one resistor subsystem (quasistatic nodes)."""
    import numpy as np
    from scipy.integrate import solve_ivp
    from scipy.sparse import coo_matrix
    from scipy.sparse.linalg import MatrixRankWarning, splu

    capacitance = positive(data, "capacitance_F")
    duration = positive(data, "duration_s")
    initial = scalar(data["initial_V"], "initial_V")
    supply = scalar(data["supply_V"], "supply_V")
    count = data["algebraic_nodes"]
    if type(count) is not int or not 0 <= count <= 1000:
        raise StudyError("algebraic_nodes must be an integer in 0–1000")
    known = count + 3  # 0 ground, 1 supply, 2 capacitor, then algebraic nodes.
    stamped_rows, stamped_cols, stamped_values = [], [], []
    links = []
    for link in data["resistors"]:
        i, j = link["a"], link["b"]
        if type(i) is not int or type(j) is not int or not (0 <= i < known and 0 <= j < known) or i == j:
            raise StudyError("invalid resistor endpoints")
        g = 1 / positive(link, "resistance_ohm")
        links.append((i, j, g))
        stamped_rows.extend([i, j, i, j])
        stamped_cols.extend([i, j, j, i])
        stamped_values.extend([g, g, -g, -g])
    matrix = coo_matrix((stamped_values, (stamped_rows, stamped_cols)), shape=(known, known)).tocsc()
    try:
        lu = splu(matrix[3:, 3:]) if count else None
    except (MatrixRankWarning, RuntimeError) as exc:
        raise StudyError("singular algebraic network: a node may be floating") from exc

    def node_voltages(cap_voltage):
        constrained = np.array([0, supply, cap_voltage])
        if not count:
            return constrained
        return np.concatenate((constrained, lu.solve(-matrix[3:, :3] @ constrained)))

    start = node_voltages(initial)
    if "algebraic_initial_V" in data:
        declared = np.asarray(data["algebraic_initial_V"], dtype=float)
        if declared.shape != (count,) or not np.allclose(declared, start[3:], rtol=0, atol=1e-8):
            raise StudyError("inconsistent algebraic initial voltages")
    row = matrix.getrow(2)
    solved = solve_ivp(lambda t, y: -(row @ node_voltages(y[0])) / capacitance,
                       (0, duration), [initial], method="BDF", rtol=2e-11, atol=1e-12, dense_output=True)
    if not solved.success:
        raise StudyError(solved.message)
    end = node_voltages(float(solved.y[0,-1]))
    return {"initial_node_V": start.tolist(), "final_node_V": end.tolist(),
            "resistor_power_W": [float(g*(end[i]-end[j])**2) for i, j, g in links],
            "algebraic_max_residual_A": float(np.max(abs((matrix @ end)[3:]))) if count else 0.0,
            "observations": {"capacitor": {"unit": "V", "samples": [
                [float(t), float(solved.sol(t)[0])] for t in np.linspace(0, duration, 101)
            ]}}}




def surface_flux_transfer(data):
    """Area-overlap transfer for parallel strips on a common surface."""
    width = positive(data, "width_m")
    source = [scalar(x, "source edge") for x in data["source_edges_m"]]
    target = [scalar(x, "target edge") for x in data["target_edges_m"]]
    flux = [scalar(x, "flux density") for x in data["source_flux_W_m2"]]
    if (len(source) < 2 or len(target) < 2 or len(flux) != len(source)-1
            or source[0] != target[0] or source[-1] != target[-1]
            or any(a >= b for axis in (source, target) for a, b in zip(axis, axis[1:]))):
        raise StudyError("flux meshes require increasing edges over the same extent")
    targets = []
    index = 0
    for a, b in zip(target, target[1:]):
        transferred = 0.0
        while index < len(flux) and source[index] < b:
            overlap = min(b, source[index+1]) - max(a, source[index])
            if overlap > 0:
                transferred += overlap * width * flux[index]
            if source[index+1] > b:
                break
            index += 1
        targets.append(transferred / ((b-a)*width))
    incoming = math.fsum(f * (b-a)*width for f, a, b in zip(flux, source, source[1:]))
    outgoing = math.fsum(f * (b-a)*width for f, a, b in zip(targets, target, target[1:]))
    return {"target_flux_W_m2": targets, "source_power_W": incoming,
            "target_power_W": outgoing, "transfer_residual_W": incoming-outgoing}



def split_wall_coolant(data):
    """Iterated constant-flux exchange between two lumped thermal solvers."""
    wall = scalar(data["wall_K"], "wall_K")
    coolant = scalar(data["coolant_K"], "coolant_K")
    inlet = scalar(data["inlet_K"], "inlet_K")
    if min(wall, coolant, inlet) < 0:
        raise StudyError("negative absolute temperature")
    wall_c = positive(data, "wall_capacity_J_K")
    fluid_c = positive(data, "coolant_capacity_J_K")
    link = positive(data, "conductance_W_K")
    advection = positive(data, "mass_flow_kg_s") * positive(data, "specific_heat_J_kg_K")
    stop = positive(data, "duration_s")
    macro = positive(data, "exchange_step_s")
    fluid_step = positive(data, "coolant_step_s")
    ambient_ledger = 0.0
    transferred = 0.0
    inputs = [(0.0, wall, coolant)]
    retries = 0
    time = 0.0
    initial_total = wall_c*wall + fluid_c*coolant
    for _ in range(200_000):
        if time >= stop:
            return {
                "wall_final_K": wall, "coolant_final_K": coolant, "interface_heat_J": transferred,
                "coolant_boundary_J": ambient_ledger, "rejected_exchange_steps": retries,
                "energy_residual_J": wall_c*wall + fluid_c*coolant - initial_total - ambient_ledger,
                "observations": {
                    name: {"unit": "K", "samples": [[row[0], row[index]] for row in inputs]}
                    for index, name in enumerate(["wall", "coolant"], 1)
                },
            }
        width = min(macro, stop-time)
        substeps = math.ceil(width / fluid_step)
        if substeps > 100_000:
            raise StudyError("too many coolant substeps at the requested step ratio")
        flux = link*(wall-coolant)
        for _ in range(32):
            next_wall = wall - width*flux/wall_c
            next_coolant = coolant
            factor = -math.expm1(-advection*(width/substeps)/fluid_c)
            target = inlet + flux/advection
            trial_boundary = 0.0
            for _ in range(substeps):
                difference = target-next_coolant
                trial_boundary += fluid_c*factor*difference - flux*width/substeps
                next_coolant += factor*difference
            target_flux = link*(next_wall-next_coolant)
            if abs(target_flux-flux) <= 1e-10*max(1.0, abs(target_flux)):
                break
            flux = target_flux
            if not math.isfinite(flux):
                target_flux = math.inf
                break
        else:
            target_flux = math.inf
        if not math.isfinite(target_flux):
            # Neither component mutated its committed state during the failed trial.
            macro = width/2
            retries += 1
            if macro <= 1e-12*stop or time + macro == time:
                raise StudyError("wall/coolant coupling failed to converge at the minimum step")
            continue
        if min(next_wall, next_coolant) < 0:
            raise StudyError("coupling produced a negative absolute temperature")
        exchange = width*flux
        ambient_ledger += trial_boundary
        transferred += exchange
        wall, coolant = next_wall, next_coolant
        time = min(stop, time + width)
        inputs.append((time, wall, coolant))
    raise StudyError("wall/coolant coupling exceeds 200000 macrosteps")




MODELS = {
    "thermal_network": thermal_network, "wall_conduction": wall_conduction,
    "expansion": expansion, "laminar_pipe": laminar_pipe, "resistor_circuit": resistor_circuit,
    "surface_flux_transfer": surface_flux_transfer, "split_wall_coolant": split_wall_coolant,
    "cohesive_opening": cohesive_opening, "bonded_coupon": bonded_coupon,
}


def analyse(data, scene=None):
    if not isinstance(data, dict) or data.get("version") != 1:
        raise StudyError("expected a version-one domain model")
    name = data.get("model")
    if not isinstance(name, str) or name not in MODELS:
        raise StudyError(f"no verified provider for domain model {name}")
    try:
        geometry = None
        if name == "bonded_coupon":
            geometry = validate_coupon_geometry(data, scene)
        elif scene is not None:
            raise StudyError(f"{name} does not accept a scene manifest")
        result = {"study": name, **MODELS[name](data)}
        if geometry is not None:
            result["geometry_binding"] = geometry
        return result
    except ImportError as exc:
        raise StudyError(f"{name} requires SciPy and NumPy") from exc
    except (KeyError, TypeError, IndexError) as exc:
        raise StudyError(f"invalid {name} input: {exc}") from exc
