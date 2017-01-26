/*
 * This file is a part of hildon
 *
 * Copyright (C) 2008 Nokia Corporation, all rights reserved.
 *
 * Contact: Rodrigo Novo <rodrigo.novo@nokia.com>
 *
 * This widget is based on MokoFingerScroll from libmokoui
 * OpenMoko Application Framework UI Library
 * Authored by Chris Lord <chris@openedhand.com>
 * Copyright (C) 2006-2007 OpenMoko Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser Public License as published by
 * the Free Software Foundation; version 2 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser Public License for more details.
 *
 */

/**
 * SECTION: hildon-pannable-area
 * @short_description: A scrolling widget designed for touch screens
 * @see_also: #GtkScrolledWindow
 *
 * #HildonPannableArea is a container widget that can be "panned" (scrolled)
 * up and down using the touchscreen with fingers. The widget has no scrollbars,
 * but it rather shows small scroll indicators to give an idea of the part of the
 * content that is visible at a time. The scroll indicators appear when a dragging
 * motion is started on the pannable area.
 *
 * The scrolling is "kinetic", meaning the motion can be "flicked" and it will
 * continue from the initial motion by gradually slowing down to an eventual stop.
 * The motion can also be stopped immediately by pressing the touchscreen over the
 * pannable area.
 */

#undef HILDON_DISABLE_DEPRECATED

#include <math.h>
#include <cairo.h>
#include <gdk/gdk.h>

#include "hildon-pannable-area.h"
#include "hildon-marshalers.h"
#include "hildon-enum-types.h"

#define SCROLL_BAR_MIN_SIZE 5
#define RATIO_TOLERANCE 0.000001
#define SCROLL_FADE_IN_TIMEOUT 50
#define SCROLL_FADE_TIMEOUT 100
#define MOTION_EVENTS_PER_SECOND 25
#define CURSOR_STOPPED_TIMEOUT 200
#define MAX_SPEED_THRESHOLD 280
#define PANNABLE_MAX_WIDTH 788
#define PANNABLE_MAX_HEIGHT 378
#define ACCEL_FACTOR 27
#define MIN_ACCEL_THRESHOLD 40
#define FAST_CLICK 125

struct _HildonPannableAreaPrivate {
  HildonPannableAreaMode mode;
  HildonMovementMode mov_mode;
  GdkWindow *event_window;
  gdouble x;		/* Used to store mouse co-ordinates of the first or */
  gdouble y;		/* previous events in a press-motion pair */
  gdouble ex;		/* Used to store mouse co-ordinates of the last */
  gdouble ey;		/* motion event in acceleration mode */
  gboolean enabled;
  gboolean button_pressed;
  guint32 last_time;	/* Last event time, to stop infinite loops */
  guint32 last_press_time;
  gint last_type;
  gboolean last_in;
  gboolean moved;
  gdouble vmin;
  gdouble vmax;
  gdouble vmax_overshooting;
  gdouble accel_vel_x;
  gdouble accel_vel_y;
  gdouble vfast_factor;
  gdouble decel;
  gdouble drag_inertia;
  gdouble scroll_time;
  gdouble vel_factor;
  guint sps;
  guint panning_threshold;
  guint scrollbar_fade_delay;
  guint bounce_steps;
  guint force;
  guint direction_error_margin;
  gdouble vel_x;
  gdouble vel_y;
  gdouble old_vel_x;
  gdouble old_vel_y;
  GdkWindow *child;
  gint child_width;
  gint child_height;
  gint ix;			/* Initial click mouse co-ordinates */
  gint iy;
  gint cx;			/* Initial click child window mouse co-ordinates */
  gint cy;
  guint idle_id;
  gdouble scroll_to_x;
  gdouble scroll_to_y;
  gdouble motion_x;
  gdouble motion_y;
  gint overshot_dist_x;
  gint overshot_dist_y;
  gint overshooting_y;
  gint overshooting_x;
  gdouble scroll_indicator_alpha;
  gint motion_event_scroll_timeout;
  gint scroll_indicator_timeout;
  gint scroll_indicator_event_interrupt;
  gint scroll_delay_counter;
  gint vovershoot_max;
  gint hovershoot_max;
  gboolean fade_in;
  gboolean initial_hint;
  gboolean initial_effect;
  gboolean low_friction_mode;
  gboolean first_drag;

  gboolean size_request_policy;
  gboolean hscroll_visible;
  gboolean vscroll_visible;
  GdkRectangle hscroll_rect;
  GdkRectangle vscroll_rect;
  guint indicator_width;

  GtkAdjustment *hadjust;
  GtkAdjustment *vadjust;
  gint x_offset;
  gint y_offset;

  GtkPolicyType vscrollbar_policy;
  GtkPolicyType hscrollbar_policy;

  GdkColor scroll_color;

  gboolean center_on_child_focus;
  gboolean center_on_child_focus_pending;

  gboolean selection_movement;

  // NEW from GtkAdjustment
  gdouble lower;
  gdouble upper;
  gdouble vvalue;
  gdouble hvalue;
  gdouble step_increment;
  gdouble page_increment;
  gdouble page_size;

  gdouble hsource;
  gdouble htarget;
  gdouble vsource;
  gdouble vtarget;

  guint duration;
  gint64 start_time;
  gint64 end_time;
  guint tick_id;
  GdkFrameClock *clock;
};

/*signals*/
enum {
  HORIZONTAL_MOVEMENT,
  VERTICAL_MOVEMENT,
  PANNING_STARTED,
  PANNING_FINISHED,
  LAST_SIGNAL
};

static guint pannable_area_signals [LAST_SIGNAL] = { 0 };

enum {
  PROP_ENABLED = 1,
  PROP_MODE,
  PROP_MOVEMENT_MODE,
  PROP_VELOCITY_MIN,
  PROP_VELOCITY_MAX,
  PROP_VEL_MAX_OVERSHOOTING,
  PROP_VELOCITY_FAST_FACTOR,
  PROP_DECELERATION,
  PROP_DRAG_INERTIA,
  PROP_SPS,
  PROP_PANNING_THRESHOLD,
  PROP_SCROLLBAR_FADE_DELAY,
  PROP_BOUNCE_STEPS,
  PROP_FORCE,
  PROP_DIRECTION_ERROR_MARGIN,
  PROP_VSCROLLBAR_POLICY,
  PROP_HSCROLLBAR_POLICY,
  PROP_VOVERSHOOT_MAX,
  PROP_HOVERSHOOT_MAX,
  PROP_SCROLL_TIME,
  PROP_INITIAL_HINT,
  PROP_LOW_FRICTION_MODE,
  PROP_SIZE_REQUEST_POLICY,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_CENTER_ON_CHILD_FOCUS,
  PROP_LAST
};

G_DEFINE_TYPE_WITH_PRIVATE (HildonPannableArea, hildon_pannable_area, GTK_TYPE_SCROLLED_WINDOW)

static void hildon_pannable_area_class_init (HildonPannableAreaClass * klass);
static void hildon_pannable_area_init (HildonPannableArea * area);
static void hildon_pannable_area_get_property (GObject * object,
                                               guint property_id,
                                               GValue * value,
                                               GParamSpec * pspec);
static void hildon_pannable_area_set_property (GObject * object,
                                               guint property_id,
                                               const GValue * value,
                                               GParamSpec * pspec);
static void hildon_pannable_area_remove_timeouts (GtkWidget * widget);
static void hildon_pannable_area_dispose (GObject * object);
static GdkWindow * hildon_pannable_area_get_topmost (GdkWindow * window,
                                                     gint x, gint y,
                                                     gint * tx, gint * ty,
                                                     GdkEventMask mask);
static void hildon_pannable_area_child_mapped (GtkWidget *widget,
                                               GdkEvent  *event,
                                               gpointer user_data);
static void hildon_pannable_area_add (GtkContainer *container, GtkWidget *child);
static void hildon_pannable_area_remove (GtkContainer *container, GtkWidget *child);
static void hildon_pannable_area_set_focus_child (GtkContainer *container,
                                                 GtkWidget *child);
static void hildon_pannable_area_center_on_child_focus (HildonPannableArea *area);


static void
hildon_pannable_area_finalize (GObject *object)
{
  HildonPannableArea *area = HILDON_PANNABLE_AREA (object);
  HildonPannableAreaPrivate *priv = area->priv;

  if (priv->tick_id)
    g_signal_handler_disconnect (priv->clock, priv->tick_id);
  if (priv->clock)
    g_object_unref (priv->clock);

  G_OBJECT_CLASS (hildon_pannable_area_parent_class)->finalize (object);
}

static void
hildon_pannable_area_class_init (HildonPannableAreaClass * class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);

  object_class->finalize     = hildon_pannable_area_finalize;
  object_class->dispose      = hildon_pannable_area_dispose;
  object_class->set_property = hildon_pannable_area_set_property;
  object_class->get_property = hildon_pannable_area_get_property;

//FIXME: Breaks scrolling.  Not adding viewport?
/*  container_class->add = hildon_pannable_area_add;
  container_class->remove = hildon_pannable_area_remove;
  container_class->set_focus_child = hildon_pannable_area_set_focus_child;
*/
  class->horizontal_movement = NULL;
  class->vertical_movement = NULL;

  g_object_class_install_property (object_class,
				   PROP_ENABLED,
				   g_param_spec_boolean ("enabled",
							 "Enabled",
							 "Enable or disable finger-scroll.",
							 TRUE,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_VSCROLLBAR_POLICY,
				   g_param_spec_enum ("vscrollbar_policy",
						      "vscrollbar policy",
						      "Visual policy of the vertical scrollbar.",
						      GTK_TYPE_POLICY_TYPE,
						      GTK_POLICY_AUTOMATIC,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_HSCROLLBAR_POLICY,
				   g_param_spec_enum ("hscrollbar_policy",
						      "hscrollbar policy",
						      "Visual policy of the horizontal scrollbar.",
						      GTK_TYPE_POLICY_TYPE,
                              GTK_POLICY_NEVER,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_MODE,
				   g_param_spec_enum ("mode",
						      "Scroll mode",
						      "Change the finger-scrolling mode.",
						      HILDON_TYPE_PANNABLE_AREA_MODE,
						      HILDON_PANNABLE_AREA_MODE_AUTO,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_MOVEMENT_MODE,
				   g_param_spec_flags ("mov_mode",
                                                       "Scroll movement mode",
                                                       "Controls if the widget can scroll vertically, horizontally or both.",
                                                       HILDON_TYPE_MOVEMENT_MODE,
                                                       HILDON_MOVEMENT_MODE_VERT,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_VELOCITY_MIN,
				   g_param_spec_double ("velocity_min",
							"Minimum scroll velocity",
							"Minimum distance the child widget should scroll "
							"per 'frame', in pixels per frame.",
							0, G_MAXDOUBLE, 10,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_VELOCITY_MAX,
				   g_param_spec_double ("velocity_max",
							"Maximum scroll velocity",
							"Maximum distance the child widget should scroll "
							"per 'frame', in pixels per frame.",
							0, G_MAXDOUBLE, 3500,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_VEL_MAX_OVERSHOOTING,
				   g_param_spec_double ("velocity_overshooting_max",
							"Maximum scroll velocity when overshooting",
							"Maximum distance the child widget should scroll "
							"per 'frame', in pixels per frame when it overshoots after hitting the edge.",
							0, G_MAXDOUBLE, 130,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_VELOCITY_FAST_FACTOR,
				   g_param_spec_double ("velocity_fast_factor",
							"Fast velocity factor",
							"Minimum velocity that is considered 'fast': "
							"children widgets won't receive button presses. "
							"Expressed as a fraction of the maximum velocity.",
							0, 1, 0.01,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_DECELERATION,
				   g_param_spec_double ("deceleration",
							"Deceleration multiplier",
							"The multiplier used when decelerating when in "
							"acceleration scrolling mode.",
							0, 1.0, 0.85,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_DRAG_INERTIA,
				   g_param_spec_double ("drag_inertia",
							"Inertia of the cursor dragging",
							"Percentage of the calculated speed in each moment we are are going to use "
                                                        "to calculate the launch speed, the other part would be the speed "
                                                        "calculated previously.",
							0, 1.0, 0.85,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_SPS,
				   g_param_spec_uint ("sps",
						      "Scrolls per second",
						      "Amount of scroll events to generate per second.",
						      0, G_MAXUINT, 20,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_PANNING_THRESHOLD,
				   g_param_spec_uint ("panning_threshold",
						      "Threshold to consider a motion event an scroll",
						      "Amount of pixels to consider a motion event an scroll, if it is less "
                                                      "it is a click detected incorrectly by the touch screen.",
						      0, G_MAXUINT, 25,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_SCROLLBAR_FADE_DELAY,
				   g_param_spec_uint ("scrollbar_fade_delay",
						      "Time before starting to fade the scrollbar",
						      "Time the scrollbar is going to be visible if the widget is not in "
                                                      "action in miliseconds",
						      0, G_MAXUINT, 1000,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_BOUNCE_STEPS,
				   g_param_spec_uint ("bounce_steps",
						      "Bounce steps",
						      "Number of steps that is going to be used to bounce when hitting the "
                                                      "edge, the rubberband effect depends on it",
						      0, G_MAXUINT, 3,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_FORCE,
				   g_param_spec_uint ("force",
						      "Multiplier of the calculated speed",
						      "Force applied to the movement, multiplies the calculated speed of the "
                                                      "user movement the cursor in the screen",
						      0, G_MAXUINT, 50,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_DIRECTION_ERROR_MARGIN,
				   g_param_spec_uint ("direction_error_margin",
						      "Margin in the direction detection",
						      "After detecting the direction of the movement (horizontal or "
                                                      "vertical), we can add this margin of error to allow the movement in "
                                                      "the other direction even apparently it is not",
						      0, G_MAXUINT, 10,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_VOVERSHOOT_MAX,
				   g_param_spec_int ("vovershoot_max",
                                                     "Vertical overshoot distance",
                                                     "Space we allow the widget to pass over its vertical limits when "
                                                     "hitting the edges, set 0 in order to deactivate overshooting.",
                                                     0, G_MAXINT, 150,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_HOVERSHOOT_MAX,
				   g_param_spec_int ("hovershoot_max",
                                                     "Horizontal overshoot distance",
                                                     "Space we allow the widget to pass over its horizontal limits when "
                                                     "hitting the edges, set 0 in order to deactivate overshooting.",
                                                     0, G_MAXINT, 150,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_SCROLL_TIME,
				   g_param_spec_double ("scroll_time",
							"Time to scroll to a position",
							"The time to scroll to a position when calling the hildon_pannable_scroll_to() function.",
							0.0, 20.0, 1.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
 				   PROP_INITIAL_HINT,
 				   g_param_spec_boolean ("initial-hint",
 							 "Initial hint",
 							 "Whether to hint the user about the pannability of the container.",
 							 TRUE,
							 G_PARAM_READWRITE |
 							 G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
 				   PROP_LOW_FRICTION_MODE,
 				   g_param_spec_boolean ("low-friction-mode",
 							 "Do not decelerate the initial velocity",
							 "Avoid decelerating the panning movement, like no friction, the widget "
                                                         "will stop in the edges or if the user clicks.",
 							 FALSE,
							 G_PARAM_READWRITE |
 							 G_PARAM_CONSTRUCT));

  /**
   * HildonPannableArea:size-request-policy:
   *
   * Controls the size request policy of the widget.
   *
   * <warning><para>
   * HildonPannableArea:size-request-policy is deprecated and should
   * not be used in newly-written code. See
   * hildon_pannable_area_set_size_request_policy()
   * </para></warning>
   *
   * Deprecated: since 2.2
   */
  g_object_class_install_property (object_class,
                                   PROP_SIZE_REQUEST_POLICY,
				   g_param_spec_enum ("size-request-policy",
                                                      "Size Requisition policy",
                                                      "Controls the size request policy of the widget.",
                                                      HILDON_TYPE_SIZE_REQUEST_POLICY,
                                                      HILDON_SIZE_REQUEST_MINIMUM,
                                                      G_PARAM_READWRITE|
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
				   PROP_HADJUSTMENT,
				   g_param_spec_object ("hadjustment",
							"Horizontal Adjustment",
							"The GtkAdjustment for the horizontal position.",
							GTK_TYPE_ADJUSTMENT,
							G_PARAM_READABLE));
  g_object_class_install_property (object_class,
				   PROP_VADJUSTMENT,
				   g_param_spec_object ("vadjustment",
							"Vertical Adjustment",
							"The GtkAdjustment for the vertical position.",
							GTK_TYPE_ADJUSTMENT,
							G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_CENTER_ON_CHILD_FOCUS,
                                   g_param_spec_boolean ("center-on-child-focus",
                                                         "Center on the child with the focus",
                                                         "Whether to center the pannable on the child that receives the focus.",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));


  gtk_widget_class_install_style_property (widget_class,
					   g_param_spec_uint
					   ("indicator-width",
					    "Width of the scroll indicators",
					    "Pixel width used to draw the scroll indicators.",
					    0, G_MAXUINT, 8,
					    G_PARAM_READWRITE));

 /**
   * HildonPannableArea::horizontal-movement:
   * @hildonpannable: the object which received the signal
   * @direction: the direction of the movement #HILDON_MOVEMENT_LEFT or #HILDON_MOVEMENT_RIGHT
   * @initial_x: the x coordinate of the point where the user clicked to start the movement
   * @initial_y: the y coordinate of the point where the user clicked to start the movement
   *
   * The horizontal-movement signal is emitted when the pannable area
   * detects a horizontal movement. The detection does not mean the
   * widget is going to move (i.e. maybe the children are smaller
   * horizontally than the screen).
   *
   * Since: 2.2
   */
  pannable_area_signals[HORIZONTAL_MOVEMENT] =
    g_signal_new ("horizontal_movement",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (HildonPannableAreaClass, horizontal_movement),
		  NULL, NULL,
		  _hildon_marshal_VOID__INT_DOUBLE_DOUBLE,
		  G_TYPE_NONE, 3,
                  G_TYPE_INT,
		  G_TYPE_DOUBLE,
		  G_TYPE_DOUBLE);

  /**
   * HildonPannableArea::vertical-movement:
   * @hildonpannable: the object which received the signal
   * @direction: the direction of the movement #HILDON_MOVEMENT_UP or #HILDON_MOVEMENT_DOWN
   * @initial_x: the x coordinate of the point where the user clicked to start the movement
   * @initial_y: the y coordinate of the point where the user clicked to start the movement
   *
   * The vertical-movement signal is emitted when the pannable area
   * detects a vertical movement. The detection does not mean the
   * widget is going to move (i.e. maybe the children are smaller
   * vertically than the screen).
   *
   * Since: 2.2
   */
  pannable_area_signals[VERTICAL_MOVEMENT] =
    g_signal_new ("vertical_movement",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (HildonPannableAreaClass, vertical_movement),
		  NULL, NULL,
		  _hildon_marshal_VOID__INT_DOUBLE_DOUBLE,
		  G_TYPE_NONE, 3,
                  G_TYPE_INT,
		  G_TYPE_DOUBLE,
		  G_TYPE_DOUBLE);

 /**
   * HildonPannableArea::panning-started:
   * @hildonpannable: the pannable area object that is going to start
   * the panning
   *
   * This signal is emitted before the panning starts. Applications
   * can return %TRUE to avoid the panning. The main difference with
   * the vertical-movement and horizontal-movement signals is those
   * gesture signals are launched no matter if the widget is going to
   * move, this signal means the widget is going to start moving. It
   * could even happen that the widget moves and there was no gesture
   * (i.e. click meanwhile the pannable is overshooting).
   *
   * Returns: %TRUE to stop the panning launch. %FALSE to continue
   * with it.
   *
   * Since: 2.2
   */
  pannable_area_signals[PANNING_STARTED] =
    g_signal_new ("panning-started",
                  G_TYPE_FROM_CLASS (object_class),
                  0,
                  0,
                  NULL, NULL,
                  _hildon_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);

 /**
   * HildonPannableArea::panning-finished:
   * @hildonpannable: the pannable area object that finished the
   * panning
   *
   * This signal is emitted after the kinetic panning has
   * finished.
   *
   * Since: 2.2
   */
  pannable_area_signals[PANNING_FINISHED] =
    g_signal_new ("panning-finished",
                  G_TYPE_FROM_CLASS (object_class),
                  0,
                  0,
                  NULL, NULL,
                  _hildon_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

}

static void
hildon_pannable_area_init (HildonPannableArea * area)
{
  HildonPannableAreaPrivate *priv;
  priv = area->priv = hildon_pannable_area_get_instance_private (area);
  area->priv->duration = 200;
  area->priv->clock = gtk_widget_get_frame_clock (GTK_WIDGET (area));

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (area),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (area), NULL);
  gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (area), NULL);

  priv->moved = FALSE;
  priv->button_pressed = FALSE;
  priv->last_time = 0;
  priv->last_press_time = 0;
  priv->last_type = 0;
  priv->vscroll_visible = TRUE;
  priv->hscroll_visible = TRUE;
  priv->indicator_width = 6;
  priv->overshot_dist_x = 0;
  priv->overshot_dist_y = 0;
  priv->overshooting_y = 0;
  priv->overshooting_x = 0;
  priv->accel_vel_x = 0;
  priv->accel_vel_y = 0;
  priv->idle_id = 0;
  priv->vel_x = 0;
  priv->vel_y = 0;
  priv->old_vel_x = 0;
  priv->old_vel_y = 0;
  priv->scroll_indicator_alpha = 0.0;
  priv->scroll_indicator_timeout = 0;
  priv->motion_event_scroll_timeout = 0;
  priv->scroll_indicator_event_interrupt = 0;
  priv->scroll_delay_counter = 0;
  priv->scrollbar_fade_delay = 0;
  priv->scroll_to_x = -1;
  priv->scroll_to_y = -1;
  priv->fade_in = FALSE;
  priv->first_drag = TRUE;
  priv->initial_effect = TRUE;
  priv->child_width = 0;
  priv->child_height = 0;
  priv->last_in = TRUE;
  priv->x_offset = 0;
  priv->y_offset = 0;
  priv->center_on_child_focus_pending = FALSE;
  priv->selection_movement = FALSE;

  priv->hadjust =
    GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  priv->vadjust =
    GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  g_object_ref_sink (G_OBJECT (priv->hadjust));
  g_object_ref_sink (G_OBJECT (priv->vadjust));
}

static void
hildon_pannable_area_get_property (GObject * object,
                                   guint property_id,
				   GValue * value,
                                   GParamSpec * pspec)
{
  HildonPannableAreaPrivate *priv = HILDON_PANNABLE_AREA (object)->priv;

  switch (property_id) {
  case PROP_ENABLED:
    g_value_set_boolean (value, priv->enabled);
    break;
  case PROP_MODE:
    g_value_set_enum (value, priv->mode);
    break;
  case PROP_MOVEMENT_MODE:
    g_value_set_flags (value, priv->mov_mode);
    break;
  case PROP_VELOCITY_MIN:
    g_value_set_double (value, priv->vmin);
    break;
  case PROP_VELOCITY_MAX:
    g_value_set_double (value, priv->vmax);
    break;
  case PROP_VEL_MAX_OVERSHOOTING:
    g_value_set_double (value, priv->vmax_overshooting);
    break;
  case PROP_VELOCITY_FAST_FACTOR:
    g_value_set_double (value, priv->vfast_factor);
    break;
  case PROP_DECELERATION:
    g_value_set_double (value, priv->decel);
    break;
  case PROP_DRAG_INERTIA:
    g_value_set_double (value, priv->drag_inertia);
    break;
  case PROP_SPS:
    g_value_set_uint (value, priv->sps);
    break;
  case PROP_PANNING_THRESHOLD:
    g_value_set_uint (value, priv->panning_threshold);
    break;
  case PROP_SCROLLBAR_FADE_DELAY:
    /* convert to miliseconds */
    g_value_set_uint (value, priv->scrollbar_fade_delay * SCROLL_FADE_TIMEOUT);
    break;
  case PROP_BOUNCE_STEPS:
    g_value_set_uint (value, priv->bounce_steps);
    break;
  case PROP_FORCE:
    g_value_set_uint (value, priv->force);
    break;
  case PROP_DIRECTION_ERROR_MARGIN:
    g_value_set_uint (value, priv->direction_error_margin);
    break;
  case PROP_VSCROLLBAR_POLICY:
    g_value_set_enum (value, priv->vscrollbar_policy);
    break;
  case PROP_HSCROLLBAR_POLICY:
    g_value_set_enum (value, priv->hscrollbar_policy);
    break;
  case PROP_VOVERSHOOT_MAX:
    g_value_set_int (value, priv->vovershoot_max);
    break;
  case PROP_HOVERSHOOT_MAX:
    g_value_set_int (value, priv->hovershoot_max);
    break;
  case PROP_SCROLL_TIME:
    g_value_set_double (value, priv->scroll_time);
    break;
  case PROP_INITIAL_HINT:
    g_value_set_boolean (value, priv->initial_hint);
    break;
  case PROP_LOW_FRICTION_MODE:
    g_value_set_boolean (value, priv->low_friction_mode);
    break;
  case PROP_SIZE_REQUEST_POLICY:
    g_value_set_enum (value, priv->size_request_policy);
    break;
  case PROP_HADJUSTMENT:
    g_value_set_object (value,
                        hildon_pannable_area_get_hadjustment
                        (HILDON_PANNABLE_AREA (object)));
    break;
  case PROP_VADJUSTMENT:
    g_value_set_object (value,
                        hildon_pannable_area_get_vadjustment
                        (HILDON_PANNABLE_AREA (object)));
    break;
  case PROP_CENTER_ON_CHILD_FOCUS:
    g_value_set_boolean (value, priv->center_on_child_focus);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
hildon_pannable_area_set_property (GObject * object,
                                   guint property_id,
				   const GValue * value,
                                   GParamSpec * pspec)
{
  HildonPannableAreaPrivate *priv = HILDON_PANNABLE_AREA (object)->priv;
  gboolean enabled;

  switch (property_id) {
  case PROP_ENABLED:
    enabled = g_value_get_boolean (value);

    if ((priv->enabled != enabled) && (gtk_widget_get_realized (GTK_WIDGET (object)))) {
      if (enabled)
	gdk_window_raise (priv->event_window);
      else
	gdk_window_lower (priv->event_window);
    }

    priv->enabled = enabled;
    break;
  case PROP_MODE:
    priv->mode = g_value_get_enum (value);
    break;
  case PROP_MOVEMENT_MODE:
    priv->mov_mode = g_value_get_flags (value);

    if (priv->mov_mode == HILDON_MOVEMENT_MODE_VERT) {
      /* Do not show horizontal scrollbar. */
      priv->vscrollbar_policy = GTK_POLICY_AUTOMATIC;
      priv->hscrollbar_policy = GTK_POLICY_NEVER;

      gtk_widget_queue_resize (GTK_WIDGET (object));
    } else if (priv->mov_mode == HILDON_MOVEMENT_MODE_HORIZ) {
      /* Do not show vertical scrollbar. */
        priv->vscrollbar_policy = GTK_POLICY_NEVER;
        priv->hscrollbar_policy = GTK_POLICY_AUTOMATIC;

        gtk_widget_queue_resize (GTK_WIDGET (object));
    }
    else {
      /* Show both scrollbars. */
        priv->vscrollbar_policy = GTK_POLICY_AUTOMATIC;
        priv->hscrollbar_policy = GTK_POLICY_AUTOMATIC;

        gtk_widget_queue_resize (GTK_WIDGET (object));
    }
    break;
  case PROP_VELOCITY_MIN:
    priv->vmin = g_value_get_double (value);
    break;
  case PROP_VELOCITY_MAX:
    priv->vmax = g_value_get_double (value);
    break;
  case PROP_VEL_MAX_OVERSHOOTING:
    priv->vmax_overshooting = g_value_get_double (value);
    break;
  case PROP_VELOCITY_FAST_FACTOR:
    priv->vfast_factor = g_value_get_double (value);
    break;
  case PROP_DECELERATION:
    priv->decel = g_value_get_double (value);
    break;
  case PROP_DRAG_INERTIA:
    priv->drag_inertia = g_value_get_double (value);
    break;
  case PROP_SPS:
    priv->sps = g_value_get_uint (value);
    break;
  case PROP_PANNING_THRESHOLD:
    {
      GtkSettings *settings = gtk_settings_get_default ();
      GtkSettingsValue svalue = { NULL, { 0, }, };

      priv->panning_threshold = g_value_get_uint (value);

      /* insure gtk dnd is the same we are using, not allowed
         different thresholds in the same application */
      svalue.origin = "panning_threshold";
      g_value_init (&svalue.value, G_TYPE_LONG);
      g_value_set_long (&svalue.value, priv->panning_threshold);
      gtk_settings_set_property_value (settings, "gtk-dnd-drag-threshold", &svalue);
      g_value_unset (&svalue.value);
    }
    break;
  case PROP_SCROLLBAR_FADE_DELAY:
    /* convert to miliseconds */
    priv->scrollbar_fade_delay = g_value_get_uint (value)/(SCROLL_FADE_TIMEOUT);
    break;
  case PROP_BOUNCE_STEPS:
    priv->bounce_steps = g_value_get_uint (value);
    break;
  case PROP_FORCE:
    priv->force = g_value_get_uint (value);
    break;
  case PROP_DIRECTION_ERROR_MARGIN:
    priv->direction_error_margin = g_value_get_uint (value);
    break;
  case PROP_VSCROLLBAR_POLICY:
    priv->vscrollbar_policy = g_value_get_enum (value);

    gtk_widget_queue_resize (GTK_WIDGET (object));
    break;
  case PROP_HSCROLLBAR_POLICY:
    priv->hscrollbar_policy = g_value_get_enum (value);

    gtk_widget_queue_resize (GTK_WIDGET (object));
    break;
  case PROP_VOVERSHOOT_MAX:
    priv->vovershoot_max = g_value_get_int (value);
    break;
  case PROP_HOVERSHOOT_MAX:
    priv->hovershoot_max = g_value_get_int (value);
    break;
  case PROP_SCROLL_TIME:
    priv->scroll_time = g_value_get_double (value);
    break;
  case PROP_INITIAL_HINT:
    priv->initial_hint = g_value_get_boolean (value);
    break;
  case PROP_LOW_FRICTION_MODE:
    priv->low_friction_mode = g_value_get_boolean (value);
    break;
  case PROP_SIZE_REQUEST_POLICY:
    hildon_pannable_area_set_size_request_policy (HILDON_PANNABLE_AREA (object),
                                                  g_value_get_enum (value));
    break;
  case PROP_CENTER_ON_CHILD_FOCUS:
    priv->center_on_child_focus = g_value_get_boolean (value);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
hildon_pannable_area_dispose (GObject * object)
{
  HildonPannableAreaPrivate *priv = HILDON_PANNABLE_AREA (object)->priv;
  GtkWidget *child = gtk_bin_get_child (GTK_BIN (object));

  hildon_pannable_area_remove_timeouts (GTK_WIDGET (object));

  if (child) {
    g_signal_handlers_disconnect_by_func (child,
                                          hildon_pannable_area_child_mapped,
                                          object);
  }

  if (priv->hadjust) {
    g_object_unref (priv->hadjust);
    priv->hadjust = NULL;
  }

  if (priv->vadjust) {
    g_object_unref (priv->vadjust);
    priv->vadjust = NULL;
  }

  if (G_OBJECT_CLASS (hildon_pannable_area_parent_class)->dispose)
    G_OBJECT_CLASS (hildon_pannable_area_parent_class)->dispose (object);
}


static void
hildon_pannable_area_remove_timeouts (GtkWidget * widget)
{
  HildonPannableAreaPrivate *priv = HILDON_PANNABLE_AREA (widget)->priv;

  if (priv->idle_id) {
    g_signal_emit (widget, pannable_area_signals[PANNING_FINISHED], 0);
    g_source_remove (priv->idle_id);
    priv->idle_id = 0;
  }

  if (priv->scroll_indicator_timeout){
    g_source_remove (priv->scroll_indicator_timeout);
    priv->scroll_indicator_timeout = 0;
  }

  if (priv->motion_event_scroll_timeout){
    g_source_remove (priv->motion_event_scroll_timeout);
    priv->motion_event_scroll_timeout = 0;
  }
}

static GdkWindow *
hildon_pannable_area_get_topmost (GdkWindow * window,
                                  gint x, gint y,
                                  gint * tx, gint * ty,
                                  GdkEventMask mask)
{
  /* Find the GdkWindow at the given point, by recursing from a given
   * parent GdkWindow. Optionally return the co-ordinates transformed
   * relative to the child window.
   */
  gint width, height;
  GList *c, *children;
  GdkWindow *selected_window = NULL;

  width = gdk_window_get_width (window);
  height = gdk_window_get_height (window);

  if ((x < 0) || (x >= width) || (y < 0) || (y >= height))
    return NULL;

  children = gdk_window_peek_children (window);

  if (!children) {
    if (tx)
      *tx = x;
    if (ty)
      *ty = y;
    selected_window = window;
  }

  for (c = children; c; c = c->next) {
    GdkWindow *child = (GdkWindow *) c->data;
    gint wx, wy;

    width = gdk_window_get_width (child);
    height = gdk_window_get_height (child);
    gdk_window_get_position (child, &wx, &wy);

    if ((x >= wx) && (x < (wx + width)) && (y >= wy) && (y < (wy + height)) &&
        (gdk_window_is_visible (child))) {

      if (gdk_window_peek_children (child)) {
        selected_window = hildon_pannable_area_get_topmost (child, x-wx, y-wy,
                                                            tx, ty, mask);
        if (!selected_window) {
          if (tx)
            *tx = x-wx;
          if (ty)
            *ty = y-wy;
          selected_window = child;
        }
      } else {
        if ((gdk_window_get_events (child)&mask)) {
          if (tx)
            *tx = x-wx;
          if (ty)
            *ty = y-wy;
          selected_window = child;
        }
      }
    }
  }

  return selected_window;
}

static void
hildon_pannable_area_child_mapped (GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer user_data)
{
  HildonPannableAreaPrivate *priv = HILDON_PANNABLE_AREA (user_data)->priv;

  if (priv->event_window != NULL && priv->enabled)
    gdk_window_raise (priv->event_window);
}

static void
hildon_pannable_area_add (GtkContainer *container, GtkWidget *child)
{
  HildonPannableAreaPrivate *priv = HILDON_PANNABLE_AREA (container)->priv;
  GtkScrollable *scroll = GTK_SCROLLABLE (child);

  g_return_if_fail (gtk_bin_get_child (GTK_BIN (container)) == NULL);

  GTK_CONTAINER_CLASS (hildon_pannable_area_parent_class)->add (container, child);

  g_signal_connect_after (child, "map-event",
                          G_CALLBACK (hildon_pannable_area_child_mapped),
                          container);

  if (scroll != NULL) {
    gtk_scrollable_set_hadjustment (scroll, priv->hadjust);
    gtk_scrollable_set_vadjustment (scroll, priv->vadjust);
  }
  else {
    g_warning ("%s: cannot add non scrollable widget, "
               "wrap it in a viewport", __FUNCTION__);
  }
}

/* call this function if you are not panning */
static void
hildon_pannable_area_center_on_child_focus      (HildonPannableArea *area)
{
  GtkWidget *focused_child = NULL;
  GtkWidget *window = NULL;

  window = gtk_widget_get_toplevel (GTK_WIDGET (area));

  if (gtk_widget_is_toplevel (window)) {
    focused_child = gtk_window_get_focus (GTK_WINDOW (window));
  }

  if (focused_child) {
    hildon_pannable_area_scroll_to_child (area, focused_child);
  }
}

static void
hildon_pannable_area_set_focus_child            (GtkContainer     *container,
                                                 GtkWidget        *child)
{
  HildonPannableArea *area = HILDON_PANNABLE_AREA (container);

  if (!area->priv->center_on_child_focus) {
    return;
  }

  if (GTK_IS_WIDGET (child)) {
    area->priv->center_on_child_focus_pending = TRUE;
  }
}

static void
hildon_pannable_area_remove (GtkContainer *container, GtkWidget *child)
{
  GtkScrollable *scroll = GTK_SCROLLABLE (child);
  g_return_if_fail (HILDON_IS_PANNABLE_AREA (container));
  g_return_if_fail (child != NULL);
  g_return_if_fail (gtk_bin_get_child (GTK_BIN (container)) == child);

  if (scroll != NULL) {
    gtk_scrollable_set_hadjustment (scroll, NULL);
    gtk_scrollable_set_vadjustment (scroll, NULL);
  }

  g_signal_handlers_disconnect_by_func (child,
                                        hildon_pannable_area_child_mapped,
                                        container);

  /* chain parent class handler to remove child */
  GTK_CONTAINER_CLASS (hildon_pannable_area_parent_class)->remove (container, child);
}

/**
 * hildon_pannable_area_new:
 *
 * Create a new pannable area widget
 *
 * Returns: the newly created #HildonPannableArea
 *
 * Since: 2.2
 */

GtkWidget *
hildon_pannable_area_new (void)
{
  return g_object_new (HILDON_TYPE_PANNABLE_AREA, NULL);
}

/**
 * hildon_pannable_area_new_full:
 * @mode: #HildonPannableAreaMode
 * @enabled: Value for the enabled property
 * @vel_min: Value for the velocity-min property
 * @vel_max: Value for the velocity-max property
 * @decel: Value for the deceleration property
 * @sps: Value for the sps property
 *
 * Create a new #HildonPannableArea widget and set various properties
 *
 * returns: the newly create #HildonPannableArea
 *
 * Since: 2.2
 */

GtkWidget *
hildon_pannable_area_new_full (gint mode, gboolean enabled,
			       gdouble vel_min, gdouble vel_max,
			       gdouble decel, guint sps)
{
  return g_object_new (HILDON_TYPE_PANNABLE_AREA,
		       "mode", mode,
		       "enabled", enabled,
		       "velocity_min", vel_min,
		       "velocity_max", vel_max,
		       "deceleration", decel, "sps", sps, NULL);
}

/**
 * hildon_pannable_area_add_with_viewport:
 * @area: A #HildonPannableArea
 * @child: Child widget to add to the viewport
 *
 *
 * Convenience function used to add a child to a #GtkViewport, and add
 * the viewport to the scrolled window for childreen without native
 * scrolling capabilities.
 *
 * Note: Left for backwards compatibility at the moment.
 * 
 * See gtk_container_add() for more information.
 *
 * Since: 2.2
 *
 * Deprecated: 3.0, No longer needed.
 */

void
hildon_pannable_area_add_with_viewport (HildonPannableArea * area,
					GtkWidget * child)
{
  g_return_if_fail (HILDON_IS_PANNABLE_AREA (area));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == NULL);

  gtk_container_add (GTK_CONTAINER (area), child);
}

static void hildon_pannable_area_on_frame_clock_update (GdkFrameClock *clock,
                                                        HildonPannableArea *area);

static void
hildon_pannable_area_begin_updating (HildonPannableArea *area)
{
  HildonPannableAreaPrivate *priv = area->priv;

  if (priv->tick_id == 0)
    {
      priv->tick_id = g_signal_connect (priv->clock, "update",
                                        G_CALLBACK (hildon_pannable_area_on_frame_clock_update), area);
      gdk_frame_clock_begin_updating (priv->clock);
    }
}

static void
hildon_pannable_area_end_updating (HildonPannableArea *area)
{
  HildonPannableAreaPrivate *priv = area->priv;

  if (priv->tick_id != 0)
    {
      g_signal_handler_disconnect (priv->clock, priv->tick_id);
      priv->tick_id = 0;
      gdk_frame_clock_end_updating (priv->clock);
    }
}

/* From clutter-easing.c, based on Robert Penner's
 * infamous easing equations, MIT license.
 */
static gdouble
ease_out_cubic (gdouble t)
{
  gdouble p = t - 1;

  return p * p * p + 1;
}

static void
hildon_pannable_area_on_frame_clock_update (GdkFrameClock *clock,
                                            HildonPannableArea *area)
{
  HildonPannableAreaPrivate *priv = area->priv;
  gint64 now;

  now = gdk_frame_clock_get_frame_time (clock);

  GtkAdjustment *hadj;
  GtkAdjustment *vadj;

  hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (area));
  vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (area));

  if (now < priv->end_time)
    {
      gdouble t;

      t = (now - priv->start_time) / (gdouble) (priv->end_time - priv->start_time);
      t = ease_out_cubic (t);
      gtk_adjustment_set_value (hadj, priv->hsource + t * (priv->htarget - priv->hsource));
      gtk_adjustment_set_value (vadj, priv->vsource + t * (priv->vtarget - priv->vsource));
    }
  else
    {
      gtk_adjustment_set_value (hadj, priv->htarget);
      gtk_adjustment_set_value (vadj, priv->vtarget);
      hildon_pannable_area_end_updating (area);
    }
}


// WORKING ON THIS.  HANDLE -1 values
static void hildon_pannable_area_set_value_internal (HildonPannableArea *area,
                                         gdouble             hvalue,
                                         gdouble             vvalue,
                                         gboolean            animate)
{
  HildonPannableAreaPrivate *priv = area->priv;

  /* don't use CLAMP() so we don't end up below lower if upper - page_size
   * is smaller than lower
   */

  GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (area));
  if (vvalue == -1)
    {
      vvalue = gtk_adjustment_get_value (vadj);
    }
  else
    {
      vvalue = MIN (vvalue, gtk_adjustment_get_upper(vadj) - gtk_adjustment_get_page_size(vadj));
      vvalue = MAX (vvalue, gtk_adjustment_get_lower(vadj));
    }

  GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (area));
  if (hvalue == -1)
    {
      hvalue = gtk_adjustment_get_value (hadj);
    }
  else
    {
      hvalue = MIN (hvalue, gtk_adjustment_get_upper(hadj) - gtk_adjustment_get_page_size(hadj));
      hvalue = MAX (hvalue, gtk_adjustment_get_lower(hadj));
    }

  area->priv->clock = gtk_widget_get_frame_clock (GTK_WIDGET (area));
  
  if (animate && priv->duration != 0 && priv->clock != NULL)
    {
      if (priv->tick_id && priv->htarget == hvalue && priv->vtarget == vvalue)
        return;

      priv->vsource = gtk_adjustment_get_value (vadj);
      priv->vtarget = vvalue;
      priv->hsource = gtk_adjustment_get_value (hadj);
      priv->htarget = hvalue;
      priv->start_time = gdk_frame_clock_get_frame_time (priv->clock);
      priv->end_time = priv->start_time + 1000 * priv->duration;
      hildon_pannable_area_begin_updating (area);
    }
  else
    {
      hildon_pannable_area_end_updating (area);
      gtk_adjustment_set_value (vadj, vvalue);
      gtk_adjustment_set_value (hadj, hvalue);
    }
}

/**
 * hildon_pannable_area_scroll_to:
 * @area: A #HildonPannableArea.
 * @x: The x coordinate of the destination point or -1 to ignore this axis.
 * @y: The y coordinate of the destination point or -1 to ignore this axis.
 *
 * Smoothly scrolls @area to ensure that (@x, @y) is a visible point
 * on the widget. To move in only one coordinate, you must set the other one
 * to -1. Notice that, in %HILDON_PANNABLE_AREA_MODE_PUSH mode, this function
 * works just like hildon_pannable_area_jump_to().
 *
 * This function is useful if you need to present the user with a particular
 * element inside a scrollable widget, like #GtkTreeView. For instance,
 * the following example shows how to scroll inside a #GtkTreeView to
 * make visible an item, indicated by the #GtkTreeIter @iter.
 *
 * <example>
 * <programlisting>
 *  GtkTreePath *path;
 *  GdkRectangle rect;
 *  gint y;
 *  <!-- -->
 *  path = gtk_tree_model_get_path (model, &amp;iter);
 *  gtk_tree_view_get_background_area (GTK_TREE_VIEW (treeview),
 *                                     path, NULL, &amp;rect);
 *  gtk_tree_view_convert_bin_window_to_tree_coords (GTK_TREE_VIEW (treeview),
 *                                                   0, rect.y, NULL, &amp;y);
 *  hildon_pannable_area_scroll_to (panarea, -1, y);
 *  gtk_tree_path_free (path);
 * </programlisting>
 * </example>
 *
 * If you want to present a child widget in simpler scenarios,
 * use hildon_pannable_area_scroll_to_child() instead.
 *
 * There is a precondition to this function: the widget must be
 * already realized. Check the hildon_pannable_area_jump_to_child() for
 * more tips regarding how to call this function during
 * initialization.
 *
 * Since: 2.2
 **/
void
hildon_pannable_area_scroll_to (HildonPannableArea *area,
				const gint x, const gint y)
{
  gboolean hscroll_visible, vscroll_visible;

  g_return_if_fail (HILDON_IS_PANNABLE_AREA (area));
  g_return_if_fail (gtk_widget_get_realized (GTK_WIDGET (area)));

  GtkAdjustment *hadj;
  GtkAdjustment *vadj;

  hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (area));
  vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (area));

  vscroll_visible = (gtk_adjustment_get_upper(vadj) - gtk_adjustment_get_lower(vadj) > gtk_adjustment_get_page_size(vadj));
  hscroll_visible = (gtk_adjustment_get_upper(hadj) - gtk_adjustment_get_lower(hadj) > gtk_adjustment_get_page_size(hadj));

  // We don't have a scroll bar present so don't do anything.
  if (((!vscroll_visible)&&(!hscroll_visible)) || (x == -1 && y == -1)) {
    return;
  }

  gdouble scroll_to_x = CLAMP (x - gtk_adjustment_get_page_size(hadj)/2,
                               gtk_adjustment_get_lower(hadj),
                               gtk_adjustment_get_upper(hadj) - gtk_adjustment_get_page_size(hadj));
  gdouble scroll_to_y = CLAMP (y - gtk_adjustment_get_page_size(vadj)/2,
                               gtk_adjustment_get_lower(vadj),
                               gtk_adjustment_get_upper(vadj) - gtk_adjustment_get_page_size(vadj));

  hildon_pannable_area_set_value_internal (area, scroll_to_x, scroll_to_y, TRUE);
/*
  if (x != -1)
    gtk_adjustment_set_value(hadj, CLAMP (x - gtk_adjustment_get_page_size(hadj)/2,
                               gtk_adjustment_get_lower(hadj),
                               gtk_adjustment_get_upper(hadj) - gtk_adjustment_get_page_size(hadj)));
  if (y != -1)
  {
    gtk_adjustment_set_value(vadj, CLAMP (y - gtk_adjustment_get_page_size(vadj)/2,
                               gtk_adjustment_get_lower(vadj),
                               gtk_adjustment_get_upper(vadj) - gtk_adjustment_get_page_size(vadj)));
  }*/
}

/**
 * hildon_pannable_area_jump_to:
 * @area: A #HildonPannableArea.
 * @x: The x coordinate of the destination point or -1 to ignore this axis.
 * @y: The y coordinate of the destination point or -1 to ignore this axis.
 *
 * Jumps the position of @area to ensure that (@x, @y) is a visible
 * point in the widget. In order to move in only one coordinate, you
 * must set the other one to -1. See hildon_pannable_area_scroll_to()
 * function for an example of how to calculate the position of
 * children in scrollable widgets like #GtkTreeview.
 *
 * There is a precondition to this function: the widget must be
 * already realized. Check the hildon_pannable_area_jump_to_child() for
 * more tips regarding how to call this function during
 * initialization.
 *
 * Since: 2.2
 **/
void
hildon_pannable_area_jump_to (HildonPannableArea *area,
                              const gint x, const gint y)
{
  gboolean hscroll_visible, vscroll_visible;

  g_return_if_fail (HILDON_IS_PANNABLE_AREA (area));
  g_return_if_fail (gtk_widget_get_realized (GTK_WIDGET (area)));

  GtkAdjustment *hadj;
  GtkAdjustment *vadj;

  hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (area));
  vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (area));

  vscroll_visible = (gtk_adjustment_get_upper(vadj) - gtk_adjustment_get_lower(vadj) > gtk_adjustment_get_page_size(vadj));
  hscroll_visible = (gtk_adjustment_get_upper(hadj) - gtk_adjustment_get_lower(hadj) > gtk_adjustment_get_page_size(hadj));

  // We don't have a scroll bar present so don't do anything.
  if (((!vscroll_visible)&&(!hscroll_visible)) || (x == -1 && y == -1)) {
    return;
  }

  gdouble jump_to_x = CLAMP (x - gtk_adjustment_get_page_size(hadj)/2,
                               gtk_adjustment_get_lower(hadj),
                               gtk_adjustment_get_upper(hadj) - gtk_adjustment_get_page_size(hadj));
  gdouble jump_to_y = CLAMP (y - gtk_adjustment_get_page_size(vadj)/2,
                               gtk_adjustment_get_lower(vadj),
                               gtk_adjustment_get_upper(vadj) - gtk_adjustment_get_page_size(vadj));

  hildon_pannable_area_set_value_internal (area, jump_to_x, jump_to_y, FALSE);

/*  if (x != -1)
    gtk_adjustment_set_value(hadj, CLAMP (x - gtk_adjustment_get_page_size(hadj)/2,
                               gtk_adjustment_get_lower(hadj),
                               gtk_adjustment_get_upper(hadj) - gtk_adjustment_get_page_size(hadj)));
  if (y != -1)
  {
    gtk_adjustment_set_value(vadj, CLAMP (y - gtk_adjustment_get_page_size(vadj)/2,
                               gtk_adjustment_get_lower(vadj),
                               gtk_adjustment_get_upper(vadj) - gtk_adjustment_get_page_size(vadj)));
  }*/
}

/**
 * hildon_pannable_area_scroll_to_child:
 * @area: A #HildonPannableArea.
 * @child: A #GtkWidget, descendant of @area.
 *
 * Smoothly scrolls until @child is visible inside @area. @child must
 * be a descendant of @area. If you need to scroll inside a scrollable
 * widget, e.g., #GtkTreeview, see hildon_pannable_area_scroll_to().
 *
 * There is a precondition to this function: the widget must be
 * already realized. Check the hildon_pannable_area_jump_to_child() for
 * more tips regarding how to call this function during
 * initialization.
 *
 * Since: 2.2
 **/
void
hildon_pannable_area_scroll_to_child (HildonPannableArea *area, GtkWidget *child)
{
  GtkWidget *bin_child;
  gint x, y;

  g_return_if_fail (gtk_widget_get_realized (GTK_WIDGET (area)));
  g_return_if_fail (HILDON_IS_PANNABLE_AREA (area));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_is_ancestor (child, GTK_WIDGET (area)));

  if (gtk_bin_get_child (GTK_BIN (area)) == NULL)
    return;

  /* We need to get to check the child of the inside the area */
  bin_child = gtk_bin_get_child (GTK_BIN (area));

  /* we check if we added a viewport */
  if (GTK_IS_VIEWPORT (bin_child)) {
    bin_child = gtk_bin_get_child (GTK_BIN (bin_child));
  }

  if (gtk_widget_translate_coordinates (child, bin_child, 0, 0, &x, &y))
    hildon_pannable_area_scroll_to (area, x, y);
}

/**
 * hildon_pannable_area_jump_to_child:
 * @area: A #HildonPannableArea.
 * @child: A #GtkWidget, descendant of @area.
 *
 * Jumps to make sure @child is visible inside @area. @child must
 * be a descendant of @area. If you want to move inside a scrollable
 * widget, like, #GtkTreeview, see hildon_pannable_area_scroll_to().
 *
 * There is a precondition to this function: the widget must be
 * already realized. You can control if the widget is ready with the
 * GTK_WIDGET_REALIZED macro. If you want to call this function during
 * the initialization process of the widget do it inside a callback to
 * the ::realize signal, using g_signal_connect_after() function.
 *
 * Since: 2.2
 **/
void
hildon_pannable_area_jump_to_child (HildonPannableArea *area, GtkWidget *child)
{
  GtkWidget *bin_child;
  gint x, y;

  g_return_if_fail (gtk_widget_get_realized (GTK_WIDGET (area)));
  g_return_if_fail (HILDON_IS_PANNABLE_AREA (area));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_is_ancestor (child, GTK_WIDGET (area)));

  if (gtk_bin_get_child (GTK_BIN (area)) == NULL)
    return;

  /* We need to get to check the child of the inside the area */
  bin_child = gtk_bin_get_child (GTK_BIN (area));

  /* we check if we added a viewport */
  if (GTK_IS_VIEWPORT (bin_child)) {
    bin_child = gtk_bin_get_child (GTK_BIN (bin_child));
  }

  if (gtk_widget_translate_coordinates (child, bin_child, 0, 0, &x, &y))
    hildon_pannable_area_jump_to (area, x, y);
}

/**
 * hildon_pannable_get_child_widget_at:
 * @area: A #HildonPannableArea.
 * @x: horizontal coordinate of the point
 * @y: vertical coordinate of the point
 *
 * Get the widget at the point (x, y) inside the pannable area. In
 * case no widget found it returns NULL.
 *
 * returns: the #GtkWidget if we find a widget, NULL in any other case
 *
 * Since: 2.2
 **/
GtkWidget*
hildon_pannable_get_child_widget_at (HildonPannableArea *area,
                                     gdouble x, gdouble y)
{
  GdkWindow *window = NULL;
  GtkWidget *child_widget = NULL;

  window = hildon_pannable_area_get_topmost
    (gtk_widget_get_window (gtk_bin_get_child (GTK_BIN (area))),
     x, y, NULL, NULL, GDK_ALL_EVENTS_MASK);

  gdk_window_get_user_data (window, (gpointer) &child_widget);

  return child_widget;
}


/**
 * hildon_pannable_area_get_hadjustment:
 * @area: A #HildonPannableArea.
 *
 * Returns the horizontal adjustment. This adjustment is the internal
 * widget adjustment used to control the animations. Do not modify it
 * directly to change the position of the pannable, to do that use the
 * pannable API. If you modify the object directly it could cause
 * artifacts in the animations.
 *
 * returns: The horizontal #GtkAdjustment
 *
 * Since: 2.2
 **/
GtkAdjustment*
hildon_pannable_area_get_hadjustment            (HildonPannableArea *area)
{

  g_return_val_if_fail (HILDON_IS_PANNABLE_AREA (area), NULL);

  return area->priv->hadjust;
}

/**
 * hildon_pannable_area_get_vadjustment:
 * @area: A #HildonPannableArea.
 *
 * Returns the vertical adjustment. This adjustment is the internal
 * widget adjustment used to control the animations. Do not modify it
 * directly to change the position of the pannable, to do that use the
 * pannable API. If you modify the object directly it could cause
 * artifacts in the animations.
 *
 * returns: The vertical #GtkAdjustment
 *
 * Since: 2.2
 **/
GtkAdjustment*
hildon_pannable_area_get_vadjustment            (HildonPannableArea *area)
{
  g_return_val_if_fail (HILDON_IS_PANNABLE_AREA (area), NULL);

  return area->priv->vadjust;
}


/**
 * hildon_pannable_area_get_size_request_policy:
 * @area: A #HildonPannableArea.
 *
 * This function returns the current size request policy of the
 * widget. That policy controls the way the size_request is done in
 * the pannable area. Check
 * hildon_pannable_area_set_size_request_policy() for a more detailed
 * explanation.
 *
 * returns: the policy is currently being used in the widget
 * #HildonSizeRequestPolicy.
 *
 * Since: 2.2
 *
 * Deprecated: See hildon_pannable_area_set_size_request_policy()
 **/
HildonSizeRequestPolicy
hildon_pannable_area_get_size_request_policy (HildonPannableArea *area)
{
  HildonPannableAreaPrivate *priv;

  g_return_val_if_fail (HILDON_IS_PANNABLE_AREA (area), FALSE);

  priv = area->priv;

  return priv->size_request_policy;
}

/**
 * hildon_pannable_area_set_size_request_policy:
 * @area: A #HildonPannableArea.
 * @size_request_policy: One of the allowed #HildonSizeRequestPolicy
 *
 * This function sets the pannable area size request policy. That
 * policy controls the way the size_request is done in the pannable
 * area. Pannable can use the size request of its children
 * (#HILDON_SIZE_REQUEST_CHILDREN) or the minimum size required for
 * the area itself (#HILDON_SIZE_REQUEST_MINIMUM), the latter is the
 * default. Recall this size depends on the scrolling policy you are
 * requesting to the pannable area, if you set #GTK_POLICY_NEVER this
 * parameter will not have any effect with
 * #HILDON_SIZE_REQUEST_MINIMUM set.
 *
 * Since: 2.2
 *
 * Deprecated: This method and the policy request is deprecated, DO
 * NOT use it in future code, the only policy properly supported in
 * gtk+ nowadays is the minimum size. Use gtk_window_set_default_size()
 * or gtk_window_set_geometry_hints() with the proper size in your case
 * to define the height of your dialogs.
 **/
void
hildon_pannable_area_set_size_request_policy (HildonPannableArea *area,
                                              HildonSizeRequestPolicy size_request_policy)
{
  HildonPannableAreaPrivate *priv;

  g_return_if_fail (HILDON_IS_PANNABLE_AREA (area));

  priv = area->priv;

  if (priv->size_request_policy == size_request_policy)
    return;

  priv->size_request_policy = size_request_policy;

  gtk_widget_queue_resize (GTK_WIDGET (area));

  g_object_notify (G_OBJECT (area), "size-request-policy");
}

/**
 * hildon_pannable_area_get_center_on_child_focus
 * @area: A #HildonPannableArea
 *
 * Gets the @area #HildonPannableArea:center-on-child-focus property
 * value.
 *
 * See #HildonPannableArea:center-on-child-focus for more information.
 *
 * Returns: the @area #HildonPannableArea:center-on-child-focus value
 *
 * Since: 2.2
 **/
gboolean
hildon_pannable_area_get_center_on_child_focus  (HildonPannableArea *area)
{
  g_return_val_if_fail (HILDON_IS_PANNABLE_AREA (area), FALSE);

  return area->priv->center_on_child_focus;
}

/**
 * hildon_pannable_area_set_center_on_child_focus
 * @area: A #HildonPannableArea
 * @value: the new value
 *
 * Sets the @area #HildonPannableArea:center-on-child-focus property
 * to @value.
 *
 * See #HildonPannableArea:center-on-child-focus for more information.
 *
 * Since: 2.2
 **/
void
hildon_pannable_area_set_center_on_child_focus  (HildonPannableArea *area,
                                                 gboolean value)
{
  g_return_if_fail (HILDON_IS_PANNABLE_AREA (area));

  area->priv->center_on_child_focus = value;
}
