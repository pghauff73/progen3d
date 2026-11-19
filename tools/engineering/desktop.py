"""Desktop authoring and execution using the same study compiler as the CLI."""
import json
from pathlib import Path
import queue
from threading import Thread
import webbrowser

from .domains import analyse
from .quantities import QuantityRegistry, StudyError
from .simulation import simulate
from .study import load_extensions, parse_study, scene_catalog
from .view import render_view, write_result


def run_project(study, scene="", kinds=""):
    path = Path(study)
    if path.suffix == ".json":
        return analyse(json.loads(path.read_text(encoding="utf-8")),
                       scene_catalog(scene) if scene else None)
    checked = parse_study(path, scene_catalog(scene) if scene else None,
                          load_extensions(kinds or None))
    return simulate(checked).results()


def launch():
    try:
        import tkinter as tk
        from tkinter import filedialog, ttk
        window = tk.Tk()
    except (ImportError, RuntimeError) as exc:
        raise StudyError(f"desktop authoring requires Tk: {exc}") from exc
    except tk.TclError as exc:
        raise StudyError(f"desktop authoring requires a graphical display: {exc}") from exc

    window.title("ProGen3D Engineering Studies")
    window.geometry("880x640")
    panel = ttk.Frame(window, padding=14)
    panel.pack(fill="both", expand=True)
    panel.columnconfigure(1, weight=1)
    panel.rowconfigure(7, weight=1)

    fields = {}
    for row, title in enumerate(("Study or domain model", "Native scene manifest", "Project quantity definitions")):
        value = tk.StringVar()
        fields[title] = value
        ttk.Label(panel, text=title).grid(row=row, column=0, sticky="w", padx=(0, 10))
        ttk.Entry(panel, textvariable=value).grid(row=row, column=1, sticky="ew", pady=3)
        def browse(field=value):
            selected = filedialog.askopenfilename()
            if selected:
                field.set(selected)
        ttk.Button(panel, text="Open…", command=browse).grid(row=row, column=2, padx=6)

    message = tk.StringVar(value="Choose a study or domain model to run.")
    ttk.Label(panel, textvariable=message).grid(row=5, columnspan=3, sticky="w", pady=9)
    quantity = tk.StringVar()
    ttk.Entry(panel, textvariable=quantity).grid(row=6, column=1, sticky="ew")
    view = tk.Text(panel, wrap="word", height=10, padx=9, pady=8)
    view.grid(row=7, columnspan=3, sticky="nsew", pady=7)
    completed = queue.Queue()
    results = []

    def write_text(text):
        view.delete("1.0", "end")
        view.insert("1.0", text)

    def run():
        paths = [field.get().strip() for field in fields.values()]
        if not paths[0]:
            message.set("Choose a study file.")
            return
        run_button.state(["disabled"])
        message.set("Running study…")

        def worker():
            try:
                completed.put((True, run_project(*paths)))
            except Exception as exc:
                completed.put((False, str(exc)))

        Thread(target=worker, daemon=True).start()
        poll()

    def poll():
        try:
            ok, result = completed.get_nowait()
        except queue.Empty:
            window.after(60, poll)
            return
        run_button.state(["!disabled"])
        if not ok:
            message.set("Study failed")
            write_text(result)
            return
        results.append(result)
        message.set(f"Completed {result['study']} · {len(results)} run(s)")
        summary = {key: value for key, value in result.items() if key not in ("observations", "accepted_steps_s")}
        write_text(json.dumps(summary, indent=2, ensure_ascii=False))

    def find():
        try:
            registry = QuantityRegistry(load_extensions(fields["Project quantity definitions"].get().strip() or None))
            query = quantity.get().strip().casefold()
            hits = [kind for kind in registry.kinds.values()
                    if query in (kind.identifier + " " + kind.label).casefold()]
            write_text("\n".join(f"{k.identifier} · {k.label}\n{k.source}" for k in hits[:100]))
            message.set(f"{len(hits)} quantities found")
        except StudyError as exc:
            message.set(str(exc))

    def plot():
        traces = [item for item in results if item.get("observations")]
        if not traces:
            message.set("Run a study with observations first.")
            return
        name = filedialog.asksaveasfilename(defaultextension=".html", initialfile="study-traces.html")
        if name:
            try:
                render_view(traces, name)
                webbrowser.open(Path(name).resolve().as_uri())
                message.set(f"Saved {Path(name).name}")
            except (OSError, ValueError) as exc:
                message.set(str(exc))

    def save():
        if not results:
            message.set("Run a study first.")
            return
        name = filedialog.asksaveasfilename(defaultextension=".json", initialfile="study-result.json")
        if name:
            try:
                write_result(name, json.dumps(results[-1], indent=2, allow_nan=False) + "\n")
                message.set(f"Saved {Path(name).name}")
            except (OSError, ValueError) as exc:
                message.set(str(exc))

    def edit_source():
        path = Path(fields["Study or domain model"].get().strip())
        try:
            source = path.read_text(encoding="utf-8")
        except (OSError, ValueError) as exc:
            message.set(str(exc))
            return
        editor = tk.Toplevel(window)
        editor.title(path.name)
        editor.geometry("790x520")
        text = tk.Text(editor, wrap="none", font="TkFixedFont", undo=True)
        text.insert("1.0", source)
        text.pack(fill="both", expand=True, padx=8, pady=8)
        feedback = tk.StringVar()
        ttk.Label(editor, textvariable=feedback).pack(side="right", padx=8)
        def save_source():
            try:
                write_result(path, text.get("1.0", "end-1c"))
                feedback.set("Saved " + path.name)
            except OSError as exc:
                feedback.set(str(exc))
        ttk.Button(editor, text="Save source", command=save_source).pack(side="left", padx=8, pady=(0, 8))

    ttk.Button(panel, text="Edit study source…", command=edit_source).grid(row=3, column=1, sticky="w", pady=4)
    run_button = ttk.Button(panel, text="Run study", command=run)
    run_button.grid(row=4, column=0, sticky="ew")
    ttk.Button(panel, text="Plot and compare…", command=plot).grid(row=4, column=1, sticky="w")
    ttk.Button(panel, text="Save latest result…", command=save).grid(row=4, column=2)
    ttk.Label(panel, text="Quantity search").grid(row=6, column=0, sticky="w")
    ttk.Button(panel, text="Find", command=find).grid(row=6, column=2)
    window.mainloop()
