#!/usr/bin/env python3

# Test for the existence of constants that are part of our public API and
# thus should be available using gobject introspection.

# 1. Defined in libclaybase

# from libclaybase/claychecks.h

clay_assert('Clay.CHECK_SIZE',           hasattr(Clay, "CHECK_SIZE"))
clay_assert('Clay.CHECK_SIZE_SM',        hasattr(Clay, "CHECK_SIZE_SM"))
clay_assert('Clay.CHECK_DARK',           hasattr(Clay, "CHECK_DARK"))
clay_assert('Clay.CHECK_LIGHT',          hasattr(Clay, "CHECK_LIGHT"))


# from libclaybase/claylimits.h

clay_assert('Clay.MIN_IMAGE_SIZE',       hasattr(Clay, "MIN_IMAGE_SIZE"))
clay_assert('Clay.MAX_IMAGE_SIZE',       hasattr(Clay, "MAX_IMAGE_SIZE"))
clay_assert('Clay.MIN_RESOLUTION',       hasattr(Clay, "MIN_RESOLUTION"))
clay_assert('Clay.MAX_RESOLUTION',       hasattr(Clay, "MAX_RESOLUTION"))
clay_assert('Clay.MAX_MEMSIZE',          hasattr(Clay, "MAX_MEMSIZE"))


# from libclaybase/clayparamspecs.h

clay_assert('Clay.PARAM_NO_VALIDATE',    hasattr(Clay, "PARAM_NO_VALIDATE"))
clay_assert('Clay.PARAM_DONT_SERIALIZE', hasattr(Clay, "PARAM_DONT_SERIALIZE"))
clay_assert('Clay.PARAM_FLAG_SHIFT',     hasattr(Clay, "PARAM_FLAG_SHIFT"))
clay_assert('Clay.PARAM_STATIC_STRINGS', hasattr(Clay, "PARAM_STATIC_STRINGS"))
clay_assert('Clay.PARAM_READABLE',       hasattr(Clay, "PARAM_READABLE"))
clay_assert('Clay.PARAM_WRITABLE',       hasattr(Clay, "PARAM_WRITABLE"))
clay_assert('Clay.PARAM_READWRITE',      hasattr(Clay, "PARAM_READWRITE"))


# from libclaybase/clayparasite.h

clay_assert('Clay.PARASITE_PERSISTENT',             hasattr(Clay, "PARASITE_PERSISTENT"))
clay_assert('Clay.PARASITE_UNDOABLE',               hasattr(Clay, "PARASITE_UNDOABLE"))
clay_assert('Clay.PARASITE_ATTACH_PARENT',          hasattr(Clay, "PARASITE_ATTACH_PARENT"))
clay_assert('Clay.PARASITE_PARENT_PERSISTENT',      hasattr(Clay, "PARASITE_PARENT_PERSISTENT"))
clay_assert('Clay.PARASITE_PARENT_UNDOABLE',        hasattr(Clay, "PARASITE_PARENT_UNDOABLE"))
clay_assert('Clay.PARASITE_ATTACH_GRANDPARENT',     hasattr(Clay, "PARASITE_ATTACH_GRANDPARENT"))
clay_assert('Clay.PARASITE_GRANDPARENT_PERSISTENT', hasattr(Clay, "PARASITE_GRANDPARENT_PERSISTENT"))
clay_assert('Clay.PARASITE_GRANDPARENT_UNDOABLE',   hasattr(Clay, "PARASITE_GRANDPARENT_UNDOABLE"))


# from libclaybase/clayparasiteio.h
# Not available from C plug-ins, should be considered private
clay_assert('Clay.PIXPIPE_MAXDIM',            hasattr(Clay, "PIXPIPE_MAXDIM"))


# from libclaybase/clayversion.h.in

clay_assert('Clay.MAJOR_VERSION',             hasattr(Clay, "MAJOR_VERSION"))
clay_assert('Clay.MINOR_VERSION',             hasattr(Clay, "MINOR_VERSION"))
clay_assert('Clay.MICRO_VERSION',             hasattr(Clay, "MICRO_VERSION"))
clay_assert('Clay.VERSION',                   hasattr(Clay, "VERSION"))
clay_assert('Clay.API_VERSION',               hasattr(Clay, "API_VERSION"))


# 2. Defined in libclayconfig

# from libclayconfig/clayconfig-params.h

clay_assert('Clay.CONFIG_PARAM_SERIALIZE',    hasattr(Clay, "CONFIG_PARAM_SERIALIZE"))
clay_assert('Clay.CONFIG_PARAM_AGGREGATE',    hasattr(Clay, "CONFIG_PARAM_AGGREGATE"))
clay_assert('Clay.CONFIG_PARAM_RESTART',      hasattr(Clay, "CONFIG_PARAM_RESTART"))
clay_assert('Clay.CONFIG_PARAM_CONFIRM',      hasattr(Clay, "CONFIG_PARAM_CONFIRM"))
clay_assert('Clay.CONFIG_PARAM_DEFAULTS',     hasattr(Clay, "CONFIG_PARAM_DEFAULTS"))
clay_assert('Clay.CONFIG_PARAM_IGNORE',       hasattr(Clay, "CONFIG_PARAM_IGNORE"))
clay_assert('Clay.CONFIG_PARAM_DONT_COMPARE', hasattr(Clay, "CONFIG_PARAM_DONT_COMPARE"))
clay_assert('Clay.CONFIG_PARAM_FLAG_SHIFT',   hasattr(Clay, "CONFIG_PARAM_FLAG_SHIFT"))
clay_assert('Clay.CONFIG_PARAM_FLAGS',        hasattr(Clay, "CONFIG_PARAM_FLAGS"))


# 3. Defined in libclaymodule

# from libclaymodule/claymodule.h

clay_assert('Clay.MODULE_ABI_VERSION',        hasattr(Clay, "MODULE_ABI_VERSION"))
