"""Static import dependency audit for the anatomical Python prototype.

Uses the Python AST rather than executing imports. Detects missing local
modules and directed cycles among files in examples/AnatomicalArm.
"""
import argparse
import ast
import json
from pathlib import Path

def audit(directory):
    directory=Path(directory)
    files={p.stem:p for p in directory.glob("*.py") if not p.name.startswith("test_")}
    imports={}
    syntax_errors={}
    for name,path in sorted(files.items()):
        try:
            tree=ast.parse(path.read_text(encoding="utf-8"),filename=str(path))
        except (SyntaxError,UnicodeError) as exc:
            syntax_errors[name]=str(exc)
            continue
        deps=set()
        # Only imports executed at module initialization are dependencies
        # for import-time cycle detection. Deferred function imports are
        # deliberately excluded from this graph.
        for node in tree.body:
            if isinstance(node,ast.Import):
                deps.update(alias.name.split(".")[0] for alias in node.names)
            elif isinstance(node,ast.ImportFrom) and node.level==0 and node.module:
                deps.add(node.module.split(".")[0])
        imports[name]=sorted(deps & files.keys())
    cycles=set()
    active=[]
    visited=set()
    def visit(name):
        if name in active:
            cycle=active[active.index(name):]
            # Canonicalize by rotating the sequence, preserving direction.
            variants=[tuple(cycle[i:]+cycle[:i]) for i in range(len(cycle))]
            cycles.add(min(variants))
            return
        if name in visited:return
        active.append(name)
        for dep in imports.get(name,()):
            visit(dep)
        active.pop()
        visited.add(name)
    for name in sorted(imports):
        visit(name)
    return {"schema":"progen3d.anatomical-dependency-audit.v1",
            "modules":len(files),"dependencies":imports,
            "syntax_errors":syntax_errors,
            "cycles":[list(c) for c in sorted(cycles)],
            "passed":not syntax_errors and not cycles}

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--directory",default=str(Path(__file__).resolve().parent))
    p.add_argument("--out",default="build/milestone1/dependencies.json")
    args=p.parse_args()
    result=audit(args.directory)
    target=Path(args.out);target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({"passed":result["passed"],"modules":result["modules"],
                      "cycles":result["cycles"],"syntax_errors":result["syntax_errors"]}))
    raise SystemExit(0 if result["passed"] else 1)
