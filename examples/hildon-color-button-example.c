/*
 * This file is a part of hildon examples
 *
 * Copyright (C) 2005, 2006 Nokia Corporation, all rights reserved.
 *
 * Author: Michael Dominic Kostrzewa <michael.kostrzewa@nokia.com>
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

#include                                        <stdio.h>
#include                                        <stdlib.h>
#include                                        <glib.h>
#include                                        <gtk/gtk.h>
#include                                        "hildon.h"

int
main                                            (int argc,
                                                 char **argv)
{
    hildon_gtk_init (&argc, &argv);

    GtkDialog *dialog = GTK_DIALOG (gtk_dialog_new ());
    GtkWidget *button = hildon_color_button_new ();
    GtkWidget *label = gtk_label_new ("Pick the color:");
    GtkWidget *hbox = gtk_hbox_new (FALSE, 12);

    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (dialog->vbox), hbox, TRUE, TRUE, 0);

    gtk_dialog_add_button (dialog, "Close", GTK_RESPONSE_CLOSE);

    gtk_widget_show_all (GTK_WIDGET (dialog));
    gtk_dialog_run (dialog);

    GdkColor *color;
    g_object_get (G_OBJECT (button), "color", &color, NULL);

    g_debug ("Color is: %d %d %d", color->red, color->green, color->blue);
    
    return 0;
}


