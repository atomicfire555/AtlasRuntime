from pathlib import Path
import sys

script_path = Path(__file__).with_name("apply_stage23.py")
code = script_path.read_text(encoding="utf-8")
old = "replace_once('\"pluginVersion\":52,', '\"pluginVersion\":53,')"
new = "replace_once(r'\\\"pluginVersion\\\":52,', r'\\\"pluginVersion\\\":53,')"
if old not in code:
    raise RuntimeError("Stage 23 generator compatibility patch target was not found")
code = code.replace(old, new, 1)
sys.argv = [str(script_path), *sys.argv[1:]]
exec(compile(code, str(script_path), "exec"), {"__name__": "__main__", "__file__": str(script_path)})
