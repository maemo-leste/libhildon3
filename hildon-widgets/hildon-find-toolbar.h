/*
 * This file is part of hildon-libs
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Luc Pionchon <luc.pionchon@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
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

#ifndef __HILDON_FIND_TOOLBAR_H__
#define __HILDON_FIND_TOOLBAR_H__

#include <gtk/gtktoolbar.h>
#include <gtk/gtkliststore.h>

G_BEGIN_DECLS

#define HILDON_TYPE_FIND_TOOLBAR (hildon_find_toolbar_get_type())
#define HILDON_FIND_TOOLBAR(object) \
  (G_TYPE_CHECK_INSTANCE_CAST((object), HILDON_TYPE_FIND_TOOLBAR, \
  HildonFindToolbar))
#define HILDON_FIND_TOOLBARClass(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), HILDON_TYPE_FIND_TOOLBAR, \
  HildonFindToolbarClass))
#define HILDON_IS_FIND_TOOLBAR(object) \
  (G_TYPE_CHECK_INSTANCE_TYPE((object), HILDON_TYPE_FIND_TOOLBAR))
#define HILDON_IS_FIND_TOOLBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), HILDON_TYPE_FIND_TOOLBAR))
#define HILDON_FIND_TOOLBAR_GET_CLASS(object) \
  (G_TYPE_INSTANCE_GET_CLASS((object), HILDON_TYPE_FIND_TOOLBAR, \
  HildonFindToolbarClass))

typedef struct _HildonFindToolbar HildonFindToolbar;
typedef struct _HildonFindToolbarClass HildonFindToolbarClass;
typedef struct _HildonFindToolbarPrivate HildonFindToolbarPrivate;

struct _HildonFindToolbar
{
  GtkToolbar parent;
  
  HildonFindToolbarPrivate *priv;
};

struct _HildonFindToolbarClass
{
  GtkToolbarClass parent_class;

  void		(*search) 		(HildonFindToolbar *toolbar, gpointer data);
  void 		(*close)		(HildonFindToolbar *toolbar, gpointer data);
  void 		(*invalid_input)	(HildonFindToolbar *toolbar, gpointer data);
  gboolean	(*history_append)	(HildonFindToolbar *tooblar, gpointer data);
};

GType		hildon_find_toolbar_get_type		(void) G_GNUC_CONST;
GtkWidget*	hildon_find_toolbar_new			(gchar *label);
GtkWidget*	hildon_find_toolbar_new_with_model	(gchar *label,
							 GtkListStore*
							 model,
							 gint column);
void		hildon_find_toolbar_highlight_entry	(HildonFindToolbar *ftb,
							 gboolean get_focus);

G_END_DECLS

#endif
