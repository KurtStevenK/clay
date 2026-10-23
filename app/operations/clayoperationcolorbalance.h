/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationcolorbalance.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#pragma once

#include "clayoperationpointfilter.h"


#define CLAY_TYPE_OPERATION_COLOR_BALANCE            (clay_operation_color_balance_get_type ())
#define CLAY_OPERATION_COLOR_BALANCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_COLOR_BALANCE, ClayOperationColorBalance))
#define CLAY_OPERATION_COLOR_BALANCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_COLOR_BALANCE, ClayOperationColorBalanceClass))
#define CLAY_IS_OPERATION_COLOR_BALANCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_COLOR_BALANCE))
#define CLAY_IS_OPERATION_COLOR_BALANCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_COLOR_BALANCE))
#define CLAY_OPERATION_COLOR_BALANCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_COLOR_BALANCE, ClayOperationColorBalanceClass))


typedef struct _ClayOperationColorBalance      ClayOperationColorBalance;
typedef struct _ClayOperationColorBalanceClass ClayOperationColorBalanceClass;

struct _ClayOperationColorBalance
{
  ClayOperationPointFilter  parent_instance;
};

struct _ClayOperationColorBalanceClass
{
  ClayOperationPointFilterClass  parent_class;
};


GType   clay_operation_color_balance_get_type (void);
