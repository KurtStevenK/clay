/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * claydashboard.c
 * Copyright (C) 2017 Ell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <gegl.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <windows.h>
#include <psapi.h>
#define HAVE_CPU_GROUP
#define HAVE_MEMORY_GROUP
#elif defined(PLATFORM_OSX)
#include <mach/mach.h>
#include <sys/times.h>
#define HAVE_CPU_GROUP
#define HAVE_MEMORY_GROUP
#else /* ! G_OS_WIN32 && ! PLATFORM_OSX */
#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#define HAVE_CPU_GROUP
#endif /* HAVE_SYS_TIMES_H */
#if defined (HAVE_UNISTD_H) && defined (HAVE_FCNTL_H)
#include <unistd.h>
#include <fcntl.h>
#ifdef _SC_PAGE_SIZE
#define HAVE_MEMORY_GROUP
#endif /* _SC_PAGE_SIZE */
#endif /* HAVE_UNISTD_H && HAVE_FCNTL_H */
#endif /* ! G_OS_WIN32 */

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/clay-gui.h"
#include "core/clay-utils.h"
#include "core/clay-parallel.h"
#include "core/clayasync.h"
#include "core/claybacktrace.h"
#include "core/claytempbuf.h"
#include "core/claywaitable.h"

#include "clayactiongroup.h"
#include "claydocked.h"
#include "claydashboard.h"
#include "claydialogfactory.h"
#include "clayhelp-ids.h"
#include "claymeter.h"
#include "claysessioninfo-aux.h"
#include "claytoggleaction.h"
#include "clayuimanager.h"
#include "claywidgets-utils.h"
#include "claywindowstrategy.h"

#include "clay-intl.h"
#include "clay-log.h"
#include "clay-version.h"


#define DEFAULT_UPDATE_INTERVAL        CLAY_DASHBOARD_UPDATE_INTERVAL_0_25_SEC
#define DEFAULT_HISTORY_DURATION       CLAY_DASHBOARD_HISTORY_DURATION_60_SEC
#define DEFAULT_LOW_SWAP_SPACE_WARNING TRUE

#define LOW_SWAP_SPACE_WARNING_ON      /* swap occupied is above */ 0.90 /* of swap limit */
#define LOW_SWAP_SPACE_WARNING_OFF     /* swap occupied is below */ 0.85 /* of swap limit */

#define CPU_ACTIVE_ON                  /* individual cpu usage is above */ 0.75
#define CPU_ACTIVE_OFF                 /* individual cpu usage is below */ 0.25

#define LOG_VERSION                    1
#define LOG_SAMPLE_FREQUENCY_MIN       1 /* samples per second */
#define LOG_SAMPLE_FREQUENCY_MAX       1000 /* samples per second */
#define LOG_DEFAULT_SAMPLE_FREQUENCY   10 /* samples per second */
#define LOG_DEFAULT_BACKTRACE          TRUE
#define LOG_DEFAULT_MESSAGES           TRUE
#define LOG_DEFAULT_PROGRESSIVE        FALSE


typedef enum
{
  VARIABLE_NONE,
  FIRST_VARIABLE,


  /* cache */
  VARIABLE_CACHE_OCCUPIED = FIRST_VARIABLE,
  VARIABLE_CACHE_MAXIMUM,
  VARIABLE_CACHE_LIMIT,

  VARIABLE_CACHE_COMPRESSION,
  VARIABLE_CACHE_HIT_MISS,

  /* swap */
  VARIABLE_SWAP_OCCUPIED,
  VARIABLE_SWAP_SIZE,
  VARIABLE_SWAP_LIMIT,

  VARIABLE_SWAP_QUEUED,
  VARIABLE_SWAP_QUEUE_STALLS,
  VARIABLE_SWAP_QUEUE_FULL,

  VARIABLE_SWAP_READ,
  VARIABLE_SWAP_READ_THROUGHPUT,
  VARIABLE_SWAP_WRITTEN,
  VARIABLE_SWAP_WRITE_THROUGHPUT,

  VARIABLE_SWAP_COMPRESSION,

#ifdef HAVE_CPU_GROUP
  /* cpu */
  VARIABLE_CPU_USAGE,
  VARIABLE_CPU_ACTIVE,
  VARIABLE_CPU_ACTIVE_TIME,
#endif

#ifdef HAVE_MEMORY_GROUP
  /* memory */
  VARIABLE_MEMORY_USED,
  VARIABLE_MEMORY_AVAILABLE,
  VARIABLE_MEMORY_SIZE,
#endif

  /* misc */
  VARIABLE_MIPMAPED,
  VARIABLE_ASSIGNED_THREADS,
  VARIABLE_ACTIVE_THREADS,
  VARIABLE_ASYNC_RUNNING,
  VARIABLE_TILE_ALLOC_TOTAL,
  VARIABLE_SCRATCH_TOTAL,
  VARIABLE_TEMP_BUF_TOTAL,


  N_VARIABLES,

  VARIABLE_SEPARATOR
} Variable;

typedef enum
{
  VARIABLE_TYPE_BOOLEAN,
  VARIABLE_TYPE_INTEGER,
  VARIABLE_TYPE_SIZE,
  VARIABLE_TYPE_SIZE_RATIO,
  VARIABLE_TYPE_INT_RATIO,
  VARIABLE_TYPE_PERCENTAGE,
  VARIABLE_TYPE_DURATION,
  VARIABLE_TYPE_RATE_OF_CHANGE
} VariableType;

typedef enum
{
  FIRST_GROUP,

  GROUP_CACHE = FIRST_GROUP,
  GROUP_SWAP,
#ifdef HAVE_CPU_GROUP
  GROUP_CPU,
#endif
#ifdef HAVE_MEMORY_GROUP
  GROUP_MEMORY,
#endif
  GROUP_MISC,

  N_GROUPS
} Group;


typedef struct _VariableInfo  VariableInfo;
typedef struct _FieldInfo     FieldInfo;
typedef struct _GroupInfo     GroupInfo;
typedef struct _VariableData  VariableData;
typedef struct _FieldData     FieldData;
typedef struct _GroupData     GroupData;

typedef void (* VariableFunc) (ClayDashboard *dashboard,
                               Variable       variable);


struct _VariableInfo
{
  const gchar   *name;
  const gchar   *title;
  const gchar   *description;
  VariableType   type;
  gboolean       exclude_from_log;
  gdouble        rgb[4];
  VariableFunc   sample_func;
  VariableFunc   reset_func;
  gconstpointer  data;
};

struct _FieldInfo
{
  Variable     variable;
  const gchar *title;
  gboolean     default_active;
  gboolean     show_in_header;
  Variable     meter_variable;
  gint         meter_value;
  gboolean     meter_cumulative;
};

struct _GroupInfo
{
  const gchar     *name;
  const gchar     *title;
  const gchar     *description;
  gboolean         default_active;
  gboolean         default_expanded;
  gboolean         has_meter;
  Variable         meter_limit;
  const Variable  *meter_led;
  const FieldInfo *fields;
};

struct _VariableData
{
  gboolean available;

  union
  {
    gboolean  boolean;
    gint      integer;
    guint64   size;           /* in bytes                   */
    struct
    {
      guint64 antecedent;
      guint64 consequent;
    } size_ratio;
    struct
    {
      gint    antecedent;
      gint    consequent;
    } int_ratio;
    gdouble   percentage;     /* from 0 to 1                */
    gdouble   duration;       /* in seconds                 */
    gdouble   rate_of_change; /* in source units per second */
  } value;

  gpointer data;
  gsize    data_size;
};

struct _FieldData
{
  gboolean          active;

  GtkCheckMenuItem *menu_item;
  GtkLabel         *value_label;
};

struct _GroupData
{
  gint              n_fields;
  gint              n_meter_values;

  gboolean          active;
  gdouble           limit;

  ClayToggleAction *action;
  GtkExpander      *expander;
  GtkLabel         *header_values_label;
  GtkButton        *menu_button;
  GtkMenu          *menu;
  ClayMeter        *meter;
  GtkGrid          *grid;

  FieldData        *fields;
};

struct _ClayDashboardPrivate
{
  Clay                         *clay;

  VariableData                  variables[N_VARIABLES];
  GroupData                     groups[N_GROUPS];

  GThread                      *thread;
  GMutex                        mutex;
  GCond                         cond;
  gboolean                      quit;
  gboolean                      update_now;

  gint                          update_idle_id;
  gint                          low_swap_space_idle_id;

  ClayDashboardUpdateInteval    update_interval;
  ClayDashboardHistoryDuration  history_duration;
  gboolean                      low_swap_space_warning;

  GOutputStream                *log_output;
  GError                       *log_error;
  ClayDashboardLogParams        log_params;
  gint64                        log_start_time;
  gint                          log_n_samples;
  gint                          log_n_markers;
  VariableData                  log_variables[N_VARIABLES];
  ClayBacktrace                *log_backtrace;
  GHashTable                   *log_addresses;
  ClayLogHandler                log_log_handler;

  GtkWidget                    *log_record_button;
  GtkLabel                     *log_add_marker_label;
};


/*  local function prototypes  */

static void       clay_dashboard_docked_iface_init              (ClayDockedInterface *iface);

static void       clay_dashboard_constructed                    (GObject             *object);
static void       clay_dashboard_dispose                        (GObject             *object);
static void       clay_dashboard_finalize                       (GObject             *object);

static void       clay_dashboard_map                            (GtkWidget           *widget);
static void       clay_dashboard_unmap                          (GtkWidget           *widget);

static void       clay_dashboard_set_aux_info                   (ClayDocked          *docked,
                                                                 GList               *aux_info);
static GList    * clay_dashboard_get_aux_info                   (ClayDocked          *docked);

static gboolean   clay_dashboard_group_expander_button_press    (ClayDashboard       *dashboard,
                                                                 GdkEventButton      *bevent,
                                                                 GtkWidget           *widget);

static void       clay_dashboard_group_action_toggled           (ClayDashboard       *dashboard,
                                                                 ClayToggleAction    *action);
static void       clay_dashboard_field_menu_item_toggled        (ClayDashboard       *dashboard,
                                                                 GtkCheckMenuItem    *item);

static gpointer   clay_dashboard_sample                         (ClayDashboard       *dashboard);

static gboolean   clay_dashboard_update                         (ClayDashboard       *dashboard);
static gboolean   clay_dashboard_low_swap_space                 (ClayDashboard       *dashboard);

static void       clay_dashboard_sample_function                (ClayDashboard       *dashboard,
                                                                 Variable             variable);
static void       clay_dashboard_sample_gegl_config             (ClayDashboard       *dashboard,
                                                                 Variable             variable);
static void       clay_dashboard_sample_gegl_stats              (ClayDashboard       *dashboard,
                                                                 Variable             variable);
static void       clay_dashboard_sample_variable_changed        (ClayDashboard       *dashboard,
                                                                 Variable             variable);
static void       clay_dashboard_sample_variable_rate_of_change (ClayDashboard       *dashboard,
                                                                 Variable             variable);
static void       clay_dashboard_sample_swap_limit              (ClayDashboard       *dashboard,
                                                                 Variable             variable);
#ifdef HAVE_CPU_GROUP
static void       clay_dashboard_sample_cpu_usage               (ClayDashboard       *dashboard,
                                                                 Variable             variable);
static void       clay_dashboard_sample_cpu_active              (ClayDashboard       *dashboard,
                                                                 Variable             variable);
static void       clay_dashboard_sample_cpu_active_time         (ClayDashboard       *dashboard,
                                                                 Variable             variable);
#endif /* HAVE_CPU_GROUP */

#ifdef HAVE_MEMORY_GROUP
static void       clay_dashboard_sample_memory_used             (ClayDashboard       *dashboard,
                                                                 Variable             variable);
static void       clay_dashboard_sample_memory_available        (ClayDashboard       *dashboard,
                                                                 Variable             variable);
static void       clay_dashboard_sample_memory_size             (ClayDashboard       *dashboard,
                                                                 Variable             variable);
#endif /* HAVE_MEMORY_GROUP */

static void       clay_dashboard_sample_object                  (ClayDashboard       *dashboard,
                                                                 GObject             *object,
                                                                 Variable             variable);

static void       clay_dashboard_update_groups                  (ClayDashboard       *dashboard);
static void       clay_dashboard_update_group                   (ClayDashboard       *dashboard,
                                                                 Group                group);
static void       clay_dashboard_update_group_values            (ClayDashboard       *dashboard,
                                                                 Group                group);

static void       clay_dashboard_group_set_active               (ClayDashboard       *dashboard,
                                                                 Group                group,
                                                                 gboolean             active);
static void       clay_dashboard_field_set_active               (ClayDashboard       *dashboard,
                                                                 Group                group,
                                                                 gint                 field,
                                                                 gboolean             active);

static void       clay_dashboard_reset_unlocked                 (ClayDashboard       *dashboard);

static void       clay_dashboard_reset_variables                (ClayDashboard       *dashboard);

static gpointer   clay_dashboard_variable_get_data              (ClayDashboard       *dashboard,
                                                                 Variable             variable,
                                                                 gsize                size);

static gboolean   clay_dashboard_variable_to_boolean            (ClayDashboard       *dashboard,
                                                                 Variable             variable);
static gdouble    clay_dashboard_variable_to_double             (ClayDashboard       *dashboard,
                                                                 Variable             variable);

static gchar    * clay_dashboard_field_to_string                (ClayDashboard       *dashboard,
                                                                 Group                group,
                                                                 gint                 field,
                                                                 gboolean             full);

static gboolean   clay_dashboard_log_printf                     (ClayDashboard       *dashboard,
                                                                 const gchar         *format,
                                                                 ...) G_GNUC_PRINTF (2, 3);
static gboolean   clay_dashboard_log_print_escaped              (ClayDashboard       *dashboard,
                                                                 const gchar         *string);
static gint64     clay_dashboard_log_time                       (ClayDashboard       *dashboard);
static void       clay_dashboard_log_sample                     (ClayDashboard       *dashboard,
                                                                 gboolean             variables_changed,
                                                                 gboolean             include_current_thread);
static void       clay_dashboard_log_add_marker_unlocked        (ClayDashboard       *dashboard,
                                                                 const gchar         *description);
static void       clay_dashboard_log_update_highlight           (ClayDashboard       *dashboard);
static void       clay_dashboard_log_update_n_markers           (ClayDashboard       *dashboard);

static void       clay_dashboard_log_write_address_map          (ClayDashboard       *dashboard,
                                                                 guintptr            *addresses,
                                                                 gint                 n_addresses,
                                                                 ClayAsync           *async);
static void       clay_dashboard_log_write_global_address_map   (ClayAsync           *async,
                                                                 ClayDashboard       *dashboard);
static void       clay_dashboard_log_log_func                   (const gchar         *log_domain,
                                                                 GLogLevelFlags       log_levels,
                                                                 const gchar         *message,
                                                                 ClayDashboard       *dashboard);

static gboolean   clay_dashboard_field_use_meter_underlay       (Group                group,
                                                                 gint                 field);

static gchar    * clay_dashboard_format_rate_of_change          (const gchar         *value);
static gchar    * clay_dashboard_format_value                   (VariableType         type,
                                                                 gdouble              value);

static void       clay_dashboard_label_set_text                 (GtkLabel            *label,
                                                                 const gchar         *text);


/*  static variables  */

static const VariableInfo variables[] =
{
  /* cache variables */

  [VARIABLE_CACHE_OCCUPIED] =
  { .name             = "cache-occupied",
    .title            = NC_("dashboard-variable", "Occupied"),
    .description      = N_("Tile cache occupied size"),
    .type             = VARIABLE_TYPE_SIZE,
    .rgb              = {0.3, 0.6, 0.3, 1.0},
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "tile-cache-total"
  },

  [VARIABLE_CACHE_MAXIMUM] =
  { .name             = "cache-maximum",
    .title            = NC_("dashboard-variable", "Maximum"),
    .description      = N_("Maximal tile cache occupied size"),
    .type             = VARIABLE_TYPE_SIZE,
    .rgb              = {0.3, 0.7, 0.8, 1.0},
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "tile-cache-total-max"
  },

  [VARIABLE_CACHE_LIMIT] =
  { .name             = "cache-limit",
    .title            = NC_("dashboard-variable", "Limit"),
    .description      = N_("Tile cache size limit"),
    .type             = VARIABLE_TYPE_SIZE,
    .sample_func      = clay_dashboard_sample_gegl_config,
    .data             = "tile-cache-size"
  },

  [VARIABLE_CACHE_COMPRESSION] =
  { .name             = "cache-compression",
    .title            = NC_("dashboard-variable", "Compression"),
    .description      = N_("Tile cache compression ratio"),
    .type             = VARIABLE_TYPE_SIZE_RATIO,
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "tile-cache-total\0"
                        "tile-cache-total-uncompressed"
  },

  [VARIABLE_CACHE_HIT_MISS] =
  { .name             = "cache-hit-miss",
    .title            = NC_("dashboard-variable", "Hit/Miss"),
    .description      = N_("Tile cache hit/miss ratio"),
    .type             = VARIABLE_TYPE_INT_RATIO,
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "tile-cache-hits\0"
                        "tile-cache-misses"
  },


  /* swap variables */

  [VARIABLE_SWAP_OCCUPIED] =
  { .name             = "swap-occupied",
    .title            = NC_("dashboard-variable", "Occupied"),
    .description      = N_("Swap file occupied size"),
    .type             = VARIABLE_TYPE_SIZE,
    .rgb              = {0.8, 0.2, 0.2, 1.0},
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "swap-total"
  },

  [VARIABLE_SWAP_SIZE] =
  { .name             = "swap-size",
    .title            = NC_("dashboard-variable", "Size"),
    .description      = N_("Swap file size"),
    .type             = VARIABLE_TYPE_SIZE,
    .rgb              = {0.8, 0.6, 0.4, 1.0},
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "swap-file-size"
  },

  [VARIABLE_SWAP_LIMIT] =
  { .name             = "swap-limit",
    .title            = NC_("dashboard-variable", "Limit"),
    .description      = N_("Swap file size limit"),
    .type             = VARIABLE_TYPE_SIZE,
    .sample_func      = clay_dashboard_sample_swap_limit,
  },

  [VARIABLE_SWAP_QUEUED] =
  { .name             = "swap-queued",
    .title            = NC_("dashboard-variable", "Queued"),
    .description      = N_("Size of data queued for writing to the swap"),
    .type             = VARIABLE_TYPE_SIZE,
    .rgb              = {0.8, 0.8, 0.2, 0.5},
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "swap-queued-total"
  },

  [VARIABLE_SWAP_QUEUE_STALLS] =
  { .name             = "swap-queue-stalls",
    .title            = NC_("dashboard-variable", "Queue stalls"),
    .description      = N_("Number of times the writing to the swap has been "
                           "stalled, due to a full queue"),
    .type             = VARIABLE_TYPE_INTEGER,
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "swap-queue-stalls"
  },

  [VARIABLE_SWAP_QUEUE_FULL] =
  { .name             = "swap-queue-full",
    .title            = NC_("dashboard-variable", "Queue full"),
    .description      = N_("Whether the swap queue is full"),
    .type             = VARIABLE_TYPE_BOOLEAN,
    .sample_func      = clay_dashboard_sample_variable_changed,
    .data             = GINT_TO_POINTER (VARIABLE_SWAP_QUEUE_STALLS)
  },

  [VARIABLE_SWAP_READ] =
  { .name             = "swap-read",
    /* Translators: this is the past participle form of "read",
     *              as in "total amount of data read from the swap".
     */
    .title            = NC_("dashboard-variable", "Read"),
    .description      = N_("Total amount of data read from the swap"),
    .type             = VARIABLE_TYPE_SIZE,
    .rgb              = {0.2, 0.4, 1.0, 0.4},
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "swap-read-total"
  },

  [VARIABLE_SWAP_READ_THROUGHPUT] =
  { .name             = "swap-read-throughput",
    .title            = NC_("dashboard-variable", "Read throughput"),
    .description      = N_("The rate at which data is read from the swap"),
    .type             = VARIABLE_TYPE_RATE_OF_CHANGE,
    .rgb              = {0.2, 0.4, 1.0, 1.0},
    .sample_func      = clay_dashboard_sample_variable_rate_of_change,
    .data             = GINT_TO_POINTER (VARIABLE_SWAP_READ)
  },

  [VARIABLE_SWAP_WRITTEN] =
  { .name             = "swap-written",
    /* Translators: this is the past participle form of "write",
     *              as in "total amount of data written to the swap".
     */
    .title            = NC_("dashboard-variable", "Written"),
    .description      = N_("Total amount of data written to the swap"),
    .type             = VARIABLE_TYPE_SIZE,
    .rgb              = {0.8, 0.3, 0.2, 0.4},
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "swap-write-total"
  },

  [VARIABLE_SWAP_WRITE_THROUGHPUT] =
  { .name             = "swap-write-throughput",
    .title            = NC_("dashboard-variable", "Write throughput"),
    .description      = N_("The rate at which data is written to the swap"),
    .type             = VARIABLE_TYPE_RATE_OF_CHANGE,
    .rgb              = {0.8, 0.3, 0.2, 1.0},
    .sample_func      = clay_dashboard_sample_variable_rate_of_change,
    .data             = GINT_TO_POINTER (VARIABLE_SWAP_WRITTEN)
  },

  [VARIABLE_SWAP_COMPRESSION] =
  { .name             = "swap-compression",
    .title            = NC_("dashboard-variable", "Compression"),
    .description      = N_("Swap compression ratio"),
    .type             = VARIABLE_TYPE_SIZE_RATIO,
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "swap-total\0"
                        "swap-total-uncompressed"
  },


#ifdef HAVE_CPU_GROUP
  /* cpu variables */

  [VARIABLE_CPU_USAGE] =
  { .name             = "cpu-usage",
    .title            = NC_("dashboard-variable", "Usage"),
    .description      = N_("Total CPU usage"),
    .type             = VARIABLE_TYPE_PERCENTAGE,
    .rgb              = {0.8, 0.7, 0.2, 1.0},
    .sample_func      = clay_dashboard_sample_cpu_usage
  },

  [VARIABLE_CPU_ACTIVE] =
  { .name             = "cpu-active",
    .title            = NC_("dashboard-variable", "Active"),
    .description      = N_("Whether the CPU is active"),
    .type             = VARIABLE_TYPE_BOOLEAN,
    .rgb              = {0.9, 0.8, 0.3, 1.0},
    .sample_func      = clay_dashboard_sample_cpu_active
  },

  [VARIABLE_CPU_ACTIVE_TIME] =
  { .name             = "cpu-active-time",
    .title            = NC_("dashboard-variable", "Active"),
    .description      = N_("Total amount of time the CPU has been active"),
    .type             = VARIABLE_TYPE_DURATION,
    .rgb              = {0.8, 0.7, 0.2, 0.4},
    .sample_func      = clay_dashboard_sample_cpu_active_time
  },
#endif /* HAVE_CPU_GROUP */


#ifdef HAVE_MEMORY_GROUP
  /* memory variables */

  [VARIABLE_MEMORY_USED] =
  { .name             = "memory-used",
    .title            = NC_("dashboard-variable", "Used"),
    .description      = N_("Amount of memory used by the process"),
    .type             = VARIABLE_TYPE_SIZE,
    .rgb              = {0.8, 0.5, 0.2, 1.0},
    .sample_func      = clay_dashboard_sample_memory_used
  },

  [VARIABLE_MEMORY_AVAILABLE] =
  { .name             = "memory-available",
    .title            = NC_("dashboard-variable", "Available"),
    .description      = N_("Amount of available physical memory"),
    .type             = VARIABLE_TYPE_SIZE,
    .rgb              = {0.8, 0.5, 0.2, 0.4},
    .sample_func      = clay_dashboard_sample_memory_available
  },

  [VARIABLE_MEMORY_SIZE] =
  { .name             = "memory-size",
    .title            = NC_("dashboard-variable", "Size"),
    .description      = N_("Physical memory size"),
    .type             = VARIABLE_TYPE_SIZE,
    .sample_func      = clay_dashboard_sample_memory_size
  },
#endif /* HAVE_MEMORY_GROUP */


  /* misc variables */

  [VARIABLE_MIPMAPED] =
  { .name             = "mipmapped",
    .title            = NC_("dashboard-variable", "Mipmapped"),
    .description      = N_("Total size of processed mipmapped data"),
    .type             = VARIABLE_TYPE_SIZE,
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "zoom-total"
  },

  [VARIABLE_ASSIGNED_THREADS] =
  { .name             = "assigned-threads",
    .title            = NC_("dashboard-variable", "Assigned"),
    .description      = N_("Number of assigned worker threads"),
    .type             = VARIABLE_TYPE_INTEGER,
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "assigned-threads"
  },

  [VARIABLE_ACTIVE_THREADS] =
  { .name             = "active-threads",
    .title            = NC_("dashboard-variable", "Active"),
    .description      = N_("Number of active worker threads"),
    .type             = VARIABLE_TYPE_INTEGER,
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "active-threads"
  },

  [VARIABLE_ASYNC_RUNNING] =
  { .name             = "async-running",
    .title            = NC_("dashboard-variable", "Async"),
    .description      = N_("Number of ongoing asynchronous operations"),
    .type             = VARIABLE_TYPE_INTEGER,
    .sample_func      = clay_dashboard_sample_function,
    .data             = clay_async_get_n_running
  },

  [VARIABLE_TILE_ALLOC_TOTAL] =
  { .name             = "tile-alloc-total",
    .title            = NC_("dashboard-variable", "Tile"),
    .description      = N_("Total size of tile memory"),
    .type             = VARIABLE_TYPE_SIZE,
    .rgb              = {0.3, 0.3, 1.0, 1.0},
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "tile-alloc-total"
  },

  [VARIABLE_SCRATCH_TOTAL] =
  { .name             = "scratch-total",
    .title            = NC_("dashboard-variable", "Scratch"),
    .description      = N_("Total size of scratch memory"),
    .type             = VARIABLE_TYPE_SIZE,
    .sample_func      = clay_dashboard_sample_gegl_stats,
    .data             = "scratch-total"
  },

  [VARIABLE_TEMP_BUF_TOTAL] =
  { .name             = "temp-buf-total",
    /* Translators:  "TempBuf" is a technical term referring to an internal
     * Clay data structure.  It's probably OK to leave it untranslated.
     */
    .title            = NC_("dashboard-variable", "TempBuf"),
    .description      = N_("Total size of temporary buffers"),
    .type             = VARIABLE_TYPE_SIZE,
    .sample_func      = clay_dashboard_sample_function,
    .data             = clay_temp_buf_get_total_memsize
  }
};

static const GroupInfo groups[] =
{
  /* cache group */
  [GROUP_CACHE] =
  { .name             = "cache",
    .title            = NC_("dashboard-group", "Cache"),
    .description      = N_("In-memory tile cache"),
    .default_active   = TRUE,
    .default_expanded = TRUE,
    .has_meter        = TRUE,
    .meter_limit      = VARIABLE_CACHE_LIMIT,
    .fields           = (const FieldInfo[])
                        {
                          { .variable         = VARIABLE_CACHE_OCCUPIED,
                            .default_active   = TRUE,
                            .show_in_header   = TRUE,
                            .meter_value      = 2
                          },
                          { .variable         = VARIABLE_CACHE_MAXIMUM,
                            .default_active   = FALSE,
                            .meter_value      = 1
                          },
                          { .variable         = VARIABLE_CACHE_LIMIT,
                            .default_active   = TRUE
                          },

                          { VARIABLE_SEPARATOR },

                          { .variable         = VARIABLE_CACHE_COMPRESSION,
                            .default_active   = FALSE
                          },
                          { .variable         = VARIABLE_CACHE_HIT_MISS,
                            .default_active   = FALSE
                          },

                          {}
                        }
  },

  /* swap group */
  [GROUP_SWAP] =
  { .name             = "swap",
    .title            = NC_("dashboard-group", "Swap"),
    .description      = N_("On-disk tile swap"),
    .default_active   = TRUE,
    .default_expanded = TRUE,
    .has_meter        = TRUE,
    .meter_limit      = VARIABLE_SWAP_LIMIT,
    .meter_led        = (const Variable[])
                        {
                          VARIABLE_SWAP_QUEUE_FULL,
                          VARIABLE_SWAP_READ_THROUGHPUT,
                          VARIABLE_SWAP_WRITE_THROUGHPUT,

                          VARIABLE_NONE
                        },
    .fields           = (const FieldInfo[])
                        {
                          { .variable         = VARIABLE_SWAP_OCCUPIED,
                            .default_active   = TRUE,
                            .show_in_header   = TRUE,
                            .meter_value      = 5
                          },
                          { .variable         = VARIABLE_SWAP_SIZE,
                            .default_active   = TRUE,
                            .meter_value      = 4
                          },
                          { .variable         = VARIABLE_SWAP_LIMIT,
                            .default_active   = TRUE
                          },

                          { VARIABLE_SEPARATOR },

                          { .variable         = VARIABLE_SWAP_QUEUED,
                            .default_active   = FALSE,
                            .meter_variable   = VARIABLE_SWAP_QUEUE_FULL,
                            .meter_value      = 3
                          },

                          { VARIABLE_SEPARATOR },

                          { .variable         = VARIABLE_SWAP_READ,
                            .default_active   = FALSE,
                            .meter_variable   = VARIABLE_SWAP_READ_THROUGHPUT,
                            .meter_value      = 2
                          },

                          { .variable         = VARIABLE_SWAP_WRITTEN,
                            .default_active   = FALSE,
                            .meter_variable   = VARIABLE_SWAP_WRITE_THROUGHPUT,
                            .meter_value      = 1
                          },

                          { VARIABLE_SEPARATOR },

                          { .variable         = VARIABLE_SWAP_COMPRESSION,
                            .default_active   = FALSE
                          },

                          {}
                        }
  },

#ifdef HAVE_CPU_GROUP
  /* cpu group */
  [GROUP_CPU] =
  { .name             = "cpu",
    .title            = NC_("dashboard-group", "CPU"),
    .description      = N_("CPU usage"),
    .default_active   = TRUE,
    .default_expanded = FALSE,
    .has_meter        = TRUE,
    .meter_led        = (const Variable[])
                        {
                          VARIABLE_CPU_ACTIVE,

                          VARIABLE_NONE
                        },
    .fields           = (const FieldInfo[])
                        {
                          { .variable         = VARIABLE_CPU_USAGE,
                            .default_active   = TRUE,
                            .show_in_header   = TRUE,
                            .meter_value      = 2
                          },

                          { VARIABLE_SEPARATOR },

                          { .variable         = VARIABLE_CPU_ACTIVE_TIME,
                            .default_active   = FALSE,
                            .meter_variable   = VARIABLE_CPU_ACTIVE,
                            .meter_value      = 1
                          },

                          {}
                        }
  },
#endif /* HAVE_CPU_GROUP */

#ifdef HAVE_MEMORY_GROUP
  /* memory group */
  [GROUP_MEMORY] =
  { .name             = "memory",
    .title            = NC_("dashboard-group", "Memory"),
    .description      = N_("Memory usage"),
    .default_active   = TRUE,
    .default_expanded = FALSE,
    .has_meter        = TRUE,
    .meter_limit      = VARIABLE_MEMORY_SIZE,
    .fields           = (const FieldInfo[])
                        {
                          { .variable         = VARIABLE_CACHE_OCCUPIED,
                            .title            = NC_("dashboard-variable", "Cache"),
                            .default_active   = FALSE,
                            .meter_value      = 4
                          },
                          { .variable         = VARIABLE_TILE_ALLOC_TOTAL,
                            .default_active   = FALSE,
                            .meter_value      = 3
                          },

                          { VARIABLE_SEPARATOR },

                          { .variable         = VARIABLE_MEMORY_USED,
                            .default_active   = TRUE,
                            .show_in_header   = TRUE,
                            .meter_value      = 2,
                            .meter_cumulative = TRUE
                          },
                          { .variable         = VARIABLE_MEMORY_AVAILABLE,
                            .default_active   = TRUE,
                            .meter_value      = 1,
                            .meter_cumulative = TRUE
                          },
                          { .variable         = VARIABLE_MEMORY_SIZE,
                            .default_active   = TRUE
                          },

                          {}
                        }
  },
#endif /* HAVE_MEMORY_GROUP */

  /* misc group */
  [GROUP_MISC] =
  { .name             = "misc",
    .title            = NC_("dashboard-group", "Misc"),
    .description      = N_("Miscellaneous information"),
    .default_active   = FALSE,
    .default_expanded = FALSE,
    .has_meter        = FALSE,
    .fields           = (const FieldInfo[])
                        {
                          { .variable       = VARIABLE_MIPMAPED,
                            .default_active = TRUE
                          },
                          { .variable       = VARIABLE_ASSIGNED_THREADS,
                            .default_active = TRUE
                          },
                          { .variable       = VARIABLE_ACTIVE_THREADS,
                            .default_active = TRUE
                          },
                          { .variable       = VARIABLE_ASYNC_RUNNING,
                            .default_active = TRUE
                          },
                          { .variable       = VARIABLE_TILE_ALLOC_TOTAL,
                            .default_active = TRUE
                          },
                          { .variable       = VARIABLE_SCRATCH_TOTAL,
                            .default_active = TRUE
                          },
                          { .variable       = VARIABLE_TEMP_BUF_TOTAL,
                            .default_active = TRUE
                          },

                          {}
                        }
  },
};


G_DEFINE_TYPE_WITH_CODE (ClayDashboard, clay_dashboard, CLAY_TYPE_EDITOR,
                         G_ADD_PRIVATE (ClayDashboard)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_dashboard_docked_iface_init))

#define parent_class clay_dashboard_parent_class

static ClayDockedInterface *parent_docked_iface = NULL;


/*  private functions  */


static void
clay_dashboard_class_init (ClayDashboardClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = clay_dashboard_constructed;
  object_class->dispose     = clay_dashboard_dispose;
  object_class->finalize    = clay_dashboard_finalize;

  widget_class->map         = clay_dashboard_map;
  widget_class->unmap       = clay_dashboard_unmap;
}

static void
clay_dashboard_init (ClayDashboard *dashboard)
{
  ClayDashboardPrivate *priv;
  GtkWidget            *box;
  GtkWidget            *scrolled_window;
  GtkWidget            *viewport;
  GtkWidget            *vbox;
  GtkWidget            *expander;
  GtkWidget            *hbox;
  GtkWidget            *button;
  GtkWidget            *image;
  GtkWidget            *menu;
  GtkWidget            *item;
  GtkWidget            *frame;
  GtkWidget            *vbox2;
  GtkWidget            *meter;
  GtkWidget            *grid;
  GtkWidget            *label;
  gint                  content_spacing;
  Group                 group;
  gint                  field;

  priv = dashboard->priv = clay_dashboard_get_instance_private (dashboard);

  g_mutex_init (&priv->mutex);
  g_cond_init (&priv->cond);

  priv->update_interval        = DEFAULT_UPDATE_INTERVAL;
  priv->history_duration       = DEFAULT_HISTORY_DURATION;
  priv->low_swap_space_warning = DEFAULT_LOW_SWAP_SPACE_WARNING;

  gtk_widget_style_get (GTK_WIDGET (dashboard),
                        "content-spacing", &content_spacing,
                        NULL);

  /* we put the dashboard inside an event box, so that it gets its own window,
   * which reduces the overhead of updating the ui, since it gets updated
   * frequently.  unfortunately, this means that the dashboard's background
   * color may be a bit off for some themes.
   */
  box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (dashboard), box, TRUE, TRUE, 0);
  gtk_widget_set_visible (box, TRUE);

  /* scrolled window */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (box), scrolled_window);
  gtk_widget_set_visible (scrolled_window, TRUE);

  /* viewport */
  viewport = gtk_viewport_new (
    gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrolled_window)),
    gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window)));
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (scrolled_window), viewport);
  gtk_widget_set_visible (viewport, TRUE);

  /* main vbox */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2 * content_spacing);
  gtk_container_add (GTK_CONTAINER (viewport), vbox);
  gtk_widget_set_visible (vbox, TRUE);

  /* construct the groups */
  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    {
      const GroupInfo *group_info = &groups[group];
      GroupData       *group_data = &priv->groups[group];

      group_data->n_fields       = 0;
      group_data->n_meter_values = 0;

      for (field = 0; group_info->fields[field].variable; field++)
        {
          const FieldInfo *field_info = &group_info->fields[field];

          group_data->n_fields++;
          group_data->n_meter_values = MAX (group_data->n_meter_values,
                                            field_info->meter_value);
        }

      group_data->fields = g_new0 (FieldData, group_data->n_fields);

      /* group expander */
      expander = gtk_expander_new (NULL);
      group_data->expander = GTK_EXPANDER (expander);
      gtk_expander_set_expanded (GTK_EXPANDER (expander),
                                 group_info->default_expanded);
      gtk_expander_set_label_fill (GTK_EXPANDER (expander), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox), expander, FALSE, FALSE, 0);

      g_object_set_data (G_OBJECT (expander),
                         "clay-dashboard-group", GINT_TO_POINTER (group));
      g_signal_connect_swapped (expander, "button-press-event",
                                G_CALLBACK (clay_dashboard_group_expander_button_press),
                                dashboard);

      /* group expander label box */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      clay_help_set_help_data (hbox,
                               g_dgettext (NULL, group_info->description),
                               NULL);
      gtk_expander_set_label_widget (GTK_EXPANDER (expander), hbox);
      gtk_widget_set_visible (hbox, TRUE);

      /* group expander label */
      label = gtk_label_new (g_dpgettext2 (NULL, "dashboard-group",
                                           group_info->title));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      clay_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                 -1);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_set_visible (label, TRUE);

      /* group expander values label */
      label = gtk_label_new (NULL);
      group_data->header_values_label = GTK_LABEL (label);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);

      g_object_bind_property (expander, "expanded",
                              label,    "visible",
                              G_BINDING_SYNC_CREATE |
                              G_BINDING_INVERT_BOOLEAN);

      /* group expander menu button */
      button = gtk_button_new ();
      group_data->menu_button = GTK_BUTTON (button);
      clay_help_set_help_data (button, _("Select fields"),
                               NULL);
      gtk_widget_set_can_focus (button, FALSE);
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_set_visible (button, TRUE);

      image = gtk_image_new_from_icon_name (CLAY_ICON_MENU_LEFT,
                                            GTK_ICON_SIZE_MENU);
      gtk_image_set_pixel_size (GTK_IMAGE (image), 12);
      gtk_image_set_from_icon_name (GTK_IMAGE (image), CLAY_ICON_MENU_LEFT,
                                    GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_set_visible (image, TRUE);

      /* group menu */
      menu = gtk_menu_new ();
      group_data->menu = GTK_MENU (menu);
      gtk_menu_attach_to_widget (GTK_MENU (menu), button, NULL);

      for (field = 0; field < group_data->n_fields; field++)
        {
          const FieldInfo *field_info = &group_info->fields[field];
          FieldData       *field_data = &group_data->fields[field];

          if (field_info->variable != VARIABLE_SEPARATOR)
            {
              const VariableInfo *variable_info = &variables[field_info->variable];

              item = gtk_check_menu_item_new_with_label (
                g_dpgettext2 (NULL, "dashboard-variable",
                              field_info->title ? field_info->title :
                                                  variable_info->title));
              field_data->menu_item = GTK_CHECK_MENU_ITEM (item);
              clay_help_set_help_data (item,
                                       g_dgettext (NULL, variable_info->description),
                                       NULL);

              g_object_set_data (G_OBJECT (item),
                                 "clay-dashboard-group", GINT_TO_POINTER (group));
              g_object_set_data (G_OBJECT (item),
                                 "clay-dashboard-field", GINT_TO_POINTER (field));
              g_signal_connect_swapped (item, "toggled",
                                        G_CALLBACK (clay_dashboard_field_menu_item_toggled),
                                        dashboard);

              clay_dashboard_field_set_active (dashboard, group, field,
                                               field_info->default_active);
            }
          else
            {
              item = gtk_separator_menu_item_new ();
            }

          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          gtk_widget_set_visible (item, TRUE);
        }

      /* group frame */
      frame = clay_frame_new (NULL);
      gtk_container_add (GTK_CONTAINER (expander), frame);
      gtk_widget_set_visible (frame, TRUE);

      /* group vbox */
      vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2 * content_spacing);
      gtk_container_add (GTK_CONTAINER (frame), vbox2);
      gtk_widget_set_visible (vbox2, TRUE);

      /* group meter */
      if (group_info->has_meter)
        {
          GeglColor *color    = gegl_color_new (NULL);
          gdouble    interval = priv->update_interval;

          meter = clay_meter_new (priv->clay, group_data->n_meter_values);
          group_data->meter = CLAY_METER (meter);
          clay_help_set_help_data (meter,
                                   g_dgettext (NULL, group_info->description),
                                   NULL);
          clay_meter_set_history_resolution (CLAY_METER (meter),
                                             interval / 1000.0);
          clay_meter_set_history_duration (CLAY_METER (meter),
                                           (gdouble) priv->history_duration / 1000.0);
          gtk_box_pack_start (GTK_BOX (vbox2), meter, FALSE, FALSE, 0);
          gtk_widget_set_visible (meter, TRUE);

          for (field = 0; field < group_data->n_fields; field++)
            {
              const FieldInfo *field_info = &group_info->fields[field];

              if (field_info->meter_value)
                {
                  const VariableInfo *variable_info = &variables[field_info->variable];

                  gegl_color_set_pixel (color, babl_format ("R'G'B'A double"), variable_info->rgb);

                  clay_meter_set_value_color (CLAY_METER (meter),
                                              field_info->meter_value - 1,
                                              color);

                  if (clay_dashboard_field_use_meter_underlay (group, field))
                    {
                      clay_meter_set_value_show_in_gauge (CLAY_METER (meter),
                                                          field_info->meter_value - 1,
                                                          FALSE);
                      clay_meter_set_value_interpolation (CLAY_METER (meter),
                                                          field_info->meter_value - 1,
                                                          CLAY_INTERPOLATION_NONE);
                    }
                }
            }

          g_object_unref (color);
        }

      /* group grid */
      grid = gtk_grid_new ();
      group_data->grid = GTK_GRID (grid);
      gtk_grid_set_row_spacing (GTK_GRID (grid), content_spacing);
      gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
      gtk_box_pack_start (GTK_BOX (vbox2), grid, FALSE, FALSE, 0);
      gtk_widget_set_visible (grid, TRUE);

      clay_dashboard_group_set_active (dashboard, group,
                                       group_info->default_active);
      clay_dashboard_update_group (dashboard, group);
    }

  /* sampler thread
   *
   * we use a separate thread for sampling, so that data is sampled even when
   * the main thread is busy
   */
  priv->thread = g_thread_new ("dashboard",
                               (GThreadFunc) clay_dashboard_sample,
                               dashboard);
}

static void
clay_dashboard_docked_iface_init (ClayDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (CLAY_TYPE_DOCKED);

  iface->set_aux_info = clay_dashboard_set_aux_info;
  iface->get_aux_info = clay_dashboard_get_aux_info;
}

static void
clay_dashboard_constructed (GObject *object)
{
  ClayDashboard        *dashboard = CLAY_DASHBOARD (object);
  ClayDashboardPrivate *priv = dashboard->priv;
  ClayUIManager        *ui_manager;
  ClayActionGroup      *action_group;
  ClayAction           *action;
  GtkWidget            *button;
  GtkWidget            *box;
  GtkWidget            *image;
  GtkWidget            *label;
  Group                 group;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ui_manager   = clay_editor_get_ui_manager (CLAY_EDITOR (dashboard));
  action_group = clay_ui_manager_get_action_group (ui_manager, "dashboard");

  /* group actions */
  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    {
      const GroupInfo       *group_info = &groups[group];
      GroupData             *group_data = &priv->groups[group];
      ClayToggleActionEntry  entry      = { 0 };

      entry.name      = g_strdup_printf ("dashboard-group-%s", group_info->name);
      entry.label     = g_dpgettext2 (NULL, "dashboard-group", group_info->title);
      entry.tooltip   = g_dgettext (NULL, group_info->description);
      entry.help_id   = CLAY_HELP_DASHBOARD_GROUPS;
      entry.is_active = group_data->active;

      clay_action_group_add_toggle_actions (action_group, "dashboard-groups",
                                            &entry, 1);

      action = clay_ui_manager_find_action (ui_manager, "dashboard", entry.name);
      group_data->action = CLAY_TOGGLE_ACTION (action);

      g_object_set_data (G_OBJECT (action),
                         "clay-dashboard-group", GINT_TO_POINTER (group));
      g_signal_connect_swapped (action, "toggled",
                                G_CALLBACK (clay_dashboard_group_action_toggled),
                                dashboard);

      g_free ((gpointer) entry.name);
    }

  button = clay_editor_add_action_button (CLAY_EDITOR (dashboard), "dashboard",
                                          "dashboard-log-record", NULL);
  priv->log_record_button = button;

  button = clay_editor_add_action_button (CLAY_EDITOR (dashboard), "dashboard",
                                          "dashboard-log-add-marker",
                                          "dashboard-log-add-empty-marker",
                                          clay_get_extend_selection_mask (),
                                          NULL);

  action = clay_action_group_get_action (action_group,
                                         "dashboard-log-add-marker");
  g_object_bind_property (action, "sensitive",
                          button, "visible",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  image = g_object_ref (gtk_bin_get_child (GTK_BIN (button)));
  gtk_container_remove (GTK_CONTAINER (button), image);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add (GTK_CONTAINER (button), box);
  gtk_widget_set_halign (GTK_WIDGET (box), GTK_ALIGN_CENTER);
  gtk_widget_set_valign (GTK_WIDGET (box), GTK_ALIGN_CENTER);
  gtk_widget_set_visible (box, TRUE);

  gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);
  g_object_unref (image);

  label = gtk_label_new (NULL);
  priv->log_add_marker_label = GTK_LABEL (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  button = clay_editor_add_action_button (CLAY_EDITOR (dashboard), "dashboard",
                                          "dashboard-reset", NULL);

  action = clay_action_group_get_action (action_group,
                                         "dashboard-reset");
  g_object_bind_property (action, "sensitive",
                          button, "visible",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  clay_action_group_update (action_group, dashboard);
}

static void
clay_dashboard_dispose (GObject *object)
{
  ClayDashboard        *dashboard = CLAY_DASHBOARD (object);
  ClayDashboardPrivate *priv      = dashboard->priv;

  if (priv->thread)
    {
      g_mutex_lock (&priv->mutex);

      priv->quit = TRUE;
      g_cond_signal (&priv->cond);

      g_mutex_unlock (&priv->mutex);

      g_clear_pointer (&priv->thread, g_thread_join);
    }

  if (priv->update_idle_id)
    {
      g_source_remove (priv->update_idle_id);
      priv->update_idle_id = 0;
    }

  if (priv->low_swap_space_idle_id)
    {
      g_source_remove (priv->low_swap_space_idle_id);
      priv->low_swap_space_idle_id = 0;
    }

  clay_dashboard_log_stop_recording (dashboard, NULL);

  clay_dashboard_reset_variables (dashboard);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_dashboard_finalize (GObject *object)
{
  ClayDashboard        *dashboard = CLAY_DASHBOARD (object);
  ClayDashboardPrivate *priv      = dashboard->priv;
  gint                  i;

  for (i = FIRST_GROUP; i < N_GROUPS; i++)
    g_free (priv->groups[i].fields);

  g_mutex_clear (&priv->mutex);
  g_cond_clear (&priv->cond);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_dashboard_map (GtkWidget *widget)
{
  ClayDashboard *dashboard = CLAY_DASHBOARD (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  clay_dashboard_update (dashboard);
}

static void
clay_dashboard_unmap (GtkWidget *widget)
{
  ClayDashboard        *dashboard = CLAY_DASHBOARD (widget);
  ClayDashboardPrivate *priv      = dashboard->priv;

  g_mutex_lock (&priv->mutex);

  if (priv->update_idle_id)
    {
      g_source_remove (priv->update_idle_id);
      priv->update_idle_id = 0;
    }

  g_mutex_unlock (&priv->mutex);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

#define AUX_INFO_UPDATE_INTERVAL        "update-interval"
#define AUX_INFO_HISTORY_DURATION       "history-duration"
#define AUX_INFO_LOW_SWAP_SPACE_WARNING "low-swap-space-warning"

static void
clay_dashboard_set_aux_info (ClayDocked *docked,
                             GList      *aux_info)
{
  ClayDashboard        *dashboard = CLAY_DASHBOARD (docked);
  ClayDashboardPrivate *priv      = dashboard->priv;
  gchar                *name;
  GList                *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      ClaySessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_UPDATE_INTERVAL))
        {
          gint                       value = atoi (aux->value);
          ClayDashboardUpdateInteval update_interval;

          for (update_interval = CLAY_DASHBOARD_UPDATE_INTERVAL_0_25_SEC;
               update_interval < value &&
               update_interval < CLAY_DASHBOARD_UPDATE_INTERVAL_4_SEC;
               update_interval *= 2);

          clay_dashboard_set_update_interval (dashboard, update_interval);
        }
      else if (! strcmp (aux->name, AUX_INFO_HISTORY_DURATION))
        {
          gint                         value = atoi (aux->value);
          ClayDashboardHistoryDuration history_duration;

          for (history_duration = CLAY_DASHBOARD_HISTORY_DURATION_15_SEC;
               history_duration < value &&
               history_duration < CLAY_DASHBOARD_HISTORY_DURATION_240_SEC;
               history_duration *= 2);

          clay_dashboard_set_history_duration (dashboard, history_duration);
        }
      else if (! strcmp (aux->name, AUX_INFO_LOW_SWAP_SPACE_WARNING))
        {
          clay_dashboard_set_low_swap_space_warning (dashboard,
                                                     ! strcmp (aux->value, "yes"));
        }
      else
        {
          Group group;
          gint  field;

          for (group = FIRST_GROUP; group < N_GROUPS; group++)
            {
              const GroupInfo *group_info = &groups[group];
              GroupData       *group_data = &priv->groups[group];

              name = g_strdup_printf ("%s-active", group_info->name);

              if (! strcmp (aux->name, name))
                {
                  gboolean active = ! strcmp (aux->value, "yes");

                  clay_dashboard_group_set_active (dashboard, group, active);

                  g_free (name);
                  goto next_aux_info;
                }

              g_free (name);

              name = g_strdup_printf ("%s-expanded", group_info->name);

              if (! strcmp (aux->name, name))
                {
                  gboolean expanded = ! strcmp (aux->value, "yes");

                  gtk_expander_set_expanded (group_data->expander, expanded);

                  g_free (name);
                  goto next_aux_info;
                }

              g_free (name);

              for (field = 0; field < group_data->n_fields; field++)
                {
                  const FieldInfo *field_info = &group_info->fields[field];

                  if (field_info->variable != VARIABLE_SEPARATOR)
                    {
                      const VariableInfo *variable_info = &variables[field_info->variable];

                      name = g_strdup_printf ("%s-%s-active",
                                              group_info->name,
                                              variable_info->name);

                      if (! strcmp (aux->name, name))
                        {
                          gboolean active = ! strcmp (aux->value, "yes");

                          clay_dashboard_field_set_active (dashboard,
                                                           group, field,
                                                           active);

                          g_free (name);
                          goto next_aux_info;
                        }

                      g_free (name);
                    }
                }
            }
        }
next_aux_info: ;
    }

  clay_dashboard_update_groups (dashboard);
}

static GList *
clay_dashboard_get_aux_info (ClayDocked *docked)
{
  ClayDashboard        *dashboard = CLAY_DASHBOARD (docked);
  ClayDashboardPrivate *priv      = dashboard->priv;
  GList                *aux_info;
  ClaySessionInfoAux   *aux;
  gchar                *name;
  gchar                *value;
  Group                 group;
  gint                  field;

  aux_info = parent_docked_iface->get_aux_info (docked);

  if (priv->update_interval != DEFAULT_UPDATE_INTERVAL)
    {
      value    = g_strdup_printf ("%d", priv->update_interval);
      aux      = clay_session_info_aux_new (AUX_INFO_UPDATE_INTERVAL, value);
      aux_info = g_list_append (aux_info, aux);
      g_free (value);
    }

  if (priv->history_duration != DEFAULT_HISTORY_DURATION)
    {
      value    = g_strdup_printf ("%d", priv->history_duration);
      aux      = clay_session_info_aux_new (AUX_INFO_HISTORY_DURATION, value);
      aux_info = g_list_append (aux_info, aux);
      g_free (value);
    }

  if (priv->low_swap_space_warning != DEFAULT_LOW_SWAP_SPACE_WARNING)
    {
      value    = priv->low_swap_space_warning ? "yes" : "no";
      aux      = clay_session_info_aux_new (AUX_INFO_LOW_SWAP_SPACE_WARNING, value);
      aux_info = g_list_append (aux_info, aux);
    }

  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    {
      const GroupInfo *group_info = &groups[group];
      GroupData       *group_data = &priv->groups[group];
      gboolean         active     = group_data->active;
      gboolean         expanded   = gtk_expander_get_expanded (group_data->expander);

      if (active != group_info->default_active)
        {
          name     = g_strdup_printf ("%s-active", group_info->name);
          value    = active ? "yes" : "no";
          aux      = clay_session_info_aux_new (name, value);
          aux_info = g_list_append (aux_info, aux);
          g_free (name);
        }

      if (expanded != group_info->default_expanded)
        {
          name     = g_strdup_printf ("%s-expanded", group_info->name);
          value    = expanded ? "yes" : "no";
          aux      = clay_session_info_aux_new (name, value);
          aux_info = g_list_append (aux_info, aux);
          g_free (name);
        }

      for (field = 0; field < group_data->n_fields; field++)
        {
          const FieldInfo *field_info = &group_info->fields[field];
          FieldData       *field_data = &group_data->fields[field];
          gboolean         active     = field_data->active;

          if (field_info->variable != VARIABLE_SEPARATOR)
            {
              const VariableInfo *variable_info = &variables[field_info->variable];

              if (active != field_info->default_active)
                {
                  name = g_strdup_printf ("%s-%s-active",
                                          group_info->name,
                                          variable_info->name);
                  value    = active ? "yes" : "no";
                  aux      = clay_session_info_aux_new (name, value);
                  aux_info = g_list_append (aux_info, aux);
                  g_free (name);
                }
            }
        }
    }

  return aux_info;
}

static gboolean
clay_dashboard_group_expander_button_press (ClayDashboard  *dashboard,
                                            GdkEventButton *bevent,
                                            GtkWidget      *widget)
{
  ClayDashboardPrivate *priv = dashboard->priv;
  Group                 group;
  GroupData            *group_data;
  GtkAllocation         expander_allocation;
  GtkAllocation         allocation;

  group      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                   "clay-dashboard-group"));
  group_data = &priv->groups[group];

  gtk_widget_get_allocation (GTK_WIDGET (group_data->expander),
                             &expander_allocation);
  gtk_widget_get_allocation (GTK_WIDGET (group_data->menu_button),
                             &allocation);

  allocation.x -= expander_allocation.x;
  allocation.y -= expander_allocation.y;

  if (bevent->button == 1                          &&
      bevent->x >= allocation.x                    &&
      bevent->x <  allocation.x + allocation.width &&
      bevent->y >= allocation.y                    &&
      bevent->y <  allocation.y + allocation.height)
    {
      gtk_menu_popup_at_widget (group_data->menu,
                                GTK_WIDGET (group_data->menu_button),
                                GDK_GRAVITY_WEST,
                                GDK_GRAVITY_NORTH_EAST,
                                (GdkEvent *) bevent);

      return TRUE;
    }

  return FALSE;
}

static void
clay_dashboard_group_action_toggled (ClayDashboard    *dashboard,
                                     ClayToggleAction *action)
{
  ClayDashboardPrivate *priv = dashboard->priv;
  Group                 group;
  GroupData            *group_data;

  group      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (action),
                                                   "clay-dashboard-group"));
  group_data = &priv->groups[group];

  group_data->active = clay_toggle_action_get_active (action);

  clay_dashboard_update_group (dashboard, group);
}

static void
clay_dashboard_field_menu_item_toggled (ClayDashboard    *dashboard,
                                        GtkCheckMenuItem *item)
{
  ClayDashboardPrivate *priv = dashboard->priv;
  Group                 group;
  GroupData            *group_data;
  gint                  field;
  FieldData            *field_data;

  group      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item),
                                                   "clay-dashboard-group"));
  group_data = &priv->groups[group];

  field      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item),
                                                   "clay-dashboard-field"));
  field_data = &group_data->fields[field];

  field_data->active = gtk_check_menu_item_get_active (item);

  clay_dashboard_update_group (dashboard, group);
}

static gpointer
clay_dashboard_sample (ClayDashboard *dashboard)
{
  ClayDashboardPrivate *priv                = dashboard->priv;
  gint64                last_sample_time    = 0;
  gint64                last_update_time    = 0;
  gboolean              seen_low_swap_space = FALSE;

  g_mutex_lock (&priv->mutex);

  while (! priv->quit)
    {
      gint64 update_interval;
      gint64 sample_interval;
      gint64 end_time;

      update_interval = priv->update_interval * G_TIME_SPAN_SECOND / 1000;

      if (priv->log_output)
        {
          sample_interval = G_TIME_SPAN_SECOND /
                            priv->log_params.sample_frequency;
        }
      else
        {
          sample_interval = update_interval;
        }

      end_time = last_sample_time + sample_interval;

      if (! g_cond_wait_until (&priv->cond, &priv->mutex, end_time) ||
          priv->update_now)
        {
          gint64   time;
          gboolean variables_changed = FALSE;
          Variable variable;
          Group    group;
          gint     field;

          time = g_get_monotonic_time ();

          /* sample all variables */
          for (variable = FIRST_VARIABLE; variable < N_VARIABLES; variable++)
            {
              const VariableInfo *variable_info      = &variables[variable];
              const VariableData *variable_data      = &priv->variables[variable];
              VariableData        prev_variable_data = *variable_data;

              variable_info->sample_func (dashboard, variable);

              variables_changed = variables_changed ||
                                  memcmp (variable_data, &prev_variable_data,
                                          sizeof (VariableData));
            }

          /* log sample */
          if (priv->log_output)
            clay_dashboard_log_sample (dashboard, variables_changed, FALSE);

          /* update gui */
          if (priv->update_now   ||
              ! priv->log_output ||
              time - last_update_time >= update_interval)
            {
              /* add samples to meters */
              for (group = FIRST_GROUP; group < N_GROUPS; group++)
                {
                  const GroupInfo *group_info = &groups[group];
                  GroupData       *group_data = &priv->groups[group];
                  gdouble         *sample;
                  gdouble          total      = 0.0;

                  if (! group_info->has_meter)
                    continue;

                  sample = g_new (gdouble, group_data->n_meter_values);

                  for (field = 0; field < group_data->n_fields; field++)
                    {
                      const FieldInfo *field_info = &group_info->fields[field];

                      if (field_info->meter_value)
                        {
                          gdouble value;

                          if (field_info->meter_variable)
                            variable = field_info->meter_variable;
                          else
                            variable = field_info->variable;

                          value = clay_dashboard_variable_to_double (dashboard,
                                                                     variable);

                          if (value &&
                              clay_dashboard_field_use_meter_underlay (group,
                                                                       field))
                            {
                              value = G_MAXDOUBLE;
                            }

                          if (field_info->meter_cumulative)
                            {
                              total += value;
                              value  = total;
                            }

                          sample[field_info->meter_value - 1] = value;
                        }
                    }

                  clay_meter_add_sample (group_data->meter, sample);

                  g_free (sample);
                }

              if (variables_changed)
                {
                  /* enqueue update source */
                  if (! priv->update_idle_id &&
                      gtk_widget_get_mapped (GTK_WIDGET (dashboard)))
                    {
                      priv->update_idle_id = g_idle_add_full (
                        G_PRIORITY_DEFAULT,
                        (GSourceFunc) clay_dashboard_update,
                        dashboard, NULL);
                    }

                  /* check for low swap space */
                  if (priv->low_swap_space_warning                      &&
                      priv->variables[VARIABLE_SWAP_OCCUPIED].available &&
                      priv->variables[VARIABLE_SWAP_LIMIT].available)
                    {
                      guint64 swap_occupied;
                      guint64 swap_limit;

                      swap_occupied = priv->variables[VARIABLE_SWAP_OCCUPIED].value.size;
                      swap_limit    = priv->variables[VARIABLE_SWAP_LIMIT].value.size;

                      if (! seen_low_swap_space &&
                          swap_occupied >= LOW_SWAP_SPACE_WARNING_ON * swap_limit)
                        {
                          if (! priv->low_swap_space_idle_id)
                            {
                              priv->low_swap_space_idle_id =
                                g_idle_add_full (G_PRIORITY_HIGH,
                                                 (GSourceFunc) clay_dashboard_low_swap_space,
                                                 dashboard, NULL);
                            }

                          seen_low_swap_space = TRUE;
                        }
                      else if (seen_low_swap_space &&
                               swap_occupied <= LOW_SWAP_SPACE_WARNING_OFF * swap_limit)
                        {
                          if (priv->low_swap_space_idle_id)
                            {
                              g_source_remove (priv->low_swap_space_idle_id);

                              priv->low_swap_space_idle_id = 0;
                            }

                          seen_low_swap_space = FALSE;
                        }
                    }
                }

              priv->update_now = FALSE;

              last_update_time = time;
            }

          last_sample_time = time;
        }
    }

  g_mutex_unlock (&priv->mutex);

  return NULL;
}

static gboolean
clay_dashboard_update (ClayDashboard *dashboard)
{
  ClayDashboardPrivate *priv = dashboard->priv;
  Group                 group;

  g_mutex_lock (&priv->mutex);

  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    clay_dashboard_update_group_values (dashboard, group);

  priv->update_idle_id = 0;

  g_mutex_unlock (&priv->mutex);

  return G_SOURCE_REMOVE;
}

static gboolean
clay_dashboard_low_swap_space (ClayDashboard *dashboard)
{
  ClayDashboardPrivate *priv = dashboard->priv;

  if (priv->clay)
    {
      GdkMonitor *monitor;
      gint        field;

      gtk_expander_set_expanded (priv->groups[GROUP_SWAP].expander, TRUE);

      for (field = 0; field < priv->groups[GROUP_SWAP].n_fields; field++)
        {
          const FieldInfo *field_info = &groups[GROUP_SWAP].fields[field];

          if (field_info->variable == VARIABLE_SWAP_OCCUPIED ||
              field_info->variable == VARIABLE_SWAP_LIMIT)
            {
              clay_dashboard_field_set_active (dashboard,
                                               GROUP_SWAP, field, TRUE);
            }
        }

      clay_dashboard_update_groups (dashboard);

      monitor = clay_get_monitor_at_pointer ();

      clay_window_strategy_show_dockable_dialog (
        CLAY_WINDOW_STRATEGY (clay_get_window_strategy (priv->clay)),
        priv->clay,
        clay_dialog_factory_get_singleton (),
        monitor,
        "clay-dashboard");

      g_mutex_lock (&priv->mutex);

      priv->low_swap_space_idle_id = 0;

      g_mutex_unlock (&priv->mutex);
    }

  return G_SOURCE_REMOVE;
}

static void
clay_dashboard_sample_function (ClayDashboard *dashboard,
                                Variable       variable)
{
  ClayDashboardPrivate *priv          = dashboard->priv;
  const VariableInfo   *variable_info = &variables[variable];
  VariableData         *variable_data = &priv->variables[variable];

  #define CALL_FUNC(result_type) \
    (((result_type (*) (void)) variable_info->data) ())

  switch (variable_info->type)
    {
    case VARIABLE_TYPE_BOOLEAN:
      variable_data->value.boolean = CALL_FUNC (gboolean);
      break;

    case VARIABLE_TYPE_INTEGER:
      variable_data->value.integer = CALL_FUNC (gint);
      break;

    case VARIABLE_TYPE_SIZE:
      variable_data->value.size = CALL_FUNC (guint64);
      break;

    case VARIABLE_TYPE_PERCENTAGE:
      variable_data->value.percentage = CALL_FUNC (gdouble);
      break;

    case VARIABLE_TYPE_DURATION:
      variable_data->value.duration = CALL_FUNC (gdouble);
      break;

    case VARIABLE_TYPE_RATE_OF_CHANGE:
      variable_data->value.rate_of_change = CALL_FUNC (gdouble);
      break;

    case VARIABLE_TYPE_SIZE_RATIO:
    case VARIABLE_TYPE_INT_RATIO:
      g_return_if_reached ();
      break;
    }

  #undef CALL_FUNC

  variable_data->available = TRUE;
}

static void
clay_dashboard_sample_gegl_config (ClayDashboard *dashboard,
                                   Variable       variable)
{
  clay_dashboard_sample_object (dashboard, G_OBJECT (gegl_config ()), variable);
}

static void
clay_dashboard_sample_gegl_stats (ClayDashboard *dashboard,
                                  Variable       variable)
{
  clay_dashboard_sample_object (dashboard, G_OBJECT (gegl_stats ()), variable);
}

static void
clay_dashboard_sample_variable_changed (ClayDashboard *dashboard,
                                        Variable       variable)
{
  ClayDashboardPrivate *priv          = dashboard->priv;
  const VariableInfo   *variable_info = &variables[variable];
  VariableData         *variable_data = &priv->variables[variable];
  Variable              var           = GPOINTER_TO_INT (variable_info->data);
  const VariableData   *var_data      = &priv->variables[var];
  gpointer              prev_value    = clay_dashboard_variable_get_data (
                                          dashboard, variable,
                                          sizeof (var_data->value));

  if (var_data->available)
    {
      variable_data->available     = TRUE;
      variable_data->value.boolean = memcmp (&var_data->value, prev_value,
                                             sizeof (var_data->value)) != 0;

      if (variable_data->value.boolean)
        memcpy (prev_value, &var_data->value, sizeof (var_data->value));
    }
  else
    {
      variable_data->available     = FALSE;
    }
}

static void
clay_dashboard_sample_variable_rate_of_change (ClayDashboard *dashboard,
                                               Variable       variable)
{
  typedef struct
  {
    gint64   last_time;
    gboolean last_available;
    gdouble  last_value;
  } Data;

  ClayDashboardPrivate *priv          = dashboard->priv;
  const VariableInfo   *variable_info = &variables[variable];
  VariableData         *variable_data = &priv->variables[variable];
  Variable              var           = GPOINTER_TO_INT (variable_info->data);
  const VariableData   *var_data      = &priv->variables[var];
  Data                 *data          = clay_dashboard_variable_get_data (
                                          dashboard, variable, sizeof (Data));
  gint64                time;

  time = g_get_monotonic_time ();

  if (time == data->last_time)
    return;

  variable_data->available = FALSE;

  if (var_data->available)
    {
      gdouble value = clay_dashboard_variable_to_double (dashboard, var);

      if (data->last_available)
        {
          variable_data->available = TRUE;
          variable_data->value.rate_of_change = (value - data->last_value) *
                                                G_TIME_SPAN_SECOND         /
                                                (time - data->last_time);
        }

      data->last_value = value;
    }

  data->last_time      = time;
  data->last_available = var_data->available;
}

static void
clay_dashboard_sample_swap_limit (ClayDashboard *dashboard,
                                  Variable       variable)
{
  typedef struct
  {
    guint64  free_space;
    gboolean has_free_space;
    gint64   last_check_time;
  } Data;

  ClayDashboardPrivate *priv          = dashboard->priv;
  VariableData         *variable_data = &priv->variables[variable];
  Data                 *data          = clay_dashboard_variable_get_data (
                                          dashboard, variable, sizeof (Data));
  gint64                time;

  /* we don't have a config option for limiting the swap size, so we simply
   * return the free space available on the filesystem containing the swap
   */

  time = g_get_monotonic_time ();

  if (time - data->last_check_time >= G_TIME_SPAN_SECOND)
    {
      gchar *swap_dir;

      g_object_get (gegl_config (),
                    "swap", &swap_dir,
                    NULL);

      data->free_space     = 0;
      data->has_free_space = FALSE;

      if (swap_dir)
        {
          GFile     *file;
          GFileInfo *info;

          file = g_file_new_for_path (swap_dir);

          info = g_file_query_filesystem_info (file,
                                               G_FILE_ATTRIBUTE_FILESYSTEM_FREE,
                                               NULL, NULL);

          if (info)
            {
              data->free_space     =
                g_file_info_get_attribute_uint64 (info,
                                                  G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
              data->has_free_space = TRUE;

              g_object_unref (info);
            }

          g_object_unref (file);

          g_free (swap_dir);
        }

      data->last_check_time = time;
    }

  variable_data->available = data->has_free_space;

  if (data->has_free_space)
    {
      variable_data->value.size = data->free_space;

      if (priv->variables[VARIABLE_SWAP_SIZE].available)
        {
          /* the swap limit is the sum of free_space and swap_size, since the
           * swap itself occupies space in the filesystem
           */
          variable_data->value.size +=
            priv->variables[VARIABLE_SWAP_SIZE].value.size;
        }
    }
}

#ifdef HAVE_CPU_GROUP

#ifdef HAVE_SYS_TIMES_H

static void
clay_dashboard_sample_cpu_usage (ClayDashboard *dashboard,
                                 Variable       variable)
{
  typedef struct
  {
    clock_t prev_clock;
    clock_t prev_usage;
  } Data;

  ClayDashboardPrivate *priv          = dashboard->priv;
  VariableData         *variable_data = &priv->variables[variable];
  Data                 *data          = clay_dashboard_variable_get_data (
                                          dashboard, variable, sizeof (Data));
  clock_t               curr_clock;
  clock_t               curr_usage;
  struct tms            tms;

  curr_clock = times (&tms);

  if (curr_clock == (clock_t) -1)
    {
      data->prev_clock = 0;

      variable_data->available = FALSE;

      return;
    }

  curr_usage = tms.tms_utime + tms.tms_stime;

  if (data->prev_clock && curr_clock != data->prev_clock)
    {
      variable_data->available         = TRUE;
      variable_data->value.percentage  = (gdouble) (curr_usage - data->prev_usage) /
                                                   (curr_clock - data->prev_clock);
      variable_data->value.percentage /= g_get_num_processors ();
    }
  else
    {
      variable_data->available         = FALSE;
    }

  data->prev_clock = curr_clock;
  data->prev_usage = curr_usage;
}

#elif defined (G_OS_WIN32)

static void
clay_dashboard_sample_cpu_usage (ClayDashboard *dashboard,
                                 Variable       variable)
{
  typedef struct
  {
    guint64 prev_time;
    guint64 prev_usage;
  } Data;

  ClayDashboardPrivate *priv            = dashboard->priv;
  VariableData         *variable_data   = &priv->variables[variable];
  Data                 *data            = clay_dashboard_variable_get_data (
                                            dashboard, variable, sizeof (Data));
  guint64               curr_time;
  guint64               curr_usage;
  FILETIME              system_time;
  FILETIME              process_creation_time;
  FILETIME              process_exit_time;
  FILETIME              process_kernel_time;
  FILETIME              process_user_time;

  if (! GetProcessTimes (GetCurrentProcess (),
                         &process_creation_time,
                         &process_exit_time,
                         &process_kernel_time,
                         &process_user_time))
    {
      data->prev_time = 0;

      variable_data->available = FALSE;

      return;
    }

  GetSystemTimeAsFileTime (&system_time);

  curr_time   = ((guint64) system_time.dwHighDateTime << 32) |
                 (guint64) system_time.dwLowDateTime;

  curr_usage  = ((guint64) process_kernel_time.dwHighDateTime << 32) |
                 (guint64) process_kernel_time.dwLowDateTime;
  curr_usage += ((guint64) process_user_time.dwHighDateTime << 32) |
                 (guint64) process_user_time.dwLowDateTime;

  if (data->prev_time && curr_time != data->prev_time)
    {
      variable_data->available         = TRUE;
      variable_data->value.percentage  = (gdouble) (curr_usage - data->prev_usage) /
                                                   (curr_time  - data->prev_time);
      variable_data->value.percentage /= g_get_num_processors ();
    }
  else
    {
      variable_data->available         = FALSE;
    }

  data->prev_time  = curr_time;
  data->prev_usage = curr_usage;
}

#endif /* G_OS_WIN32 */

static void
clay_dashboard_sample_cpu_active (ClayDashboard *dashboard,
                                  Variable       variable)
{
  typedef struct
  {
    gboolean active;
  } Data;

  ClayDashboardPrivate *priv          = dashboard->priv;
  VariableData         *variable_data = &priv->variables[variable];
  Data                 *data          = clay_dashboard_variable_get_data (
                                          dashboard, variable, sizeof (Data));
  gboolean              active        = FALSE;

  if (priv->variables[VARIABLE_CPU_USAGE].available)
    {
      if (! data->active)
        {
          active =
            priv->variables[VARIABLE_CPU_USAGE].value.percentage *
            g_get_num_processors () > CPU_ACTIVE_ON;
        }
      else
        {
          active =
            priv->variables[VARIABLE_CPU_USAGE].value.percentage *
            g_get_num_processors () > CPU_ACTIVE_OFF;
        }

      variable_data->available = TRUE;
    }
  else
    {
      variable_data->available = FALSE;
    }

  data->active                 = active;
  variable_data->value.boolean = active;
}

static void
clay_dashboard_sample_cpu_active_time (ClayDashboard *dashboard,
                                       Variable       variable)
{
  typedef struct
  {
    gint64 prev_time;
    gint64 active_time;
  } Data;

  ClayDashboardPrivate *priv          = dashboard->priv;
  VariableData         *variable_data = &priv->variables[variable];
  Data                 *data          = clay_dashboard_variable_get_data (
                                          dashboard, variable, sizeof (Data));
  gint64                curr_time;

  curr_time = g_get_monotonic_time ();

  if (priv->variables[VARIABLE_CPU_ACTIVE].available)
    {
      gboolean active = priv->variables[VARIABLE_CPU_ACTIVE].value.boolean;

      if (active && data->prev_time)
        data->active_time += curr_time - data->prev_time;
    }

  data->prev_time = curr_time;

  variable_data->available      = TRUE;
  variable_data->value.duration = data->active_time / 1000000.0;
}

#endif /* HAVE_CPU_GROUP */

#ifdef HAVE_MEMORY_GROUP
#ifdef PLATFORM_OSX
static void
clay_dashboard_sample_memory_used (ClayDashboard *dashboard,
                                   Variable       variable)
{
#ifdef TASK_VM_INFO_REV0_COUNT
  task_vm_info_data_t         info;
  mach_msg_type_number_t      infoCount;
#endif

  ClayDashboardPrivate        *priv          = dashboard->priv;
  VariableData                *variable_data = &priv->variables[variable];

  variable_data->available = FALSE;
#ifndef TASK_VM_INFO_REV0_COUNT /* phys_footprint added in REV1 */
  struct mach_task_basic_info info;
  mach_msg_type_number_t      infoCount      = MACH_TASK_BASIC_INFO_COUNT;

  if( task_info(mach_task_self (), MACH_TASK_BASIC_INFO,
                             (task_info_t)&info, &infoCount ) != KERN_SUCCESS )
    return;      /* Can't access? */

  variable_data->available  = TRUE;
  variable_data->value.size = info.resident_size;
#else
  infoCount = TASK_VM_INFO_COUNT;

  if( task_info(mach_task_self (), TASK_VM_INFO,
                             (task_info_t)&info, &infoCount ) != KERN_SUCCESS )
    return;      /* Can't access? */
  variable_data->available  = TRUE;
  variable_data->value.size = info.phys_footprint;
#endif /* ! TASK_VM_INFO_REV0_COUNT */
}

static void
clay_dashboard_sample_memory_available (ClayDashboard *dashboard,
                                        Variable       variable)
{
  ClayDashboardPrivate        *priv          = dashboard->priv;
  VariableData                *variable_data = &priv->variables[variable];
  vm_statistics_data_t        info;
  mach_msg_type_number_t      infoCount      = HOST_VM_INFO_COUNT;

  variable_data->available = FALSE;


  if( host_statistics(mach_host_self (), HOST_VM_INFO,
                             (host_info_t)&info, &infoCount ) != KERN_SUCCESS )
    return;      /* Can't access? */

  variable_data->available  = TRUE;
  variable_data->value.size = info.free_count * PAGE_SIZE;
}

#elif defined(G_OS_WIN32)
static void
clay_dashboard_sample_memory_used (ClayDashboard *dashboard,
                                   Variable       variable)
{
  ClayDashboardPrivate       *priv          = dashboard->priv;
  VariableData               *variable_data = &priv->variables[variable];
  PROCESS_MEMORY_COUNTERS     pmc           = {};

  variable_data->available = FALSE;

  if (! GetProcessMemoryInfo (GetCurrentProcess (),
                              (PPROCESS_MEMORY_COUNTERS) &pmc,
                              sizeof (pmc)) ||
      pmc.cb != sizeof (pmc))
    {
      return;
    }

  variable_data->available  = TRUE;
  /* PrivateUsage includes reserved but unused memory so we use WorkingSetSize, see #7346 */
  variable_data->value.size = pmc.WorkingSetSize;
}

static void
clay_dashboard_sample_memory_available (ClayDashboard *dashboard,
                                        Variable       variable)
{
  ClayDashboardPrivate *priv          = dashboard->priv;
  VariableData         *variable_data = &priv->variables[variable];
  MEMORYSTATUSEX        ms;

  variable_data->available = FALSE;

  ms.dwLength = sizeof (ms);

  if (! GlobalMemoryStatusEx (&ms))
    return;

  variable_data->available  = TRUE;
  variable_data->value.size = ms.ullAvailPhys;
}

#elif defined(__OpenBSD__)
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/sysctl.h>

static void
clay_dashboard_sample_memory_used (ClayDashboard *dashboard,
                                   Variable       variable)
{
  ClayDashboardPrivate *priv          = dashboard->priv;
  VariableData         *variable_data = &priv->variables[variable];
  struct rusage         rusage;

  variable_data->available = FALSE;

  if (getrusage (RUSAGE_SELF, &rusage) == -1)
    return;
  variable_data->available  = TRUE;
  variable_data->value.size = (guint64) (rusage.ru_maxrss * 1024);
}

static void
clay_dashboard_sample_memory_available (ClayDashboard *dashboard,
                                        Variable       variable)
{
  ClayDashboardPrivate *priv            = dashboard->priv;
  VariableData         *variable_data   = &priv->variables[variable];
  int                   mib[]           = { CTL_HW, HW_PHYSMEM64 };
  int64_t               result;
  size_t                sz              = sizeof(int64_t);

  variable_data->available = FALSE;

  if (sysctl (mib, 2, &result, &sz, NULL, 0) == -1)
    return;
  variable_data->available  = TRUE;
  variable_data->value.size = (guint64) result;
}

#else /* ! G_OS_WIN32 && ! PLATFORM_OSX */
static void
clay_dashboard_sample_memory_used (ClayDashboard *dashboard,
                                   Variable       variable)
{
  ClayDashboardPrivate *priv          = dashboard->priv;
  VariableData         *variable_data = &priv->variables[variable];
  static gboolean       initialized   = FALSE;
  static long           page_size;
  static gint           fd            = -1;
  gchar                 buffer[128];
  gint                  size;
  unsigned long long    resident;
  unsigned long long    shared;

  if (! initialized)
    {
      page_size = sysconf (_SC_PAGE_SIZE);

      if (page_size > 0)
        fd = open ("/proc/self/statm", O_RDONLY);

      initialized = TRUE;
    }

  variable_data->available = FALSE;

  if (fd < 0)
    return;

  if (lseek (fd, 0, SEEK_SET))
    return;

  size = read (fd, buffer, sizeof (buffer) - 1);

  if (size <= 0)
    return;

  buffer[size] = '\0';

#ifndef _UCRT
  if (sscanf (buffer, "%*u %llu %llu", &resident, &shared) != 2)
#else
  if (sscanf_s (buffer, "%*u %llu %llu", &resident, &shared) != 2)
#endif
    return;

  variable_data->available  = TRUE;
  variable_data->value.size = (guint64) (resident - shared) * page_size;
}

static void
clay_dashboard_sample_memory_available (ClayDashboard *dashboard,
                                        Variable       variable)
{
  ClayDashboardPrivate *priv            = dashboard->priv;
  VariableData         *variable_data   = &priv->variables[variable];
  static gboolean       initialized     = FALSE;
  static gint64         last_check_time = 0;
  static gint           fd;
  static guint64        available;
  static gboolean       has_available   = FALSE;
  gint64                time;

  if (! initialized)
    {
      fd = open ("/proc/meminfo", O_RDONLY);

      initialized = TRUE;
    }

  variable_data->available = FALSE;

  if (fd < 0)
    return;

  /* we don't have a config option for limiting the swap size, so we simply
   * return the free space available on the filesystem containing the swap
   */

  time = g_get_monotonic_time ();

  if (time - last_check_time >= G_TIME_SPAN_SECOND)
    {
      gchar  buffer[512];
      gint   size;
      gchar *str;

      last_check_time = time;

      has_available = FALSE;

      if (lseek (fd, 0, SEEK_SET))
        return;

      size = read (fd, buffer, sizeof (buffer) - 1);

      if (size <= 0)
        return;

      buffer[size] = '\0';

      str = strstr (buffer, "MemAvailable:");

      if (! str)
        return;

      available = strtoull (str + 13, &str, 0);

      if (! str)
        return;

      for (; *str; str++)
        {
          if (*str == 'k')
            {
              available <<= 10;
              break;
            }
          else if (*str == 'M')
            {
              available <<= 20;
              break;
            }
        }

      if (! *str)
        return;

      has_available = TRUE;
    }

  if (! has_available)
    return;

  variable_data->available  = TRUE;
  variable_data->value.size = available;
}

#endif

static void
clay_dashboard_sample_memory_size (ClayDashboard *dashboard,
                                   Variable       variable)
{
  ClayDashboardPrivate *priv          = dashboard->priv;
  VariableData         *variable_data = &priv->variables[variable];

  variable_data->value.size = clay_get_physical_memory_size ();
  variable_data->available  = variable_data->value.size > 0;
}

#endif /* HAVE_MEMORY_GROUP */

static void
clay_dashboard_sample_object (ClayDashboard *dashboard,
                              GObject       *object,
                              Variable       variable)
{
  ClayDashboardPrivate *priv          = dashboard->priv;
  GObjectClass         *klass         = G_OBJECT_GET_CLASS (object);
  const VariableInfo   *variable_info = &variables[variable];
  VariableData         *variable_data = &priv->variables[variable];

  variable_data->available = FALSE;

  switch (variable_info->type)
    {
    case VARIABLE_TYPE_BOOLEAN:
      if (g_object_class_find_property (klass, variable_info->data))
        {
          variable_data->available = TRUE;

          g_object_get (object,
                        variable_info->data, &variable_data->value.boolean,
                        NULL);
        }
      break;

    case VARIABLE_TYPE_INTEGER:
      if (g_object_class_find_property (klass, variable_info->data))
        {
          variable_data->available = TRUE;

          g_object_get (object,
                        variable_info->data, &variable_data->value.integer,
                        NULL);
        }
      break;

    case VARIABLE_TYPE_SIZE:
      if (g_object_class_find_property (klass, variable_info->data))
        {
          variable_data->available = TRUE;

          g_object_get (object,
                        variable_info->data, &variable_data->value.size,
                        NULL);
        }
      break;

    case VARIABLE_TYPE_SIZE_RATIO:
      {
        const gchar *antecedent = variable_info->data;
        const gchar *consequent = antecedent + strlen (antecedent) + 1;

        if (g_object_class_find_property (klass, antecedent) &&
            g_object_class_find_property (klass, consequent))
          {
            variable_data->available = TRUE;

            g_object_get (object,
                          antecedent, &variable_data->value.size_ratio.antecedent,
                          consequent, &variable_data->value.size_ratio.consequent,
                          NULL);
          }
      }
      break;

    case VARIABLE_TYPE_INT_RATIO:
      {
        const gchar *antecedent = variable_info->data;
        const gchar *consequent = antecedent + strlen (antecedent) + 1;

        if (g_object_class_find_property (klass, antecedent) &&
            g_object_class_find_property (klass, consequent))
          {
            variable_data->available = TRUE;

            g_object_get (object,
                          antecedent, &variable_data->value.int_ratio.antecedent,
                          consequent, &variable_data->value.int_ratio.consequent,
                          NULL);
          }
      }
      break;

    case VARIABLE_TYPE_PERCENTAGE:
      if (g_object_class_find_property (klass, variable_info->data))
        {
          variable_data->available = TRUE;

          g_object_get (object,
                        variable_info->data, &variable_data->value.percentage,
                        NULL);
        }
      break;

    case VARIABLE_TYPE_DURATION:
      if (g_object_class_find_property (klass, variable_info->data))
        {
          variable_data->available = TRUE;

          g_object_get (object,
                        variable_info->data, &variable_data->value.duration,
                        NULL);
        }
      break;

    case VARIABLE_TYPE_RATE_OF_CHANGE:
      if (g_object_class_find_property (klass, variable_info->data))
        {
          variable_data->available = TRUE;

          g_object_get (object,
                        variable_info->data, &variable_data->value.rate_of_change,
                        NULL);
        }
      break;
    }
}

static void
clay_dashboard_update_groups (ClayDashboard *dashboard)
{
  Group group;

  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    clay_dashboard_update_group (dashboard, group);
}

static void
clay_dashboard_update_group (ClayDashboard *dashboard,
                             Group          group)
{
  ClayDashboardPrivate *priv       = dashboard->priv;
  const GroupInfo      *group_info = &groups[group];
  GroupData            *group_data = &priv->groups[group];
  gint                  n_rows;
  gboolean              add_separator;
  gint                  field;

  gtk_widget_set_visible (GTK_WIDGET (group_data->expander),
                          group_data->active);

  if (! group_data->active)
    return;

  n_rows        = 0;
  add_separator = FALSE;

  for (field = 0; field < group_data->n_fields; field++)
    {
      const FieldInfo *field_info = &group_info->fields[field];
      const FieldData *field_data = &group_data->fields[field];

      if (field_info->variable != VARIABLE_SEPARATOR)
        {
          if (group_info->has_meter && field_info->meter_value)
            {
              clay_meter_set_value_active (group_data->meter,
                                           field_info->meter_value - 1,
                                           field_data->active);
            }

          if (field_data->active)
            {
              if (add_separator)
                {
                  add_separator = FALSE;
                  n_rows++;
                }

              n_rows++;
            }
        }
      else
        {
          if (n_rows > 0)
            add_separator = TRUE;
        }
    }

  clay_gtk_container_clear (GTK_CONTAINER (group_data->grid));

  n_rows        = 0;
  add_separator = FALSE;

  for (field = 0; field < group_data->n_fields; field++)
    {
      const FieldInfo *field_info = &group_info->fields[field];
      FieldData       *field_data = &group_data->fields[field];

      if (field_info->variable != VARIABLE_SEPARATOR)
        {
          const VariableInfo *variable_info = &variables[field_info->variable];
          GtkWidget          *separator;
          GtkWidget          *color_area;
          GtkWidget          *label;
          const gchar        *description;
          gchar              *str;

          if (! field_data->active)
            continue;

          description = g_dgettext (NULL, variable_info->description);

          if (add_separator)
            {
              separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
              gtk_widget_set_hexpand (separator, TRUE);
              gtk_grid_attach (group_data->grid, separator,
                               0, n_rows, 3, 1);
              gtk_widget_set_visible (separator, TRUE);

              add_separator = FALSE;
              n_rows++;
            }

          if (group_info->has_meter && field_info->meter_value)
            {
              GeglColor *color = gegl_color_new (NULL);

              gegl_color_set_pixel (color, babl_format ("R'G'B'A double"), variable_info->rgb);
              color_area = clay_color_area_new (color, CLAY_COLOR_AREA_FLAT, 0);
              clay_help_set_help_data (color_area, description,
                                       NULL);
              gtk_widget_set_size_request (color_area, 5, 5);
              gtk_widget_set_valign (color_area, GTK_ALIGN_CENTER);
              gtk_grid_attach (group_data->grid, color_area,
                               0, n_rows, 1, 1);
              gtk_widget_set_visible (color_area, TRUE);

              g_object_unref (color);
            }

          str = g_strdup_printf ("%s:",
                                 g_dpgettext2 (NULL, "dashboard-variable",
                                               field_info->title ?
                                                 field_info->title :
                                                 variable_info->title));

          label = gtk_label_new (str);
          clay_help_set_help_data (label, description,
                                   NULL);
          gtk_label_set_xalign (GTK_LABEL (label), 0.0);
          gtk_grid_attach (group_data->grid, label,
                           1, n_rows, 1, 1);
          gtk_widget_set_visible (label, TRUE);

          g_free (str);

          label = gtk_label_new (NULL);
          field_data->value_label = GTK_LABEL (label);
          clay_help_set_help_data (label, description,
                                   NULL);
          gtk_label_set_xalign (GTK_LABEL (label), 0.0);
          gtk_widget_set_hexpand (label, TRUE);
          gtk_grid_attach (group_data->grid, label,
                           2, n_rows, 1, 1);
          gtk_widget_set_visible (label, TRUE);

          n_rows++;
        }
      else
        {
          if (n_rows > 0)
            add_separator = TRUE;
        }
    }

  g_mutex_lock (&priv->mutex);

  clay_dashboard_update_group_values (dashboard, group);

  g_mutex_unlock (&priv->mutex);
}

static void
clay_dashboard_update_group_values (ClayDashboard *dashboard,
                                    Group          group)
{
  ClayDashboardPrivate *priv       = dashboard->priv;
  const GroupInfo      *group_info = &groups[group];
  GroupData            *group_data = &priv->groups[group];
  gdouble               limit      = 0.0;
  GString              *header_values;
  gint                  field;

  if (! group_data->active)
    return;

  if (group_info->has_meter)
    {
      if (group_info->meter_limit)
        {
          const VariableData *variable_data = &priv->variables[group_info->meter_limit];

          if (variable_data->available)
            {
              limit = clay_dashboard_variable_to_double (dashboard,
                                                        group_info->meter_limit);
            }
          else
            {
              for (field = 0; field < group_data->n_fields; field++)
                {
                  const FieldInfo *field_info = &group_info->fields[field];
                  const FieldData *field_data = &group_data->fields[field];

                  if (field_info->meter_value && field_data->active)
                    {
                      gdouble value;

                      value = clay_dashboard_variable_to_double (dashboard,
                                                                 field_info->variable);

                      limit = MAX (limit, value);
                    }
                }
            }

          clay_meter_set_range (group_data->meter, 0.0, limit);
        }

      if (group_info->meter_led)
        {
          gdouble         rgb[3] = {0.0, 0.0, 0.0};
          gboolean        active = FALSE;
          const Variable *var;

          for (var = group_info->meter_led; *var; var++)
            {
              if (clay_dashboard_variable_to_boolean (dashboard, *var))
                {
                  const VariableInfo *variable_info = &variables[*var];

                  rgb[0] = MAX (rgb[0], variable_info->rgb[0]);
                  rgb[1] = MAX (rgb[1], variable_info->rgb[1]);
                  rgb[2] = MAX (rgb[2], variable_info->rgb[2]);

                  active = TRUE;
                }
            }

          if (active)
            {
              GeglColor *color = gegl_color_new (NULL);

              gegl_color_set_pixel (color, babl_format ("R'G'B' double"), rgb);
              clay_meter_set_led_color (group_data->meter, color);
              g_object_unref (color);
            }

          clay_meter_set_led_active (group_data->meter, active);
        }
    }

  group_data->limit = limit;

  header_values = g_string_new (NULL);

  for (field = 0; field < group_data->n_fields; field++)
    {
      const FieldInfo *field_info = &group_info->fields[field];
      const FieldData *field_data = &group_data->fields[field];

      if (field_data->active)
        {
          gchar *text;

          text = clay_dashboard_field_to_string (dashboard,
                                                 group, field, TRUE);

          clay_dashboard_label_set_text (field_data->value_label, text);

          g_free (text);

          if (field_info->show_in_header)
            {
              text = clay_dashboard_field_to_string (dashboard,
                                                     group, field, FALSE);

              if (header_values->len > 0)
                g_string_append (header_values, ", ");

              g_string_append (header_values, text);

              g_free (text);
            }
        }
    }

  if (header_values->len > 0)
    {
      g_string_prepend (header_values, "(");
      g_string_append  (header_values, ")");
    }

  clay_dashboard_label_set_text (group_data->header_values_label,
                                 header_values->str);

  g_string_free (header_values, TRUE);
}

static void
clay_dashboard_group_set_active (ClayDashboard *dashboard,
                                 Group          group,
                                 gboolean       active)
{
  ClayDashboardPrivate *priv       = dashboard->priv;
  GroupData            *group_data = &priv->groups[group];

  if (active != group_data->active)
    {
      group_data->active = active;

      if (group_data->action)
        {
          g_signal_handlers_block_by_func (group_data->action,
                                           clay_dashboard_group_action_toggled,
                                           dashboard);

          clay_toggle_action_set_active (group_data->action, active);

          g_signal_handlers_unblock_by_func (group_data->action,
                                             clay_dashboard_group_action_toggled,
                                             dashboard);
        }
    }
}

static void
clay_dashboard_field_set_active (ClayDashboard *dashboard,
                                 Group          group,
                                 gint           field,
                                 gboolean       active)
{
  ClayDashboardPrivate *priv       = dashboard->priv;
  GroupData            *group_data = &priv->groups[group];
  FieldData            *field_data = &group_data->fields[field];

  if (active != field_data->active)
    {
      field_data->active = active;

      g_signal_handlers_block_by_func (field_data->menu_item,
                                       clay_dashboard_field_menu_item_toggled,
                                       dashboard);

      gtk_check_menu_item_set_active (field_data->menu_item, active);

      g_signal_handlers_unblock_by_func (field_data->menu_item,
                                         clay_dashboard_field_menu_item_toggled,
                                         dashboard);
    }
}

static void
clay_dashboard_reset_unlocked (ClayDashboard *dashboard)
{
  ClayDashboardPrivate *priv;
  Group                 group;

  priv = dashboard->priv;

  gegl_reset_stats ();

  clay_dashboard_reset_variables (dashboard);

  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    {
      GroupData *group_data = &priv->groups[group];

      if (group_data->meter)
        clay_meter_clear_history (group_data->meter);
    }
}

static void
clay_dashboard_reset_variables (ClayDashboard *dashboard)
{
  ClayDashboardPrivate *priv = dashboard->priv;
  Variable              variable;

  for (variable = FIRST_VARIABLE; variable < N_VARIABLES; variable++)
    {
      const VariableInfo *variable_info = &variables[variable];
      VariableData       *variable_data = &priv->variables[variable];

      if (variable_info->reset_func)
        variable_info->reset_func (dashboard, variable);

      g_clear_pointer (&variable_data->data, g_free);
      variable_data->data_size = 0;
    }
}

static gpointer
clay_dashboard_variable_get_data (ClayDashboard *dashboard,
                                  Variable       variable,
                                  gsize          size)
{
  ClayDashboardPrivate *priv          = dashboard->priv;
  VariableData         *variable_data = &priv->variables[variable];

  if (variable_data->data_size != size)
    {
      variable_data->data = g_realloc (variable_data->data, size);

      if (variable_data->data_size < size)
        {
          memset ((guint8 *) variable_data->data + variable_data->data_size,
                  0, size - variable_data->data_size);
        }

      variable_data->data_size = size;
    }

  return variable_data->data;
}

static gboolean
clay_dashboard_variable_to_boolean (ClayDashboard *dashboard,
                                    Variable       variable)
{
  ClayDashboardPrivate *priv          = dashboard->priv;
  const VariableInfo   *variable_info = &variables[variable];
  const VariableData   *variable_data = &priv->variables[variable];

  if (variable_data->available)
    {
      switch (variable_info->type)
        {
        case VARIABLE_TYPE_BOOLEAN:
          return variable_data->value.boolean;

        case VARIABLE_TYPE_INTEGER:
          return variable_data->value.integer != 0;

        case VARIABLE_TYPE_SIZE:
          return variable_data->value.size > 0;

        case VARIABLE_TYPE_SIZE_RATIO:
          return variable_data->value.size_ratio.antecedent != 0 &&
                 variable_data->value.size_ratio.consequent != 0;

        case VARIABLE_TYPE_INT_RATIO:
          return variable_data->value.int_ratio.antecedent != 0 &&
                 variable_data->value.int_ratio.consequent != 0;

        case VARIABLE_TYPE_PERCENTAGE:
          return variable_data->value.percentage != 0.0;

        case VARIABLE_TYPE_DURATION:
          return variable_data->value.duration != 0.0;

        case VARIABLE_TYPE_RATE_OF_CHANGE:
          return variable_data->value.rate_of_change != 0.0;
        }
    }

  return FALSE;
}

static gdouble
clay_dashboard_variable_to_double (ClayDashboard *dashboard,
                                   Variable       variable)
{
  ClayDashboardPrivate *priv          = dashboard->priv;
  const VariableInfo   *variable_info = &variables[variable];
  const VariableData   *variable_data = &priv->variables[variable];

  if (variable_data->available)
    {
      switch (variable_info->type)
        {
        case VARIABLE_TYPE_BOOLEAN:
          return variable_data->value.boolean ? 1.0 : 0.0;

        case VARIABLE_TYPE_INTEGER:
          return variable_data->value.integer;

        case VARIABLE_TYPE_SIZE:
          return variable_data->value.size;

        case VARIABLE_TYPE_SIZE_RATIO:
          if (variable_data->value.size_ratio.consequent)
            {
              return (gdouble) variable_data->value.size_ratio.antecedent /
                     (gdouble) variable_data->value.size_ratio.consequent;
            }
          break;

        case VARIABLE_TYPE_INT_RATIO:
          if (variable_data->value.int_ratio.consequent)
            {
              return (gdouble) variable_data->value.int_ratio.antecedent /
                     (gdouble) variable_data->value.int_ratio.consequent;
            }
          break;

        case VARIABLE_TYPE_PERCENTAGE:
          return variable_data->value.percentage;

        case VARIABLE_TYPE_DURATION:
          return variable_data->value.duration;

        case VARIABLE_TYPE_RATE_OF_CHANGE:
          return variable_data->value.rate_of_change;
        }
    }

  return 0.0;
}

static gchar *
clay_dashboard_field_to_string (ClayDashboard *dashboard,
                                Group          group,
                                gint           field,
                                gboolean       full)
{
  ClayDashboardPrivate *priv          = dashboard->priv;
  const GroupInfo      *group_info    = &groups[group];
  const GroupData      *group_data    = &priv->groups[group];
  const FieldInfo      *field_info    = &group_info->fields[field];
  const VariableInfo   *variable_info = &variables[field_info->variable];
  const VariableData   *variable_data = &priv->variables[field_info->variable];
  /* Translators: "N/A" is an abbreviation for "not available" */
  const gchar          *str           = C_("dashboard-value", "N/A");
  gboolean              static_str    = TRUE;
  gboolean              show_limit    = TRUE;

  if (variable_data->available)
    {
      switch (variable_info->type)
        {
        case VARIABLE_TYPE_BOOLEAN:
          str = variable_data->value.boolean ? C_("dashboard-value", "Yes") :
                                               C_("dashboard-value", "No");
          break;

        case VARIABLE_TYPE_INTEGER:
          str        = g_strdup_printf ("%d", variable_data->value.integer);
          static_str = FALSE;
          break;

        case VARIABLE_TYPE_SIZE:
          str        = g_format_size_full (variable_data->value.size,
                                           G_FORMAT_SIZE_IEC_UNITS);
          static_str = FALSE;
          break;

        case VARIABLE_TYPE_SIZE_RATIO:
          {
            if (variable_data->value.size_ratio.consequent)
              {
                gdouble value;

                value = 100.0 * variable_data->value.size_ratio.antecedent /
                                variable_data->value.size_ratio.consequent;

                str        = g_strdup_printf ("%d%%", SIGNED_ROUND (value));
                static_str = FALSE;
              }
          }
          break;

        case VARIABLE_TYPE_INT_RATIO:
          {
            gdouble  min;
            gdouble  max;
            gdouble  antecedent;
            gdouble  consequent;

            antecedent = variable_data->value.int_ratio.antecedent;
            consequent = variable_data->value.int_ratio.consequent;

            min = MIN (ABS (antecedent), ABS (consequent));
            max = MAX (ABS (antecedent), ABS (consequent));

            if (min)
              {
                antecedent /= min;
                consequent /= min;
              }
            else if (max)
              {
                antecedent /= max;
                consequent /= max;
              }

            if (max)
              {
                str        = g_strdup_printf ("%g:%g",
                                              RINT (100.0 * antecedent) / 100.0,
                                              RINT (100.0 * consequent) / 100.0);
                static_str = FALSE;
              }
          }
          break;

        case VARIABLE_TYPE_PERCENTAGE:
          str        = g_strdup_printf ("%d%%",
                                        SIGNED_ROUND (100.0 * variable_data->value.percentage));
          static_str = FALSE;
          show_limit = FALSE;
          break;

        case VARIABLE_TYPE_DURATION:
          str        = g_strdup_printf ("%02d:%02d:%04.1f",
                                        (gint) floor (variable_data->value.duration / 3600.0),
                                        (gint) floor (fmod (variable_data->value.duration / 60.0, 60.0)),
                                        floor (fmod (variable_data->value.duration, 60.0) * 10.0) / 10.0);
          static_str = FALSE;
          show_limit = FALSE;
          break;

        case VARIABLE_TYPE_RATE_OF_CHANGE:
          /* Translators:  This string reports the rate of change of a measured
           * value.  The "%g" is replaced by a certain quantity, and the "/s"
           * is an abbreviation for "per second".
           */
          str        = g_strdup_printf (_("%g/s"),
                                        variable_data->value.rate_of_change);
          static_str = FALSE;
          break;
        }

      if (show_limit                   &&
          variable_data->available     &&
          field_info->meter_value      &&
          ! field_info->meter_variable &&
          group_data->limit)
        {
          gdouble  value;
          gchar   *tmp;

          value = clay_dashboard_variable_to_double (dashboard,
                                                     field_info->variable);

          if (full)
            {
              tmp = g_strdup_printf ("%s (%d%%)",
                                     str,
                                     SIGNED_ROUND (100.0 * value /
                                                   group_data->limit));
            }
          else
            {
              tmp = g_strdup_printf ("%d%%",
                                     SIGNED_ROUND (100.0 * value /
                                                   group_data->limit));
            }

          if (! static_str)
            g_free ((gpointer) str);

          str        = tmp;
          static_str = FALSE;
        }
      else if (full                         &&
               field_info->meter_variable   &&
               variables[field_info->meter_variable].type ==
               VARIABLE_TYPE_RATE_OF_CHANGE &&
               priv->variables[field_info->meter_variable].available)
        {
          gdouble  value;
          gchar   *value_str;
          gchar   *rate_of_change_str;
          gchar   *tmp;

          value = clay_dashboard_variable_to_double (dashboard,
                                                     field_info->meter_variable);

          value_str = clay_dashboard_format_value (variable_info->type, value);

          rate_of_change_str = clay_dashboard_format_rate_of_change (value_str);

          g_free (value_str);

          tmp = g_strdup_printf ("%s (%s)", str, rate_of_change_str);

          g_free (rate_of_change_str);

          if (! static_str)
            g_free ((gpointer) str);

          str        = tmp;
          static_str = FALSE;
        }
    }

  if (static_str)
    return g_strdup (str);
  else
    return (gpointer) str;
}

static gboolean
clay_dashboard_log_printf (ClayDashboard *dashboard,
                           const gchar   *format,
                           ...)
{
  ClayDashboardPrivate *priv = dashboard->priv;
  va_list               args;
  gboolean              result;

  if (priv->log_error)
    return FALSE;

  va_start (args, format);

  result = g_output_stream_vprintf (priv->log_output,
                                    NULL, NULL,
                                    &priv->log_error,
                                    format, args);

  va_end (args);

  return result;
}

static gboolean
clay_dashboard_log_print_escaped (ClayDashboard *dashboard,
                                  const gchar   *string)
{
  ClayDashboardPrivate *priv = dashboard->priv;
  gchar                 buffer[1024];
  const gchar          *s;
  gint                  i;

  if (priv->log_error)
    return FALSE;

  i = 0;

  #define FLUSH()                                                 \
    G_STMT_START                                                  \
      {                                                           \
        if (! g_output_stream_write_all (priv->log_output,        \
                                         buffer, i, NULL,         \
                                         NULL, &priv->log_error)) \
          {                                                       \
            return FALSE;                                         \
          }                                                       \
                                                                  \
        i = 0;                                                    \
      }                                                           \
    G_STMT_END

  #define RESERVE(n)                   \
    G_STMT_START                       \
      {                                \
        if (i + (n) > sizeof (buffer)) \
          FLUSH ();                    \
      }                                \
    G_STMT_END

  for (s = string; *s; s++)
    {
      #define ESCAPE(from, to)                      \
        case from:                                  \
          RESERVE (sizeof (to) - 1);                \
          memcpy (&buffer[i], to, sizeof (to) - 1); \
          i += sizeof (to) - 1;                     \
          break;

      switch (*s)
        {
        ESCAPE ('"',  "&quot;")
        ESCAPE ('\'', "&apos;")
        ESCAPE ('<',  "&lt;")
        ESCAPE ('>',  "&gt;")
        ESCAPE ('&',  "&amp;")

        default:
          RESERVE (1);
          buffer[i++] = *s;
          break;
        }

      #undef ESCAPE
    }

  FLUSH ();

  #undef FLUSH
  #undef RESERVE

  return TRUE;
}

static gint64
clay_dashboard_log_time (ClayDashboard *dashboard)
{
  ClayDashboardPrivate *priv = dashboard->priv;

  return g_get_monotonic_time () - priv->log_start_time;
}

static void
clay_dashboard_log_sample (ClayDashboard *dashboard,
                           gboolean       variables_changed,
                           gboolean       include_current_thread)
{
  ClayDashboardPrivate *priv      = dashboard->priv;
  ClayBacktrace        *backtrace = NULL;
  GArray               *addresses = NULL;
  gboolean              empty     = TRUE;
  Variable              variable;

  #define NONEMPTY()                              \
    G_STMT_START                                  \
      {                                           \
        if (empty)                                \
          {                                       \
            clay_dashboard_log_printf (dashboard, \
                                       ">\n");    \
                                                  \
            empty = FALSE;                        \
          }                                       \
      }                                           \
    G_STMT_END

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "<sample id=\"%d\" t=\"%lld\"",
                             priv->log_n_samples,
                             (long long) clay_dashboard_log_time (dashboard));

  if (priv->log_n_samples == 0 || variables_changed)
    {
      NONEMPTY ();

      clay_dashboard_log_printf (dashboard,
                                 "<vars>\n");

      for (variable = FIRST_VARIABLE; variable < N_VARIABLES; variable++)
        {
          const VariableInfo *variable_info     = &variables[variable];
          const VariableData *variable_data     = &priv->variables[variable];
          VariableData       *log_variable_data = &priv->log_variables[variable];

          if (variable_info->exclude_from_log)
            continue;

          if (priv->log_n_samples > 0 &&
              ! memcmp (variable_data, log_variable_data,
                        sizeof (VariableData)))
            {
              continue;
            }

          *log_variable_data = *variable_data;

          if (variable_data->available)
            {
              #define LOG_VAR(format, ...)                          \
                clay_dashboard_log_printf (dashboard,               \
                                           "<%s>" format "</%s>\n", \
                                           variable_info->name,     \
                                           __VA_ARGS__,             \
                                           variable_info->name)

              #define LOG_VAR_FLOAT(value)                                  \
                G_STMT_START                                                \
                  {                                                         \
                    gchar buffer[G_ASCII_DTOSTR_BUF_SIZE];                  \
                                                                            \
                    LOG_VAR ("%s", g_ascii_dtostr (buffer, sizeof (buffer), \
                                                   value));                 \
                  }                                                         \
                G_STMT_END

              switch (variable_info->type)
                {
                case VARIABLE_TYPE_BOOLEAN:
                  LOG_VAR (
                    "%d",
                    variable_data->value.boolean);
                  break;

                case VARIABLE_TYPE_INTEGER:
                  LOG_VAR (
                    "%d",
                    variable_data->value.integer);
                  break;

                case VARIABLE_TYPE_SIZE:
                  LOG_VAR (
                    "%llu",
                    (unsigned long long) variable_data->value.size);
                  break;

                case VARIABLE_TYPE_SIZE_RATIO:
                  LOG_VAR (
                    "%llu/%llu",
                    (unsigned long long) variable_data->value.size_ratio.antecedent,
                    (unsigned long long) variable_data->value.size_ratio.consequent);
                  break;

                case VARIABLE_TYPE_INT_RATIO:
                  LOG_VAR (
                    "%d:%d",
                    variable_data->value.int_ratio.antecedent,
                    variable_data->value.int_ratio.consequent);
                  break;

                case VARIABLE_TYPE_PERCENTAGE:
                  LOG_VAR_FLOAT (
                    variable_data->value.percentage);
                  break;

                case VARIABLE_TYPE_DURATION:
                  LOG_VAR_FLOAT (
                    variable_data->value.duration);
                  break;

                case VARIABLE_TYPE_RATE_OF_CHANGE:
                  LOG_VAR_FLOAT (
                    variable_data->value.rate_of_change);
                  break;
                }

              #undef LOG_VAR
              #undef LOG_VAR_FLOAT
            }
          else
            {
              clay_dashboard_log_printf (dashboard,
                                         "<%s />\n",
                                         variable_info->name);
            }
        }

      clay_dashboard_log_printf (dashboard,
                                 "</vars>\n");
    }

  if (priv->log_params.backtrace)
    backtrace = clay_backtrace_new (include_current_thread);

  if (backtrace)
    {
      gboolean backtrace_empty = TRUE;
      gint     n_threads;
      gint     thread;

      #define BACKTRACE_NONEMPTY()                           \
        G_STMT_START                                         \
          {                                                  \
            if (backtrace_empty)                             \
              {                                              \
                NONEMPTY ();                                 \
                                                             \
                clay_dashboard_log_printf (dashboard,        \
                                           "<backtrace>\n"); \
                                                             \
                backtrace_empty = FALSE;                     \
              }                                              \
          }                                                  \
        G_STMT_END

      if (priv->log_backtrace)
        {
          n_threads = clay_backtrace_get_n_threads (priv->log_backtrace);

          for (thread = 0; thread < n_threads; thread++)
            {
              guintptr thread_id;

              thread_id = clay_backtrace_get_thread_id (priv->log_backtrace,
                                                        thread);

              if (clay_backtrace_find_thread_by_id (backtrace,
                                                    thread_id, thread) < 0)
                {
                  const gchar *thread_name;

                  BACKTRACE_NONEMPTY ();

                  thread_name =
                    clay_backtrace_get_thread_name (priv->log_backtrace,
                                                    thread);

                  clay_dashboard_log_printf (dashboard,
                                             "<thread id=\"%llu\"",
                                             (unsigned long long) thread_id);

                  if (thread_name)
                    {
                      clay_dashboard_log_printf (dashboard,
                                                 " name=\"");
                      clay_dashboard_log_print_escaped (dashboard, thread_name);
                      clay_dashboard_log_printf (dashboard,
                                                 "\"");
                    }

                  clay_dashboard_log_printf (dashboard,
                                             " />\n");
                }
            }
        }

      n_threads = clay_backtrace_get_n_threads (backtrace);

      for (thread = 0; thread < n_threads; thread++)
        {
          guintptr     thread_id;
          const gchar *thread_name;
          gint         last_running  = -1;
          gint         running;
          gint         last_n_frames = -1;
          gint         n_frames;
          gint         n_head        = 0;
          gint         n_tail        = 0;
          gint         frame;

          thread_id   = clay_backtrace_get_thread_id     (backtrace, thread);
          thread_name = clay_backtrace_get_thread_name   (backtrace, thread);

          running     = clay_backtrace_is_thread_running (backtrace, thread);
          n_frames    = clay_backtrace_get_n_frames      (backtrace, thread);

          if (priv->log_backtrace)
            {
              gint other_thread = clay_backtrace_find_thread_by_id (
                priv->log_backtrace, thread_id, thread);

              if (other_thread >= 0)
                {
                  gint n;
                  gint i;

                  last_running  = clay_backtrace_is_thread_running (
                    priv->log_backtrace, other_thread);
                  last_n_frames = clay_backtrace_get_n_frames (
                    priv->log_backtrace, other_thread);

                  n = MIN (n_frames, last_n_frames);

                  for (i = 0; i < n; i++)
                    {
                      if (clay_backtrace_get_frame_address (backtrace,
                                                            thread, i) !=
                          clay_backtrace_get_frame_address (priv->log_backtrace,
                                                            other_thread, i))
                        {
                          break;
                        }
                    }

                  n_head  = i;
                  n      -= i;

                  for (i = 0; i < n; i++)
                    {
                      if (clay_backtrace_get_frame_address (backtrace,
                                                            thread, -i - 1) !=
                          clay_backtrace_get_frame_address (priv->log_backtrace,
                                                            other_thread, -i - 1))
                        {
                          break;
                        }
                    }

                  n_tail = i;
                }
            }

          if (running         == last_running  &&
              n_frames        == last_n_frames &&
              n_head + n_tail == n_frames)
            {
              continue;
            }

          BACKTRACE_NONEMPTY ();

          clay_dashboard_log_printf (dashboard,
                                     "<thread id=\"%llu\"",
                                     (unsigned long long) thread_id);

          if (thread_name)
            {
              clay_dashboard_log_printf (dashboard,
                                         " name=\"");
              clay_dashboard_log_print_escaped (dashboard, thread_name);
              clay_dashboard_log_printf (dashboard,
                                         "\"");
            }

          clay_dashboard_log_printf (dashboard,
                                     " running=\"%d\"",
                                     running);

          if (n_head > 0)
            {
              clay_dashboard_log_printf (dashboard,
                                         " head=\"%d\"",
                                         n_head);
            }

          if (n_tail > 0)
            {
              clay_dashboard_log_printf (dashboard,
                                         " tail=\"%d\"",
                                         n_tail);
            }

          if (n_frames == 0 || n_head + n_tail < n_frames)
            {
              clay_dashboard_log_printf (dashboard,
                                          ">\n");

              for (frame = n_head; frame < n_frames - n_tail; frame++)
                {
                  guintptr address;

                  address = clay_backtrace_get_frame_address (backtrace,
                                                              thread, frame);

                  clay_dashboard_log_printf (dashboard,
                                             "<frame address=\"0x%llx\" />\n",
                                             (unsigned long long) address);

                  if (g_hash_table_add (priv->log_addresses,
                                        (gpointer) address) &&
                      priv->log_params.progressive)
                    {
                      if (! addresses)
                        {
                          addresses = g_array_new (FALSE, FALSE,
                                                   sizeof (guintptr));
                        }

                      g_array_append_val (addresses, address);
                    }
                }

              clay_dashboard_log_printf (dashboard,
                                         "</thread>\n");
            }
          else
            {
              clay_dashboard_log_printf (dashboard,
                                         " />\n");
            }
        }

      if (! backtrace_empty)
        {
          clay_dashboard_log_printf (dashboard,
                                     "</backtrace>\n");
        }

      #undef BACKTRACE_NONEMPTY
    }
  else if (priv->log_backtrace)
    {
      NONEMPTY ();

      clay_dashboard_log_printf (dashboard,
                                 "<backtrace />\n");
    }

  clay_backtrace_free (priv->log_backtrace);
  priv->log_backtrace = backtrace;

  if (empty)
    {
      clay_dashboard_log_printf (dashboard,
                                 " />\n");
    }
  else
    {
      clay_dashboard_log_printf (dashboard,
                                 "</sample>\n");
    }

  if (addresses)
    {
      clay_dashboard_log_write_address_map (dashboard,
                                            (guintptr *) addresses->data,
                                            addresses->len,
                                            NULL);

      g_array_free (addresses, TRUE);
    }

  if (priv->log_params.progressive)
    g_output_stream_flush (priv->log_output, NULL, NULL);

  #undef NONEMPTY

  priv->log_n_samples++;
}

static void
clay_dashboard_log_add_marker_unlocked (ClayDashboard *dashboard,
                                        const gchar   *description)
{
  ClayDashboardPrivate *priv = dashboard->priv;

  priv->log_n_markers++;

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "<marker id=\"%d\" t=\"%lld\"",
                             priv->log_n_markers,
                             (long long) clay_dashboard_log_time (dashboard));

  if (description && description[0])
    {
      clay_dashboard_log_printf (dashboard,
                                 ">\n");
      clay_dashboard_log_print_escaped (dashboard, description);
      clay_dashboard_log_printf (dashboard,
                                 "\n"
                                 "</marker>\n");
    }
  else
    {
      clay_dashboard_log_printf (dashboard,
                                 " />\n");
    }

  clay_dashboard_log_update_n_markers (dashboard);
}

static void
clay_dashboard_log_update_highlight (ClayDashboard *dashboard)
{
  ClayDashboardPrivate *priv = dashboard->priv;
  GtkReliefStyle        default_relief;

  gtk_widget_style_get (GTK_WIDGET (dashboard),
                        "button-relief", &default_relief,
                        NULL);

  clay_button_set_suggested (priv->log_record_button,
                             clay_dashboard_log_is_recording (dashboard),
                             default_relief);
}

static void
clay_dashboard_log_update_n_markers (ClayDashboard *dashboard)
{
  ClayDashboardPrivate *priv = dashboard->priv;
  gchar                 buffer[32];

  g_snprintf (buffer, sizeof (buffer), "%d", priv->log_n_markers + 1);

  gtk_label_set_text (priv->log_add_marker_label, buffer);
}

static gint
clay_dashboard_log_compare_addresses (gconstpointer a1,
                                      gconstpointer a2)
{
  guintptr address1 = *(const guintptr *) a1;
  guintptr address2 = *(const guintptr *) a2;

  if (address1 < address2)
    return -1;
  else if (address1 > address2)
    return +1;
  else
    return 0;
}

static void
clay_dashboard_log_write_address_map (ClayDashboard *dashboard,
                                      guintptr      *addresses,
                                      gint           n_addresses,
                                      ClayAsync     *async)
{
  ClayBacktraceAddressInfo infos[2];
  gint                     i;
  gint                     n;

  if (n_addresses == 0)
    return;

  qsort (addresses, n_addresses, sizeof (guintptr),
         clay_dashboard_log_compare_addresses);

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "<address-map>\n");

  n = 0;

  for (i = 0; i < n_addresses; i++)
    {
      ClayBacktraceAddressInfo       *info      = &infos[n       % 2];
      const ClayBacktraceAddressInfo *prev_info = &infos[(n + 1) % 2];

      if (async && clay_async_is_canceled (async))
        break;

      if (clay_backtrace_get_address_info (addresses[i], info))
        {
          gboolean empty = TRUE;

          #define NONEMPTY()                              \
            G_STMT_START                                  \
              {                                           \
                if (empty)                                \
                  {                                       \
                    clay_dashboard_log_printf (dashboard, \
                                               ">\n");    \
                                                          \
                    empty = FALSE;                        \
                  }                                       \
              }                                           \
            G_STMT_END

          clay_dashboard_log_printf (dashboard,
                                     "\n"
                                     "<address value=\"0x%llx\"",
                                     (unsigned long long) addresses[i]);

          if (n == 0 || strcmp (info->object_name, prev_info->object_name))
            {
              NONEMPTY ();

              if (info->object_name[0])
                {
                  clay_dashboard_log_printf (dashboard,
                                             "<object>");
                  clay_dashboard_log_print_escaped (dashboard,
                                                    info->object_name);
                  clay_dashboard_log_printf (dashboard,
                                             "</object>\n");
                }
              else
                {
                  clay_dashboard_log_printf (dashboard,
                                             "<object />\n");
                }
            }

          if (n == 0 || strcmp (info->symbol_name, prev_info->symbol_name))
            {
              NONEMPTY ();

              if (info->symbol_name[0])
                {
                  clay_dashboard_log_printf (dashboard,
                                             "<symbol>");
                  clay_dashboard_log_print_escaped (dashboard,
                                                    info->symbol_name);
                  clay_dashboard_log_printf (dashboard,
                                             "</symbol>\n");
                }
              else
                {
                  clay_dashboard_log_printf (dashboard,
                                             "<symbol />\n");
                }
            }

          if (n == 0 || info->symbol_address != prev_info->symbol_address)
            {
              NONEMPTY ();

              if (info->symbol_address)
                {
                  clay_dashboard_log_printf (dashboard,
                                             "<base>0x%llx</base>\n",
                                             (unsigned long long)
                                               info->symbol_address);
                }
              else
                {
                  clay_dashboard_log_printf (dashboard,
                                             "<base />\n");
                }
            }

          if (n == 0 || strcmp (info->source_file, prev_info->source_file))
            {
              NONEMPTY ();

              if (info->source_file[0])
                {
                  clay_dashboard_log_printf (dashboard,
                                             "<source>");
                  clay_dashboard_log_print_escaped (dashboard,
                                                    info->source_file);
                  clay_dashboard_log_printf (dashboard,
                                             "</source>\n");
                }
              else
                {
                  clay_dashboard_log_printf (dashboard,
                                             "<source />\n");
                }
            }

          if (n == 0 || info->source_line != prev_info->source_line)
            {
              NONEMPTY ();

              if (info->source_line)
                {
                  clay_dashboard_log_printf (dashboard,
                                             "<line>%d</line>\n",
                                             info->source_line);
                }
              else
                {
                  clay_dashboard_log_printf (dashboard,
                                             "<line />\n");
                }
            }

          if (empty)
            {
              clay_dashboard_log_printf (dashboard,
                                         " />\n");
            }
          else
            {
              clay_dashboard_log_printf (dashboard,
                                         "</address>\n");
            }

          #undef NONEMPTY

          n++;
        }
    }

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "</address-map>\n");
}

static void
clay_dashboard_log_write_global_address_map (ClayAsync     *async,
                                             ClayDashboard *dashboard)
{
  ClayDashboardPrivate *priv = dashboard->priv;
  gint                  n_addresses;

  n_addresses = g_hash_table_size (priv->log_addresses);

  if (n_addresses > 0)
    {
      guintptr *addresses;
      GList    *iter;
      gint      i;

      addresses = g_new (guintptr, n_addresses);

      for (iter = g_hash_table_get_keys (priv->log_addresses), i = 0;
           iter;
           iter = g_list_next (iter), i++)
        {
          addresses[i] = (guintptr) iter->data;
        }

      clay_dashboard_log_write_address_map (dashboard,
                                            addresses, n_addresses,
                                            async);

      g_free (addresses);
    }

  clay_async_finish (async, NULL);
}

static void
clay_dashboard_log_log_func (const gchar    *log_domain,
                             GLogLevelFlags  log_levels,
                             const gchar    *message,
                             ClayDashboard  *dashboard)
{
  ClayDashboardPrivate *priv      = dashboard->priv;
  const gchar          *log_level = NULL;
  gchar                *description;

  g_mutex_lock (&priv->mutex);

  switch (log_levels & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_ERROR:    log_level = "ERROR";    break;
    case G_LOG_LEVEL_CRITICAL: log_level = "CRITICAL"; break;
    case G_LOG_LEVEL_WARNING:  log_level = "WARNING";  break;
    case G_LOG_LEVEL_MESSAGE:  log_level = "MESSAGE";  break;
    case G_LOG_LEVEL_INFO:     log_level = "INFO";     break;
    case G_LOG_LEVEL_DEBUG:    log_level = "DEBUG";    break;
    default:                   log_level = "UNKNOWN";  break;
    }

  description = g_strdup_printf ("[%s] %s: %s", log_domain, log_level, message);

  clay_dashboard_log_add_marker_unlocked (dashboard, description);

  clay_dashboard_log_sample (dashboard, FALSE, TRUE);

  g_free (description);

  g_mutex_unlock (&priv->mutex);
}

static gboolean
clay_dashboard_field_use_meter_underlay (Group group,
                                         gint  field)
{
  const GroupInfo    *group_info = &groups[group];
  Variable            variable   = group_info->fields[field].variable;
  const VariableInfo *variable_info;

  if (group_info->fields[field].meter_variable)
    variable = group_info->fields[field].meter_variable;

  variable_info = &variables [variable];

  return variable_info->type == VARIABLE_TYPE_BOOLEAN ||
         (group_info->fields[field].meter_variable    &&
          variable_info->type == VARIABLE_TYPE_RATE_OF_CHANGE);
}

static gchar *
clay_dashboard_format_rate_of_change (const gchar *value)
{
  /* Translators:  This string reports the rate of change of a measured value.
   * The first "%s" is replaced by a certain quantity, usually followed by a
   * unit of measurement (e.g., "10 bytes"). and the final "/s" is an
   * abbreviation for "per second" (so the full string would read
   * "10 bytes/s", that is, "10 bytes per second".
   */
  return g_strdup_printf (_("%s/s"), value);
}

static gchar *
clay_dashboard_format_value (VariableType type,
                             gdouble      value)
{
  switch (type)
    {
    case VARIABLE_TYPE_BOOLEAN:
      return g_strdup (value ? C_("dashboard-value", "Yes") :
                               C_("dashboard-value", "No"));

    case VARIABLE_TYPE_INTEGER:
      return g_strdup_printf ("%g", value);

    case VARIABLE_TYPE_SIZE:
      return g_format_size_full (value, G_FORMAT_SIZE_IEC_UNITS);

    case VARIABLE_TYPE_SIZE_RATIO:
      if (isfinite (value))
        return g_strdup_printf ("%d%%", SIGNED_ROUND (100.0 * value));
      break;

    case VARIABLE_TYPE_INT_RATIO:
      if (isfinite (value))
        {
          gdouble  min;
          gdouble  max;
          gdouble  antecedent;
          gdouble  consequent;

          antecedent = value;
          consequent = 1.0;

          min = MIN (ABS (antecedent), ABS (consequent));
          max = MAX (ABS (antecedent), ABS (consequent));

          if (min)
            {
              antecedent /= min;
              consequent /= min;
            }
          else
            {
              antecedent /= max;
              consequent /= max;
            }

          return g_strdup_printf ("%g:%g",
                                  RINT (100.0 * antecedent) / 100.0,
                                  RINT (100.0 * consequent) / 100.0);
        }
      else if (isinf (value))
        {
          return g_strdup ("1:0");
        }
      break;

    case VARIABLE_TYPE_PERCENTAGE:
      return g_strdup_printf ("%d%%", SIGNED_ROUND (100.0 * value));

    case VARIABLE_TYPE_DURATION:
      return g_strdup_printf ("%02d:%02d:%04.1f",
                              (gint) floor (value / 3600.0),
                              (gint) floor (fmod (value / 60.0, 60.0)),
                              floor (fmod (value, 60.0) * 10.0) / 10.0);

    case VARIABLE_TYPE_RATE_OF_CHANGE:
      {
        gchar buf[64];

        g_snprintf (buf, sizeof (buf), "%g", value);

        return clay_dashboard_format_rate_of_change (buf);
      }
    }

  return g_strdup (_("N/A"));
}

static void
clay_dashboard_label_set_text (GtkLabel    *label,
                               const gchar *text)
{
  /* the strcmp() reduces the overhead of gtk_label_set_text() when the
   * text hasn't changed
   */
  if (g_strcmp0 (gtk_label_get_text (label), text))
    gtk_label_set_text (label, text);
}


/*  public functions  */


GtkWidget *
clay_dashboard_new (Clay            *clay,
                    ClayMenuFactory *menu_factory)
{
  ClayDashboard *dashboard;

  dashboard = g_object_new (CLAY_TYPE_DASHBOARD,
                            "menu-factory",    menu_factory,
                            "menu-identifier", "<Dashboard>",
                            "ui-path",         "/dashboard-popup",
                            NULL);

  dashboard->priv->clay = clay;

  return GTK_WIDGET (dashboard);
}

gboolean
clay_dashboard_log_start_recording (ClayDashboard                 *dashboard,
                                    GFile                         *file,
                                    const ClayDashboardLogParams  *params,
                                    GError                       **error)
{
  ClayDashboardPrivate  *priv;
  ClayUIManager         *ui_manager;
  ClayActionGroup       *action_group;
  gchar                 *version;
  gchar                **envp;
  gchar                **env;
  GParamSpec           **pspecs;
  guint                  n_pspecs;
  gboolean               has_backtrace;
  Variable               variable;
  guint                  i;

  g_return_val_if_fail (CLAY_IS_DASHBOARD (dashboard), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  priv = dashboard->priv;

  g_return_val_if_fail (! clay_dashboard_log_is_recording (dashboard), FALSE);

  if (! params)
    params = clay_dashboard_log_get_default_params (dashboard);

  priv->log_params = *params;

  if (g_getenv ("CLAY_PERFORMANCE_LOG_SAMPLE_FREQUENCY"))
    {
      priv->log_params.sample_frequency =
        atoi (g_getenv ("CLAY_PERFORMANCE_LOG_SAMPLE_FREQUENCY"));
    }

  if (g_getenv ("CLAY_PERFORMANCE_LOG_BACKTRACE"))
    {
      priv->log_params.backtrace =
        atoi (g_getenv ("CLAY_PERFORMANCE_LOG_BACKTRACE")) ? 1 : 0;
    }

  if (g_getenv ("CLAY_PERFORMANCE_LOG_MESSAGES"))
    {
      priv->log_params.messages =
        atoi (g_getenv ("CLAY_PERFORMANCE_LOG_MESSAGES")) ? 1 : 0;
    }

  if (g_getenv ("CLAY_PERFORMANCE_LOG_PROGRESSIVE"))
    {
      priv->log_params.progressive =
        atoi (g_getenv ("CLAY_PERFORMANCE_LOG_PROGRESSIVE")) ? 1 : 0;
    }

  priv->log_params.sample_frequency = CLAMP (priv->log_params.sample_frequency,
                                             LOG_SAMPLE_FREQUENCY_MIN,
                                             LOG_SAMPLE_FREQUENCY_MAX);

  g_mutex_lock (&priv->mutex);

  if (priv->log_params.progressive     &&
      g_file_query_exists (file, NULL) &&
      ! g_file_delete (file, NULL, error))
    {
      g_mutex_unlock (&priv->mutex);

      return FALSE;
    }

  priv->log_output = G_OUTPUT_STREAM (g_file_replace (file,
                                      NULL, FALSE, G_FILE_CREATE_NONE, NULL,
                                      error));

  if (! priv->log_output)
    {
      g_mutex_unlock (&priv->mutex);

      return FALSE;
    }

  priv->log_error      = NULL;
  priv->log_start_time = g_get_monotonic_time ();
  priv->log_n_samples  = 0;
  priv->log_n_markers  = 0;
  priv->log_backtrace  = NULL;
  priv->log_addresses  = g_hash_table_new (NULL, NULL);

  if (priv->log_params.backtrace)
    has_backtrace = clay_backtrace_start ();
  else
    has_backtrace = FALSE;

  clay_dashboard_log_printf (dashboard,
                             "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                             "<clay-performance-log version=\"%d\">\n",
                             LOG_VERSION);

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "<params>\n"
                             "<sample-frequency>%d</sample-frequency>\n"
                             "<backtrace>%d</backtrace>\n"
                             "<messages>%d</messages>\n"
                             "<progressive>%d</progressive>\n"
                             "</params>\n",
                             priv->log_params.sample_frequency,
                             has_backtrace,
                             priv->log_params.messages,
                             priv->log_params.progressive);

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "<info>\n");

  version = clay_version (TRUE, FALSE);

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "<clay-version>\n");
  clay_dashboard_log_print_escaped (dashboard, version);
  clay_dashboard_log_printf (dashboard,
                             "</clay-version>\n");

  g_free (version);

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "<env>\n");

  envp = g_get_environ ();

  for (env = envp; *env; env++)
    {
      if (g_str_has_prefix (*env, "BABL_") ||
          g_str_has_prefix (*env, "GEGL_") ||
          g_str_has_prefix (*env, "CLAY_"))
        {
          gchar       *delim = strchr (*env, '=');
          const gchar *s;

          if (! delim)
            continue;

          for (s = *env;
               s != delim && (g_ascii_isalnum (*s) || *s == '_' || *s == '-');
               s++);

          if (s != delim)
            continue;

          *delim = '\0';

          clay_dashboard_log_printf (dashboard,
                                     "<%s>",
                                     *env);
          clay_dashboard_log_print_escaped (dashboard, delim + 1);
          clay_dashboard_log_printf (dashboard,
                                     "</%s>\n",
                                     *env);
        }
    }

  g_strfreev (envp);

  clay_dashboard_log_printf (dashboard,
                             "</env>\n");

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "<gegl-config>\n");

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (gegl_config ()),
                                           &n_pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      const GParamSpec *pspec     = pspecs[i];
      GValue            value     = {};
      GValue            str_value = {};

      g_value_init (&value,     pspec->value_type);
      g_value_init (&str_value, G_TYPE_STRING);

      g_object_get_property (G_OBJECT (gegl_config ()), pspec->name, &value);

      if (g_value_transform (&value, &str_value))
        {
          clay_dashboard_log_printf (dashboard,
                                     "<%s>",
                                     pspec->name);
          clay_dashboard_log_print_escaped (dashboard,
                                            g_value_get_string (&str_value));
          clay_dashboard_log_printf (dashboard,
                                     "</%s>\n",
                                     pspec->name);
        }

      g_value_unset (&str_value);
      g_value_unset (&value);
    }

  g_free (pspecs);

  clay_dashboard_log_printf (dashboard,
                             "</gegl-config>\n");

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "</info>\n");

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "<var-defs>\n");

  for (variable = FIRST_VARIABLE; variable < N_VARIABLES; variable++)
    {
      const VariableInfo *variable_info = &variables[variable];
      const gchar        *type          = "";

      if (variable_info->exclude_from_log)
        continue;

      switch (variable_info->type)
        {
        case VARIABLE_TYPE_BOOLEAN:        type = "boolean";        break;
        case VARIABLE_TYPE_INTEGER:        type = "integer";        break;
        case VARIABLE_TYPE_SIZE:           type = "size";           break;
        case VARIABLE_TYPE_SIZE_RATIO:     type = "size-ratio";     break;
        case VARIABLE_TYPE_INT_RATIO:      type = "int-ratio";      break;
        case VARIABLE_TYPE_PERCENTAGE:     type = "percentage";     break;
        case VARIABLE_TYPE_DURATION:       type = "duration";       break;
        case VARIABLE_TYPE_RATE_OF_CHANGE: type = "rate-of-change"; break;
        }

      clay_dashboard_log_printf (dashboard,
                                 "<var name=\"%s\" type=\"%s\" desc=\"",
                                 variable_info->name,
                                 type);
      clay_dashboard_log_print_escaped (dashboard,
                                        /* intentionally untranslated */
                                        variable_info->description);
      clay_dashboard_log_printf (dashboard,
                                 "\" />\n");
    }

  clay_dashboard_log_printf (dashboard,
                             "</var-defs>\n");

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "<samples>\n");

  if (priv->log_error)
    {
      GCancellable *cancellable = g_cancellable_new ();

      clay_backtrace_stop ();

      /* Cancel the overwrite initiated by g_file_replace(). */
      g_cancellable_cancel (cancellable);
      g_output_stream_close (priv->log_output, cancellable, NULL);
      g_object_unref (cancellable);

      g_clear_object (&priv->log_output);

      g_propagate_error (error, priv->log_error);
      priv->log_error = NULL;

      g_mutex_unlock (&priv->mutex);

      return FALSE;
    }

  clay_dashboard_reset_unlocked (dashboard);

  if (priv->log_params.messages)
    {
      priv->log_log_handler = clay_log_set_handler (
        TRUE,
        G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
        (GLogFunc) clay_dashboard_log_log_func,
        dashboard);
    }

  priv->update_now = TRUE;
  g_cond_signal (&priv->cond);

  g_mutex_unlock (&priv->mutex);

  clay_dashboard_log_update_n_markers (dashboard);

  ui_manager   = clay_editor_get_ui_manager (CLAY_EDITOR (dashboard));
  action_group = clay_ui_manager_get_action_group (ui_manager, "dashboard");

  clay_action_group_update (action_group, dashboard);

  clay_dashboard_log_update_highlight (dashboard);

  return TRUE;
}

gboolean
clay_dashboard_log_stop_recording (ClayDashboard  *dashboard,
                                   GError        **error)
{
  ClayDashboardPrivate *priv;
  ClayUIManager        *ui_manager;
  ClayActionGroup      *action_group;
  gboolean              result = TRUE;

  g_return_val_if_fail (CLAY_IS_DASHBOARD (dashboard), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  priv = dashboard->priv;

  if (! clay_dashboard_log_is_recording (dashboard))
    return TRUE;

  g_mutex_lock (&priv->mutex);

  if (priv->log_log_handler)
    {
      clay_log_remove_handler (priv->log_log_handler);

      priv->log_log_handler = 0;
    }

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "</samples>\n");


  if (! priv->log_params.progressive &&
      g_hash_table_size (priv->log_addresses) > 0)
    {
      ClayAsync *async;

      async = clay_parallel_run_async_independent (
        (ClayRunAsyncFunc) clay_dashboard_log_write_global_address_map,
        dashboard);

      clay_wait (priv->clay, CLAY_WAITABLE (async),
                 _("Resolving symbol information..."));

      g_object_unref (async);
    }

  clay_dashboard_log_printf (dashboard,
                             "\n"
                             "</clay-performance-log>\n");

  if (priv->log_params.backtrace)
    clay_backtrace_stop ();

  if (! priv->log_error)
    {
      g_output_stream_close (priv->log_output, NULL, &priv->log_error);
    }
  else
    {
      GCancellable *cancellable = g_cancellable_new ();

      /* Cancel the overwrite initiated by g_file_replace(). */
      g_cancellable_cancel (cancellable);
      g_output_stream_close (priv->log_output, cancellable, NULL);
      g_object_unref (cancellable);
    }

  g_clear_object (&priv->log_output);

  if (priv->log_error)
    {
      g_propagate_error (error, priv->log_error);
      priv->log_error = NULL;

      result = FALSE;
    }

  g_clear_pointer (&priv->log_backtrace, clay_backtrace_free);
  g_clear_pointer (&priv->log_addresses, g_hash_table_unref);

  g_mutex_unlock (&priv->mutex);

  ui_manager   = clay_editor_get_ui_manager (CLAY_EDITOR (dashboard));
  action_group = clay_ui_manager_get_action_group (ui_manager, "dashboard");

  clay_action_group_update (action_group, dashboard);

  clay_dashboard_log_update_highlight (dashboard);

  return result;
}

gboolean
clay_dashboard_log_is_recording (ClayDashboard *dashboard)
{
  ClayDashboardPrivate *priv;

  g_return_val_if_fail (CLAY_IS_DASHBOARD (dashboard), FALSE);

  priv = dashboard->priv;

  return priv->log_output != NULL;
}

const ClayDashboardLogParams *
clay_dashboard_log_get_default_params (ClayDashboard *dashboard)
{
  static const ClayDashboardLogParams default_params =
  {
    .sample_frequency = LOG_DEFAULT_SAMPLE_FREQUENCY,
    .backtrace        = LOG_DEFAULT_BACKTRACE,
    .messages         = LOG_DEFAULT_MESSAGES,
    .progressive      = LOG_DEFAULT_PROGRESSIVE
  };

  g_return_val_if_fail (CLAY_IS_DASHBOARD (dashboard), NULL);

  return &default_params;
}

void
clay_dashboard_log_add_marker (ClayDashboard *dashboard,
                               const gchar   *description)
{
  ClayDashboardPrivate *priv;

  g_return_if_fail (CLAY_IS_DASHBOARD (dashboard));
  g_return_if_fail (clay_dashboard_log_is_recording (dashboard));

  priv = dashboard->priv;

  g_mutex_lock (&priv->mutex);

  clay_dashboard_log_add_marker_unlocked (dashboard, description);

  g_mutex_unlock (&priv->mutex);
}

void
clay_dashboard_reset (ClayDashboard *dashboard)
{
  ClayDashboardPrivate *priv;

  g_return_if_fail (CLAY_IS_DASHBOARD (dashboard));

  priv = dashboard->priv;

  g_mutex_lock (&priv->mutex);

  clay_dashboard_reset_unlocked (dashboard);

  priv->update_now = TRUE;
  g_cond_signal (&priv->cond);

  g_mutex_unlock (&priv->mutex);
}

void
clay_dashboard_set_update_interval (ClayDashboard              *dashboard,
                                    ClayDashboardUpdateInteval  update_interval)
{
  ClayDashboardPrivate *priv;

  g_return_if_fail (CLAY_IS_DASHBOARD (dashboard));

  priv = dashboard->priv;

  if (update_interval != priv->update_interval)
    {
      Group group;

      g_mutex_lock (&priv->mutex);

      priv->update_interval = update_interval;

      for (group = FIRST_GROUP; group < N_GROUPS; group++)
        {
          GroupData *group_data = &priv->groups[group];

          if (group_data->meter)
            {
              gdouble interval = update_interval;

              clay_meter_set_history_resolution (group_data->meter,
                                                 interval / 1000.0);
            }
        }

      priv->update_now = TRUE;
      g_cond_signal (&priv->cond);

      g_mutex_unlock (&priv->mutex);
    }
}

ClayDashboardUpdateInteval
clay_dashboard_get_update_interval (ClayDashboard *dashboard)
{
  g_return_val_if_fail (CLAY_IS_DASHBOARD (dashboard), DEFAULT_UPDATE_INTERVAL);

  return dashboard->priv->update_interval;
}

void
clay_dashboard_set_history_duration (ClayDashboard                *dashboard,
                                     ClayDashboardHistoryDuration  history_duration)
{
  ClayDashboardPrivate *priv;

  g_return_if_fail (CLAY_IS_DASHBOARD (dashboard));

  priv = dashboard->priv;

  if (history_duration != priv->history_duration)
    {
      Group group;

      g_mutex_lock (&priv->mutex);

      priv->history_duration = history_duration;

      for (group = FIRST_GROUP; group < N_GROUPS; group++)
        {
          GroupData *group_data = &priv->groups[group];

          if (group_data->meter)
            {
              clay_meter_set_history_duration (group_data->meter,
                                               (gdouble) history_duration / 1000.0);
            }
        }

      priv->update_now = TRUE;
      g_cond_signal (&priv->cond);

      g_mutex_unlock (&priv->mutex);
    }
}

ClayDashboardHistoryDuration
clay_dashboard_get_history_duration (ClayDashboard *dashboard)
{
  g_return_val_if_fail (CLAY_IS_DASHBOARD (dashboard), DEFAULT_HISTORY_DURATION);

  return dashboard->priv->history_duration;
}

void
clay_dashboard_set_low_swap_space_warning (ClayDashboard *dashboard,
                                           gboolean       low_swap_space_warning)
{
  ClayDashboardPrivate *priv;

  g_return_if_fail (CLAY_IS_DASHBOARD (dashboard));

  priv = dashboard->priv;

  if (low_swap_space_warning != priv->low_swap_space_warning)
    {
      g_mutex_lock (&priv->mutex);

      priv->low_swap_space_warning = low_swap_space_warning;

      g_mutex_unlock (&priv->mutex);
    }
}

gboolean
clay_dashboard_get_low_swap_space_warning (ClayDashboard *dashboard)
{
  g_return_val_if_fail (CLAY_IS_DASHBOARD (dashboard), DEFAULT_LOW_SWAP_SPACE_WARNING);

  return dashboard->priv->low_swap_space_warning;
}

void
clay_dashboard_menu_setup (ClayUIManager *manager,
                           const gchar   *ui_path)
{
  Group group;

  g_return_if_fail (CLAY_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  for (group = FIRST_GROUP; group < N_GROUPS; group++)
    {
      const GroupInfo *group_info = &groups[group];
      gchar           *action_name;

      action_name = g_strdup_printf ("dashboard-group-%s", group_info->name);

      clay_ui_manager_add_ui (manager, "/Dashboard Menu/Groups/[Groups]",
                              action_name, FALSE);

      g_free (action_name);
    }
}
