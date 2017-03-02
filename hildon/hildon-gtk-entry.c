#include "hildon-gtk.h"
#include "hildon-gtk-entry.h"
#include "hildon-enum-types.h"

#include <gtk/gtk.h>

#include <string.h>

#include <libintl.h>

#ifdef ENABLE_NLS
#define P_(String) g_dgettext("hildon-libs-properties",String)
#else
#define P_(String) (String)
#endif

// Define to remain compatible with fremantle-gtk2 source:
#define GTK_PARAM_READABLE G_PARAM_READABLE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB
#define GTK_PARAM_WRITABLE G_PARAM_WRITABLE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB
#define GTK_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB

enum {
  PROP_HILDON_PLACEHOLDER_TEXT = 43 /* PROP_EDITING_CANCELED + 1 */,
  PROP_HILDON_INPUT_MODE,
  PROP_HILDON_INPUT_DEFAULT
};

/**
 * hildon_gtk_entry_set_placeholder_text:
 * @entry: a #GtkEntry
 * @placeholder_text: a string to be displayed when @entry is empty
 * and unfocused or %NULL to remove current placeholder text.
 *
 * Sets a text string to be displayed when @entry is empty and unfocused.
 * This can be provided to give a visual hint of the expected contents
 * of the #GtkEntry.
 *
 * Since: maemo 5
 **/
void
hildon_gtk_entry_set_placeholder_text (GtkEntry    *entry,
                                       const gchar *placeholder_text)
{
  g_return_if_fail (GTK_IS_ENTRY (entry));

  gtk_entry_set_placeholder_text (entry, placeholder_text);

  g_object_notify (G_OBJECT (entry), "hildon-placeholder-text");
}

/**
 * hildon_gtk_entry_get_placeholder_text:
 * @entry: a #GtkEntry
 *
 * Gets the text to be displayed if @entry is empty and unfocused.
 *
 * Returns: a string or %NULL if no placeholder text is set
 *
 * Since: maemo 5
 **/
const gchar *
hildon_gtk_entry_get_placeholder_text (GtkEntry *entry)
{
  g_return_val_if_fail (GTK_IS_ENTRY (entry), NULL);

  return gtk_entry_get_placeholder_text (entry);
}

void
hildon_gtk_entry_set_input_mode (GtkEntry           *entry,
                                 HildonGtkInputMode  mode)
{
  g_return_if_fail (GTK_IS_ENTRY (entry));

  if (hildon_gtk_entry_get_input_mode (entry) != mode)
    {
      gtk_entry_set_visibility (entry,
                                mode & HILDON_GTK_INPUT_MODE_INVISIBLE
                                ? FALSE : TRUE);
//      g_object_set (G_OBJECT (entry->im_context),
//                    "hildon-input-mode", mode, NULL);
      g_object_notify (G_OBJECT (entry), "hildon-input-mode");
  }
}

HildonGtkInputMode
hildon_gtk_entry_get_input_mode (GtkEntry *entry)
{
  HildonGtkInputMode mode;

  g_return_val_if_fail (GTK_IS_ENTRY (entry), FALSE);

//  g_object_get (G_OBJECT (entry->im_context),
//                "hildon-input-mode", &mode, NULL);

  return mode;
}

/**
 * hildon_gtk_entry_set_input_default:
 * @entry: a #GtkEntry
 * @mode: a #HildonGtkInputMode
 *
 * Sets the default input mode of the widget.
 *
 * Since: maemo 5.0
 */
void
hildon_gtk_entry_set_input_default (GtkEntry           *entry,
                                    HildonGtkInputMode  mode)
{
  g_return_if_fail (GTK_IS_ENTRY (entry));

  if (hildon_gtk_entry_get_input_default (entry) != mode)
    {
//      g_object_set (G_OBJECT (entry->im_context),
//                    "hildon-input-default", mode, NULL);
      g_object_notify (G_OBJECT (entry), "hildon-input-default");
    }
}

/**
 * hildon_gtk_entry_get_input_default:
 * @entry: a #GtkEntry
 *
 * Gets the default input mode of the widget.
 *
 * Return value: the default input mode of the widget.
 *
 * Since: maemo 5.0
 */
HildonGtkInputMode
hildon_gtk_entry_get_input_default (GtkEntry *entry)
{
  HildonGtkInputMode mode;

  g_return_val_if_fail (GTK_IS_ENTRY (entry), FALSE);

//  g_object_get (G_OBJECT (entry->im_context),
//                "hildon-input-default", &mode, NULL);

  return mode;
}


static void (*old_gtk_entry_set_property)(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void (*old_gtk_entry_get_property)(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static void
hildon_gtk_entry_set_property (GObject         *object,
                               guint            prop_id,
                               const GValue    *value,
                               GParamSpec      *pspec)
{
  GtkEntry *entry = GTK_ENTRY (object);

  if (!strcmp(pspec->name, "visibility"))
  {
    /* converting to hildon input mode first then through
     * that mode changing function to reach compatible with
     * the gtk original visibility changing
     */
    HildonGtkInputMode mode = hildon_gtk_entry_get_input_mode (entry);

    if (g_value_get_boolean (value))
        mode &= ~HILDON_GTK_INPUT_MODE_INVISIBLE;
    else
        mode |= HILDON_GTK_INPUT_MODE_INVISIBLE;

    hildon_gtk_entry_set_input_mode (entry, mode);

    return;
  }

  switch (prop_id)
    {
    case PROP_HILDON_PLACEHOLDER_TEXT:
      hildon_gtk_entry_set_placeholder_text (entry, g_value_get_string (value));
      break;

    case PROP_HILDON_INPUT_MODE:
      hildon_gtk_entry_set_input_mode (entry, g_value_get_flags (value));
      break;

    case PROP_HILDON_INPUT_DEFAULT:
      hildon_gtk_entry_set_input_default (entry, g_value_get_flags (value));
      break;
    default:
      old_gtk_entry_set_property(object, prop_id, value, pspec);
      break;
    }
}

static void
hildon_gtk_entry_get_property (GObject         *object,
                               guint            prop_id,
                               GValue          *value,
                               GParamSpec      *pspec)
{
  GtkEntry *entry = GTK_ENTRY (object);

  switch (prop_id)
    {
    case PROP_HILDON_PLACEHOLDER_TEXT:
      g_value_set_string (value, hildon_gtk_entry_get_placeholder_text (entry));
      break;

    case PROP_HILDON_INPUT_MODE:
      g_value_set_flags (value, hildon_gtk_entry_get_input_mode (entry));
      break;

    case PROP_HILDON_INPUT_DEFAULT:
      g_value_set_flags (value, hildon_gtk_entry_get_input_default (entry));
      break;
    default:
      old_gtk_entry_get_property (object, prop_id, value, pspec);
      break;
    }
}

void
hildon_subclass_gtk_entry(void)
{
  GObjectClass *gobject_class;
  GtkEntryClass *entry_class;
  GtkEntry *entry;
  g_warning("hildon_subclass_gtk_entry");

  entry = GTK_ENTRY(gtk_entry_new());
  entry_class = GTK_ENTRY_GET_CLASS(entry);
  gobject_class = G_OBJECT_CLASS(entry_class);

  old_gtk_entry_set_property = gobject_class->set_property;
  old_gtk_entry_get_property = gobject_class->get_property;
  gobject_class->set_property = hildon_gtk_entry_set_property;
  gobject_class->get_property = hildon_gtk_entry_get_property;

  /**
  * GtkEntry:hildon-placeholder-text:
  *
  * Text to be displayed in the #GtkEntry when it is empty.
  *
  * Since: maemo 5
  */
  g_object_class_install_property (gobject_class,
                                   PROP_HILDON_PLACEHOLDER_TEXT,
                                   g_param_spec_string ("hildon-placeholder-text",
                                                        P_("Hildon Placeholder text"),
                                                        P_("Text to be displayed when the entry is empty"),
                                                        "",
                                                        G_PARAM_READWRITE));

  /**
   * GtkEntry:hildon-input-mode:
   *
   * Allowed characters and input mode for the entry. See #HildonGtkInputMode.
   *
   * Since: maemo 2.0
   * Stability: Unstable
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_HILDON_INPUT_MODE,
                                   g_param_spec_flags ("hildon-input-mode",
                                                       P_("Hildon input mode"),
                                                       P_("Define widget's input mode"),
                                                       HILDON_TYPE_GTK_INPUT_MODE,
                                                       HILDON_GTK_INPUT_MODE_FULL |
                                                       HILDON_GTK_INPUT_MODE_AUTOCAP |
                                                       HILDON_GTK_INPUT_MODE_DICTIONARY,
                                                       GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  /**
   * GtkEntry:hildon-input-default:
   *
   * Default input mode for this IM context.  See #HildonGtkInputMode.
   * The default setting for this property is %HILDON_GTK_INPUT_MODE_FULL,
   * which means that the default input mode to be used is up to the
   * implementation of the IM context.
   *
   * Since: maemo 5
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_HILDON_INPUT_DEFAULT,
                                   g_param_spec_flags ("hildon-input-default",
                                                       P_("Hildon input default"),
                                                       P_("Define widget's default input mode"),
                                                       HILDON_TYPE_GTK_INPUT_MODE,
                                                       HILDON_GTK_INPUT_MODE_FULL,
                                                       GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

