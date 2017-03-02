#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "hildon-gtk.h"
#include "hildon-gtk-widget.h"
#include "hildon-gtk-marshalers.h"

#include <gdk/gdkx.h>
#include <libintl.h>

#ifdef ENABLE_NLS
#define P_(String) g_dgettext("hildon-libs-properties",String)
#else
#define P_(String) (String)
#endif

#define HILDON_HEIGHT_FINGER    70

#define HILDON_HEIGHT_THUMB     105

#define HILDON_WIDTH_FULLSCREEN (gdk_screen_get_width (gdk_screen_get_default ()))

#define HILDON_WIDTH_HALFSCREEN (HILDON_WIDTH_FULLSCREEN / 2)
#if 0
enum {
  PROP_TAP_AND_HOLD = 24 /* PROP_DOUBLE_BUFFERED + 1 */
};

enum {
  INSENSITIVE_PRESS,
  TAP_AND_HOLD,
  TAP_AND_HOLD_SETUP,
  TAP_AND_HOLD_QUERY,
  LAST_SIGNAL
};

static guint widget_signals[LAST_SIGNAL] = { 0 };

#define TAP_AND_HOLD_TIMER_COUNTER 6
#define TAP_AND_HOLD_TIMER_INTERVAL 100

typedef struct
{
  GtkWidget *menu;
  guint timer_id;

  GtkMenuPositionFunc func;
  gint x, y;
  gint timer_counter;
  gint signals_connected : 1;
  guint interval;
  GdkWindow *tah_on_window;

#ifdef TAP_AND_HOLD_ANIMATION
  GdkPixbufAnimation *anim;
  GdkPixbufAnimationIter *iter;
#endif
} TahData;

static gboolean
gtk_widget_tap_and_hold_button_press (GtkWidget *widget,
                                      GdkEvent  *event,
                                      TahData   *td);
static gboolean
gtk_widget_tap_and_hold_event_stop (GtkWidget *widget,
                                    gpointer   unused,
                                    TahData   *td);
#endif
/**
 * hildon_gtk_widget_set_theme_size:
 * @widget: A #GtkWidget
 * @size: #HildonSizeType flags indicating the size of the widget
 *
 * This function sets the requested size of a widget using one of the
 * predefined sizes.
 *
 * It also changes the widget name (see gtk_widget_set_name()) so it
 * can be themed accordingly.
 *
 * Since: maemo 2.0
 * Stability: Unstable
 **/
void
hildon_gtk_widget_set_theme_size (GtkWidget      *widget,
                                  HildonSizeType  size)
{
  gint width = -1;
  gint height = -1;
  gchar *widget_name = NULL;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  /* Requested height */
  if (size & HILDON_SIZE_FINGER_HEIGHT)
    {
      height = HILDON_HEIGHT_FINGER;
      widget_name = "-finger";
    }
  else if (size & HILDON_SIZE_THUMB_HEIGHT)
    {
      height = HILDON_HEIGHT_THUMB;
      widget_name = "-thumb";
    }

  if (widget_name)
    widget_name = g_strconcat (g_type_name (G_OBJECT_TYPE (widget)),
                               widget_name, NULL);

    /* Requested width */
  if (size & HILDON_SIZE_HALFSCREEN_WIDTH)
    width = HILDON_WIDTH_HALFSCREEN;
  else if (size & HILDON_SIZE_FULLSCREEN_WIDTH)
    width = HILDON_WIDTH_FULLSCREEN;

  gtk_widget_set_size_request (widget, width, height);

  if (widget_name)
    {
      gtk_widget_set_name (widget, widget_name);
      g_free (widget_name);
    }
}

/* -- Tap and hold implementation -- */

/* FIXME - those must be in GtkMenu */
#if 0
static gint context_menu_counter = 0;
static void
_gtk_menu_push_context_menu_behavior (void)
{
  context_menu_counter++;
}

static void
_gtk_menu_pop_context_menu_behavior (void)
{
  g_return_if_fail (context_menu_counter > 0);

  context_menu_counter--;
}


static TahData*
gtk_widget_peek_tah_data (GtkWidget *widget)
{
  TahData *td = g_object_get_data (G_OBJECT (widget),
                                   "MaemoGtkWidget-tap-and-hold");
  return td;
}

static void
tap_and_hold_stop_animation (TahData *td)
{
#ifdef TAP_AND_HOLD_ANIMATION
  if (td->tah_on_window)
      gdk_window_set_cursor (td->tah_on_window, NULL);
  td->tah_on_window = NULL;

  if (td->anim)
      g_object_unref (td->anim);
  td->anim = NULL;

  if (td->iter)
      g_object_unref (td->iter);
  td->iter = NULL;
#endif
}

static void
tap_and_hold_free_data (gpointer data)
{
  TahData *td = data;
  if (td)
    {
      if (td->timer_id)
          g_source_remove (td->timer_id);
      td->timer_id = 0;

      if (GTK_IS_MENU (td->menu))
          g_object_unref (td->menu);
      td->menu = NULL;

      tap_and_hold_stop_animation (td);

      g_free (td);
    }
}

static void
gtk_widget_set_tah_data (GtkWidget *widget, TahData *td)
{
  g_object_set_data_full (G_OBJECT (widget), "MaemoGtkWidget-tap-and-hold",
                          td, tap_and_hold_free_data);
}

static TahData*
gtk_widget_get_tah_data (GtkWidget *widget)
{
  TahData *td = gtk_widget_peek_tah_data (widget);
  if (!td)
    {
      td = g_new0 (TahData, 1);
      td->interval = TAP_AND_HOLD_TIMER_INTERVAL;
      gtk_widget_set_tah_data (widget, td);
    }
  return td;
}

static void
tap_and_hold_remove_timer (GtkWidget *widget)
{
  TahData *td = gtk_widget_peek_tah_data (widget);
  if (td)
    {
      if (td->timer_id)
        {
          g_source_remove (td->timer_id);
          td->timer_id = 0;
        }

      td->x = td->y = td->timer_counter = 0;
      tap_and_hold_stop_animation (td);
    }
}

#ifdef TAP_AND_HOLD_ANIMATION
static GdkPixbufAnimation *
tap_and_hold_load_animation_for_screen (GdkScreen *screen)
{
  GtkIconTheme *theme;
  GtkIconInfo *info;
  const char *filename = NULL;
  GdkPixbufAnimation *anim = NULL;
  GError *error = NULL;

  theme = gtk_icon_theme_get_for_screen (screen);

  info = gtk_icon_theme_lookup_icon (theme, "qgn_indi_tap_hold_a",
                                     GTK_ICON_SIZE_BUTTON,
                                     GTK_ICON_LOOKUP_NO_SVG);
  if (info)
      filename = gtk_icon_info_get_filename (info);
  if (!info || !filename)
    {
      g_warning ("Unable to find tap and hold icon filename");
      goto out;
    }

  anim = gdk_pixbuf_animation_new_from_file (filename, &error);
  if (!anim)
    {
      g_warning ("Unable to load tap and hold animation: %s", error->message);
      goto out;
    }

out:
  if (info)
      gtk_icon_info_free (info);

  if (error)
      g_error_free (error);

  return anim;
}
#endif

static void
tap_and_hold_init_animation (TahData *td)
{
#ifdef TAP_AND_HOLD_ANIMATION
  if (!td->anim)
    {
      td->anim =
              tap_and_hold_load_animation_for_screen (
                  gdk_drawable_get_screen (td->tah_on_window));
    }

  if (td->anim)
    {
      if (td->iter)
          g_object_unref (td->iter);
      td->iter = gdk_pixbuf_animation_get_iter (td->anim, NULL);

      td->interval = gdk_pixbuf_animation_iter_get_delay_time (td->iter);
    }
#endif
}

static gboolean
tap_and_hold_animation_timeout (GtkWidget *widget)
{
#ifdef TAP_AND_HOLD_ANIMATION
  TahData *td = gtk_widget_peek_tah_data (widget);

  if (!td || !GDK_IS_WINDOW (td->tah_on_window))
    {
      tap_and_hold_remove_timer (widget);
      return FALSE;
    }

  if (td->anim)
    {
      guint new_interval = 0;
      GTimeVal time;
      GdkScreen *screen;
      GdkPixbuf *pic;
      GdkCursor *cursor;
      const gchar *x_hot, *y_hot;
      gint x, y;

      g_get_current_time (&time);
      screen = gdk_screen_get_default ();
      pic = gdk_pixbuf_animation_iter_get_pixbuf (td->iter);

      pic = gdk_pixbuf_copy (pic);

      if (!GDK_IS_PIXBUF (pic))
        return TRUE;

      x_hot = gdk_pixbuf_get_option (pic, "x_hot");
      y_hot = gdk_pixbuf_get_option (pic, "y_hot");
      x = (x_hot) ? atoi(x_hot) : gdk_pixbuf_get_width(pic) / 2;
      y = (y_hot) ? atoi(y_hot) : gdk_pixbuf_get_height(pic) / 2;

      cursor = gdk_cursor_new_from_pixbuf (gdk_display_get_default (), pic,
                                           x, y);
      g_object_unref (pic);

      if (!cursor)
          return TRUE;

      gdk_window_set_cursor (td->tah_on_window, cursor);
      gdk_cursor_unref (cursor);

      gdk_pixbuf_animation_iter_advance (td->iter, &time);

      new_interval = gdk_pixbuf_animation_iter_get_delay_time (td->iter);

      if (new_interval != td->interval && td->timer_counter)
        {
          td->interval = new_interval;
          td->timer_id = g_timeout_add (td->interval,
                                        (GSourceFunc)gtk_widget_tap_and_hold_timeout, widget);
          return FALSE;
        }
    }
#endif
  return TRUE;
}

/**
 * gtk_widget_tap_and_hold_menu_position_top:
 * @menu: a #GtkMenu
 * @x: x cordinate to be returned
 * @y: y cordinate to be returned
 * @push_in: If going off screen, push it pack on the screen
 * @widget: a #GtkWidget
 *
 * Pre-made menu positioning function.
 * It positiones the @menu over the @widget.
 *
 * Since: maemo 1.0
 * Stability: Unstable
 **/
void
gtk_widget_tap_and_hold_menu_position_top (GtkWidget *menu,
                                           gint      *x,
                                           gint      *y,
                                           gboolean  *push_in,
                                           GtkWidget *widget)
{
  /*
   * This function positiones the menu above widgets.
   * This is a modified version of the position function
   * gtk_combo_box_position_over.
   */
  GtkWidget *topw;
  GtkRequisition requisition;
  gint screen_width = 0;
  gint menu_xpos = 0;
  gint menu_ypos = 0;
  gint w_xpos = 0, w_ypos = 0;
  gtk_widget_size_request (menu, &requisition);

  topw = gtk_widget_get_toplevel (widget);
  gdk_window_get_origin (topw->window, &w_xpos, &w_ypos);

  menu_xpos += widget->allocation.x + w_xpos;
  menu_ypos += widget->allocation.y + w_ypos - requisition.height;

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
      menu_xpos = menu_xpos + widget->allocation.width - requisition.width;

  screen_width = gdk_screen_get_width (gtk_widget_get_screen (widget));

  if (menu_xpos < w_xpos)
      menu_xpos = w_xpos;
  else if ((menu_xpos + requisition.width) > screen_width)
      menu_xpos -= ((menu_xpos + requisition.width) - screen_width);
  if (menu_ypos < w_ypos)
      menu_ypos = w_ypos;

  *x = menu_xpos;
  *y = menu_ypos;
  *push_in = TRUE;
}

/**
 * gtk_widget_tap_and_hold_setup:
 * @widget : a #GtkWidget
 * @menu : a #GtkMenu or %NULL
 * @func : a #GtkMenuPositionFunc or %NULL
 * @flags : a #GtkWidgetTapAndHoldFlags
 *
 * Setups the tap and hold functionality to the @widget.
 * The @menu is shown when the functionality is activated.
 * If the @menu is wanted to be positioned in a different way than the
 * gtk+ default, the menuposition @func can be passed as a third parameter.
 * Fourth parameter, @flags is deprecated and has no effect.
 *
 * Since: maemo 1.0
 * Stability: Unstable
 */
void
gtk_widget_tap_and_hold_setup (GtkWidget                *widget,
                               GtkWidget                *menu,
                               GtkCallback               func,
                               GtkWidgetTapAndHoldFlags  flags)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (menu == NULL || GTK_IS_MENU (menu));

  g_signal_emit (widget, widget_signals[TAP_AND_HOLD_SETUP], 0, menu, func,
                 flags);
}

static void
gtk_widget_real_tap_and_hold_setup (GtkWidget                *widget,
                                    GtkWidget                *menu,
                                    GtkCallback               func,
                                    GtkWidgetTapAndHoldFlags  flags)
{
  TahData *td;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (menu == NULL || GTK_IS_MENU (menu));

  td = gtk_widget_get_tah_data (widget);
  if (td->signals_connected)
      return;

  if (menu != NULL)
    {
      g_object_ref_sink (menu);

      if (gtk_menu_get_attach_widget (GTK_MENU (menu)) == NULL)
          gtk_menu_attach_to_widget (GTK_MENU (menu), widget, NULL);
    }

  td->menu = menu;
  td->func = (GtkMenuPositionFunc)func;
  td->signals_connected = TRUE;
  td->timer_counter = 0;

  g_signal_connect (widget, "button-press-event",
                    G_CALLBACK (gtk_widget_tap_and_hold_button_press), td);
  g_signal_connect (widget, "button-release-event",
                    G_CALLBACK (gtk_widget_tap_and_hold_event_stop), td);
  g_signal_connect (widget, "leave-notify-event",
                    G_CALLBACK (gtk_widget_tap_and_hold_event_stop), td);
  g_signal_connect (widget, "drag-begin",
                    G_CALLBACK (gtk_widget_tap_and_hold_event_stop), td);
}

static void
gtk_widget_real_tap_and_hold (GtkWidget *widget)
{
  TahData *td = gtk_widget_peek_tah_data (widget);
  if (td && GTK_IS_MENU (td->menu))
      gtk_menu_popup (GTK_MENU (td->menu), NULL, NULL,
                      (GtkMenuPositionFunc)td->func,
                      widget, 1, gdk_x11_get_server_time (widget->window));
}

static gboolean
gtk_widget_tap_and_hold_timeout (GtkWidget *widget)
{
  TahData *td = gtk_widget_peek_tah_data (widget);
  gboolean result;
  gint x = 0, y = 0;

  GDK_THREADS_ENTER ();

  if (!td || !GDK_IS_WINDOW (td->tah_on_window))
    {
      tap_and_hold_remove_timer (widget);

      GDK_THREADS_LEAVE ();
      return FALSE;
    }

  /* A small timeout before starting the tap and hold */
  if (td->timer_counter == TAP_AND_HOLD_TIMER_COUNTER)
    {
      td->timer_counter--;

      GDK_THREADS_LEAVE ();
      return TRUE;
    }

  result = tap_and_hold_animation_timeout (widget);

  if (td->timer_counter)
      td->timer_counter--;
  else
      td->timer_id = 0;

  gdk_display_get_pointer (gdk_drawable_get_display (td->tah_on_window),
                           NULL, &x, &y, NULL);

  /* Did we dragged too far from the start point */
  if (gtk_drag_check_threshold (widget, td->x, td->y, x, y))
    {
      tap_and_hold_remove_timer (widget);

      GDK_THREADS_LEAVE ();
      return FALSE;
    }

  /* Was that the last cycle -> tah starts */
  if (!td->timer_id)
    {
      tap_and_hold_remove_timer (widget);

      _gtk_menu_push_context_menu_behavior ();

      g_signal_emit (widget, widget_signals[TAP_AND_HOLD], 0);

      _gtk_menu_pop_context_menu_behavior ();

      GDK_THREADS_LEAVE ();
      return FALSE;
    }

  GDK_THREADS_LEAVE ();
  return result;
}

static gboolean
gtk_widget_tap_and_hold_query_accumulator (GSignalInvocationHint *ihint,
                                           GValue                *return_accu,
                                           const GValue          *handler_return,
                                           gpointer               dummy)
{
  gboolean tap_and_hold_not_allowed;

  /* The semantics of the tap-and-hold-query return value differs from
   * the normal event signal handlers.
   */

  tap_and_hold_not_allowed = g_value_get_boolean (handler_return);
  g_value_set_boolean (return_accu, tap_and_hold_not_allowed);

  /* Now that a single handler has run, we stop emission. */
  return FALSE;
}

static gboolean
gtk_widget_real_tap_and_hold_query (GtkWidget *widget,
                    GdkEvent  *event)
{
  return FALSE;
}

static gboolean
gtk_widget_tap_and_hold_query (GtkWidget *widget,
                   GdkEvent  *event)
{
  gboolean return_value = FALSE;

  g_signal_emit (G_OBJECT (widget), widget_signals[TAP_AND_HOLD_QUERY],
                 0, event, &return_value);

  return return_value;
}

static gboolean
gtk_widget_tap_and_hold_button_press (GtkWidget *widget,
                                      GdkEvent  *event,
                                      TahData   *td)
{
  if (event->button.type == GDK_2BUTTON_PRESS)
      return FALSE;

  if (!gtk_widget_tap_and_hold_query (widget, event) && !td->timer_id)
    {
      GdkWindow *root_window;
      gdk_display_get_pointer (gtk_widget_get_display (widget),
                   NULL, &td->x, &td->y, NULL);

      td->timer_counter = TAP_AND_HOLD_TIMER_COUNTER;
      /* We set the cursor in the root window for the TAH animation to be
         visible in every possible case, like windows completely covering
         some widget to filter events.
      */
      root_window = gtk_widget_get_root_window (widget);
      if (root_window == NULL)
        {
          /* We are getting events from a widget that's not in a
             hierarchy, it might happen (like putting a dummy widget
             as user_data in a GdkWindow). Try really hard to get
             the root window
          */
          root_window = gdk_screen_get_root_window (gdk_screen_get_default ());
        }
      td->tah_on_window = root_window;
      tap_and_hold_init_animation (td);
      td->timer_id = g_timeout_add (td->interval,
                    (GSourceFunc)
                    gtk_widget_tap_and_hold_timeout, widget);
    }
  return FALSE;
}

static gboolean
gtk_widget_tap_and_hold_event_stop (GtkWidget *widget,
                                    gpointer   unused,
                                    TahData   *td)
{
  if (td->timer_id)
      tap_and_hold_remove_timer (widget);

  return FALSE;
}


/**
 * gtk_widget_insensitive_press:
 * @widget: a #GtkWidget
 *
 * Emits the "insensitive-press" signal.
 *
 * Deprecated: Use hildon_helper_set_insensitive_message() instead.
 *
 * Since: maemo 1.0
 * Stability: Unstable
 */
void
gtk_widget_insensitive_press ( GtkWidget *widget )
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_signal_emit(widget, widget_signals[INSENSITIVE_PRESS], 0);
}
#endif
void
hildon_subclass_gtk_widget(void)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;
  g_warning("hildon_subclass_gtk_widget");

  widget_class = g_type_class_ref(GTK_TYPE_WIDGET);
  gobject_class = G_OBJECT_CLASS(widget_class);
#if 0
  /**
   * GtkWidget:tap-and-hold-state:
   *
   * Sets the state (#GtkStateType) to be used to the tap and hold
   * functionality. The default is GTK_STATE_NORMAL.
   *
   * Deprecated: Functionality for setting and getting this propery is not
   * implemented.
   *
   * Since: maemo 1.0
   * Stability: Unstable
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TAP_AND_HOLD,
                                   g_param_spec_int ("tap-and-hold-state",
                                                     P_("Tap and hold State type"),
                                                     P_("Sets the state to be used to the tap and hold functionality. The default is GTK_STATE_NORMAL"),
                                                     0,
                                                     4, /*4 == Last state in GTK+-2.0*/
                                                     GTK_STATE_NORMAL,
                                                     G_PARAM_READWRITE));

  /**
    * GtkWidget::insensitive-press:
    * @widget: the object which received the signal
    *
    * If a widget is insensitive and it receives click event,
    * the signal is emited.  Signal is made to clarify situations where
    * a widget is not easily noticable as an insensitive widget.
    *
    * Deprecated: Use hildon_helper_set_insensitive_message() instead.
    *
    * Since: maemo 1.0
    * Stability: Unstable
    */
  widget_signals[INSENSITIVE_PRESS] =
          g_signal_new ("insensitive_press",
                        G_TYPE_FROM_CLASS (gobject_class),
                        G_SIGNAL_RUN_FIRST,
                        0,
                        NULL, NULL,
                        gtk_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);
  /**
    * GtkWidget::tap-and-hold:
    * @widget: the object which received the signal
    *
    * The signal is emited when tap and hold activity occurs.
    *
    * Since: maemo 1.0
    * Stability: Unstable
    */
  widget_signals[TAP_AND_HOLD] =
          g_signal_new_class_handler ("tap_and_hold",
                                      G_TYPE_FROM_CLASS (gobject_class),
                                      G_SIGNAL_RUN_LAST,
                                      G_CALLBACK (gtk_widget_real_tap_and_hold),
                                      NULL, NULL,
                                      gtk_marshal_VOID__VOID,
                                      G_TYPE_NONE, 0);
  /**
    * GtkWidget::tap-and-hold-setup:
    * @widget: the object which received the signal
    * @menu: the menu to be opened.
    * @func: the menu position function
    * @flags: deprecated
    *
    * Enables the tap and hold functionality to the @widget.
    * Usually a @menu is used at tap and hold signal,
    * but this is optional.  Setup can be run and some other functionality
    * may be connected to it as well.  Usually this signal is not used,
    * instead the virtual function is over written.
    *
    * Since: maemo 1.0
    * Stability: Unstable
    */
  widget_signals[TAP_AND_HOLD_SETUP] =
          g_signal_new_class_handler ("tap_and_hold_setup",
                                      G_TYPE_FROM_CLASS (gobject_class),
                                      G_SIGNAL_RUN_LAST,
                                      G_CALLBACK (gtk_widget_real_tap_and_hold_setup),
                                      NULL, NULL,
                                      /*FIXME -- OBJECT_POINTER_FLAGS*/
                                      _hildon_gtk_marshal_VOID__OBJECT_UINT_FLAGS,
                                      G_TYPE_NONE, 3,
                                      G_TYPE_OBJECT,
                                      G_TYPE_POINTER,
                                      G_TYPE_UINT);

  /**
    * GtkWidget::tap-and-hold-query:
    * @widget: the object which received the signal
    * @returns: %FALSE if tap and hold is allowed to be started
    *
    * Signal is used in a situation where tap and hold is not allowed to be
    * started in some mysterious reason.  A good mysterious reason could be,
    * a widget which area is big and only part of it is allowed to start
    * tap and hold.
    *
    * Since: maemo 1.0
    * Stability: Unstable
    */
  widget_signals[TAP_AND_HOLD_QUERY] =
          g_signal_new_class_handler ("tap_and_hold_query",
                                      G_TYPE_FROM_CLASS (gobject_class),
                                      G_SIGNAL_RUN_LAST,
                                      G_CALLBACK (gtk_widget_real_tap_and_hold_query),
                                      gtk_widget_tap_and_hold_query_accumulator, NULL,
                                      _hildon_gtk_marshal_BOOLEAN__BOXED,
                                      G_TYPE_BOOLEAN, 1,
                                      GDK_TYPE_EVENT);
#endif
}

