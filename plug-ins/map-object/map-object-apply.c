/******************************************************/
/* Apply mapping and shading on the whole input image */
/******************************************************/

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "map-object-main.h"
#include "map-object-image.h"
#include "map-object-shade.h"
#include "map-object-apply.h"

#include "libclay/stdplugins-intl.h"


/*************/
/* Main loop */
/*************/

gdouble       imat[4][4];
gfloat        rotmat[16];
static gfloat a[16], b[16];

void
init_compute (void)
{
  gint i;

  switch (mapvals.maptype)
    {
      case MAP_SPHERE:

        /* Rotate the equator/northpole axis */
        /* ================================= */

        clay_vector3_set (&mapvals.firstaxis,  0.0, 0.0, -1.0);
        clay_vector3_set (&mapvals.secondaxis, 0.0, 1.0,  0.0);

        clay_vector3_rotate (&mapvals.firstaxis,
                             clay_deg_to_rad (mapvals.alpha),
                             clay_deg_to_rad (mapvals.beta),
                             clay_deg_to_rad (mapvals.gamma));
        clay_vector3_rotate (&mapvals.secondaxis,
                             clay_deg_to_rad (mapvals.alpha),
                             clay_deg_to_rad (mapvals.beta),
                             clay_deg_to_rad (mapvals.gamma));

        /* Compute the 2D bounding box of the sphere spanned by the axis */
        /* ============================================================= */

        compute_bounding_box ();

        get_ray_color = get_ray_color_sphere;

        break;

      case MAP_PLANE:

        /* Rotate the plane axis */
        /* ===================== */

        clay_vector3_set (&mapvals.firstaxis,  1.0, 0.0, 0.0);
        clay_vector3_set (&mapvals.secondaxis, 0.0, 1.0, 0.0);
        clay_vector3_set (&mapvals.normal,     0.0, 0.0, 1.0);

        clay_vector3_rotate (&mapvals.firstaxis,
                             clay_deg_to_rad (mapvals.alpha),
                             clay_deg_to_rad (mapvals.beta),
                             clay_deg_to_rad (mapvals.gamma));
        clay_vector3_rotate (&mapvals.secondaxis,
                             clay_deg_to_rad (mapvals.alpha),
                             clay_deg_to_rad (mapvals.beta),
                             clay_deg_to_rad (mapvals.gamma));

        mapvals.normal = clay_vector3_cross_product (&mapvals.firstaxis,
                                                     &mapvals.secondaxis);

        if (mapvals.normal.z < 0.0)
          clay_vector3_mul (&mapvals.normal, -1.0);

        /* Initialize intersection matrix */
        /* ============================== */

        imat[0][1] = -mapvals.firstaxis.x;
        imat[1][1] = -mapvals.firstaxis.y;
        imat[2][1] = -mapvals.firstaxis.z;

        imat[0][2] = -mapvals.secondaxis.x;
        imat[1][2] = -mapvals.secondaxis.y;
        imat[2][2] = -mapvals.secondaxis.z;

        imat[0][3] = mapvals.position.x - mapvals.viewpoint.x;
        imat[1][3] = mapvals.position.y - mapvals.viewpoint.y;
        imat[2][3] = mapvals.position.z - mapvals.viewpoint.z;

        get_ray_color = get_ray_color_plane;

        break;

      case MAP_BOX:
        get_ray_color = get_ray_color_box;

        clay_vector3_set (&mapvals.firstaxis,  1.0, 0.0, 0.0);
        clay_vector3_set (&mapvals.secondaxis, 0.0, 1.0, 0.0);
        clay_vector3_set (&mapvals.normal,     0.0, 0.0, 1.0);

        ident_mat (rotmat);

        rotatemat (mapvals.alpha, &mapvals.firstaxis, a);

        matmul (a, rotmat, b);

        memcpy (rotmat, b, sizeof (gfloat) * 16);

        rotatemat (mapvals.beta, &mapvals.secondaxis, a);
        matmul (a, rotmat, b);

        memcpy (rotmat, b, sizeof (gfloat) * 16);

        rotatemat (mapvals.gamma, &mapvals.normal, a);
        matmul (a, rotmat, b);

        memcpy (rotmat, b, sizeof (gfloat) * 16);

        /* Set up pixel regions for the box face images */
        /* ============================================ */

        for (i = 0; i < 6; i++)
          {
            box_drawables[i] = clay_drawable_get_by_id (mapvals.boxmap_id[i]);

            box_buffers[i] = clay_drawable_get_buffer (box_drawables[i]);
          }

        break;

      case MAP_CYLINDER:
        get_ray_color = get_ray_color_cylinder;

        clay_vector3_set (&mapvals.firstaxis,  1.0, 0.0, 0.0);
        clay_vector3_set (&mapvals.secondaxis, 0.0, 1.0, 0.0);
        clay_vector3_set (&mapvals.normal,     0.0, 0.0, 1.0);

        ident_mat (rotmat);

        rotatemat (mapvals.alpha, &mapvals.firstaxis, a);

        matmul (a, rotmat, b);

        memcpy (rotmat, b, sizeof (gfloat) * 16);

        rotatemat (mapvals.beta, &mapvals.secondaxis, a);
        matmul (a, rotmat, b);

        memcpy (rotmat, b, sizeof (gfloat) * 16);

        rotatemat (mapvals.gamma, &mapvals.normal, a);
        matmul (a, rotmat, b);

        memcpy (rotmat, b, sizeof (gfloat) * 16);

        /* Set up pixel regions for the cylinder cap images */
        /* ================================================ */

        for (i = 0; i < 2; i++)
          {
            cylinder_drawables[i] = clay_drawable_get_by_id (mapvals.cylindermap_id[i]);;

            cylinder_buffers[i] = clay_drawable_get_buffer (cylinder_drawables[i]);
          }
        break;
    }

  max_depth = (gint) mapvals.maxdepth;
}

static void
render (gdouble   x,
        gdouble   y,
        gdouble  *col,
        gpointer  data)
{
  ClayVector3 pos;

  pos.x = x / (gdouble) width;
  pos.y = y / (gdouble) height;
  pos.z = 0.0;

  get_ray_color (&pos, col);
}

static void
show_progress (gint     min,
               gint     max,
               gint     curr,
               gpointer data)
{
  clay_progress_update ((gdouble) curr / (gdouble) max);
}

/**************************************************/
/* Performs map-to-sphere on the whole input image */
/* and updates or creates a new Clay image.       */
/**************************************************/

void
compute_image (void)
{
  gint         xcount, ycount;
  gdouble      color[4];
  glong        progress_counter = 0;
  ClayVector3  p;
  ClayImage   *new_image    = NULL;
  ClayLayer   *new_layer    = NULL;
  gboolean     insert_layer = FALSE;

  init_compute ();

  if (mapvals.create_new_image)
    new_image = clay_image_new (width, height, CLAY_RGB);
  else
    new_image = image;

  clay_image_undo_group_start (new_image);

  if (mapvals.create_new_image ||
      mapvals.create_new_layer ||
      (mapvals.transparent_background &&
       ! clay_drawable_has_alpha (output_drawable)))
    {
      gchar *layername[] = {_("Map to plane"),
                            _("Map to sphere"),
                            _("Map to box"),
                            _("Map to cylinder"),
                            _("Background")};

      new_layer = clay_layer_new (new_image,
                                  layername[mapvals.create_new_image ? 4 :
                                            mapvals.maptype],
                                  width, height,
                                  mapvals.transparent_background ?
                                  CLAY_RGBA_IMAGE :
                                  CLAY_RGB_IMAGE,
                                  100.0,
                                  clay_image_get_default_new_layer_mode (new_image));

      insert_layer = TRUE;
      output_drawable = CLAY_DRAWABLE (new_layer);
    }

  dest_buffer = clay_drawable_get_shadow_buffer (output_drawable);

  switch (mapvals.maptype)
    {
    case MAP_PLANE:
      clay_progress_init (_("Map to plane"));
      break;
    case MAP_SPHERE:
      clay_progress_init (_("Map to sphere"));
      break;
    case MAP_BOX:
      clay_progress_init (_("Map to box"));
      break;
    case MAP_CYLINDER:
      clay_progress_init (_("Map to cylinder"));
      break;
    }

  if (! mapvals.antialiasing)
    {
      for (ycount = 0; ycount < height; ycount++)
        {
          for (xcount = 0; xcount < width; xcount++)
            {
              p = int_to_pos (xcount, ycount);
              (* get_ray_color) (&p, color);
              poke (xcount, ycount, color, NULL);

              progress_counter++;
            }

          clay_progress_update ((gdouble) progress_counter /
                                (gdouble) maxcounter);
        }
    }
  else
    {
      clay_adaptive_supersample_area (0, 0,
                                      width - 1, height - 1,
                                      max_depth,
                                      mapvals.pixelthreshold,
                                      render,
                                      NULL,
                                      poke,
                                      NULL,
                                      show_progress,
                                      NULL);
    }

  clay_progress_update (1.0);

  g_object_unref (source_buffer);
  g_object_unref (dest_buffer);

  if (insert_layer)
    clay_image_insert_layer (new_image, new_layer, NULL, 0);

  clay_drawable_merge_shadow (output_drawable, TRUE);
  clay_drawable_update (output_drawable, 0, 0, width, height);

  if (new_image != image)
    {
      clay_display_new (new_image);
      clay_displays_flush ();
    }

  clay_image_undo_group_end (new_image);
}

void
copy_from_config (ClayProcedureConfig *config)
{
  ClayDrawable *box_front  = NULL;
  ClayDrawable *box_back   = NULL;
  ClayDrawable *box_top    = NULL;
  ClayDrawable *box_bottom = NULL;
  ClayDrawable *box_left   = NULL;
  ClayDrawable *box_right  = NULL;
  ClayDrawable *cyl_top    = NULL;
  ClayDrawable *cyl_bottom = NULL;
  GeglColor    *color;

  mapvals.maptype = clay_procedure_config_get_choice_id (config, "map-type");

  mapvals.lightsource.type =
    clay_procedure_config_get_choice_id (config, "light-type");

  g_object_get (config,
                "viewpoint-x",            &mapvals.viewpoint.x,
                "viewpoint-y",            &mapvals.viewpoint.y,
                "viewpoint-z",            &mapvals.viewpoint.z,
                "position-x",             &mapvals.position.x,
                "position-y",             &mapvals.position.y,
                "position-z",             &mapvals.position.z,
                "first-axis-x",           &mapvals.firstaxis.x,
                "first-axis-y",           &mapvals.firstaxis.y,
                "first-axis-z",           &mapvals.firstaxis.z,
                "second-axis-x",          &mapvals.secondaxis.x,
                "second-axis-y",          &mapvals.secondaxis.y,
                "second-axis-z",          &mapvals.secondaxis.z,
                "rotation-angle-x",       &mapvals.alpha,
                "rotation-angle-y",       &mapvals.beta,
                "rotation-angle-z",       &mapvals.gamma,
                "light-color",            &color,
                "light-position-x",       &mapvals.lightsource.position.x,
                "light-position-y",       &mapvals.lightsource.position.y,
                "light-position-z",       &mapvals.lightsource.position.z,
                "light-direction-x",      &mapvals.lightsource.direction.x,
                "light-direction-y",      &mapvals.lightsource.direction.y,
                "light-direction-z",      &mapvals.lightsource.direction.z,
                "ambient-intensity",      &mapvals.material.ambient_int,
                "diffuse-intensity",      &mapvals.material.diffuse_int,
                "diffuse-reflectivity",   &mapvals.material.diffuse_ref,
                "specular-reflectivity",  &mapvals.material.specular_ref,
                "highlight",              &mapvals.material.highlight,
                "antialiasing",           &mapvals.antialiasing,
                "depth",                  &mapvals.maxdepth,
                "threshold",              &mapvals.pixelthreshold,
                "tiled",                  &mapvals.tiled,
                "new-image",              &mapvals.create_new_image,
                "new-layer",              &mapvals.create_new_layer,
                "transparent-background", &mapvals.transparent_background,
                "sphere-radius",          &mapvals.radius,
                "box-front-drawable",     &box_front,
                "box-back-drawable",      &box_back,
                "box-top-drawable",       &box_top,
                "box-bottom-drawable",    &box_bottom,
                "box-left-drawable",      &box_left,
                "box-right-drawable",     &box_right,
                "x-scale",                &mapvals.scale.x,
                "y-scale",                &mapvals.scale.y,
                "z-scale",                &mapvals.scale.z,
                "cyl-top-drawable",       &cyl_top,
                "cyl-bottom-drawable",    &cyl_bottom,
                "cylinder-radius",        &mapvals.cylinder_radius,
                "cylinder-length",        &mapvals.cylinder_length,
                NULL);

  if (box_front)
    mapvals.boxmap_id[0] = clay_item_get_id (CLAY_ITEM (box_front));
  if (box_back)
    mapvals.boxmap_id[1] = clay_item_get_id (CLAY_ITEM (box_back));
  if (box_top)
    mapvals.boxmap_id[2] = clay_item_get_id (CLAY_ITEM (box_top));
  if (box_bottom)
    mapvals.boxmap_id[3] = clay_item_get_id (CLAY_ITEM (box_bottom));
  if (box_left)
    mapvals.boxmap_id[4] = clay_item_get_id (CLAY_ITEM (box_left));
  if (box_right)
    mapvals.boxmap_id[5] = clay_item_get_id (CLAY_ITEM (box_right));

  if (cyl_top)
    mapvals.cylindermap_id[0] = clay_item_get_id (CLAY_ITEM (cyl_top));
  if (cyl_bottom)
    mapvals.cylindermap_id[1] = clay_item_get_id (CLAY_ITEM (cyl_bottom));

  if (color == NULL)
    color = gegl_color_new ("white");

  /* TODO: Use GeglColor directly in this plug-in */
  gegl_color_get_pixel (color, babl_format ("R'G'B'A double"),
                        mapvals.lightsource.color);
  g_object_unref (color);
}
