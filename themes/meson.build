subdir('Default')
subdir('Clay')
subdir('Cappuccino')
subdir('Espresso')
subdir('Midnight')
subdir('Forest')
subdir('Rose')
subdir('Sunset')
subdir('Violet')
subdir('Lane')
subdir('Neon')
subdir('Matrix')
subdir('Ubuntu')
subdir('Windows')
subdir('macOS')
subdir('Dracula')
subdir('Nord')
subdir('Solarized')
subdir('Gruvbox')
subdir('TokyoNight')
subdir('Monokai')
subdir('Catppuccin')
subdir('Synthwave')
subdir('Cyberpunk')
subdir('Ocean')
subdir('Mint')
subdir('Lavender')
subdir('Autumn')
subdir('Sakura')
subdir('Paper')
subdir('Amber')
subdir('Slate')
subdir('Coral')
subdir('Graphite')
subdir('Sepia')
subdir('System')

themes = [
  'System',
]

foreach theme : themes
  install_data(
    theme / 'clay.css',
    install_dir: claydatadir / 'themes' / theme,
  )
endforeach

# Colour schemes shared by all themes (see app/gui/themes.c)
install_data(
  files(
    'schemes/brown.css',
    'schemes/parchment.css',
    'schemes/cappuccino.css',
    'schemes/espresso.css',
    'schemes/midnight.css',
    'schemes/forest.css',
    'schemes/rose.css',
    'schemes/sunset.css',
    'schemes/violet.css',
    'schemes/neon.css',
    'schemes/matrix.css',
    'schemes/ubuntu.css',
    'schemes/dracula.css',
    'schemes/nord.css',
    'schemes/solarized.css',
    'schemes/gruvbox.css',
    'schemes/ocean.css',
    'schemes/sakura.css',
    'schemes/amber.css',
    'schemes/sepia.css',
  ),
  install_dir: claydatadir / 'theme-schemes',
)
