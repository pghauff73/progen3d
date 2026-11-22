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
