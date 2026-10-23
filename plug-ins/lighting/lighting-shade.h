#ifndef __LIGHTING_SHADE_H__
#define __LIGHTING_SHADE_H__

typedef void (* get_ray_func) (ClayVector3 *vector,
                               gdouble     *color_sum);

void get_ray_color                 (ClayVector3 *position,
                                    gdouble     *color_sum);
void get_ray_color_no_bilinear     (ClayVector3 *position,
                                    gdouble     *color_sum);
void get_ray_color_ref             (ClayVector3 *position,
                                    gdouble     *color_sum);
void get_ray_color_no_bilinear_ref (ClayVector3 *position,
                                    gdouble     *color_sum);

void    precompute_init               (gint         w,
                                       gint         h);
void    precompute_normals            (gint         x1,
                                       gint         x2,
                                       gint         y);
void    interpol_row                  (gint         x1,
                                       gint         x2,
                                       gint         y);

#endif  /* __LIGHTING_SHADE_H__ */
