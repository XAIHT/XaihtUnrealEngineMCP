#!/usr/bin/env python3
"""
scaffold_unreal_project.py
==========================

Deterministically scaffold a BRAND-NEW Unreal Engine C++ project from the
XaihtUnrealEngineMCP ``MCPGameProject`` template — fully renamed, with the
``UnrealMCP`` editor plugin bundled, an EngineAssociation set for UE 5.8, and a
freshly generated Visual Studio solution — so it opens in Visual Studio 2026 and
compiles + runs cleanly under Unreal Engine 5.8+.

The caller supplies ONLY two things:

    python scaffold_unreal_project.py --name <ProjectName> --dest <DirectoryLocation>

Everything else is automatic and self-contained (stdlib only, Windows):

  * validates ``--name`` is a legal Unreal C++ module identifier,
  * copies the template EXCLUDING every stale/generated artifact
    (Intermediate / Binaries / Saved / DerivedDataCache / .vs / .git / *.sln)
    so nothing carries a hard-coded old-engine path forward,
  * renames every file + in-file token that carries the module name
    ``MCPGameProject`` to ``<ProjectName>`` (the .uproject, both *.Target.cs,
    the module folder + its .Build.cs/.cpp/.h, and the DefaultEngine.ini game
    redirects),
  * discovers the UE 5.8 engine (explicit flag → registry → common install dirs)
    and, unless told not to, generates the Visual Studio solution against it,
  * (optionally) builds the Editor target and/or opens the solution.

Because generation/build/launch always use an EXPLICIT engine path, the whole
flow is independent of whether UE 5.8 is registered in the Windows registry.

Exit codes: 0 = success, 2 = fatal (nothing was half-written past a refusal).
The final stdout line is machine-readable: ``SCAFFOLD_RESULT: {json}``.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys

try:
    import winreg  # Windows only
except ImportError:  # pragma: no cover - non-Windows fallback
    winreg = None


TEMPLATE_MODULE = "MCPGameProject"

# Directories that must NEVER be copied — they are build artifacts / caches and
# would drag a stale engine path (UE_5.7) or prebuilt 5.7 binaries into the new
# project, breaking the "compiles cleanly under 5.8" goal.
COPY_EXCLUDE_DIRS = {
    "Intermediate", "Binaries", "Saved", "DerivedDataCache",
    ".vs", ".git", ".idea", "__pycache__",
}
COPY_EXCLUDE_EXTS = {".sln", ".suo", ".sdf", ".opensdf"}
COPY_EXCLUDE_SUFFIXES = (".VC.db", ".VC.opendb")

# Reserved names we refuse so we do not generate a project that cannot compile
# or that collides with the bundled plugin / engine modules.
CPP_KEYWORDS = {
    "alignas", "alignof", "and", "asm", "auto", "bool", "break", "case", "catch",
    "char", "class", "const", "constexpr", "continue", "default", "delete", "do",
    "double", "else", "enum", "explicit", "export", "extern", "false", "float",
    "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace",
    "new", "noexcept", "not", "nullptr", "operator", "or", "private", "protected",
    "public", "register", "return", "short", "signed", "sizeof", "static", "struct",
    "switch", "template", "this", "throw", "true", "try", "typedef", "typename",
    "union", "unsigned", "using", "virtual", "void", "volatile", "while",
}
UE_RESERVED_MODULES = {
    "unrealmcp", "engine", "core", "coreuobject", "unrealed", "editor", "slate",
    "slatecore", "umg", "inputcore", "projects",
}

# Non-essential integration plugins that some UE installs lack; scaffolded projects
# mark these Optional so a missing copy is skipped rather than failing the build.
KNOWN_OPTIONAL_PLUGINS = {"VisualStudioTools"}

DEFAULT_ENGINE_VERSION = "5.8"


def log(msg):
    print(msg, flush=True)


def die(msg):
    print("SCAFFOLD ERROR: " + msg, flush=True)
    print("SCAFFOLD_RESULT: " + json.dumps({"ok": False, "error": msg}), flush=True)
    sys.exit(2)


# --------------------------------------------------------------------------- #
# Validation
# --------------------------------------------------------------------------- #
def validate_name(name):
    if not name or not re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", name):
        die(
            "project name %r is not a valid Unreal C++ module identifier. Use only "
            "letters, digits and underscores; it must NOT start with a digit and "
            "must contain NO spaces or dashes (e.g. 'MyCoolGame', not 'My Cool-Game')."
            % name
        )
    if len(name) > 60:
        die("project name is too long (max 60 characters).")
    if name in CPP_KEYWORDS:
        die("project name %r is a reserved C++ keyword." % name)
    if name.lower() in UE_RESERVED_MODULES:
        die(
            "project name %r collides with an Unreal engine / plugin module name "
            "(the bundled plugin is 'UnrealMCP'). Pick a different name." % name
        )
    return name


# --------------------------------------------------------------------------- #
# Engine discovery
# --------------------------------------------------------------------------- #
def _has_build_bat(root):
    return bool(root) and os.path.isfile(
        os.path.join(root, "Engine", "Build", "BatchFiles", "Build.bat")
    )


def resolve_engine(explicit, version):
    """Resolve a UE engine root that actually has Build.bat. Registry-independent
    fallbacks first-class, because a manually-extracted 5.8 build may not be
    registered under HKLM EpicGames\\Unreal Engine\\<ver>."""
    tried = []

    if explicit:
        tried.append(explicit)
        if _has_build_bat(explicit):
            return explicit, tried

    # Registry (launcher installs write this).
    if winreg is not None:
        try:
            key_path = r"SOFTWARE\EpicGames\Unreal Engine\%s" % version
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, key_path) as k:
                val, _ = winreg.QueryValueEx(k, "InstalledDirectory")
                if val:
                    tried.append(val)
                    if _has_build_bat(val):
                        return val, tried
        except OSError:
            pass

    # Common on-disk install locations.
    for base in (
        r"C:\Program Files\Epic Games",
        r"D:\Program Files\Epic Games",
        r"E:\Program Files\Epic Games",
        r"C:\Epic Games",
        r"D:\Epic Games",
    ):
        cand = os.path.join(base, "UE_%s" % version)
        tried.append(cand)
        if _has_build_bat(cand):
            return cand, tried

    return None, tried


# --------------------------------------------------------------------------- #
# Copy (with excludes)
# --------------------------------------------------------------------------- #
def _ignore(dirpath, names):
    ignored = set()
    for n in names:
        full = os.path.join(dirpath, n)
        if os.path.isdir(full) and n in COPY_EXCLUDE_DIRS:
            ignored.add(n)
            continue
        ext = os.path.splitext(n)[1].lower()
        if ext in COPY_EXCLUDE_EXTS:
            ignored.add(n)
            continue
        if any(n.endswith(s) for s in COPY_EXCLUDE_SUFFIXES):
            ignored.add(n)
    return ignored


# --------------------------------------------------------------------------- #
# Rename + rewrite
# --------------------------------------------------------------------------- #
def _rewrite_tokens(path, name):
    """Byte-exact replace of the template module token; preserves encoding + EOLs."""
    with open(path, "rb") as f:
        data = f.read()
    new = data.replace(TEMPLATE_MODULE.encode("utf-8"), name.encode("utf-8"))
    if new != data:
        with open(path, "wb") as f:
            f.write(new)
        return True
    return False


def _safe_move(src, dst):
    if os.path.abspath(src) == os.path.abspath(dst):
        return
    os.replace(src, dst)


def rename_project(project_dir, name):
    """Rename every file/folder + in-file token that carries the module name."""
    src = os.path.join(project_dir, "Source")

    # 1) Module source folder: Source/MCPGameProject/ -> Source/<Name>/
    old_mod_dir = os.path.join(src, TEMPLATE_MODULE)
    new_mod_dir = os.path.join(src, name)
    if os.path.isdir(old_mod_dir):
        _safe_move(old_mod_dir, new_mod_dir)

    # 2) Files inside the module folder: .Build.cs / .cpp / .h
    for ext in (".Build.cs", ".cpp", ".h"):
        old = os.path.join(new_mod_dir, TEMPLATE_MODULE + ext)
        new = os.path.join(new_mod_dir, name + ext)
        if os.path.isfile(old):
            _safe_move(old, new)

    # 3) Target rules: MCPGameProject.Target.cs / MCPGameProjectEditor.Target.cs
    for old_base, new_base in (
        (TEMPLATE_MODULE + ".Target.cs", name + ".Target.cs"),
        (TEMPLATE_MODULE + "Editor.Target.cs", name + "Editor.Target.cs"),
    ):
        old = os.path.join(src, old_base)
        new = os.path.join(src, new_base)
        if os.path.isfile(old):
            _safe_move(old, new)

    # 4) The .uproject
    old_uproject = os.path.join(project_dir, TEMPLATE_MODULE + ".uproject")
    new_uproject = os.path.join(project_dir, name + ".uproject")
    if os.path.isfile(old_uproject):
        _safe_move(old_uproject, new_uproject)

    # 5) In-file token rewrite across every text file that could reference it.
    rewritten = []
    rewrite_targets = [
        new_uproject,
        os.path.join(src, name + ".Target.cs"),
        os.path.join(src, name + "Editor.Target.cs"),
        os.path.join(new_mod_dir, name + ".Build.cs"),
        os.path.join(new_mod_dir, name + ".cpp"),
        os.path.join(new_mod_dir, name + ".h"),
        os.path.join(project_dir, "Config", "DefaultEngine.ini"),
        os.path.join(project_dir, "Config", "DefaultGame.ini"),
        os.path.join(project_dir, "Config", "DefaultEditor.ini"),
        os.path.join(project_dir, "Config", "DefaultInput.ini"),
    ]
    for tgt in rewrite_targets:
        if os.path.isfile(tgt) and _rewrite_tokens(tgt, name):
            rewritten.append(os.path.basename(tgt))

    return new_uproject, rewritten


def set_engine_association(uproject_path, engine_version):
    """Set EngineAssociation and guarantee Modules[].Name == <project name>."""
    with open(uproject_path, "r", encoding="utf-8-sig") as f:
        data = json.load(f)
    data["EngineAssociation"] = engine_version
    name = os.path.splitext(os.path.basename(uproject_path))[0]
    for mod in data.get("Modules", []) or []:
        if mod.get("Name") in (TEMPLATE_MODULE, name):
            mod["Name"] = name
    # Defensive: mark non-essential integration plugins Optional so a missing
    # copy in the target engine is skipped (a warning) instead of failing the
    # build with "Unable to find plugin '...'". VisualStudioTools (the VS
    # Integration Tool) is not bundled with every UE install.
    for plug in data.get("Plugins", []) or []:
        if plug.get("Name") in KNOWN_OPTIONAL_PLUGINS:
            plug["Optional"] = True
            plug.pop("SupportedTargetPlatforms", None)
    with open(uproject_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
        f.write("\n")


def add_module_redirects(project_dir, name):
    """Append ActiveGameNameRedirects so any Content asset still referencing the
    old /Script/MCPGameProject module resolves to /Script/<Name>."""
    ini = os.path.join(project_dir, "Config", "DefaultEngine.ini")
    if not os.path.isfile(ini):
        return
    with open(ini, "r", encoding="utf-8-sig") as f:
        text = f.read()
    add = (
        '+ActiveGameNameRedirects=(OldGameName="MCPGameProject",'
        'NewGameName="/Script/%s")\n'
        '+ActiveGameNameRedirects=(OldGameName="/Script/MCPGameProject",'
        'NewGameName="/Script/%s")\n' % (name, name)
    )
    if "/Script/MCPGameProject" in add and add.strip() in text:
        return
    if "[/Script/Engine.Engine]" in text:
        text = text.replace(
            "[/Script/Engine.Engine]", "[/Script/Engine.Engine]\n" + add, 1
        )
    else:
        text = text.rstrip() + "\n\n[/Script/Engine.Engine]\n" + add
    with open(ini, "w", encoding="utf-8") as f:
        f.write(text)


def patch_vs_tools_ue58(project_dir):
    """Ensure the Visual Studio Tools plugin compiles on UE 5.8.

    Visual Studio 2026 ships / auto-injects the "Visual Studio Integration Tools" plugin
    whose VisualStudioToolsBlueprintBreakpointExtension.cpp does
    `#include <BlueprintGraphClasses.h>` — an aggregate header Epic REMOVED in UE 5.8
    (IWYU cleanup), so it fails to compile with C1083 and takes the whole build down
    (MSB3073, Build.bat exit code 6). The only symbol used from that header is
    UK2Node_CallFunction, so the minimal, targeted fix is `#include <K2Node_CallFunction.h>`.

    The template already bundles the fixed plugin, so a normal scaffold ships it correct.
    This keeps the scaffold correct even if that bundled copy is ever refreshed from a
    still-broken upstream. Idempotent: a no-op when the file is missing or already fixed.
    Returns True if it patched.
    """
    cpp = os.path.join(
        project_dir, "Plugins", "VisualStudioTools", "Source", "VisualStudioTools",
        "Private", "VisualStudioToolsBlueprintBreakpointExtension.cpp",
    )
    if not os.path.isfile(cpp):
        return False
    try:
        with open(cpp, "r", encoding="utf-8") as f:
            text = f.read()
    except OSError:
        return False
    broken = "#include <BlueprintGraphClasses.h>"
    if broken not in text:
        return False
    text = text.replace(broken, "#include <K2Node_CallFunction.h>")
    with open(cpp, "w", encoding="utf-8") as f:
        f.write(text)
    return True


# --------------------------------------------------------------------------- #
# Plugin hardening (optionalize plugins the resolved engine does not ship)
# --------------------------------------------------------------------------- #
def _uplugin_names_under(root):
    """Lowercased basenames of every .uplugin found anywhere under ``root`` (one walk)."""
    names = set()
    if root and os.path.isdir(root):
        for _dp, _dirs, files in os.walk(root):
            for fn in files:
                if fn.lower().endswith(".uplugin"):
                    names.add(os.path.splitext(fn)[0].lower())
    return names


def harden_missing_plugins(uproject_path, engine_root):
    """Set ``Optional: true`` on every ENABLED plugin that the resolved engine does
    NOT ship (and the project does not bundle), so a plugin missing from THIS
    machine's engine can never hard-fail the build with "Unable to find plugin
    '...'". Engine plugins live under <engine>/Engine/Plugins; project plugins under
    <project>/Plugins. Returns the list of plugin names that were made optional."""
    if not engine_root or not os.path.isdir(engine_root):
        return []
    project_dir = os.path.dirname(uproject_path)
    available = (
        _uplugin_names_under(os.path.join(engine_root, "Engine", "Plugins"))
        | _uplugin_names_under(os.path.join(project_dir, "Plugins"))
    )
    with open(uproject_path, "r", encoding="utf-8-sig") as f:
        data = json.load(f)
    changed = []
    for plug in data.get("Plugins", []) or []:
        pname = plug.get("Name")
        if not pname or not plug.get("Enabled", False) or plug.get("Optional"):
            continue
        if pname.lower() in available:
            continue
        plug["Optional"] = True
        changed.append(pname)
    if changed:
        with open(uproject_path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2)
            f.write("\n")
    return changed


# --------------------------------------------------------------------------- #
# Engine actions
# --------------------------------------------------------------------------- #
def _run(cmd, cwd, timeout):
    log("  $ " + " ".join('"%s"' % c if " " in c else c for c in cmd))
    try:
        p = subprocess.run(
            cmd, cwd=cwd, timeout=timeout,
            capture_output=True, text=True, encoding="utf-8", errors="replace",
        )
    except subprocess.TimeoutExpired:
        return 1, "", "timed out after %ss" % timeout
    tail = (p.stdout or "")[-2500:]
    if p.stdout:
        log(tail)
    if p.returncode != 0 and p.stderr:
        log("  [stderr] " + (p.stderr or "")[-1500:])
    return p.returncode, p.stdout or "", p.stderr or ""


def generate_solution(engine_root, uproject_path, timeout):
    project_dir = os.path.dirname(uproject_path)
    name = os.path.splitext(os.path.basename(uproject_path))[0]
    sln = os.path.join(project_dir, name + ".sln")
    build_bat = os.path.join(engine_root, "Engine", "Build", "BatchFiles", "Build.bat")
    ubt = os.path.join(
        engine_root, "Engine", "Binaries", "DotNET", "UnrealBuildTool",
        "UnrealBuildTool.exe",
    )

    attempts = []
    if os.path.isfile(build_bat):
        attempts.append([build_bat, "-projectfiles",
                         "-project=" + uproject_path, "-game", "-progress"])
    if os.path.isfile(ubt):
        attempts.append([ubt, "-projectfiles",
                         "-project=" + uproject_path, "-game", "-progress"])

    for cmd in attempts:
        rc, _out, _err = _run(cmd, project_dir, timeout)
        if os.path.isfile(sln):
            return True, sln
    return os.path.isfile(sln), sln


def build_editor(engine_root, uproject_path, timeout):
    project_dir = os.path.dirname(uproject_path)
    name = os.path.splitext(os.path.basename(uproject_path))[0]
    build_bat = os.path.join(engine_root, "Engine", "Build", "BatchFiles", "Build.bat")
    if not os.path.isfile(build_bat):
        return False
    rc, _o, _e = _run(
        [build_bat, name + "Editor", "Win64", "Development",
         "-project=" + uproject_path, "-waitmutex"],
        project_dir, timeout,
    )
    return rc == 0


def open_solution(sln):
    try:
        os.startfile(sln)  # noqa: launches VS via the .sln association
        return True
    except Exception as e:  # pragma: no cover
        log("  could not open solution automatically: %s" % e)
        return False


# --------------------------------------------------------------------------- #
# Main
# --------------------------------------------------------------------------- #
def main():
    ap = argparse.ArgumentParser(
        description="Scaffold a new UE 5.8 C++ project (with the UnrealMCP plugin) "
                    "from the XaihtUnrealEngineMCP template."
    )
    ap.add_argument("--name", required=True, help="New project name (valid C++ module identifier)")
    ap.add_argument("--dest", required=True, help="Parent directory the project folder is created in")
    ap.add_argument("--template", default="", help="Template MCPGameProject dir (default: next to this script)")
    ap.add_argument("--engine", default="", help="Explicit UE engine root (default: auto-discover)")
    ap.add_argument("--engine-version", default=DEFAULT_ENGINE_VERSION, help="UE version (default 5.8)")
    ap.add_argument("--generate", dest="generate", action="store_true", default=True,
                    help="Generate the Visual Studio solution (default: on)")
    ap.add_argument("--no-generate", dest="generate", action="store_false")
    ap.add_argument("--build", action="store_true", default=False, help="Also build the Editor target (slow)")
    ap.add_argument("--open", dest="open_sln", action="store_true", default=False, help="Open the .sln when done")
    ap.add_argument("--force", action="store_true", default=False, help="Overwrite an existing destination")
    ap.add_argument("--gen-timeout", type=int, default=900, help="Seconds for solution generation")
    ap.add_argument("--build-timeout", type=int, default=3600, help="Seconds for the editor build")
    args = ap.parse_args()

    name = validate_name(args.name.strip())

    # Template resolution.
    script_dir = os.path.dirname(os.path.abspath(__file__))
    template = args.template.strip() or os.path.join(script_dir, "MCPGameProject")
    template = os.path.abspath(template)
    if not os.path.isfile(os.path.join(template, TEMPLATE_MODULE + ".uproject")):
        die("template not found or invalid (no %s.uproject) at: %s" % (TEMPLATE_MODULE, template))
    if not os.path.isdir(os.path.join(template, "Plugins", "UnrealMCP")):
        die("template is missing the UnrealMCP plugin at: %s\\Plugins\\UnrealMCP" % template)

    # Destination.
    dest_parent = os.path.abspath(args.dest.strip())
    project_dir = os.path.join(dest_parent, name)
    if os.path.exists(project_dir):
        if not args.force:
            die("destination already exists: %s  (pass --force to overwrite)" % project_dir)
        shutil.rmtree(project_dir)
    os.makedirs(dest_parent, exist_ok=True)

    log("=" * 70)
    log("  Scaffolding Unreal project '%s'" % name)
    log("  template : %s" % template)
    log("  dest     : %s" % project_dir)
    log("=" * 70)

    # 1) Copy (excluding build artifacts).
    log("[1/5] Copying template (excluding Intermediate/Binaries/Saved/.vs/.git/*.sln)...")
    shutil.copytree(template, project_dir, ignore=_ignore)

    # 2) Rename files + tokens.
    log("[2/5] Renaming module '%s' -> '%s'..." % (TEMPLATE_MODULE, name))
    uproject, rewritten = rename_project(project_dir, name)
    if not os.path.isfile(uproject):
        die("rename failed: expected %s" % uproject)
    log("      rewrote tokens in: %s" % ", ".join(rewritten))

    # 3) EngineAssociation + module redirects.
    log("[3/5] Setting EngineAssociation='%s' + module redirects..." % args.engine_version)
    set_engine_association(uproject, args.engine_version)
    add_module_redirects(project_dir, name)
    if patch_vs_tools_ue58(project_dir):
        log("      Patched Visual Studio Tools plugin for UE 5.8 "
            "(BlueprintGraphClasses.h -> K2Node_CallFunction.h).")

    result = {
        "ok": True,
        "name": name,
        "project_dir": project_dir,
        "uproject": uproject,
        "engine_version": args.engine_version,
        "engine_root": "",
        "solution": "",
        "generated": False,
        "built": False,
        "opened": False,
        "notes": [],
    }

    # 4) Engine discovery + solution generation.
    engine_root, tried = resolve_engine(args.engine.strip(), args.engine_version)
    if engine_root:
        result["engine_root"] = engine_root
        log("[4/5] Engine: %s" % engine_root)
        # Harden: any enabled plugin this engine does not ship (and the project
        # does not bundle) is marked Optional so it can never hard-fail the build.
        missing = harden_missing_plugins(uproject, engine_root)
        if missing:
            note = ("Plugins not shipped by this engine were marked Optional so "
                    "they cannot block the build: %s" % ", ".join(missing))
            result["notes"].append(note)
            log("      " + note)
    else:
        result["notes"].append(
            "UE %s engine not found (looked in: %s). Solution NOT generated — "
            "pass --engine <UE root>, or open the .uproject and let Unreal generate it."
            % (args.engine_version, "; ".join(tried))
        )
        log("[4/5] WARNING: " + result["notes"][-1])

    if args.generate and engine_root:
        log("      Generating Visual Studio solution (may take 1-2 min)...")
        ok_gen, sln = generate_solution(engine_root, uproject, args.gen_timeout)
        result["generated"] = ok_gen
        result["solution"] = sln if ok_gen else ""
        if ok_gen:
            log("      Solution: %s" % sln)
        else:
            result["notes"].append(
                "Solution generation did not produce %s. You can generate it "
                "manually: right-click the .uproject -> 'Generate Visual Studio "
                "project files', or run Build.bat -projectfiles." % os.path.basename(sln)
            )
            log("      WARNING: " + result["notes"][-1])

    # 5) Optional build / open.
    if args.build and engine_root:
        log("[5/5] Building %sEditor Win64 Development (this is slow)..." % name)
        result["built"] = build_editor(engine_root, uproject, args.build_timeout)
    if args.open_sln and result["solution"]:
        result["opened"] = open_solution(result["solution"])

    log("=" * 70)
    log("  DONE. Project ready at: %s" % project_dir)
    if result["solution"]:
        log("  Open in Visual Studio: %s" % result["solution"])
        log("  Then set config 'Development Editor | Win64', build, and press F5 to run.")
    log("=" * 70)
    print("SCAFFOLD_RESULT: " + json.dumps(result), flush=True)
    sys.exit(0)


if __name__ == "__main__":
    main()
