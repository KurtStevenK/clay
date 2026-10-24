import sys
sys.path.insert(0, r"T:\Apps\clay\tools\clay-mcp")
from _rpc import clay

code = r"""
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gio, GLib

info = {'wins': []}
for w in Gtk.Window.list_toplevels():
    title = w.get_title() if hasattr(w, 'get_title') else None
    acts = []
    try:
        if hasattr(w, 'list_action_names'):
            acts = [a for a in w.list_action_names() if 'zoom' in a]
    except Exception:
        pass
    groups = []
    try:
        # GtkApplicationWindow
        if hasattr(w, 'get_action_group'):
            for prefix in ('win', 'view', 'app', 'image'):
                g = w.get_action_group(prefix)
                if g:
                    groups.append((prefix, list(g.list_actions())[:20] if hasattr(g, 'list_actions') else str(g)))
    except Exception as e:
        groups.append(('err', str(e)))
    info['wins'].append({'title': title, 'type': type(w).__name__, 'zoom_acts': acts, 'groups': groups})

# Try activate
err = None
for w in Gtk.Window.list_toplevels():
    for name in ('view-zoom-1-1', 'zoom-1-1', 'win.view-zoom-1-1'):
        try:
            w.activate_action(name, None)
            err = 'activated ' + name
            break
        except Exception as e:
            err = str(e)
    if err and err.startswith('activated'):
        break
info['activate'] = err
raise RuntimeError(repr(info)[:4000])
"""
try:
    print(clay("execute_python", {"code": code}))
except RuntimeError as e:
    print(e)
