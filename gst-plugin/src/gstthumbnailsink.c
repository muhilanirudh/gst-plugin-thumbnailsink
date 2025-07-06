/* gstthumbnailsink.c */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gstthumbnailsink.h"
#include <gst/gst.h>

#define DEFAULT_MAX_RATE 1

GST_DEBUG_CATEGORY_STATIC (gst_thumbnailsink_debug);
#define GST_CAT_DEFAULT gst_thumbnailsink_debug

static GstStaticPadTemplate sink_template =
  GST_STATIC_PAD_TEMPLATE ("sink",
      GST_PAD_SINK,
      GST_PAD_ALWAYS,
      GST_STATIC_CAPS_ANY /* accept all raw caps */
);

enum { PROP_0, PROP_SILENT };

/* Forward declarations */
static void     gst_thumbnailsink_set_property  (GObject      *object,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void     gst_thumbnailsink_get_property  (GObject      *object,
                                            guint         prop_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);
static gboolean gst_thumbnailsink_sink_event    (GstPad       *pad,
                                            GstObject    *parent,
                                            GstEvent     *event);
static gboolean thumbnailsink_init             (GstPlugin    *plugin);

static GstPadProbeReturn
log_probe_cb (GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
  static guint64 count = 0;
  g_print("Thumbnail buffer %" G_GUINT64_FORMAT "\n", count++);
  return GST_PAD_PROBE_OK;
}


G_DEFINE_TYPE (GstThumbnailsink, gst_thumbnailsink, GST_TYPE_BIN);

static void
gst_thumbnailsink_class_init (GstThumbnailsinkClass *klass)
{
  GObjectClass    *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  /* Install properties */
  gobject_class->set_property = gst_thumbnailsink_set_property;
  gobject_class->get_property = gst_thumbnailsink_get_property;
  g_object_class_install_property (
      gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent",
                            "Suppress logging?", FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* Add pad template */
  gst_element_class_add_pad_template (
      element_class,
      gst_static_pad_template_get (&sink_template));

  gst_element_class_set_static_metadata (
      element_class,
      "thumbnailsink",
      "Filter/Effect/Video",
      "Generate one JPEG thumbnail per second",
      "user1 <user@hostname.org>");
}

static void
gst_thumbnailsink_init (GstThumbnailsink *filter)
{
  /* Initialize state */
  filter->silent = FALSE;
  filter->thumb_count = 0;

  /* Create elements */
  filter->convert    = gst_element_factory_make ("videoconvert",   "thumb_convert");
  filter->videorate  = gst_element_factory_make ("videorate",      "thumb_rate");
  filter->capsfilter = gst_element_factory_make ("capsfilter",     "thumb_caps");
  filter->jpegenc    = gst_element_factory_make ("jpegenc",        "thumb_enc");
  filter->filesink   = gst_element_factory_make ("multifilesink",  "thumb_sink");

  /* Configure videorate: one frame per second */
  g_object_set (filter->videorate,
                "drop-only", TRUE,
                "max-rate", DEFAULT_MAX_RATE,
                NULL);

  /* Configure capsfilter: enforce 1 fps */
  {
    GstCaps *caps = gst_caps_new_simple (
      "video/x-raw",
      "framerate", GST_TYPE_FRACTION, 1, 1,
      NULL);
    g_object_set (filter->capsfilter, "caps", caps, NULL);
    gst_caps_unref (caps);
  }

  /* Configure multifilesink */
  g_object_set (filter->filesink,
                "location", "thumb-%05d.jpg",
                NULL);

  /* Build internal pipeline: convert → rate → caps → encode → sink */
  gst_bin_add_many (GST_BIN (filter),
                    filter->convert,
                    filter->videorate,
                    filter->capsfilter,
                    filter->jpegenc,
                    filter->filesink,
                    NULL);
  gst_element_link_many (filter->convert,
                         filter->videorate,
                         filter->capsfilter,
                         filter->jpegenc,
                         filter->filesink,
                         NULL);

  /* Expose a ghost sink on the convert’s sink pad */
  {
    GstPad *cv_sink = gst_element_get_static_pad (
        filter->convert, "sink");
    filter->sinkpad = gst_ghost_pad_new_from_template (
        "sink",
        cv_sink,
        gst_element_class_get_pad_template (
            GST_ELEMENT_GET_CLASS (filter), "sink"));
    gst_pad_set_event_function (filter->sinkpad,
        GST_DEBUG_FUNCPTR (gst_thumbnailsink_sink_event));
    gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);
    gst_object_unref (cv_sink);

    /* Optional: Debug probe to count thumbnails */
    GstPad *pad = gst_element_get_static_pad(filter->capsfilter, "src");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback) log_probe_cb, NULL, NULL);
    gst_object_unref(pad);
  }
}


static void
gst_thumbnailsink_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GstThumbnailsink *filter = GST_THUMBNAILSINK (object);
  if (prop_id == PROP_SILENT)
    filter->silent = g_value_get_boolean (value);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
gst_thumbnailsink_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GstThumbnailsink *filter = GST_THUMBNAILSINK (object);
  if (prop_id == PROP_SILENT)
    g_value_set_boolean (value, filter->silent);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static gboolean
gst_thumbnailsink_sink_event (GstPad    *pad,
                              GstObject *parent,
                              GstEvent  *event)
{
  return gst_pad_event_default (pad, parent, event);
}


static gboolean
thumbnailsink_init (GstPlugin *plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_thumbnailsink_debug, "thumbnailsink", 0,
                          "Thumbnail Generator");
  return gst_element_register (plugin,
                               "thumbnailsink",
                               GST_RANK_NONE,
                               GST_TYPE_THUMBNAILSINK);
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    thumbnailsink,
    "Generate JPEG thumbnails at 1fps",
    thumbnailsink_init,
    "1.0",
    "LGPL",
    "GStreamer",
    "https://gstreamer.freedesktop.org/"
)
