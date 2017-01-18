/*
 * This file is a part of hildon
 *
 * GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GTK Calendar Widget
 * Copyright (C) 1998 Cesar Miquel and Shawn T. Amundson
 *
 * HldonCalendar modifications
 * Copyright (C) 2005, 2006 Nokia Corporation. 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __HILDON_CALENDAR_H__
#define __HILDON_CALENDAR_H__


#include <gtk/gtk.h>


G_BEGIN_DECLS

#define HILDON_TYPE_CALENDAR                  (hildon_calendar_get_type ())
#define HILDON_CALENDAR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_CALENDAR, HildonCalendar))
#define HILDON_CALENDAR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_CALENDAR, HildonCalendarClass))
#define HILDON_IS_CALENDAR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_CALENDAR))
#define HILDON_IS_CALENDAR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_CALENDAR))
#define HILDON_CALENDAR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_CALENDAR, HildonCalendarClass))


typedef struct _HildonCalendar	       HildonCalendar;
typedef struct _HildonCalendarClass       HildonCalendarClass;

typedef struct _HildonCalendarPrivate     HildonCalendarPrivate;

/**
 * HildonCalendarDisplayOptions:
 * @HILDON_CALENDAR_SHOW_HEADING: Specifies that the month and year should be displayed.
 * @HILDON_CALENDAR_SHOW_DAY_NAMES: Specifies that three letter day descriptions should be present.
 * @HILDON_CALENDAR_NO_MONTH_CHANGE: Prevents the user from switching months with the calendar.
 * @HILDON_CALENDAR_SHOW_WEEK_NUMBERS: Displays each week numbers of the current year, down the
 * left side of the calendar.
 * @HILDON_CALENDAR_SHOW_DETAILS: Just show an indicator, not the full details
 * text when details are provided. See hildon_calendar_set_detail_func().
 *
 * These options can be used to influence the display and behaviour of a #HildonCalendar.
 */
typedef enum
{
  HILDON_CALENDAR_SHOW_HEADING		= 1 << 0,
  HILDON_CALENDAR_SHOW_DAY_NAMES		= 1 << 1,
  HILDON_CALENDAR_NO_MONTH_CHANGE		= 1 << 2,
  HILDON_CALENDAR_SHOW_WEEK_NUMBERS	= 1 << 3,
  HILDON_CALENDAR_SHOW_DETAILS		= 1 << 5
} HildonCalendarDisplayOptions;

/**
 * HildonCalendarDetailFunc:
 * @calendar: a #HildonCalendar.
 * @year: the year for which details are needed.
 * @month: the month for which details are needed.
 * @day: the day of @month for which details are needed.
 * @user_data: the data passed with hildon_calendar_set_detail_func().
 *
 * This kind of functions provide Pango markup with detail information for the
 * specified day. Examples for such details are holidays or appointments. The
 * function returns %NULL when no information is available.
 *
 * Since: 2.14
 *
 * Returns: (nullable) (transfer full): Newly allocated string with Pango markup
 *     with details for the specified day or %NULL.
 */
typedef gchar* (*HildonCalendarDetailFunc) (HildonCalendar *calendar,
                                            guint        year,
                                            guint        month,
                                            guint        day,
                                            gpointer     user_data);

struct _HildonCalendar
{
  GtkWidget widget;

  HildonCalendarPrivate *priv;
};

struct _HildonCalendarClass
{
  GtkWidgetClass parent_class;
  
  /* Signal handlers */
  void (* month_changed)		(HildonCalendar *calendar);
  void (* day_selected)			(HildonCalendar *calendar);
  void (* day_selected_double_click)	(HildonCalendar *calendar);
  void (* prev_month)			(HildonCalendar *calendar);
  void (* next_month)			(HildonCalendar *calendar);
  void (* prev_year)			(HildonCalendar *calendar);
  void (* next_year)			(HildonCalendar *calendar);

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};



GType	   hildon_calendar_get_type	(void) G_GNUC_CONST;

GtkWidget* hildon_calendar_new		(void);


void       hildon_calendar_select_month	(HildonCalendar *calendar,
					 guint	      month,
					 guint	      year);

void	   hildon_calendar_select_day	(HildonCalendar *calendar,
					 guint	      day);


void       hildon_calendar_mark_day	(HildonCalendar *calendar,
					 guint	      day);

void       hildon_calendar_unmark_day	(HildonCalendar *calendar,
					 guint	      day);

void	   hildon_calendar_clear_marks	(HildonCalendar *calendar);



void	   hildon_calendar_set_display_options (HildonCalendar    	      *calendar,
					     HildonCalendarDisplayOptions flags);

HildonCalendarDisplayOptions
           hildon_calendar_get_display_options (HildonCalendar   	      *calendar);

void	   hildon_calendar_get_date	(HildonCalendar *calendar, 
					 guint	     *year,
					 guint	     *month,
					 guint	     *day);


void       hildon_calendar_set_detail_func (HildonCalendar           *calendar,
                                         HildonCalendarDetailFunc  func,
                                         gpointer               data,
                                         GDestroyNotify         destroy);


void       hildon_calendar_set_detail_width_chars (HildonCalendar    *calendar,
                                                gint            chars);

void       hildon_calendar_set_detail_height_rows (HildonCalendar    *calendar,
                                                gint            rows);


gint       hildon_calendar_get_detail_width_chars (HildonCalendar    *calendar);

gint       hildon_calendar_get_detail_height_rows (HildonCalendar    *calendar);


gboolean   hildon_calendar_get_day_is_marked      (HildonCalendar    *calendar,
                                                guint           day);

G_END_DECLS

#endif /* __HILDON_CALENDAR_H__ */

