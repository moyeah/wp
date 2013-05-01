#include <string.h>
#include <errno.h>
#include <webkit2/webkit2.h>

#include "wb_window.h"

static const gchar **uri_arguments = NULL;

static const GOptionEntry command_line_options[] = 
{
  {
     G_OPTION_REMAINING, 0, 0,
     G_OPTION_ARG_FILENAME_ARRAY, &uri_arguments, 0,
     "[URL...]"
  },
  { 0, 0, 0, 0, 0, 0, 0 }
};

static gchar*
argument_to_url (const char *filename)
{
  GFile *gfile = g_file_new_for_commandline_arg (filename);
  gchar *file_url = g_file_get_uri (gfile);
  g_object_unref (gfile);

  return file_url;
}

static gboolean
is_valid_parameter_type (GType g_param_type)
{
  return (g_param_type == G_TYPE_BOOLEAN ||
            g_param_type == G_TYPE_STRING ||
	    g_param_type == G_TYPE_INT ||
	    g_param_type == G_TYPE_FLOAT);
}

static gboolean
parse_option_entry_callback (const gchar    *option_name_full,
                             const gchar    *value,
			     WebKitSettings *webkit_settings,
			     GError         *error)
{
  if (strlen (option_name_full) <= 2)
  {
    g_set_error (&error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
                 "Invalid option %s", option_name_full);
    return FALSE;
  }

  const gchar *option_name = option_name_full + 2;
  GParamSpec *spec = g_object_class_find_property (G_OBJECT_GET_CLASS (
                                                     webkit_settings),
                                                   option_name);
  if (!spec)
  {
    g_set_error (&error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
                 "Cannot find web setting for option %s", option_name_full);
    return FALSE;
  }

  switch (G_PARAM_SPEC_VALUE_TYPE (spec))
  {
    case G_TYPE_BOOLEAN:
    {
      gboolean property_value = !(value &&
                                    g_ascii_strcasecmp (value, "true") &&
				    strcmp (value, "1"));
      g_object_set (G_OBJECT (webkit_settings), option_name,
                    property_value, NULL);
      break;
    }
    case G_TYPE_STRING:
    {
      g_object_set (G_OBJECT (webkit_settings), option_name, value, NULL);
      break;
    }
    case G_TYPE_INT:
    {
      glong property_value;
      gchar *end;

      errno = 0;
      property_value = g_ascii_strtoll (value, &end, 0);
      if (errno == ERANGE || property_value > G_MAXINT ||
            property_value < G_MININT)
      {
        g_set_error (&error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     "Integer value '%s' for %s out of range",
                     value, option_name_full);
        return FALSE;
      }
      if (errno || value == end)
      {
        g_set_error (&error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     "Cannot parse integer value '%s' for %s",
                     value, option_name_full);
	return FALSE;
      }
      g_object_set (G_OBJECT (webkit_settings), option_name,
                    property_value, NULL);
      break;
    }
    case G_TYPE_FLOAT:
    {
      gdouble property_value;
      gchar *end;

      errno = 0;
      property_value = g_ascii_strtod (value, &end);
      if (errno == ERANGE || property_value > G_MAXFLOAT ||
            property_value < G_MINFLOAT)
      {
        g_set_error (&error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     "Float value '%s' for %s out of range",
                     value, option_name_full);
        return FALSE;
      }
      if (errno || value == end)
      {
        g_set_error (&error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     "Cannot parse float value '%s' for %s",
                     value, option_name_full);
	return FALSE;
      }
      g_object_set (G_OBJECT (webkit_settings), option_name,
                    property_value, NULL);
      break;
    }
    default:
      g_assert_not_reached ();
  }

  return TRUE;
}

static GOptionEntry*
get_option_entries_from_webkit_settings (WebKitSettings *webkit_settings)
{
  GParamSpec **property_specs;
  GOptionEntry *option_entries;
  guint num_properties;
  guint num_entries = 0;
  guint i;

  property_specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (
                                                     webkit_settings),
                                                   &num_properties);
  if (!property_specs)
    return NULL;

  option_entries = g_new0 (GOptionEntry, num_properties + 1);
  for (i = 0; i < num_properties; i++)
  {
    GParamSpec *param = property_specs[i];

    if (!param || !(param->flags & G_PARAM_WRITABLE) ||
          (param->flags &G_PARAM_CONSTRUCT_ONLY))
      continue;

    GType g_param_type = G_PARAM_SPEC_VALUE_TYPE (param);
    if (!is_valid_parameter_type (g_param_type))
      continue;

    GOptionEntry *option_entry = &option_entries[num_entries++];
    option_entry->long_name = g_param_spec_get_name (param);

    if (g_param_type == G_TYPE_BOOLEAN && (strstr (option_entry->long_name,
                                                   "enable")))
      option_entry->flags = G_OPTION_FLAG_OPTIONAL_ARG;
    
    option_entry->arg = G_OPTION_ARG_CALLBACK;
    option_entry->arg_data = parse_option_entry_callback;
    option_entry->description = g_param_spec_get_blurb (param);
    option_entry->arg_description = g_type_name (g_param_type);
  }
  g_free (property_specs);

  return option_entries;
}

static gboolean
add_settings_group_to_context (GOptionContext *context,
                               WebKitSettings *webkit_settings)
{
  GOptionEntry *option_entries =
    get_option_entries_from_webkit_settings (webkit_settings);

  if (!option_entries)
    return FALSE;

  GOptionGroup *web_settings_group =
    g_option_group_new ("websettings",
                        "WebKitSettings writable properties for default",
			"WebKitSettings properties",
			webkit_settings,
			NULL);
  g_option_group_add_entries (web_settings_group, option_entries);
  g_free (option_entries);

  g_option_context_add_group (context, web_settings_group);

  return TRUE;
}

static void
create_wb_window (const gchar *uri, WebKitSettings *webkit_settings)
{
  GtkWidget *web_view = webkit_web_view_new ();
  GtkWidget *main_window = wb_window_new (WEBKIT_WEB_VIEW (web_view),
                                          NULL);
  gchar *url = argument_to_url (uri);

  if (webkit_settings)
  {
    webkit_web_view_set_settings (WEBKIT_WEB_VIEW (web_view),
                                  webkit_settings);
    g_object_unref (webkit_settings);
  }

  wb_window_load_uri (WB_WINDOW (main_window), url);
  g_free (url);

  gtk_widget_grab_focus (web_view);
  gtk_widget_show_all (main_window);
}

int
main (int argc, char *argv[])
{
  gtk_init (&argc, &argv);

  GOptionContext *context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context,
                                     command_line_options, 0);

  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  WebKitSettings *webkit_settings = webkit_settings_new ();
  webkit_settings_set_enable_developer_extras (webkit_settings, TRUE);
  if (!add_settings_group_to_context (context, webkit_settings))
  {
    g_object_unref (webkit_settings);
    webkit_settings = 0;
  }

  GError *error = 0;
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_printerr ("Cannot parse arguments: %s\n", error->message);
    g_error_free (error);
    g_option_context_free (context);

    return 1;
  }
  g_option_context_free (context);

#ifdef WEBKIT_EXEC_PATH
  g_setenv ("WEBKIT_INSPECTOR_PATH", WEBKIT_EXEC_PATH "resources/inspector",
            FALSE);
#endif /* WEBKIT_EXEC_PATH */

#ifdef WEBKIT_INJECTED_BUNDLE_PATH
  g_setenv ("WEBKIT_INJECTED_BUNDLE_PATH", WEBKIT_INJECTED_BUNDLE_PATH,
            FALSE);
#endif /* WEBKIT_INJECTED_BUNDLE_PATH */

// TODO
//  webkit_web_context_set_favicon_database_directory (
//    webkit_web_context_get_default (), NULL);

  if (uri_arguments)
  {
    int i;
    for (i = 0; uri_arguments[i]; i++)
      create_wb_window (uri_arguments[i], webkit_settings);
  }
  else
    create_wb_window ("https://www.google.pt", webkit_settings);

  gtk_main ();

  return 0;
}
