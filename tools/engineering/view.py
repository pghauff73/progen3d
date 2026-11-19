"""An offline observation viewer for persisted study and domain results."""
import json
from tempfile import NamedTemporaryFile
from pathlib import Path

from .quantities import StudyError


def write_result(output, content):
    path = Path(output)
    temp = None
    try:
        with NamedTemporaryFile("w", encoding="utf-8", dir=path.parent, delete=False) as stream:
            temp = Path(stream.name)
            stream.write(content)
        temp.replace(path)
    finally:
        if temp is not None:
            temp.unlink(missing_ok=True)


def render_view(results, output):
    for result in results:
        if not isinstance(result, dict) or not isinstance(result.get("observations"), dict):
            raise StudyError("viewer requires result files with observations")
        for trace in result["observations"].values():
            if not isinstance(trace, dict) or not isinstance(trace.get("samples"), list) or not isinstance(trace.get("unit"), str):
                raise StudyError("invalid observation")
    payload = json.dumps(results, ensure_ascii=False, allow_nan=False).replace("<", "\\u003c")
    template = Path(__file__).with_name("trace_view.html").read_text(encoding="utf-8")
    write_result(output, template.replace("__RESULTS__", payload))
