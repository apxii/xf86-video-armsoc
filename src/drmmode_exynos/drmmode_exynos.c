/*
 * Copyright © 2013 ARM Limited.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "../drmmode_driver.h"
#include <stddef.h>
#include <xf86drmMode.h>
#include <xf86drm.h>
#include <sys/ioctl.h>

#include <exynos_drmif.h>

/* Cursor dimensions
 * Technically we probably don't have any size limit.. since we
 * are just using an overlay... but xserver will always create
 * cursor images in the max size, so don't use width/height values
 * that are too big
 */
#define CURSORW  (64)
#define CURSORH  (64)

/*
 * Padding added down each side of cursor image. This is a workaround for a bug
 * causing corruption when the cursor reaches the screen edges.
 */
#define CURSORPAD (16)

#define ALIGN(val, align)	(((val) + (align) - 1) & ~((align) - 1))

static int init_plane_for_cursor(int drm_fd, uint32_t plane_id)
{
	int res = -1;
	drmModeObjectPropertiesPtr props;
	props = drmModeObjectGetProperties(drm_fd, plane_id,
			DRM_MODE_OBJECT_PLANE);

	if (props) {
		int i;

		for (i = 0; i < props->count_props; i++) {
			drmModePropertyPtr this_prop;
			this_prop = drmModeGetProperty(drm_fd, props->props[i]);

			if (this_prop) {
				if (!strncmp(this_prop->name, "zpos",
							DRM_PROP_NAME_LEN)) {
					res = drmModeObjectSetProperty(drm_fd,
							plane_id,
							DRM_MODE_OBJECT_PLANE,
							this_prop->prop_id,
							1);
					drmModeFreeProperty(this_prop);
					break;
				}
				drmModeFreeProperty(this_prop);
			}
		}
		drmModeFreeObjectProperties(props);
	}

	return res;
}

static int create_custom_gem(int fd, struct armsoc_create_gem *create_gem)
{
	struct exynos_device *ed;
	struct exynos_bo *bo;

	ed = malloc(sizeof(*ed));
	if(!ed)
		return -1;

	ed->fd = fd;

	/* make pitch a multiple of 64 bytes for best performance */
	create_gem->pitch = ALIGN(create_gem->width * ((create_gem->bpp + 7) / 8), 64);
	create_gem->size = create_gem->height * create_gem->pitch;

	bo = exynos_bo_create(ed, create_gem->size, EXYNOS_BO_NONCONTIG | EXYNOS_BO_WC);
	if (!bo)
		return -1;

	create_gem->handle = bo->handle;

	return 0;
}

struct drmmode_interface exynos_interface = {
	1                     /* use_page_flip_events */,
	1                     /* use_early_display */,
	CURSORW               /* cursor width */,
	CURSORH               /* cursor_height */,
	CURSORPAD             /* cursor padding */,
	HWCURSOR_API_PLANE    /* cursor_api */,
	init_plane_for_cursor /* init_plane_for_cursor */,
	0                     /* vblank_query_supported */,
	create_custom_gem     /* create_custom_gem */,
};

struct drmmode_interface *drmmode_interface_get_implementation(int drm_fd)
{
	return &exynos_interface;
}
