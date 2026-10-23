/*
 * TWAIN Plug-in
 * Copyright (C) 1999 Craig Setera
 * Craig Setera <setera@home.com>
 * 03/31/1999
 *
 * Updated for Mac OS X support
 * Brion Vibber <brion@pobox.com>
 * 07/22/2004
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
 *
 *
 * Based on (at least) the following plug-ins:
 * Screenshot
 * GIF
 * Randomize
 *
 * Any suggestions, bug-reports or patches are welcome.
 *
 * This plug-in interfaces to the TWAIN support library in order
 * to capture images from TWAIN devices directly into Clay images.
 * The plug-in is capable of acquiring the following type of
 * images:
 * - B/W (1 bit images translated to grayscale B/W)
 * - Grayscale up to 16 bits per pixel
 * - RGB up to 16 bits per sample (24, 30, 36, etc.)
 * - Paletted images (both Gray and RGB)
 *
 * Prerequisites:
 * Should compile and run on both Win32 and Mac OS X 10.3 (possibly
 * also on 10.2).
 *
 * Known problems:
 * - Multiple image transfers will hang the plug-in.  The current
 *   configuration compiles with a maximum of single image transfers.
 * - On Mac OS X, canceling doesn't always close things out fully.
 * - Epson TWAIN driver on Mac OS X crashes the plugin when scanning.
 */

/*
 * Revision history
 *  (02/07/99)  v0.1   First working version (internal)
 *  (02/09/99)  v0.2   First release to anyone other than myself
 *  (02/15/99)  v0.3   Added image dump and read support for debugging
 *  (03/31/99)  v0.5   Added support for multi-byte samples and paletted
 *                     images.
 *  (07/23/04)  v0.6   Added Mac OS X support.
 */
#include "config.h"

#include <glib.h>		/* Needed when compiling with gcc */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tw_platform.h"
#include "tw_local.h"

#include "libclay/clay.h"
#include "libclay/stdplugins-intl.h"

#include "tw_func.h"
#include "tw_util.h"

#ifdef _DEBUG
#include "tw_dump.h"
#endif /* _DEBUG */

/*
 * Plug-in Definitions
 */
#define PLUG_IN_NAME        "twain-acquire"
#define PLUG_IN_DESCRIPTION _("Capture an image from a TWAIN datasource")
#define PLUG_IN_HELP        _("This plug-in will capture an image from a TWAIN datasource")
#define PLUG_IN_AUTHOR      "Craig Setera (setera@home.com)"
#define PLUG_IN_COPYRIGHT   "Copyright 2004 by Craig Setera"
#define PLUG_IN_VERSION     "v0.6 (07/22/2004)"

#ifdef _DEBUG
#define PLUG_IN_D_NAME      "twain-acquire-dump"
#define PLUG_IN_R_NAME      "twain-acquire-read"
#endif /* _DEBUG */

/*
 * Application definitions
 */
#define MAX_IMAGES 1

/*
 * Definition of the run states
 */
#define RUN_STANDARD 0
#define RUN_DUMP     1
#define RUN_READDUMP 2

/* Global variables */
pTW_SESSION twSession = NULL;

#ifdef _DEBUG
static int twain_run_mode = RUN_STANDARD;
#endif

/* Forward declarations */
void preTransferCallback   (void             *clientData);
int  beginTransferCallback (pTW_IMAGEINFO     imageInfo,
                            void             *clientData);
int  dataTransferCallback  (pTW_IMAGEINFO     imageInfo,
                            pTW_IMAGEMEMXFER  imageMemXfer,
                            void             *clientData);
int  endTransferCallback   (int               completionState,
                            int               pendingCount,
                            void             *clientData);
void postTransferCallback  (int               pendingCount,
                            void             *clientData);


typedef struct _Twain      Twain;
typedef struct _TwainClass TwainClass;

struct _Twain
{
  ClayPlugIn      parent_instance;
};

struct _TwainClass
{
  ClayPlugInClass parent_class;
};


#define TWAIN_TYPE  (twain_get_type ())
#define TWAIN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TWAIN_TYPE, Twain))

GType                   twain_get_type         (void);

static GList          * twain_query_procedures (ClayPlugIn           *plug_in);
static ClayProcedure  * twain_create_procedure (ClayPlugIn           *plug_in,
                                                const gchar          *name);

static ClayValueArray * twain_run              (ClayProcedure        *procedure,
                                                ClayRunMode           run_mode,
                                                ClayImage            *image,
                                                ClayDrawable        **drawables,
                                                ClayProcedureConfig  *config,
                                                gpointer              run_data);

G_DEFINE_TYPE (Twain, twain, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (TWAIN_TYPE)
DEFINE_STD_SET_I18N


static void
twain_class_init (TwainClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = twain_query_procedures;
  plug_in_class->create_procedure = twain_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
twain_init (Twain *twain)
{
}

static GList *
twain_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_NAME));
}

static ClayProcedure *
twain_create_procedure (ClayPlugIn  *plug_in,
                        const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_NAME))
    {
      procedure = clay_image_procedure_new (plug_in, name,
                                            CLAY_PDB_PROC_TYPE_PLUGIN,
                                            twain_run, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");
      clay_procedure_set_sensitivity_mask (procedure,
                                           CLAY_PROCEDURE_SENSITIVE_DRAWABLE     |
                                           CLAY_PROCEDURE_SENSITIVE_DRAWABLES    |
                                           CLAY_PROCEDURE_SENSITIVE_NO_DRAWABLES |
                                           CLAY_PROCEDURE_SENSITIVE_NO_IMAGE);

      clay_procedure_set_menu_label (procedure, _("_Scanner/Camera..."));
      clay_procedure_add_menu_path (procedure, "<Image>/File/Create");

      clay_procedure_set_documentation (procedure,
                                        PLUG_IN_DESCRIPTION,
                                        PLUG_IN_HELP,
                                        name);
      clay_procedure_set_attribution (procedure,
                                      PLUG_IN_AUTHOR,
                                      PLUG_IN_COPYRIGHT,
                                      PLUG_IN_VERSION);

      clay_procedure_add_core_object_array_return_value (procedure, "images",
                                                         "Array of acquired images",
                                                         "Array of acquired images",
                                                         CLAY_TYPE_IMAGE,
                                                         G_PARAM_READWRITE);
     }

  return procedure;
}

#if 0
/* Data structure holding data between runs */
/* Currently unused... Eventually may be used
 * to track dialog data.
 */
typedef struct
{
  gchar  sourceName[34];
  gfloat xResolution;
  gfloat yResolution;
  gint   xOffset;
  gint   yOffset;
  gint   width;
  gint   height;
  gint   imageType;
} TwainValues;

/* Default Twain values */
static TwainValues twainvals =
{
  "",
  100.0, 100.0,
  0, 0,
  0, 0,
  TWPT_RGB
};
#endif

/* The standard callback functions */
TXFR_CB_FUNCS standardCbFuncs =
{
  preTransferCallback,
  beginTransferCallback,
  dataTransferCallback,
  endTransferCallback,
  postTransferCallback
};

/******************************************************************
 * Dump handling
 ******************************************************************/

#ifdef _DEBUG
/* The dumper callback functions */
TXFR_CB_FUNCS dumperCbFuncs =
{
  dumpPreTransferCallback,
  dumpBeginTransferCallback,
  dumpDataTransferCallback,
  dumpEndTransferCallback,
  dumpPostTransferCallback
};

void
setRunMode (char *argv[])
{
  char *exeName = strrchr (argv[0], '\\') + 1;

  LogMessage ("Executable name: %s\n", exeName);

  if (!_stricmp (exeName, DUMP_NAME))
    twain_run_mode = RUN_DUMP;

  if (!_stricmp (exeName, READDUMP_NAME))
    twain_run_mode = RUN_READDUMP;
}
#endif /* _DEBUG */


int
scanImage (void)
{
#ifdef _DEBUG
  if (twain_run_mode == RUN_READDUMP)
    {
      readDumpedImage (twSession);
      return 0;
    }
  else
#endif /* _DEBUG */
    return getImage (twSession);
}

/*
 * initTwainAppIdentity
 *
 * Initialize and return our application's identity for
 * the TWAIN runtime.
 */
static pTW_IDENTITY
getAppIdentity (void)
{
  pTW_IDENTITY appIdentity = g_new (TW_IDENTITY, 1);

  /* Set up the application identity */
  appIdentity->Id = 0;
  appIdentity->Version.MajorNum = 0;
  appIdentity->Version.MinorNum = 1;
  appIdentity->Version.Language = TWLG_USA;
  appIdentity->Version.Country = TWCY_USA;
  strcpy(appIdentity->Version.Info, "Clay TWAIN 0.6");
  appIdentity->ProtocolMajor = TWON_PROTOCOLMAJOR;
  appIdentity->ProtocolMinor = TWON_PROTOCOLMINOR;
  appIdentity->SupportedGroups = DG_IMAGE;
  strcpy(appIdentity->Manufacturer, "Craig Setera");
  strcpy(appIdentity->ProductFamily, "Clay");
  strcpy(appIdentity->ProductName, "Clay");

  return appIdentity;
}

/*
 * initializeTwain
 *
 * Do the necessary TWAIN initialization.  This sets up
 * our TWAIN session information.  The session stuff is
 * something built by me on top of the standard TWAIN
 * datasource manager calls.
 */
pTW_SESSION
initializeTwain (void)
{
  pTW_IDENTITY appIdentity;

  /* Get our application's identity */
  appIdentity = getAppIdentity ();

  /* Create a new session object */
  twSession = newSession (appIdentity);

  /* Register our image transfer callback functions */
#ifdef _DEBUG
  if (twain_run_mode == RUN_DUMP)
    registerTransferCallbacks (twSession, &dumperCbFuncs, NULL);
  else
#endif /* _DEBUG */
    registerTransferCallbacks (twSession, &standardCbFuncs, NULL);

  return twSession;
}

/******************************************************************
 * Clay Plug-in entry points
 ******************************************************************/

/* Return values storage */
static GList *image_list  = NULL;
static gint   image_count = 0;

/*
 * run
 *
 * The plug-in is being requested to run.
 * Capture an image from a TWAIN datasource
 */
static ClayValueArray *
twain_run (ClayProcedure        *procedure,
           ClayRunMode           run_mode,
           ClayImage            *image,
           ClayDrawable        **drawables,
           ClayProcedureConfig  *config,
           gpointer              run_data)
{
  /* Initialize the return values
   * Always return at least the status to the caller.
   */
  ClayPDBStatusType   status      = CLAY_PDB_SUCCESS;
  ClayValueArray     *return_vals = NULL;
  ClayImage         **images;
  GList              *list;
  gint                num_images;
  gint                i;

  gegl_init (NULL, NULL);

  /* Before we get any further, verify that we have
   * TWAIN and that there is actually a datasource
   * to be used in doing the acquire.
   */
  if (! twainIsAvailable ())
    {
      GError *error = NULL;

      g_set_error (&error, CLAY_PLUG_IN_ERROR, 0,
                   _("TWAIN driver not found, scanning not available"));
      return clay_procedure_new_return_values (procedure, CLAY_PDB_EXECUTION_ERROR,
                                               error);
    }

  if (run_mode == CLAY_RUN_NONINTERACTIVE)
    /* Currently, we don't do non-interactive calls.
     * Bail if someone tries to call us non-interactively
     */
    return clay_procedure_new_return_values (procedure, CLAY_PDB_CALLING_ERROR, NULL);

  /* Have we succeeded so far? */
  if (status == CLAY_PDB_SUCCESS)
    twainMain ();

  /* Check to make sure we got at least one valid
   * image.
   */
  if (image_count > 0)
    {
      /* An image was captured from the TWAIN
       * datasource.  Do final Interactive
       * steps.
       */

      num_images = g_list_length (image_list);
      images     = g_new0 (ClayImage *, num_images + 1);

      for (list = image_list, i = 0;
           list;
           list = g_list_next (list), i++)
        {
          images[i] = list->data;
        }

      g_list_free (image_list);

      /* Set return values */
      return_vals = clay_procedure_new_return_values (procedure, status, NULL);
      CLAY_VALUES_TAKE_CORE_OBJECT_ARRAY (return_vals, 1, images);

      return return_vals;
    }
  else
    {
      return clay_procedure_new_return_values (procedure, CLAY_PDB_EXECUTION_ERROR,
                                               NULL);
    }
}

/***********************************************************************
 * Image transfer callback functions
 ***********************************************************************/

/* Data used to carry data between each of
 * the callback function calls.
 */
typedef struct
{
  ClayImage    *image;
  ClayLayer    *layer;
  GeglBuffer   *buffer;
  const Babl   *format;
  pTW_PALETTE8  paletteData;
  int           totalPixels;
  int           completedPixels;
} ClientDataStruct, *pClientDataStruct;

/*
 * preTransferCallback
 *
 * This callback function is called before any images
 * are transferred.  Set up the one time only stuff.
 */
void
preTransferCallback (void *clientData)
{
  /* Initialize our progress dialog */
  clay_progress_init (_("Transferring data from scanner/camera"));
}

/*
 * beginTransferCallback
 *
 * The following function is called at the beginning
 * of each image transfer.
 */
int
beginTransferCallback (pTW_IMAGEINFO  imageInfo,
                       void          *clientData)
{
  pClientDataStruct theClientData = g_new (ClientDataStruct, 1);

  const Babl        *format;
  ClayImageBaseType  imageType;
  ClayImageType      layerType;
  ClayPrecision      precision;

  gint               bpc = imageInfo->BitsPerPixel /
                           imageInfo->SamplesPerPixel;


#ifdef _DEBUG
  logBegin (imageInfo, clientData);
#endif

  /* Decide on the image type */
  switch (imageInfo->PixelType)
    {
    case TWPT_BW:
      /* Set up the image and layer types */
      imageType = CLAY_GRAY;
      layerType = CLAY_GRAY_IMAGE;
      precision = CLAY_PRECISION_U8_NON_LINEAR;
      format    = babl_format ("Y' u8");
      break;

    case TWPT_GRAY:
      /* Set up the image and layer types */
      imageType = CLAY_GRAY;
      layerType = CLAY_GRAY_IMAGE;

      switch (bpc)
        {
        case 8:
          precision = CLAY_PRECISION_U8_NON_LINEAR;
          format    = babl_format ("Y' u8");
          break;

        case 16:
          precision = CLAY_PRECISION_U16_NON_LINEAR;
          format    = babl_format ("Y' u16");
          break;

        default:
          return FALSE;
        }
      break;

    case TWPT_RGB:
      /* Set up the image and layer types */
      imageType = CLAY_RGB;
      layerType = CLAY_RGB_IMAGE;

      switch (bpc)
        {
        case 8:
          precision = CLAY_PRECISION_U8_NON_LINEAR;
          format    = babl_format ("R'G'B' u8");
          break;

        case 16:
          precision = CLAY_PRECISION_U16_NON_LINEAR;
          format    = babl_format ("R'G'B' u16");
          break;

        default:
          return FALSE;
        }
      break;

    case TWPT_PALETTE:
      /* Get the palette data */
      theClientData->paletteData = g_new (TW_PALETTE8, 1);
      twSession->twRC = callDSM (APP_IDENTITY (twSession),
                                 DS_IDENTITY (twSession),
                                 DG_IMAGE, DAT_PALETTE8, MSG_GET,
                                 (TW_MEMREF) theClientData->paletteData);
      if (twSession->twRC != TWRC_SUCCESS)
        return FALSE;

      switch (theClientData->paletteData->PaletteType)
        {
        case TWPA_RGB:
          /* Set up the image and layer types */
          imageType = CLAY_RGB;
          layerType = CLAY_RGB_IMAGE;
          precision = CLAY_PRECISION_U8_NON_LINEAR;

          format = babl_format ("R'G'B' u8");
          break;

        case TWPA_GRAY:
          /* Set up the image and layer types */
          imageType = CLAY_GRAY;
          layerType = CLAY_GRAY_IMAGE;
          precision = CLAY_PRECISION_U8_NON_LINEAR;

          format = babl_format ("Y' u8");
          break;

        default:
          return FALSE;
        }
      break;

    default:
      /* We don't know how to deal with anything other than
       * the types listed above.  Bail for any other image
       * type.
       */
      return FALSE;
    }

  /* Create the Clay image */
  theClientData->image = clay_image_new_with_precision (imageInfo->ImageWidth,
                                                        imageInfo->ImageLength,
                                                        imageType,
                                                        precision);

  /* Set the actual resolution */
  clay_image_set_resolution (theClientData->image,
                             FIX32ToFloat (imageInfo->XResolution),
                             FIX32ToFloat (imageInfo->YResolution));
  clay_image_set_unit (theClientData->image, clay_unit_inch ());

  /* Create a layer */
  theClientData->layer = clay_layer_new (theClientData->image,
                                         _("Background"),
                                         imageInfo->ImageWidth,
                                         imageInfo->ImageLength,
                                         layerType, 100,
                                         CLAY_LAYER_MODE_NORMAL);

  /* Add the layer to the image */
  clay_image_insert_layer (theClientData->image,
                           theClientData->layer, NULL, 0);

  /* Update the progress dialog */
  theClientData->totalPixels     = imageInfo->ImageWidth * imageInfo->ImageLength;
  theClientData->completedPixels = 0;

  clay_progress_update (0.0);

  theClientData->buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (theClientData->layer));
  theClientData->format = format;

  /* Store our client data for the data transfer callbacks */
  if (clientData)
    g_free (clientData);

  setClientData (twSession, (void *) theClientData);

  /* Make sure to return TRUE to continue the image
   * transfer
   */
  return TRUE;
}

/*
 * bitTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source if
 * the image type is Black/White.
 *
 * Black and white data is unpacked from bit data
 * into byte data and written into a gray scale Clay
 * image.
 */
static char bitMasks[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
static int
bitTransferCallback (pTW_IMAGEINFO     imageInfo,
                     pTW_IMAGEMEMXFER  imageMemXfer,
                     void             *clientData)
{
  int   row, col, offset;
  char *srcBuf;
  char *destBuf;
  int   rows = imageMemXfer->Rows;
  int   cols = imageMemXfer->Columns;
  pClientDataStruct theClientData = (pClientDataStruct) clientData;

  /* Allocate a buffer as necessary */
  destBuf = gegl_scratch_new (char, rows * cols);

  /* Unpack the image data from bits into bytes */
  srcBuf = (char *) imageMemXfer->Memory.TheMem;
  offset = 0;
  for (row = 0; row < rows; row++)
    {
      for (col = 0; col < cols; col++)
        {
          char byte = srcBuf[(row * imageMemXfer->BytesPerRow) + (col / 8)];
          destBuf[offset++] = ((byte & bitMasks[col % 8]) != 0) ? 255 : 0;
        }
    }

  /* Update the complete chunk */
  gegl_buffer_set (theClientData->buffer,
                   GEGL_RECTANGLE (imageMemXfer->XOffset, imageMemXfer->YOffset,
                                   cols, rows), 0,
                   theClientData->format, destBuf,
                   GEGL_AUTO_ROWSTRIDE);

  /* Free the buffer */
  gegl_scratch_free (destBuf);

  /* Update the user on our progress */
  theClientData->completedPixels += (cols * rows);
  clay_progress_update ((double) theClientData->completedPixels /
                        (double) theClientData->totalPixels);

  return TRUE;
}

/*
 * directTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source if
 * the image type is Grayscale or RGB.
 */
static int
directTransferCallback (pTW_IMAGEINFO     imageInfo,
                        pTW_IMAGEMEMXFER  imageMemXfer,
                        void             *clientData)
{
  int rows = imageMemXfer->Rows;
  int cols = imageMemXfer->Columns;
  pClientDataStruct theClientData = (pClientDataStruct) clientData;

  /* Update the complete chunk */
  gegl_buffer_set (theClientData->buffer,
                   GEGL_RECTANGLE (imageMemXfer->XOffset, imageMemXfer->YOffset,
                                   cols, rows), 0,
                   theClientData->format, imageMemXfer->Memory.TheMem,
                   imageMemXfer->BytesPerRow);

  /* Update the user on our progress */
  theClientData->completedPixels += (cols * rows);
  clay_progress_update ((double) theClientData->completedPixels /
                        (double) theClientData->totalPixels);

  return TRUE;
}

/*
 * palettedTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source if
 * the image type is paletted.  This does not create
 * an indexed image type in Clay because for some
 * reason it does not allow creation of a specific
 * palette.  This function will create an RGB or Gray
 * image and use the palette to set the details of
 * the pixels.
 */
static int
palettedTransferCallback (pTW_IMAGEINFO     imageInfo,
                          pTW_IMAGEMEMXFER  imageMemXfer,
                          void             *clientData)
{
  int   channelsPerEntry;
  int   row, col;
  int   rows = imageMemXfer->Rows;
  int   cols = imageMemXfer->Columns;
  char *destBuf;
  char *destPtr = NULL;
  char *srcPtr = NULL;

  /* Get the client data */
  pClientDataStruct theClientData = (pClientDataStruct) clientData;

  /* Look up the palette entry size */
  channelsPerEntry =
    (theClientData->paletteData->PaletteType == TWPA_RGB) ? 3 : 1;

  /* Allocate a buffer as necessary */
  destBuf = gegl_scratch_new (char, rows * cols * channelsPerEntry);

  /* Work through the rows */
  destPtr = destBuf;
  for (row = 0; row < rows; row++)
    {
      srcPtr = (char *) ((char *) imageMemXfer->Memory.TheMem +
                         (row * imageMemXfer->BytesPerRow));

      /* Work through the columns */
      for (col = 0; col < cols; col++)
        {
          /* Get the palette index */
          int index = (unsigned char) *srcPtr;

          srcPtr++;

          switch (theClientData->paletteData->PaletteType)
            {
            case TWPA_GRAY:
              *destPtr = theClientData->paletteData->Colors[index].Channel1;
              destPtr++;
              break;

            case TWPA_RGB:
              *destPtr = theClientData->paletteData->Colors[index].Channel1;
              destPtr++;
              *destPtr = theClientData->paletteData->Colors[index].Channel2;
              destPtr++;
              *destPtr = theClientData->paletteData->Colors[index].Channel3;
              destPtr++;
            }
        }
    }

  /* Send the complete chunk */
  gegl_buffer_set (theClientData->buffer,
                   GEGL_RECTANGLE (imageMemXfer->XOffset, imageMemXfer->YOffset,
                                   cols, rows), 0,
                   theClientData->format, destBuf,
                   GEGL_AUTO_ROWSTRIDE);

  /* Free the buffer */
  gegl_scratch_free (destBuf);

  /* Update the user on our progress */
  theClientData->completedPixels += (cols * rows);
  clay_progress_update ((double) theClientData->completedPixels /
                        (double) theClientData->totalPixels);

  return TRUE;
}

/*
 * dataTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source.
 */
int
dataTransferCallback (pTW_IMAGEINFO     imageInfo,
                      pTW_IMAGEMEMXFER  imageMemXfer,
                      void             *clientData)
{
#ifdef _DEBUG
  logData (imageInfo, imageMemXfer, clientData);
#endif

  /* Choose the appropriate transfer handler */
  switch (imageInfo->PixelType)
    {
    case TWPT_PALETTE:
      return palettedTransferCallback (imageInfo, imageMemXfer, clientData);

    case TWPT_BW:
      return bitTransferCallback (imageInfo, imageMemXfer, clientData);

    case TWPT_GRAY:
    case TWPT_RGB:
      return directTransferCallback (imageInfo, imageMemXfer, clientData);

    default:
      return FALSE;
    }
}

/*
 * endTransferCallback
 *
 * The following function is called at the end of the
 * image transfer.  The caller will be handed
 * the image transfer completion state.  The
 * following values (defined in twain.h) are
 * possible:
 *
 * TWRC_XFERDONE
 *  The transfer completed successfully
 * TWRC_CANCEL
 *  The transfer was completed by the user
 * TWRC_FAILURE
 *  The transfer failed.
 */
int
endTransferCallback (int   completionState,
                     int   pendingCount,
                     void *clientData)
{
  pClientDataStruct theClientData = (pClientDataStruct) clientData;

  LogMessage ("endTransferCallback: CompState = %d, pending = %d\n",
              completionState, pendingCount);

  /* Clean up and detach from the drawable */
  g_object_unref (theClientData->buffer);

  /* Make sure to check our return code */
  if (completionState == TWRC_XFERDONE)
    {
      /* We have a completed image transfer */
      image_list = g_list_append (image_list, theClientData->image);
      image_count++;

      /* Display the image */
      LogMessage ("Displaying image %d\n",
                  clay_image_get_id (theClientData->image));
      clay_display_new (theClientData->image);
    }
  else
    {
      /* The transfer did not complete successfully */
      LogMessage ("Deleting image\n");
      clay_image_delete (theClientData->image);
    }

  /* Shut down if we have received all of the possible images */
  return (image_count < MAX_IMAGES);
}

/*
 * postTransferCallback
 *
 * This callback function will be called
 * after all possible images have been
 * transferred.
 */
void
postTransferCallback (int   pendingCount,
                      void *clientData)
{
  /* Shut things down. */
  if (pendingCount != 0)
    cancelPendingTransfers(twSession);

  /* This will close the datasource and datasource
   * manager.  Then the message queue will be shut
   * down and the run() procedure will finally be
   * able to finish.
   */
  disableDS (twSession);
  closeDS (twSession);
  closeDSM (twSession);

  /* Post a message to close up the application */
  twainQuitApplication ();
}
