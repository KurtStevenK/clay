#.env files read by clay_environ_table_load_env_file in clayenvirontable.c


install_data(
  'default.env',
  install_dir: clayplugindir / 'environ',
)


if relocatable_bundle or platform_windows and not meson.is_cross_build()
  pyclay_config = configuration_data()
  if relocatable_bundle
    pyclay_config.set('DONTWRITEBYTECODE_SETTING', 'PYTHONDONTWRITEBYTECODE=1')
  else
    pyclay_config.set('DONTWRITEBYTECODE_SETTING', '')
  endif
  if platform_windows 
    pyclay_config.set('HIGHDPI_SETTING', '__COMPAT_LAYER=HIGHDPIAWARE')
  else
    pyclay_config.set('HIGHDPI_SETTING', '')
  endif

  configure_file(
    input : 'pyclay.env.in',
    output: 'pyclay.env',
    configuration: pyclay_config,
    install: true,
    install_dir: clayplugindir / 'environ',
  )
endif
