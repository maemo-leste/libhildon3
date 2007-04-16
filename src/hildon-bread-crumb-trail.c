/*
 * This file is a part of hildon
 *
 * Copyright (C) 2007 Nokia Corporation, all rights reserved.
 *
 * Contact: Michael Dominic Kostrzewa <michael.kostrzewa@nokia.com>
 * Author: Xan Lopez <xan.lopez@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "hildon-bread-crumb-trail.h"

#define HILDON_BREAD_CRUMB_TRAIL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_TYPE_BREAD_CRUMB_TRAIL, HildonBreadCrumbTrailPrivate))

struct _HildonBreadCrumbTrailPrivate
{
  GtkWidget *back_button;
  GList *item_list;
  gchar *path_separator;
};

/* Signals */

enum {
  CRUMB_CLICKED,
  LAST_SIGNAL
};

/* Properties */

enum {
  PROP_0
};

static void hildon_bread_crumb_trail_size_request (GtkWidget *widget,
                                                   GtkRequisition *requisition);
static void hildon_bread_crumb_trail_size_allocate (GtkWidget *widget,
                                                    GtkAllocation *allocation);
static void hildon_bread_crumb_trail_add (GtkContainer *container,
                                          GtkWidget *widget);
static void hildon_bread_crumb_trail_forall (GtkContainer *container,
                                             gboolean include_internals,
                                             GtkCallback callback,
                                             gpointer callback_data);
static void hildon_bread_crumb_trail_remove (GtkContainer *container,
                                             GtkWidget *widget);
static void hildon_bread_crumb_trail_finalize (GObject *object);
static void hildon_bread_crumb_trail_scroll_back (GtkWidget *button,
                                                  HildonBreadCrumbTrail *bct);
static void hildon_bread_crumb_trail_update_back_button_sensitivity (HildonBreadCrumbTrail *bct);

static guint bread_crumb_trail_signals[LAST_SIGNAL] = { 0 };

/* GType methods */

G_DEFINE_TYPE (HildonBreadCrumbTrail, hildon_bread_crumb_trail, GTK_TYPE_CONTAINER)

static void
hildon_bread_crumb_trail_class_init (HildonBreadCrumbTrailClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*)klass;
  GtkObjectClass *object_class = (GtkObjectClass*)klass;
  GtkWidgetClass *widget_class = (GtkWidgetClass*)klass;
  GtkContainerClass *container_class = (GtkContainerClass*)klass;

  /* GObject signals */
  gobject_class->finalize = hildon_bread_crumb_trail_finalize;

  /* GtkWidget signals */
  widget_class->size_request = hildon_bread_crumb_trail_size_request;
  widget_class->size_allocate = hildon_bread_crumb_trail_size_allocate;

  /* GtkContainer signals */
  container_class->add = hildon_bread_crumb_trail_add;
  container_class->forall = hildon_bread_crumb_trail_forall;
  container_class->remove = hildon_bread_crumb_trail_remove;

  /* Style properties */

#define _BREAD_CRUMB_TRAIL_MINIMUM_WIDTH 10

  /* FIXME: is this the best way to do it? */
  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("minimum-width",
                                                             "Minimum width",
                                                             "The minimum width in characters the children widgets will request",
                                                             0,
                                                             G_MAXINT,
                                                             _BREAD_CRUMB_TRAIL_MINIMUM_WIDTH,
                                                             G_PARAM_READABLE));

#define _BREAD_CRUMB_TRAIL_MAXIMUM_WIDTH 250

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("maximum-width",
                                                             "Maximum width",
                                                             "The maximum width in characters the children widgets will be allocated",
                                                             0,
                                                             G_MAXINT,
                                                             _BREAD_CRUMB_TRAIL_MAXIMUM_WIDTH,
                                                             G_PARAM_READABLE));
                                                             
#define _BREAD_CRUMB_TRAIL_ARROW_SIZE 34

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("arrow-size",
                                                             "Arrow size",
                                                             "Size of the back button arrow",
                                                             0,
                                                             G_MAXINT,
                                                             _BREAD_CRUMB_TRAIL_ARROW_SIZE,
                                                             G_PARAM_READABLE));
  /* Signals */
  bread_crumb_trail_signals[CRUMB_CLICKED] =
    g_signal_new ("crumb-clicked",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (HildonBreadCrumbTrailClass, crumb_clicked),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);
                  
  /* Private data */
  g_type_class_add_private (gobject_class, sizeof (HildonBreadCrumbTrailPrivate));
}

static void
hildon_bread_crumb_trail_finalize (GObject *object)
{
  HildonBreadCrumbTrailPrivate *priv = HILDON_BREAD_CRUMB_TRAIL (object)->priv;

  g_free (priv->path_separator);
  g_list_free (priv->item_list);

  G_OBJECT_CLASS (hildon_bread_crumb_trail_parent_class)->finalize (object);
}

static void
hildon_bread_crumb_trail_size_request (GtkWidget *widget,
                                       GtkRequisition *requisition)
{
  GList *p;
  GtkRequisition child_requisition;
  HildonBreadCrumbTrail *bct;
  HildonBreadCrumbTrailPrivate *priv;
  gint minimum_width, width = 0;
  PangoLayout *layout;
  gchar *tmp = NULL;

  bct= HILDON_BREAD_CRUMB_TRAIL (widget);
  priv = bct->priv;

  requisition->height = 0;
  requisition->width = 0;

  gtk_widget_size_request (priv->back_button, &child_requisition);
  g_debug ("BACK BUTTON REQUEST %d %d", child_requisition.width, child_requisition.width);
  requisition->width = child_requisition.width;
  requisition->height = child_requisition.height;

  if (priv->item_list)
    {
      /* Add minimum width for one item */
      /* TODO: this can be probably cached */
      gtk_widget_style_get (widget,
                            "minimum-width", &minimum_width,
                            NULL);

      tmp = g_strnfill ((gsize)minimum_width, 'm');
      layout = gtk_widget_create_pango_layout (widget, tmp);
      g_free (tmp);
      pango_layout_get_size (layout, &width, NULL);
      requisition->width += PANGO_PIXELS (width);
      g_object_unref (layout);
    }

  /* Button requisitions */
  for (p = priv->item_list; p; p = p->next)
    {
      gtk_widget_size_request (GTK_WIDGET (p->data), &child_requisition);
    }

  /* Border width */
  requisition->width += GTK_CONTAINER (widget)->border_width * 2;
  requisition->height += GTK_CONTAINER (widget)->border_width * 2;

  widget->requisition = *requisition;
}

/* Document me please */

static void
hildon_bread_crumb_trail_size_allocate (GtkWidget *widget,
                                        GtkAllocation *allocation)
{
  GtkRequisition req;
  gint natural_width, natural_height;
  HildonBreadCrumb *item;
  GtkAllocation child_allocation;
  GtkRequisition child_requisition;
  GtkWidget *child;
  gint allocation_width;
  gint border_width, width;
  gint extra_space;
  gint maximum_width;
  gint desired_width;
  GList *p, *first_show, *first_hide;
  HildonBreadCrumbTrailPrivate *priv = HILDON_BREAD_CRUMB_TRAIL (widget)->priv;

  widget->allocation = *allocation;

  border_width = (gint) GTK_CONTAINER (widget)->border_width;
  allocation_width = allocation->width - 2 * border_width;

  /* Allocate the back button */
  child_allocation.x = allocation->x;
  child_allocation.y = allocation->y;
  gtk_widget_get_child_requisition (priv->back_button, &child_requisition);
  child_allocation.width = child_requisition.width;
  child_allocation.height = child_requisition.height;
  gtk_widget_size_allocate (priv->back_button, &child_allocation);
  child_allocation.x += child_allocation.width;

  /* If there are no buttons there's nothing else to do */
  if (priv->item_list == NULL)
    return;

  /* We find out how many buttons can we show, starting from the
     the last one in the logical path (the first item in the list) */

  width = child_allocation.width;
  p = priv->item_list;
  first_show = NULL;
  first_hide = NULL; 
  extra_space = 0;

  for (p = priv->item_list; p; p = p->next)
    {
      item = HILDON_BREAD_CRUMB (p->data);
      child = GTK_WIDGET (item);

      /* Does the widget fit with its natural size? */
      hildon_bread_crumb_get_natural_size (item,
                                           &natural_width,
                                           &natural_height);

      gtk_widget_style_get (widget,
                            "maximum-width", &maximum_width,
                            NULL);

      desired_width = MIN (natural_width, maximum_width);

      if (width + desired_width <= allocation_width)
        {
          /* Yes, it does */
          first_show = p;
          first_hide = p->next;
          width += desired_width;
          g_debug ("It fits %s at x %d, width %d", hildon_bread_crumb_get_text (item),
                   child_allocation.x, desired_width);
        }
      else
        {
          /* No, it doesn't. Allocate as much as possible
             and stop */
          child_allocation.width = allocation_width - width;

          gtk_widget_size_request (child, &req);

          if (child_allocation.width > req.width)
            {
              first_hide = p->next;
              gtk_widget_size_allocate (child, &child_allocation);
              gtk_widget_show (child);
              gtk_widget_set_child_visible (child, TRUE);
              g_debug ("It doesn't. Allocated %s at x: %d, width %d, size_req width is %d",
                       hildon_bread_crumb_get_text (item), 
                       child_allocation.x, child_allocation.width,
                       req.width);
              child_allocation.x += child_allocation.width;
            }
          else
            {
              extra_space = child_allocation.width;
            }

          break;
        }
    }

  /* Not enough items to fill the breadcrumb? */
  if (p == NULL && width < allocation_width)
    {
      extra_space = allocation_width - width;
    }

  /* Allocate the other buttons */
  for (p = first_show; p; p = p->prev)
    {
      item = HILDON_BREAD_CRUMB (p->data);
      child = GTK_WIDGET (item);

      /* Does the widget fit with its natural size? */
      hildon_bread_crumb_get_natural_size (item,
                                           &natural_width,
                                           &natural_height);

      gtk_widget_style_get (widget,
                            "maximum-width", &maximum_width,
                            NULL);

      desired_width = MIN (natural_width, maximum_width);

      /* If I'm the last and there's extra space, use it */
      if (p->prev == NULL && extra_space != 0)
        {
          desired_width += extra_space;
        }

      child_allocation.width = desired_width;
      gtk_widget_size_allocate (child, &child_allocation);
      gtk_widget_show (child);
      gtk_widget_set_child_visible (child, TRUE);
      g_debug ("Natural Size. Allocated %s at x: %d, width %d",
               hildon_bread_crumb_get_text (item), 
               child_allocation.x, child_allocation.width);
      child_allocation.x += child_allocation.width;
    }

  for (p = first_hide; p; p = p->next)
    {
      item = HILDON_BREAD_CRUMB (p->data);
      child = GTK_WIDGET (item);

      g_debug ("Hiding %s", hildon_bread_crumb_get_text (item));
      gtk_widget_hide (GTK_WIDGET (item));
      gtk_widget_set_child_visible (GTK_WIDGET (item), FALSE);
    }
}

static gpointer
get_bread_crumb_id (HildonBreadCrumb *item)
{
  return g_object_get_data (G_OBJECT (item), "bread-crumb-id");
}

static void
crumb_clicked_cb (GtkWidget *button,
                  HildonBreadCrumbTrail *bct)
{
  GtkWidget *child;
  HildonBreadCrumbTrailPrivate *priv;

  priv = bct->priv;

  child = GTK_WIDGET (priv->item_list->data);

  /* We remove the tip of the list until we hit the clicked button */
  while (child != button)
    {
      gtk_container_remove (GTK_CONTAINER (bct), child);

      child = GTK_WIDGET (priv->item_list->data);
    }

  /* We need the item for the ID */
  g_signal_emit (bct, bread_crumb_trail_signals[CRUMB_CLICKED], 0,
                 get_bread_crumb_id (HILDON_BREAD_CRUMB (child)));
}

static void
hildon_bread_crumb_trail_add (GtkContainer *container,
                              GtkWidget *widget)
{
  gtk_widget_set_parent (widget, GTK_WIDGET (container));
}

static void
hildon_bread_crumb_trail_forall (GtkContainer *container,
                                 gboolean include_internals,
                                 GtkCallback callback,
                                 gpointer callback_data)
{
  g_return_if_fail (callback != NULL);
  g_return_if_fail (HILDON_IS_BREAD_CRUMB_TRAIL (container));

  GList *children;
  HildonBreadCrumbTrailPrivate *priv = HILDON_BREAD_CRUMB_TRAIL (container)->priv;

  children = priv->item_list;

  while (children)
    {
      GtkWidget *child;
      child = GTK_WIDGET (children->data);
      children = children->next;

      (*callback) (child, callback_data);
    }

  if (include_internals && priv->back_button)
    {
      (*callback) (priv->back_button, callback_data);
    }
}

static void
hildon_bread_crumb_trail_remove (GtkContainer *container,
                                 GtkWidget *widget)
{
  GList *p;
  HildonBreadCrumbTrailPrivate *priv;
  gboolean was_visible = GTK_WIDGET_VISIBLE (widget);

  priv = HILDON_BREAD_CRUMB_TRAIL (container)->priv;

  p = priv->item_list;

  while (p)
    {
      if (widget == GTK_WIDGET (p->data))
        {
          g_signal_handlers_disconnect_by_func (widget, G_CALLBACK (crumb_clicked_cb),
                                                HILDON_BREAD_CRUMB_TRAIL (container));
          gtk_widget_unparent (widget);

          priv->item_list = g_list_remove_link (priv->item_list, p);
          g_list_free (p);

          hildon_bread_crumb_trail_update_back_button_sensitivity (HILDON_BREAD_CRUMB_TRAIL (container));

          if (was_visible)
            {
              gtk_widget_queue_resize (GTK_WIDGET (container));
            }
        }

      p = p->next;
    }
}

static void
hildon_bread_crumb_trail_update_back_button_sensitivity (HildonBreadCrumbTrail *bct)
{
  guint list_length;
  HildonBreadCrumbTrailPrivate *priv = bct->priv;

  list_length = g_list_length (priv->item_list);

  if (list_length == 0 || list_length == 1)
    {
      gtk_widget_set_sensitive (priv->back_button, FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (priv->back_button, TRUE);
    }
}

static GtkWidget*
create_back_button (HildonBreadCrumbTrail *bct)
{
  GtkWidget *button;
  GtkWidget *arrow;
  gint arrow_size;

  gtk_widget_push_composite_child ();

  button = gtk_button_new ();
  gtk_widget_set_name (button, "hildon-bread-crumb-back-button");
  gtk_widget_set_sensitive (button, FALSE);

  arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);
  gtk_widget_style_get (GTK_WIDGET (bct),
                        "arrow-size", &arrow_size,
                        NULL);
  gtk_widget_set_size_request (arrow, arrow_size, arrow_size);

  gtk_container_add (GTK_CONTAINER (button), arrow);
  gtk_container_add (GTK_CONTAINER (bct), button);
  gtk_widget_show_all (button);

  gtk_widget_pop_composite_child ();

  return button;
}

static void
hildon_bread_crumb_trail_init (HildonBreadCrumbTrail *bct)
{
  HildonBreadCrumbTrailPrivate *priv = HILDON_BREAD_CRUMB_TRAIL_GET_PRIVATE (bct);

  GTK_WIDGET_SET_FLAGS (bct, GTK_NO_WINDOW);
  gtk_widget_set_redraw_on_allocate (GTK_WIDGET (bct), FALSE);

  bct->priv = priv;
  priv->item_list = NULL;

  priv->back_button = create_back_button (bct);
  g_signal_connect (priv->back_button, "clicked",
                    G_CALLBACK (hildon_bread_crumb_trail_scroll_back),
                    bct);
}

static void
hildon_bread_crumb_trail_scroll_back (GtkWidget *button,
                                      HildonBreadCrumbTrail *bct)
{
  HildonBreadCrumb *item;

  hildon_bread_crumb_trail_pop (bct);

  item = HILDON_BREAD_CRUMB (bct->priv->item_list->data);

  g_signal_emit (bct, bread_crumb_trail_signals[CRUMB_CLICKED], 0,
                 get_bread_crumb_id (item));
}

static GtkWidget*
create_crumb_button (HildonBreadCrumbTrail *bct,
                     const gchar *text,
                     gpointer id,
                     GDestroyNotify destroy)
{
  GtkWidget *item;

  item = hildon_bread_crumb_new (text);

  if (id)
    g_object_set_data_full (G_OBJECT (item), "bread-crumb-id", id, destroy);

  g_signal_connect (G_OBJECT (item), "clicked",
                    G_CALLBACK (crumb_clicked_cb), bct);

  return item;
}


/* PUBLIC API */

/**
 * hildon_bread_crumb_trail_new:
 * 
 * Creates a new #HildonBreadCrumbTrail widget.
 *
 * Returns: a #GtkWidget pointer of newly created bread crumb trail
 * widget
 */

GtkWidget*
hildon_bread_crumb_trail_new (void)
{
  return GTK_WIDGET (g_object_new (HILDON_TYPE_BREAD_CRUMB_TRAIL, NULL));
}

/* This is stupidly duplicated from the simple API, consolidate */

/**
 * hildon_bread_crumb_trail_push:
 * @bct: pointer to #HildonBreadCrumbTrail
 * @item: the #HildonBreadCrumb to be added to the trail
 * @id: optional id for the bread crumb
 * @destroy: GDestroyNotify callback to be called when the bread crumb is destroyed
 *
 * Adds a new bread crumb to the end of the trail.
 */

void
hildon_bread_crumb_trail_push (HildonBreadCrumbTrail *bct,
                               HildonBreadCrumb *item,
                               gpointer id,
                               GDestroyNotify destroy)
{
  GtkWidget *widget;
  HildonBreadCrumbTrailPrivate *priv;;

  g_return_if_fail (HILDON_IS_BREAD_CRUMB_TRAIL (bct));
  g_return_if_fail (HILDON_IS_BREAD_CRUMB (item));

  priv = bct->priv;

  widget = GTK_WIDGET (item);

  gtk_container_add (GTK_CONTAINER (bct), widget);
  gtk_widget_show (widget);

  if (id)
    g_object_set_data_full (G_OBJECT (item), "bread-crumb-id", id, destroy);

  g_signal_connect (G_OBJECT (item), "clicked",
                    G_CALLBACK (crumb_clicked_cb), bct);

  priv->item_list = g_list_prepend (priv->item_list, widget);

  hildon_bread_crumb_trail_update_back_button_sensitivity (bct);
}

/**
 * hildon_bread_crumb_trail_push_text:
 * @bct: pointer to #HildonBreadCrumbTrail
 * @text: content of the new bread crumb
 * @id: optional id for the bread crumb
 * @destroy: GDestroyNotify callback to be called when the bread crumb is destroyed
 *
 * Adds a new bread crumb to the end of the trail containing the specified text.
 */

void
hildon_bread_crumb_trail_push_text (HildonBreadCrumbTrail *bct,
                                    const gchar *text,
                                    gpointer id,
                                    GDestroyNotify destroy)
{
  GtkWidget *widget;
  HildonBreadCrumbTrailPrivate *priv;

  g_return_if_fail (HILDON_IS_BREAD_CRUMB_TRAIL (bct));
  g_return_if_fail (text != NULL);

  priv = bct->priv;

  widget = create_crumb_button (bct, text, id, destroy);
  gtk_container_add (GTK_CONTAINER (bct), widget);
  gtk_widget_show (widget);

  priv->item_list = g_list_prepend (priv->item_list, widget);

  hildon_bread_crumb_trail_update_back_button_sensitivity (bct);
}

/**
 * hildon_bread_crumb_trail_pop:
 * @bct: pointer to #HildonBreadCrumbTrail
 *
 * Removes the last bread crumb from the trail.
 */

void
hildon_bread_crumb_trail_pop (HildonBreadCrumbTrail *bct)
{
  GtkWidget *child;
  HildonBreadCrumbTrailPrivate *priv;

  g_return_if_fail (HILDON_IS_BREAD_CRUMB_TRAIL (bct));

  priv = bct->priv;

  if (priv->item_list == NULL)
    return;

  if (priv->item_list)
    {
      child = GTK_WIDGET (priv->item_list->data);
      gtk_container_remove (GTK_CONTAINER (bct), child);
    }

  hildon_bread_crumb_trail_update_back_button_sensitivity (bct);
}

/**
 * hildon_bread_crumb_trail_clear:
 * @bct: pointer to #HildonBreadCrumbTrail
 *
 * Removes all the bread crumbs from the bread crumb trail.
 */

void
hildon_bread_crumb_trail_clear (HildonBreadCrumbTrail *bct)
{
  HildonBreadCrumbTrailPrivate *priv;

  g_return_if_fail (HILDON_IS_BREAD_CRUMB_TRAIL (bct));

  priv = bct->priv;

  while (priv->item_list)
    {
      hildon_bread_crumb_trail_pop (bct);
    }
}