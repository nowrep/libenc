#include "surface.h"
#include "util.h"
#include "dev.h"

#include <unistd.h>
#include <va/va_drmcommon.h>
#include <vector>

enc_surface::enc_surface()
{
}

enc_surface::~enc_surface()
{
   if (dev)
      vaDestroySurfaces(dev->dpy, &surface_id, 1);
}

bool enc_surface::create(const struct enc_surface_params *params)
{
   dev = const_cast<enc_dev *>(params->dev);
   color_standard = params->color_standard;

   uint32_t rt_format = 0;
   uint32_t va_fourcc = 0;
   switch (params->format) {
   case ENC_FORMAT_NV12:
      rt_format = VA_RT_FORMAT_YUV420;
      va_fourcc = VA_FOURCC_NV12;
      break;
   case ENC_FORMAT_P010:
      rt_format = VA_RT_FORMAT_YUV420_10;
      va_fourcc = VA_FOURCC_P010;
      break;
   case ENC_FORMAT_RGBA:
      rt_format = VA_RT_FORMAT_RGB32;
      va_fourcc = VA_FOURCC_RGBA;
      break;
   case ENC_FORMAT_BGRA:
      rt_format = VA_RT_FORMAT_RGB32;
      va_fourcc = VA_FOURCC_BGRA;
      break;
   case ENC_FORMAT_RGBA10:
      rt_format = VA_RT_FORMAT_RGB32_10;
      va_fourcc = VA_FOURCC_A2R10G10B10;
      break;
   case ENC_FORMAT_BGRA10:
      rt_format = VA_RT_FORMAT_RGB32_10;
      va_fourcc = VA_FOURCC_A2B10G10R10;
      break;
   default:
      std::cerr << "Invalid surface format " << params->format << std::endl;
      return false;
   }

   std::vector<VASurfaceAttrib> attribs;

   VASurfaceAttrib attrib;
   attrib.type = VASurfaceAttribPixelFormat;
   attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
   attrib.value.type = VAGenericValueTypeInteger;
   attrib.value.value.i = va_fourcc;
   attribs.push_back(attrib);

   VADRMPRIMESurfaceDescriptor desc = {};
   if (params->dmabuf) {
      desc.fourcc = va_fourcc;
      desc.width = params->dmabuf->width;
      desc.height = params->dmabuf->height;
      desc.num_objects = 1;
      desc.objects[0].fd = params->dmabuf->fd;
      desc.objects[0].drm_format_modifier = params->dmabuf->modifier;
      desc.num_layers = params->dmabuf->num_layers;
      for (uint32_t i = 0; i < desc.num_layers; i++) {
         desc.layers[i].drm_format = params->dmabuf->layers[i].drm_format;
         desc.layers[i].num_planes = params->dmabuf->layers[i].num_planes;
         for (uint32_t j = 0; j < desc.layers[i].num_planes; j++) {
             desc.layers[i].offset[j] = params->dmabuf->layers[i].offset[j];
             desc.layers[i].pitch[j] = params->dmabuf->layers[i].pitch[j];
         }
      }

      attrib.type = VASurfaceAttribMemoryType;
      attrib.value.type = VAGenericValueTypeInteger;
      attrib.value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2;
      attribs.push_back(attrib);

      attrib.type = VASurfaceAttribExternalBufferDescriptor;
      attrib.value.type = VAGenericValueTypePointer;
      attrib.value.value.p = &desc;
      attribs.push_back(attrib);
   }

   VAStatus status = vaCreateSurfaces(dev->dpy, rt_format, params->width, params->height, &surface_id, 1, attribs.data(), attribs.size());
   return va_check(status, "vaCreateSurfaces");
}

bool enc_surface::export_dmabuf(struct enc_dmabuf *dmabuf)
{
   if (!dev)
      return false;

   VADRMPRIMESurfaceDescriptor desc;
   VAStatus status = vaExportSurfaceHandle(dev->dpy, surface_id, VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2, VA_EXPORT_SURFACE_COMPOSED_LAYERS | VA_EXPORT_SURFACE_READ_WRITE, &desc);
   if (!va_check(status, "vaExportSurfaceHandle"))
      return false;

   if (desc.num_objects != 1) {
      std::cerr << "Expected 1 object, got " << desc.num_objects << std::endl;
      for (uint32_t i = 0; i < desc.num_objects; i++)
         close(desc.objects[i].fd);
      return false;
   }

   dmabuf->fd = desc.objects[0].fd;
   dmabuf->width = desc.width;
   dmabuf->height = desc.height;
   dmabuf->modifier = desc.objects[0].drm_format_modifier;
   dmabuf->num_layers = desc.num_layers;
   for (uint32_t i = 0; i < desc.num_layers; i++) {
      dmabuf->layers[i].drm_format = desc.layers[i].drm_format;
      dmabuf->layers[i].num_planes = desc.layers[i].num_planes;
      for (uint32_t j = 0; j < desc.layers[i].num_planes; j++) {
         dmabuf->layers[i].offset[j] = desc.layers[i].offset[j];
         dmabuf->layers[i].pitch[j] = desc.layers[i].pitch[j];
      }
   }
   return true;
}

static VAProcColorStandardType va_color_standard(enc_color_standard standard)
{
   switch (standard) {
   case ENC_COLOR_STANDARD_SRGB:
      return VAProcColorStandardSRGB;
   case ENC_COLOR_STANDARD_BT601:
      return VAProcColorStandardBT601;
   case ENC_COLOR_STANDARD_BT709:
      return VAProcColorStandardBT709;
   default:
      return VAProcColorStandardNone;
   }
}

bool enc_surface::copy(struct enc_surface *out)
{
   VAContextID context_id = dev->get_proc_context();

   VAStatus status = vaBeginPicture(dev->dpy, context_id, out->surface_id);
   if (!va_check(status, "vaBeginPicture"))
      return false;

   VAProcPipelineParameterBuffer proc = {};
   proc.surface = surface_id;
   proc.surface_color_standard = va_color_standard(color_standard);
   proc.output_color_standard = va_color_standard(out->color_standard);

   VABufferID buf;
   status = vaCreateBuffer(dev->dpy, context_id, VAProcPipelineParameterBufferType, sizeof(proc), 1, &proc, &buf);
   if (!va_check(status, "vaCreateBuffer"))
      return false;

   status = vaRenderPicture(dev->dpy, context_id, &buf, 1);
   vaDestroyBuffer(dev->dpy, buf);

   if (!va_check(status, "vaRenderPicture"))
      return false;

   status = vaEndPicture(dev->dpy, context_id);
   return va_check(status, "vaEndPicture");
}
