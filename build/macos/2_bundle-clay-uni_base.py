#!/usr/bin/env python3
import os
import platform
import re
import shutil
import subprocess
import stat
import sys
from pathlib import Path
from glob import glob

# This script is used to create a Clay .app bundle on macOS. A bundle
# is used as source of files for making the .dmg installer
BUILD_DIR = os.getenv("MESON_BUILD_ROOT")
if not BUILD_DIR:
  # Let's prevent contributors from creating broken bundles
  print("\033[31m(ERROR)\033[0m: Script called standalone. Please build Clay targeting DMG installer creation.")
  sys.exit(1)
# Get variables from MESON_BUILD_ROOT/config.h that can be used on this script
config_vars = {}
with open("config.h") as file:
  for line in file:
    match = re.match(r'^#\s*define\s+(\S+)(?:\s+(.*))?$', line)
    if match:
      key, value = match.groups()
      if value is None or not value.strip():
        value = "1" #needed when there is no explicit value
      else:
        value = value.strip().strip('"').strip("'")
      config_vars[key] = value
globals().update(config_vars)
if not config_vars.get("ENABLE_RELOCATABLE_RESOURCES"):
  print("\n\033[31m(ERROR)\033[0m: No relocatable Clay build found. You can build Clay with '-Drelocatable-bundle=yes' to make a build suitable for .app creation.")
  sys.exit(1)


# Bundle deps and Clay files
CLAY_SOURCE = Path(os.getenv("MESON_SOURCE_ROOT"))

## System prefix: it is OPT_PREFIX (see 1_build-deps-macports)
OPT_PREFIX = Path(os.getenv("OPT_PREFIX"))
## Clay prefix: as set at meson configure time
CLAY_PREFIX = Path(os.getenv("MESON_INSTALL_DESTDIR_PREFIX"))

## Bundle dir: we make a "perfect" bundle separated from CLAY_PREFIX
#NOTE: The bundling script need to set $OPT_PREFIX to our dist scripts
#fallback code be able to identify what arch they are distributing
CLAY_DISTRIB = Path(CLAY_SOURCE) / f"clay-{platform.machine()}.app" / "Contents"

def bundle(src_root, pattern, option="None", override=None):
  ## Search for targets in search path
  src_root = Path(src_root)
  paths_to_bundle = list(src_root.glob(pattern))
  if not paths_to_bundle:
    print(f"\033[31m(ERROR)\033[0m: not found {src_root}/{pattern}")
    sys.exit(1)
  for src_path in paths_to_bundle:
    ## Copy found targets to bundle path
    symlink_cleanup = "--preserve-symlink" not in option
    if "--dest" in option:
      dest_path = CLAY_DISTRIB / Path(override) / src_path.name
    elif "--rename" in option:
      dest_path = CLAY_DISTRIB / Path(override)
    elif "bin/" in pattern:
      symlink_cleanup = False
      dest_path = CLAY_DISTRIB / "MacOS" / Path(src_path.relative_to(src_root)).name
    elif "lib/" in pattern:
      #we bundle on lib/ to avoid 'bundle format unrecognized, invalid, or unsuitable' on signing
      dest_path = CLAY_DISTRIB / "lib" / src_path.relative_to(src_root / "lib")
      #Needed by app/main.c (not anymore since we bundle directly on lib)
      #link_lib_path = Path(f"{CLAY_DISTRIB}/lib")
      #link_lib_path.unlink(missing_ok=True)
      #link_lib_path.symlink_to(os.path.relpath(Path(f"{CLAY_DISTRIB}/Frameworks"), link_lib_path.parent))
    elif "share/" in pattern:
      dest_path = CLAY_DISTRIB / "Resources" / src_path.relative_to(src_root / "share")
      #Needed by app/main.c, app/config/claycoreconfig.c, app/core/claydata.c, libclaywidgets/claywidgets-private.c and plug-ins/common/file-wmf.c
      link_share_path = Path(f"{CLAY_DISTRIB}/share")
      link_share_path.unlink(missing_ok=True)
      link_share_path.symlink_to(os.path.relpath(Path(f"{CLAY_DISTRIB}/Resources"), link_share_path.parent))
    elif "etc/" in pattern:
      dest_path = CLAY_DISTRIB / "etc" / src_path.relative_to(src_root / "etc")
      #Needed by app/main.c
      #link_etc_path = Path(f"{CLAY_DISTRIB}/etc")
      #link_etc_path.unlink(missing_ok=True)
      #link_etc_path.symlink_to(os.path.relpath(Path(f"{CLAY_DISTRIB}/SharedSupport"), link_etc_path.parent))
    dest_path.parent.mkdir(parents=True, exist_ok=True)
    current_mode = dest_path.parent.stat().st_mode
    os.chmod(dest_path.parent, current_mode | stat.S_IWUSR)
    print(f"Bundling {src_path} to {dest_path.parent}")
    if src_path.is_dir():
      try:
        shutil.copytree(src_path, dest_path, dirs_exist_ok=True)
        current_mode = dest_path.stat().st_mode
        os.chmod(dest_path, current_mode | stat.S_IWUSR)
      except shutil.Error as e:
        print(f"\033[33m(WARNING)\033[0m: {dest_path} seems to already exist and have permission problems")
    else:
      if not str(src_path).endswith(".typelib"):
        ## Homebrew ships read-only files; make an existing copy writable so it can be replaced
        if dest_path.is_file() and not dest_path.is_symlink():
          os.chmod(dest_path, dest_path.stat().st_mode | stat.S_IWUSR)
        try:
          shutil.copy2(src_path, dest_path, follow_symlinks=symlink_cleanup)
        except shutil.Error as e:
          print(f"\033[33m(WARNING)\033[0m: {dest_path} seems to already exist and have permission problems")
        if "MacOS/" in str(dest_path) and not dest_path.is_symlink():
          os.chmod(dest_path, 0o755)
      else:
        ## Process .typelib dependencies (as relocatable)
        tmp_gir_dir = CLAY_DISTRIB / "tmp"
        tmp_gir_dir.mkdir(parents=True, exist_ok=True)
        def set_typelib_rpath(typelib, prefix):
          #Same as 'tools/make-mac-typelib.py reloc'
          typelib_path = Path(f"{prefix}/lib/girepository-1.0/{typelib}.typelib")
          target_path = dest_path.parent / typelib_path.name
          if typelib_path.exists() and not target_path.exists():
            if typelib_path != src_path:
              print(f"Bundling {typelib_path} to {dest_path.parent}")
            tmp_gir_path = Path(f"{tmp_gir_dir}/{typelib}.gir")
            shutil.copy2(Path(f"{prefix}/share/gir-1.0/{typelib}.gir"), tmp_gir_path)
            text = tmp_gir_path.read_text()
            text = re.sub(r'shared-library="([^"]+)"', lambda m: 'shared-library="' + ",".join("@rpath/" + os.path.basename(p) for p in m.group(1).split(",")) + '"', text)
            tmp_gir_path.write_text(text)
            subprocess.run(["g-ir-compiler", f"--includedir={tmp_gir_dir}", str(tmp_gir_path), "-o", target_path], check=True)
        def process_typelib(path, typelib_list=None):
          set_typelib_rpath(Path(path).stem, CLAY_PREFIX)
          if typelib_list is None:
            typelib_list = set()
          cmd = ['g-ir-inspect', '--print-typelibs', os.path.basename(path).split('-')[0]]
          ## Clay's own typelibs are not on g-ir-inspect's default search path
          env = dict(os.environ, GI_TYPELIB_PATH=f"{CLAY_PREFIX}/lib/girepository-1.0:{OPT_PREFIX}/lib/girepository-1.0")
          result = subprocess.run(cmd, capture_output=True, text=True, env=env)
          for line in result.stdout.splitlines():
            typelib = line.replace("typelib: ", "").strip()
            if typelib and typelib not in typelib_list:
              typelib_list.add(typelib)
              for prefix in [CLAY_PREFIX, OPT_PREFIX]:
                typelib_path = Path(f"{prefix}/lib/girepository-1.0/{typelib}.typelib")
                target_path = dest_path.parent / typelib_path.name
                if typelib_path.exists() and not target_path.exists():
                  set_typelib_rpath(typelib, prefix)
                process_typelib(typelib, typelib_list)
        process_typelib(src_path)
        shutil.rmtree(tmp_gir_dir)

def clean(base_path, pattern):
  base_path = Path(base_path)
  first_found = False
  for parent_path in base_path.glob(os.path.dirname(pattern)):
    for path in parent_path.rglob(os.path.basename(pattern)):
      if path.exists():
        if not first_found:
           print(f"Cleaning {base_path}/{pattern}")
           first_found = True
        if path.is_dir():
          shutil.rmtree(path)
        else:
          path.unlink()


## PREPARE BUNDLE
CLAY_DISTRIB.mkdir(parents=True, exist_ok=True)
### Prevent Git going crazy
(CLAY_DISTRIB / ".." / ".gitignore").write_text("*\n")
### Info.plist (it will be configured by 3_dist-clay-apple script)
shutil.copy2(Path(f"{CLAY_SOURCE}/build/macos/Info.plist"), CLAY_DISTRIB)
### Icon: no Assets.car (Liquid Glass). It drew the Clay "C" too small in the Dock,
### so macOS uses AppIcon.icns. clay-dist-local.sh drops CFBundleIconName without Assets.car.
(CLAY_DISTRIB / "Resources").mkdir(parents=True, exist_ok=True)
shutil.copy2(Path(f"{BUILD_DIR}/clay-data/images/logo/clay.icns"), CLAY_DISTRIB / "Resources/AppIcon.icns")
shutil.copy2(Path(f"{BUILD_DIR}/build/macos/fileicon-xcf.icns"), CLAY_DISTRIB / "Resources/fileicon-xcf.icns")
shutil.copy2(Path(f"{BUILD_DIR}/build/macos/fileicon.icns"), CLAY_DISTRIB / "Resources/fileicon.icns")


## BUNDLE BASE (BARE MINIMUM TO RUN GTK APPS).
### Needed for file dialogs (only .compiled file is needed on macOS)
bundle(OPT_PREFIX, "share/glib-*/schemas/gschemas.compiled")
### Mostly bogus since we do use macOS API directly from clay to open remote files
if os.path.exists(OPT_PREFIX / "bin/port"):
  bundle(OPT_PREFIX, "lib/libproxy/libpxbackend*.dylib", "--dest", "lib")
bundle(OPT_PREFIX, "lib/gio")
### Needed to not crash UI. See: https://github.com/KurtStevenK/clay/-/issues/6165
bundle(OPT_PREFIX, "share/icons/Adwaita")
### Needed by GTK to use icon themes. See: https://github.com/KurtStevenK/clay/-/issues/5080
bundle(CLAY_PREFIX, "share/icons/hicolor")
### Needed to loading icons in GUI
bundle(OPT_PREFIX, "lib/gdk-pixbuf-*/*/loaders/libpixbufloader*svg*")
bundle(OPT_PREFIX, "lib/gdk-pixbuf-*/*/loaders.cache")
loaders_cache = glob(f"{CLAY_DISTRIB}/lib/gdk-pixbuf-*/*/loaders.cache")
text = Path(loaders_cache[0]).read_text()
new_text = text.replace(f"{OPT_PREFIX}/lib/", "")
Path(loaders_cache[0]).write_text(new_text)
### Needed for printing support
bundle(OPT_PREFIX, "lib/gtk-3.0/*.*.*/printbackends/*.so")
### Needed for macOS emoji keyboard support (with im-quartz.so)
bundle(OPT_PREFIX, "lib/gtk-3.0/*.*.*/immodules/*.so")
if os.path.exists(OPT_PREFIX / "bin/port"):
  bundle(OPT_PREFIX, "etc/gtk-3.0/gtk.immodules", "--rename", "lib/gtk-3.0/3.0.0/immodules.cache")
else: #os.path.exists(OPT_PREFIX / "bin/brew"):
  bundle(OPT_PREFIX, "lib/gtk-3.0/*.*.*/immodules.cache")
im_cache = glob(f"{CLAY_DISTRIB}/lib/gtk-3.0/*.*.*/immodules.cache")
text = Path(im_cache[0]).read_text()
new_text = re.sub(r'/.*?(?=gtk-3\.0/)', '', text)
Path(im_cache[0]).write_text(new_text)
### Needed for MacOS-style keyboard shortcuts
bundle(OPT_PREFIX, "share/themes/Mac")


## CORE FEATURES.
bundle(CLAY_PREFIX, "lib/libbabl*-*.*.*.dylib")
bundle(CLAY_PREFIX, "lib/babl-*")
bundle(CLAY_PREFIX, "lib/libgegl*-*.*.*.dylib")
bundle(CLAY_PREFIX, "lib/gegl-*")
bundle(CLAY_PREFIX, "lib/libclay*-*.*.*.dylib")
### Runtime libs loaded by name or pulled in by Homebrew deps, when the build found them
### (SDL3 is dlopen-ed by Clay, libomp is needed by libraw-based GEGL ops)
if glob(f"{OPT_PREFIX}/lib/libSDL3*.dylib"):
  bundle(OPT_PREFIX, "lib/libSDL3*.dylib")
if (OPT_PREFIX / "opt/libomp/lib/libomp.dylib").exists():
  bundle(OPT_PREFIX, "opt/libomp/lib/libomp.dylib", "--dest", "lib")
bundle(CLAY_PREFIX, "lib/clay")
bundle(CLAY_PREFIX, "share/clay")
lang_array = [Path(f).stem for f in glob(str(Path(CLAY_SOURCE)/"po/*.po"))]
for lang in lang_array:
  bundle(CLAY_PREFIX, f"share/locale/{lang}/LC_MESSAGES/*.mo")
  # Needed for eventually used widgets, GTK inspector etc
  if glob(f"{OPT_PREFIX}/share/locale/{lang}/LC_MESSAGES/gtk*.mo"):
    bundle(OPT_PREFIX, f"share/locale/{lang}/LC_MESSAGES/gtk*.mo")
  # FIXME: For language list in text tool options (not working)
  if glob(f"{OPT_PREFIX}/share/locale/{lang}/LC_MESSAGES/iso_639_3.mo"):
    bundle(OPT_PREFIX, f"share/locale/{lang}/LC_MESSAGES/iso_639_3.mo")
# GTK (built with QUARTZ_RELOCATION) looks for its catalogs (gtk30.mo etc.) at
# <bundle>/Contents/Resources/share/locale, so its stock strings get translated,
# including the "Hide"/"Quit"/"Services"... entries of the macOS app menu.
gtk_locale_link = CLAY_DISTRIB / "Resources" / "share" / "locale"
gtk_locale_link.parent.mkdir(parents=True, exist_ok=True)
gtk_locale_link.unlink(missing_ok=True)
gtk_locale_link.symlink_to(Path("..") / "locale")
bundle(CLAY_PREFIX, "etc/clay")


## OTHER FEATURES AND PLUG-INS.
### Support for non .PAT patterns: https://github.com/KurtStevenK/clay/-/issues/12351
bundle(OPT_PREFIX, "lib/gdk-pixbuf-*/*/loaders/libpixbufloader-bmp*")
bundle(OPT_PREFIX, "lib/gdk-pixbuf-*/*/loaders/libpixbufloader-gif*")
bundle(OPT_PREFIX, "lib/gdk-pixbuf-*/*/loaders/libpixbufloader-tiff*")
### FIXME: mypaint brushes (needs patching https://github.com/Homebrew/homebrew-core/pull/262039)
bundle(OPT_PREFIX, "share/mypaint-data/2.0")
### Needed for fontconfig
bundle(OPT_PREFIX, "etc/fonts")
#### Avoid writing in the system and avoid other programs breaking the cache
fonts_conf = CLAY_DISTRIB / "etc/fonts/fonts.conf"
text = fonts_conf.read_text()
new_text = text.replace(
  f"{OPT_PREFIX}/var",
  f"~/Library/Application Support/Clay/{CLAY_APP_VERSION}"
)
fonts_conf.write_text(new_text)
### Needed for 'th' word breaking in Text tool etc
bundle(OPT_PREFIX, "share/libthai")
### Needed for full CJK and Cyrillic support in file-pdf
bundle(OPT_PREFIX, "share/poppler")
#### Needed for signature support in file-pdf lib
if os.path.exists(OPT_PREFIX / "bin/port"):
  bundle(OPT_PREFIX, "lib/nss/libssl3.dylib", "--dest", "lib")
  bundle(OPT_PREFIX, "lib/nss/libsmime3.dylib", "--dest", "lib")
  bundle(OPT_PREFIX, "lib/nss/libnssutil3.dylib", "--dest", "lib")
  bundle(OPT_PREFIX, "lib/nss/libnss3.dylib", "--dest", "lib")
  bundle(OPT_PREFIX, "lib/nspr/*.dylib", "--dest", "lib")
### Needed for file-ps work
if os.path.exists(OPT_PREFIX / "bin/port"):
  bundle(OPT_PREFIX, "share/ghostscript/1*/iccprofiles/*.icc", "--dest", "Resources/ghostscript/iccprofiles")
  bundle(OPT_PREFIX, "share/ghostscript/1*/Resource/Init/*", "--dest", "Resources/ghostscript/Resource/Init")
  bundle(OPT_PREFIX, "share/ghostscript/1*/Resource/Font/*", "--dest", "Resources/ghostscript/Resource/Font")
else: #os.path.exists(OPT_PREFIX / "bin/brew"):
  bundle(OPT_PREFIX, "share/ghostscript/iccprofiles/*.icc")
  bundle(OPT_PREFIX, "share/ghostscript/Resource/Init")
  bundle(OPT_PREFIX, "share/ghostscript/Resource/Font")
### Needed for file-wmf work
if os.path.exists(OPT_PREFIX / "bin/port"):
  bundle(OPT_PREFIX, "share/fonts/libwmf/*", "--dest", "Resources/libwmf/fonts")
else: #os.path.exists(OPT_PREFIX / "bin/brew"):
  bundle(OPT_PREFIX, "Cellar/libwmf/*/share/libwmf/fonts/*", "--dest", "Resources/libwmf/fonts")
### Needed for 'Show image graph'.
#### See: https://github.com/KurtStevenK/clay/-/issues/6045
bundle(OPT_PREFIX, "bin/dot", "--dest", "MacOS")
#### See: https://github.com/KurtStevenK/clay/-/issues/12119
bundle(OPT_PREFIX, "lib/graphviz/libgvplugin_dot*.dylib")
bundle(OPT_PREFIX, "lib/graphviz/libgvplugin_pango*.dylib")
bundle(OPT_PREFIX, "lib/graphviz/config*")
### Binaries for GObject Introspection support. See: https://github.com/KurtStevenK/clay/-/issues/13170
bundle(CLAY_PREFIX, "lib/girepository-*/*.typelib")
bundle(OPT_PREFIX, "lib/libgirepository-*.dylib")
#### Python support
bundle(OPT_PREFIX, f"bin/python{PYTHON_VERSION}", "--rename", "MacOS/python3")
if os.path.exists(OPT_PREFIX / "bin/brew") or (os.path.exists(OPT_PREFIX / "bin/port") and os.getenv('GITLAB_CI')):
  bundle(OPT_PREFIX, f"Frameworks/Python.framework/Versions/{PYTHON_VERSION}", "--dest", "lib/Python.framework/Versions")
elif os.path.exists(OPT_PREFIX / "bin/port"):
  bundle(OPT_PREFIX, f"Library/Frameworks/Python.framework/Versions/{PYTHON_VERSION}", "--dest", "lib/Python.framework/Versions")
bundle(OPT_PREFIX, f"lib/python{PYTHON_VERSION}/site-packages/*", "--dest", f"lib/Python.framework/Versions/{PYTHON_VERSION}/lib/python{PYTHON_VERSION}/site-packages")
clean(CLAY_DISTRIB, "lib/Python.framework/*.pyc")
#####Needed for internet connection on python. See: https://github.com/KurtStevenK/clay/-/issues/14722
pythonpath = Path(f"{CLAY_DISTRIB}/lib/Python.framework/Versions/{PYTHON_VERSION}/lib/python{PYTHON_VERSION}")
for d in (pythonpath, pythonpath / "site-packages"):
  sitecustomize = d / "sitecustomize.py"
  ## Homebrew's Python has no certifi
  code = """\nimport os\ntry:\n    import certifi\n    # Only set if not already configured by user\n    if not os.environ.get('SSL_CERT_FILE'):\n        os.environ['SSL_CERT_FILE'] = certifi.where()\nexcept (ImportError, AttributeError):\n    pass\n"""
  if sitecustomize.exists() and code.strip() not in sitecustomize.read_text():
    sitecustomize.write_text(sitecustomize.read_text() + code)
  elif not sitecustomize.exists():
    sitecustomize.write_text(code)
#####Needed since we use [[NSBundle mainBundle] bundlePath] on libclaybase/clayenv.c
real_path = Path(f"{CLAY_DISTRIB}/share")
link_path = Path(f"{CLAY_DISTRIB}/lib/Python.framework/Versions/{PYTHON_VERSION}/Resources/Python.app/Contents/share")
link_path.unlink(missing_ok=True)
link_path.parent.mkdir(parents=True, exist_ok=True)
link_path.symlink_to(os.path.relpath(real_path, link_path.parent))
#### lua is buggy, and hard to bundle due to LUA_*PATH etc (see AppImage script)
#if os.path.exists(OPT_PREFIX / "bin/port"):
  #bundle(OPT_PREFIX, "bin/luajit", "--dest", "MacOS")
  #bundle(OPT_PREFIX, "lib/lua")
  #bundle(OPT_PREFIX, "share/lua")


## MAIN EXECUTABLES AND DEPENDENCIES
### Minimal (and some additional) executables for the 'MacOS' folder
bundle(CLAY_PREFIX, "bin/clay*")
### Script-Fu scripts call the interpreter by its Clay name in their shebang
bundle(CLAY_PREFIX, "bin/clay-script-fu-interpreter*")
if os.path.exists(OPT_PREFIX / "bin/brew"):
  bundle(OPT_PREFIX, "Cellar/libarchive/*/lib/libarchive.*.dylib", "--dest", "lib")
### Bundled just to promote GEGL. See: https://github.com/KurtStevenK/clay/-/issues/10580
bundle(CLAY_PREFIX, "bin/gegl")
### Deps (DYLIBs) of the binaries in 'MacOS' and 'lib' dirs
### We save the list of already copied DLLs to keep a state between 2_bundle-clay-uni_dep runs.
done_dylib = Path(f"{BUILD_DIR}/done-dylib.list")
done_dylib.unlink(missing_ok=True)
done_symbols = Path(f"{BUILD_DIR}/done-symbols.list")
done_symbols.unlink(missing_ok=True)
binaries_to_dsym = []
for dir in ["MacOS", "lib"]:
  search_dir = CLAY_DISTRIB / dir
  print(f"Searching for dependencies of {search_dir} in {CLAY_PREFIX} and {OPT_PREFIX}")
  for root, dirs, files in os.walk(search_dir):
    dirs[:] = [d for d in dirs if ".dSYM" not in d]
    for file in files:
      dep = Path(os.path.join(root, file))
      if ".dSYM" not in str(dep) and not dep.is_symlink() and open(dep, "rb").read(4) in (b'\xfe\xed\xfa\xce', b'\xce\xfa\xed\xfe', b'\xfe\xed\xfa\xcf', b'\xcf\xfa\xed\xfe', b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe\xca', b'\xca\xfe\xba\xbf', b'\xbf\xba\xfe\xca'):
        subprocess.run([
          sys.executable, f"{CLAY_SOURCE}/tools/lib_bundle.py",
          str(dep), f"{CLAY_PREFIX}/", f"{OPT_PREFIX}/",
          str(CLAY_DISTRIB), "--output-dll-list", done_dylib,
          "--output-dylib-symbols-list", done_symbols
        ], check=True)
        binaries_to_dsym.append(dep)


## .DSYM/DWARF DEBUG SYMBOLS (from babl, gegl and Clay binaries)
for line in done_dylib.read_text().splitlines():
  if line.strip():
    bundled_dep = CLAY_DISTRIB / "lib" / line.strip()
    if bundled_dep not in binaries_to_dsym and bundled_dep.exists():
      binaries_to_dsym.append(bundled_dep)
for binary in binaries_to_dsym:
  result = subprocess.run(["dsymutil", "--no-output", binary], capture_output=True, text=True)
  if not "no debug symbols" in result.stdout + result.stderr and not "unable to open object file" in result.stdout + result.stderr:
    print(f"(INFO): generating debug symbols file as {binary}.dSYM")
    try:
      subprocess.run(["dsymutil", binary, "-o", f"{binary}.dSYM"], check=True, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError as e:
      sys.stderr.write(f"Failed to generate debug symbols from {binary}: {e}\n")
  elif "unable to open object file" in result.stdout + result.stderr and not str(OPT_PREFIX) in result.stdout + result.stderr and "Python.framework" not in str(binary):
    print(f"\n\033[31m(ERROR)\033[0m: {binary} is orphaned from .o file for .dSYM generation. Please make sure its build dir is present")
    sys.exit(1)


## DEVELOPMENT FILES ON UNIX-STYLE, NO MACOS-STYLE/.FRAMEWORK (see https://developer.gimp.org/resource/sdk/)
### (to build GEGL filters and Clay plug-ins).
clean(CLAY_DISTRIB, "lib/*.a")
bundle(CLAY_PREFIX, "include/clay-*", "--dest", "include")
bundle(CLAY_PREFIX, "include/babl-*", "--dest", "include")
bundle(CLAY_PREFIX, "include/gegl-*", "--dest", "include")
bundle(CLAY_PREFIX, "lib/libclay*-?.?.dylib", "--preserve-symlink")
bundle(CLAY_PREFIX, "lib/libbabl-?.?.dylib", "--preserve-symlink")
bundle(CLAY_PREFIX, "lib/libgegl-?.?.dylib", "--preserve-symlink")
bundle(CLAY_PREFIX, "lib/pkgconfig/clay*")
bundle(CLAY_PREFIX, "lib/pkgconfig/babl*")
bundle(CLAY_PREFIX, "lib/pkgconfig/gegl*")
### Files needed by babl, gegl and clay development libraries (not core clay app)
processed_pcs = set()
pcs_to_process = [f.stem for f in Path(CLAY_DISTRIB / "lib/pkgconfig").glob("*.pc")]
while pcs_to_process:
  current_pc = pcs_to_process.pop(0)
  if current_pc in processed_pcs:
    continue
  requires = subprocess.run(["pkg-config", "--print-requires", "--print-requires-private", current_pc], capture_output=True, text=True)
  if requires.returncode == 0:
    deps = [line.split()[0] for line in requires.stdout.splitlines() if line.strip()]
    for dep in deps:
      if dep not in processed_pcs:

        #1.Bundle the pkgconfig file (.pc)
        pc_path = f"lib/pkgconfig/{dep}.pc"
        # Homebrew keeps header-only .pc files (e.g. xproto) in share/pkgconfig
        src_pc_path = pc_path if (OPT_PREFIX / pc_path).exists() else f"share/pkgconfig/{dep}.pc"
        if (OPT_PREFIX / src_pc_path).exists() and not (CLAY_DISTRIB / pc_path).exists():
          bundle(OPT_PREFIX, src_pc_path, "--dest", "lib/pkgconfig")
          # Replace the hardcoded prefix with a relocatable one
          dest_pc = CLAY_DISTRIB / pc_path
          os.chmod(dest_pc, dest_pc.stat().st_mode | stat.S_IWUSR)
          pc_content = dest_pc.read_text()
          # Homebrew .pc files point into Cellar/<name>/<version> or opt/<name>, but the bundle is flat
          pc_content = re.sub(rf'{re.escape(str(OPT_PREFIX))}/(?:Cellar/[^/\s]+/[^/\s]+|opt/[^/\s]+)(?=[/\s]|$)', "${pcfiledir}/../..", pc_content, flags=re.MULTILINE)
          new_pc_content = pc_content.replace(str(OPT_PREFIX), "${pcfiledir}/../..")
          dest_pc.write_text(new_pc_content)

          #2.Bundle the corresponding library (unversioned .dylib only, since static .a not available on all MacPorts)
          libs = subprocess.run(["pkg-config", "--libs-only-l", dep], capture_output=True, text=True)
          if libs.returncode == 0:
            for arg in libs.stdout.split():
              if arg.startswith("-l"):
                lib_name = f"lib/lib{arg[2:]}.dylib"
                if os.path.lexists(OPT_PREFIX / lib_name) and not os.path.lexists(CLAY_DISTRIB / lib_name):
                  # Resolve fully: Homebrew links point into Cellar, which is not bundled
                  versioned_lib = os.path.basename(os.path.realpath(OPT_PREFIX / lib_name))
                  if (CLAY_DISTRIB / "lib" / versioned_lib).exists():
                    print(f"Bundling {OPT_PREFIX / lib_name} to {CLAY_DISTRIB / 'lib'}")
                    (CLAY_DISTRIB / lib_name).symlink_to(versioned_lib)

          #3.Bundle the headers (.h)
          includedir = subprocess.run(["pkg-config", "--cflags-only-I", dep], capture_output=True, text=True)
          if includedir.returncode == 0:
            for arg in includedir.stdout.split():
              if arg.startswith("-I"):
                include_path = Path(arg[2:])
                if str(include_path).startswith(str(OPT_PREFIX)) and include_path.name != "include" and not (CLAY_DISTRIB / include_path.relative_to(OPT_PREFIX)).exists():
                  bundle(OPT_PREFIX, str(include_path.relative_to(OPT_PREFIX)), "--dest", "include")
          #Fallback: try dep/
          h_dir = f"include/{dep}"
          if (OPT_PREFIX / h_dir).exists() and not (CLAY_DISTRIB / h_dir).exists():
            bundle(OPT_PREFIX, h_dir, "--dest", "include")
          #Fallback: try dep/ without lib prefix nor -version suffix
          real_dep = re.sub(r'[-._\d]+$', '', re.sub(r'^lib', '', dep))
          h_dir = f"include/{real_dep}"
          if real_dep != dep and (OPT_PREFIX / h_dir).exists() and not (CLAY_DISTRIB / h_dir).exists():
            bundle(OPT_PREFIX, h_dir, "--dest", "include")
          #Fallback: try dep*.h
          for h_file in (OPT_PREFIX / "include").glob(f"{dep}*.h"):
            if not (CLAY_DISTRIB / f"include/{h_file.name}").exists():
              bundle(OPT_PREFIX, f"include/{h_file.name}", "--dest", "include")
          #Fallback: try dep*.h without lib prefix nor -version suffix
          for h_file in (OPT_PREFIX / "include").glob(f"{real_dep}*.h"):
            if not (CLAY_DISTRIB / f"include/{h_file.name}").exists():
              bundle(OPT_PREFIX, f"include/{h_file.name}", "--dest", "include")

          pcs_to_process.append(dep)
  processed_pcs.add(current_pc)
### Special-case
link_path = Path(f"{CLAY_DISTRIB}/lib/libz.dylib")
link_path.symlink_to(os.path.relpath(Path(f"{CLAY_DISTRIB}/lib/libz.1.dylib"), link_path.parent))
link_path = Path(f"{CLAY_DISTRIB}/lib/libzstd.dylib")
link_path.symlink_to(os.path.relpath(Path(f"{CLAY_DISTRIB}/lib/libzstd.1.dylib"), link_path.parent))
bundle(OPT_PREFIX, "lib/glib-2.0", "--dest", "lib")
bundle(OPT_PREFIX, "include/brotli", "--dest", "include")
#### Some ports hardcode their prefix on #include (e.g. gi18n.h -> MacPorts libintl.h)
prefixed_include = re.compile(rf'(#\s*include\s*[<"]){re.escape(str(OPT_PREFIX))}/include/([^>"]+)')
for h_file in (CLAY_DISTRIB / "include").rglob("*.h"):
  h_content = h_file.read_text(errors="surrogateescape")
  for h_name in set(m.group(2) for m in prefixed_include.finditer(h_content)):
    if not (CLAY_DISTRIB / f"include/{h_name}").exists():
      bundle(OPT_PREFIX, f"include/{h_name}", "--dest", str(Path("include") / Path(h_name).parent))
  if prefixed_include.search(h_content):
    h_file.write_text(prefixed_include.sub(r'\1\2', h_content), errors="surrogateescape")

### Make the Clay SDK .pc files relocatable (Clay's own .pc files still carry the build prefix)
for pc_file in (CLAY_DISTRIB / "lib/pkgconfig").glob("*.pc"):
  pc_text = pc_file.read_text(errors="ignore")
  if str(CLAY_PREFIX) in pc_text:
    os.chmod(pc_file, pc_file.stat().st_mode | stat.S_IWUSR)
    pc_file.write_text(pc_text.replace(str(CLAY_PREFIX), "${pcfiledir}/../.."))

### Test if all bundled .pc, libs and headers are fine
print(f"Testing if Clay SDK works")
####First we check if any hardcoded .pc or header would be passed to claytool
build_prefixes = [str(prefix).lower() for prefix in (OPT_PREFIX, CLAY_PREFIX)]
abs_include = re.compile(r'^\s*#\s*include\s*[<"]/', re.MULTILINE)
for sdk_file in list((CLAY_DISTRIB / "include").rglob("*")) + list((CLAY_DISTRIB / "lib/pkgconfig").glob("*.pc")):
  if sdk_file.is_file():
    sdk_content = sdk_file.read_text(errors="ignore").lower()
    if any(prefix in sdk_content for prefix in build_prefixes) or abs_include.search(sdk_content):
      print(f"\033[31m(ERROR)\033[0m: Clay SDK file {sdk_file.relative_to(CLAY_DISTRIB)} hardcodes {OPT_PREFIX} or {CLAY_PREFIX}")
      sys.exit(1)
####Then we check if any hardcoding is still passed to claytool at runtime
clean_tests_env = os.environ.copy()
clean_tests_env.pop("PKG_CONFIG_PATH", None)
clean_tests_path = Path(BUILD_DIR) / "tools/claytool-tests"
os.chdir(clean_tests_path)
claysdk_tests = {
  "--build-noui"   : "c-hello-world",
  "--build"        : "c-hello-world-ui",
  "--build-geglop" : "clay-tutorial-meta-op.dylib",
}
for build_option, test_binary in claysdk_tests.items():
  test_source = os.path.splitext(test_binary)[0] + ".c"
  result = subprocess.run([CLAY_DISTRIB / "MacOS/claytool", build_option, clean_tests_path / test_source], env=clean_tests_env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
  flags_output = result.stdout + result.stderr
  opt_prefix = os.getenv('OPT_PREFIX', "none")
  leaked_flags = [
      f"-I{opt_prefix}", f"-L{opt_prefix}"
    ]
  if result.returncode != 0 or not (clean_tests_path / test_binary).exists():
    print(f"\033[31m(ERROR)\033[0m: Clay SDK test failed at runtime for {test_source}. Output:\n{flags_output}")
    sys.exit(1)
  elif any(flag in flags_output for flag in leaked_flags):
    print(f"\033[31m(ERROR)\033[0m: Clay SDK test failed under the hood for {test_source} (the SDK is leaking to MacPorts or Homebrew). Output:\n{flags_output}")
    sys.exit(1)
os.chdir(Path(BUILD_DIR))
