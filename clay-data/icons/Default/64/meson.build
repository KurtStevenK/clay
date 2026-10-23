icons_dir = size + 'x' + size

if have_vector_icons
  icon_dialog_question = custom_target('icon-dialog-question',
    input : [ '../scalable/dialog-question.svg' ],
    output: [ 'dialog-question.png' ],
    command: [
      colorsvg2png, '@INPUT@', '@OUTPUT@', size,
    ],
    install: false,
  )
  icon_wilber_eek = custom_target('icon-wilber-eek',
    input : [ '../scalable/clay-wilber-eek.svg' ],
    output: [ 'clay-wilber-eek.png' ],
    command: [
      colorsvg2png, '@INPUT@', '@OUTPUT@', size,
    ],
    install: false,
  )
else
  foreach icon : source_icons
    # Install the color variant.
    icon_name = icon + '.png'
    outdir    = clayiconsdir / theme / icons_dir / 'apps'
    if icon == 'clay-wilber'
      input_path = meson.project_source_root() / 'clay-data/images/logo/clay-logo-shadow.svg'
    else
      input_path = '../scalable/' + icon + '.svg'
    endif

    icon_tgt = custom_target(icon_name,
                             input : [ input_path ],
                             output: [ icon_name ],
                             command: [
                               colorsvg2png, '@INPUT@', '@OUTPUT@', size,
                             ],
                             build_by_default: true,
                             install: true,
                             install_dir: outdir)

    if icon == 'dialog-question'
      icon_dialog_question = icon_tgt
    elif icon == 'clay-wilber-eek'
      icon_wilber_eek = icon_tgt
    endif

    # Install the symbolic variant.
    icon_name = icon + '-symbolic.symbolic.png'
    outdir    = clayiconsdir / theme / icons_dir / 'apps'
    if icon == 'clay-wilber'
      input_path = meson.project_source_root() / 'clay-data/images/logo/clay-wilber-symbolic.svg'
    else
      input_path = '../scalable/' + icon + '-symbolic.svg'
    endif

    custom_target(icon_name,
                  input : [ input_path ],
                  output: [ icon_name ],
                  command: [
                    gtk_encode_symbolic_svg, '-o', '@OUTDIR@', '@INPUT@', icons_dir
                  ],
                  build_by_default: true,
                  install: true,
                  install_dir: outdir)
  endforeach
endif
