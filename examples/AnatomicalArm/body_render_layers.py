"""Choose a nonduplicated rendering subset of whole-body anatomical meshes.

The source assembly deliberately retains original and stitched variants for
inspection. Rendering both produces coincident triangles and z-fighting.
"""
def render_subset(model,mode="skin"):
    if mode not in ("skin","clothed","diagnostic"):
        raise ValueError("unknown body render mode")
    if mode=="diagnostic":
        return dict(model)
    selected={}
    for name,mesh in model.items():
        if name in ("torso_skin","torso_garment"):
            if name=="torso_"+("skin" if mode=="skin" else "garment"):
                selected[name]=mesh
            continue
        if name.endswith("_patch"):
            if name.endswith("_"+mode+"_patch"):
                selected[name]=mesh
            continue
        if "_arm_" in name or "_leg_" in name:
            # Patches replace skin and sleeve/garment envelopes. Retain bones
            # only in diagnostic mode to avoid displaying internal geometry.
            continue
        if name.endswith("_collar") or name.endswith("_joint"):
            continue
    return selected

def check_duplicate_geometry(model):
    """Find exact duplicate triangle coordinates across objects.

    Diagnostic only; nearly coincident and intersecting faces are not detected.
    """
    owners={}
    duplicates=[]
    for name,(vertices,faces) in model.items():
        for face in faces:
            key=tuple(sorted(tuple(round(c,9) for c in vertices[i]) for i in face))
            previous=owners.get(key)
            if previous is not None:
                duplicates.append((previous,name))
            else:
                owners[key]=name
    return {"duplicate_triangles":len(duplicates),
            "example_pairs":duplicates[:10]}

def boundary_audit(model):
    """Report open boundary loops for each selected render object.

    An open rim is not automatically a defect (wrists and ankles are open).
    This audit makes unfinished shoulder/hip connections measurable.
    """
    from surface_stitch import edge_incidence
    report={}
    for name,(vertices,faces) in model.items():
        incidence=edge_incidence(faces)
        boundary=[edge for edge,n in incidence.items() if n==1]
        degree={}
        for a,b in boundary:
            degree[a]=degree.get(a,0)+1
            degree[b]=degree.get(b,0)+1
        unseen=set(degree)
        components=0
        adjacency={v:set() for v in degree}
        for a,b in boundary:
            adjacency[a].add(b);adjacency[b].add(a)
        while unseen:
            components+=1
            stack=[unseen.pop()]
            while stack:
                for neighbor in adjacency[stack.pop()]:
                    if neighbor in unseen:
                        unseen.remove(neighbor)
                        stack.append(neighbor)
        report[name]={"boundary_edges":len(boundary),
                      "boundary_components":components,
                      "nonmanifold_edges":sum(n>2 for n in incidence.values()),
                      "closed_boundary_loops":components if all(d==2 for d in degree.values()) else None,
                      "watertight":bool(incidence) and all(n==2 for n in incidence.values())}
    return report
