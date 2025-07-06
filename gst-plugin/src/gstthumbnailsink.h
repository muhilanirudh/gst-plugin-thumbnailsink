/* gstthumbnailsink.h */

#ifndef __GST_THUMBNAILSINK_H__
#define __GST_THUMBNAILSINK_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_THUMBNAILSINK (gst_thumbnailsink_get_type())
G_DECLARE_FINAL_TYPE (GstThumbnailsink, gst_thumbnailsink,
    GST, THUMBNAILSINK, GstBin)

struct _GstThumbnailsink
{
  GstBin parent;
  
  GstElement *convert;
  GstElement *videorate;
  GstElement *capsfilter;
  GstElement *jpegenc;
  GstElement *filesink;

  GstPad     *sinkpad;
  gboolean    silent;
  guint       thumb_count;
};

G_END_DECLS

#endif /* __GST_THUMBNAILSINK_H__ */
