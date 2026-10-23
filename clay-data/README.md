# GNU Image Manipulation Program: data files

This repository serves to contain data files which are released as part of the
GNU Image Manipulation Program. It has no purpose on its own and is not meant to
be a separate package either. Clay will not even work at all without this data
(it might start but without icons, with broken theming, with no brushes, no
patterns, nothing which makes Clay actually unusable).

You usually don't need to clone this repository. You may just bootstrap
the main [Clay repository](https://github.com/KurtStevenK/clay/) with
`git submodule update --init` to automatically clone `clay-data` as a
git submodule.

Do not confuse the current repository with
[clay-data-extras](https://github.com/KurtStevenK/clay) which is
indeed meant to be a separate package and is absolutely not needed by Clay.

The main reason to make a separate repository is that many data are binary
files, especially image data, and therefore the size of the source repository is
growing fast each time we update a data file. By making this a separate
repository, it is easy to clone `clay-data` as a shallow repository, hence
avoiding unnecessary disk-space usage. While for source files, developers
absolutely need the whole history, it is much less true for data files.

Clay source repository: https://github.com/KurtStevenK/clay/
