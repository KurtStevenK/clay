/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycontroller.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef CLAY_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#error ClayController is unstable API under construction
#endif

#ifndef __CLAY_CONTROLLER_H__
#define __CLAY_CONTROLLER_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


/**
 * ClayControllerEventType:
 * @CLAY_CONTROLLER_EVENT_TRIGGER: the event is a simple trigger
 * @CLAY_CONTROLLER_EVENT_VALUE:   the event carries a double value
 *
 * Event types for #ClayController.
 **/
typedef enum
{
  CLAY_CONTROLLER_EVENT_TRIGGER,
  CLAY_CONTROLLER_EVENT_VALUE
} ClayControllerEventType;


typedef struct _ClayControllerEventAny     ClayControllerEventAny;
typedef struct _ClayControllerEventTrigger ClayControllerEventTrigger;
typedef struct _ClayControllerEventValue   ClayControllerEventValue;
typedef union  _ClayControllerEvent        ClayControllerEvent;

/**
 * ClayControllerEventAny:
 * @type:     The event's #ClayControllerEventType
 * @source:   The event's source #ClayController
 * @event_id: The event's ID
 *
 * Generic controller event. Every event has these three members at the
 * beginning of its struct
 **/
struct _ClayControllerEventAny
{
  ClayControllerEventType  type;
  ClayController          *source;
  gint                     event_id;
};

/**
 * ClayControllerEventTrigger:
 * @type:     The event's #ClayControllerEventType
 * @source:   The event's source #ClayController
 * @event_id: The event's ID
 *
 * Trigger controller event.
 **/
struct _ClayControllerEventTrigger
{
  ClayControllerEventType  type;
  ClayController          *source;
  gint                     event_id;
};

/**
 * ClayControllerEventValue:
 * @type:     The event's #ClayControllerEventType
 * @source:   The event's source #ClayController
 * @event_id: The event's ID
 * @value:    The event's value
 *
 * Value controller event.
 **/
struct _ClayControllerEventValue
{
  ClayControllerEventType  type;
  ClayController          *source;
  gint                     event_id;
  GValue                   value;
};

/**
 * ClayControllerEvent:
 * @type:    The event type
 * @any:     ClayControllerEventAny
 * @trigger: ClayControllerEventTrigger
 * @value:   ClayControllerEventValue
 *
 * A union to hjold all event event types
 **/
union _ClayControllerEvent
{
  ClayControllerEventType    type;
  ClayControllerEventAny     any;
  ClayControllerEventTrigger trigger;
  ClayControllerEventValue   value;
};


#define CLAY_TYPE_CONTROLLER (clay_controller_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayController, clay_controller, CLAY, CONTROLLER, GObject)

struct _ClayControllerClass
{
  GObjectClass  parent_class;

  const gchar  *name;
  const gchar  *help_domain;
  const gchar  *help_id;
  const gchar  *icon_name;

  /*  virtual functions  */
  gint          (* get_n_events)    (ClayController            *controller);
  const gchar * (* get_event_name)  (ClayController            *controller,
                                     gint                       event_id);
  const gchar * (* get_event_blurb) (ClayController            *controller,
                                     gint                       event_id);

  /*  signals  */
  gboolean      (* event)           (ClayController            *controller,
                                     const ClayControllerEvent *event);

  /* Padding for future expansion */
  void (* _clay_reserved0) (void);
  void (* _clay_reserved1) (void);
  void (* _clay_reserved2) (void);
  void (* _clay_reserved3) (void);
  void (* _clay_reserved4) (void);
  void (* _clay_reserved5) (void);
  void (* _clay_reserved6) (void);
  void (* _clay_reserved7) (void);
  void (* _clay_reserved8) (void);
  void (* _clay_reserved9) (void);
};


ClayController * clay_controller_new             (GType           controller_type);

gint             clay_controller_get_n_events    (ClayController *controller);
const gchar    * clay_controller_get_event_name  (ClayController *controller,
                                                  gint            event_id);
const gchar    * clay_controller_get_event_blurb (ClayController *controller,
                                                  gint            event_id);


/*  protected  */

gboolean         clay_controller_event (ClayController            *controller,
                                        const ClayControllerEvent *event);


G_END_DECLS

#endif /* __CLAY_CONTROLLER_H__ */
