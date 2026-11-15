/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * <linux/usb/gadget.h>
 *
 * We call the USB "gadget" API (for the "device" side of USB) the
 * master/slave API. The slave side is the gadget.
 */

#ifndef __LINUX_USB_GADGET_H
#define __LINUX_USB_GADGET_H

#include <linux/errno.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/usb/ch9.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/workqueue.h>

struct usb_ep;

/**
 * struct usb_ep_caps - endpoint capabilities description
 * @type_control: Endpoint supports control type
 * @type_iso: Endpoint supports isochronous type
 * @type_bulk: Endpoint supports bulk type
 * @type_int: Endpoint supports interrupt type
 * @dir_in: Endpoint supports IN direction
 * @dir_out: Endpoint supports OUT direction
 */
struct usb_ep_caps {
	unsigned type_control:1;
	unsigned type_iso:1;
	unsigned type_bulk:1;
	unsigned type_int:1;
	unsigned dir_in:1;
	unsigned dir_out:1;
};

/**
 * struct usb_request - describes one i/o request
 * @buf: Buffer used for data.
 * @dma: DMA address for bulk/intr endpoints.
 * @length: Length of that data
 * @stream_id: The stream id, when USB3.0 bulk streams are being used
 * @no_interrupt: If true, hints that no completion irq is needed.
 * @zero: If true, when requests are loaded, they must sink a
 *	zero length packet (unless @length is zero) to terminate
 *	the transfer.
 * @short_not_ok: When read, causes a host controller to use
 *	short reads to terminate the transfer.
 * @complete: Completion handler.
 * @context: For use by the completion handler.
 * @list: For use by the host controller driver.
 * @status: For use by the host controller driver.
 * @actual: The number of bytes transferred.
 */
struct usb_request {
	void			*buf;
	unsigned		length;
	dma_addr_t		dma;

	unsigned		stream_id:16;
	unsigned		no_interrupt:1;
	unsigned		zero:1;
	unsigned		short_not_ok:1;

	void			(*complete)(struct usb_ep *ep,
					struct usb_request *req);
	void			*context;
	struct list_head	list;

	int			status;
	unsigned		actual;
};

/* ----------------------------------------------------------------------- */

struct usb_ep_ops {
	int (*enable) (struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc);
	int (*disable) (struct usb_ep *ep);

	struct usb_request *(*alloc_request) (struct usb_ep *ep,
		gfp_t gfp_flags);
	void (*free_request) (struct usb_ep *ep, struct usb_request *req);

	int (*queue) (struct usb_ep *ep, struct usb_request *req,
		gfp_t gfp_flags);
	int (*dequeue) (struct usb_ep *ep, struct usb_request *req);

	int (*set_halt) (struct usb_ep *ep, int value);
	int (*set_wedge) (struct usb_ep *ep);

	int (*fifo_status) (struct usb_ep *ep);
	void (*fifo_flush) (struct usb_ep *ep);
};

/**
 * struct usb_ep - device side representation of USB endpoint
 */
struct usb_ep {
	void			*driver_data;

	const char		*name;
	const struct usb_ep_ops	*ops;
	struct list_head	ep_list;
	struct usb_ep_caps	caps;
	bool			claimed;
	unsigned		maxpacket:16;
	unsigned		maxpacket_limit:16;
	unsigned		max_streams:16;
	unsigned		mult:2;
	unsigned		maxburst:5;
	u8			address;
	const struct usb_endpoint_descriptor	*desc;
	const struct usb_ss_ep_comp_descriptor	*comp_desc;
};

/* ----------------------------------------------------------------------- */

#if IS_ENABLED(CONFIG_USB_GADGET)
int usb_ep_queue(struct usb_ep *ep, struct usb_request *req, gfp_t gfp_flags);
int usb_ep_dequeue(struct usb_ep *ep, struct usb_request *req);
int usb_ep_set_halt(struct usb_ep *ep);
int usb_ep_clear_halt(struct usb_ep *ep);
int usb_ep_set_wedge(struct usb_ep *ep);
int usb_ep_fifo_status(struct usb_ep *ep);
void usb_ep_fifo_flush(struct usb_ep *ep);
#else
static inline int usb_ep_queue(struct usb_ep *ep, struct usb_request *req,
			       gfp_t gfp_flags)
{
	return 0;
}
static inline int usb_ep_dequeue(struct usb_ep *ep, struct usb_request *req)
{
	return 0;
}
static inline int usb_ep_set_halt(struct usb_ep *ep)
{
	return 0;
}
static inline int usb_ep_clear_halt(struct usb_ep *ep)
{
	return 0;
}
static inline int usb_ep_set_wedge(struct usb_ep *ep)
{
	return 0;
}
static inline int usb_ep_fifo_status(struct usb_ep *ep)
{
	return 0;
}
static inline void usb_ep_fifo_flush(struct usb_ep *ep)
{
}
#endif

/* ----------------------------------------------------------------------- */

struct usb_dcd_config_params {
	__u8  bMaxBurst;
	__le16 wMaxPacketSize;
};

struct usb_gadget;
struct usb_gadget_driver;

/**
 * struct usb_gadget_ops - operations provided by a USB Device Controller
 */
struct usb_gadget_ops {
	int	(*get_frame)(struct usb_gadget *);
	int	(*wakeup)(struct usb_gadget *);
	int	(*set_selfpowered) (struct usb_gadget *, int is_selfpowered);
	int	(*vbus_session) (struct usb_gadget *, int is_active);
	int	(*vbus_draw) (struct usb_gadget *, unsigned mA);
	int	(*pullup) (struct usb_gadget *, int is_on);
	int	(*ioctl)(struct usb_gadget *,
				unsigned code, unsigned long param);
	void	(*get_config_params)(struct usb_dcd_config_params *);
	int	(*udc_start)(struct usb_gadget *,
			     struct usb_gadget_driver *);
	int	(*udc_stop)(struct usb_gadget *);
	struct usb_ep *(*match_ep)(struct usb_gadget *,
			struct usb_endpoint_descriptor *,
			struct usb_ss_ep_comp_descriptor *);

	int	(*check_config)(struct usb_gadget *gadget);
	/* readonly to gadget driver */
	const struct usb_gadget_ops *ops;

	/**
	 * udc_set_speed - Chooses the maximum speed
	 */
	void	(*udc_set_speed)(struct usb_gadget *gadget,
				 enum usb_device_speed);

	/**
	 * udc_async_callbacks - Enables or disables asynchronous callbacks.
	 */
	void	(*udc_async_callbacks)(struct usb_gadget *gadget, bool enable);
};

/**
 * struct usb_gadget - represents a usb slave device
 */
struct usb_gadget {
	struct work_struct		work;
	const struct usb_gadget_ops	*ops;
	struct usb_ep			*ep0;
	struct list_head		ep_list;
	enum usb_device_speed		speed;
	enum usb_device_speed		max_speed;
	enum usb_device_state		state;
	const char			*name;
	struct device			dev;
	unsigned			out_epnum;
	unsigned			in_epnum;
	unsigned			is_dualspeed:1;
	unsigned			is_otg:1;
	unsigned			is_a_peripheral:1;
	unsigned			b_hnp_enable:1;
	unsigned			a_hnp_support:1;
	unsigned			a_alt_hnp_support:1;
	unsigned			quirk_ep0_mps_limit:1;
};

static inline void set_gadget_data(struct usb_gadget *gadget, void *data)
{
	gadget->dev.driver_data = data;
}

static inline void *get_gadget_data(struct usb_gadget *gadget)
{
	return gadget->dev.driver_data;
}

static inline struct usb_gadget *dev_to_usb_gadget(struct device *dev)
{
	return container_of(dev, struct usb_gadget, dev);
}

#define gadget_for_each_ep(tmp, gadget) \
	list_for_each_entry(tmp, &(gadget)->ep_list, ep_list)

static inline size_t
usb_ep_align_maybe(struct usb_gadget *g, struct usb_ep *ep, size_t len)
{
	if (g->quirk_ep0_mps_limit)
		return roundup(len, ep->maxpacket);
	else
		return len;
}

static inline int gadget_is_dualspeed(struct usb_gadget *g)
{
	return g->is_dualspeed;
}

static inline int gadget_is_superspeed(struct usb_gadget *g)
{
	return g->max_speed >= USB_SPEED_SUPER;
}

static inline int gadget_is_superspeed_plus(struct usb_gadget *g)
{
	return g->max_speed >= USB_SPEED_SUPER_PLUS;
}

static inline int gadget_is_otg(struct usb_gadget *g)
{
	return g->is_otg;
}

static inline int usb_gadget_frame_number(struct usb_gadget *gadget)
{
	return gadget->ops->get_frame(gadget);
}

static inline int usb_gadget_wakeup(struct usb_gadget *gadget)
{
	if (!gadget->ops->wakeup)
		return -EOPNOTSUPP;
	return gadget->ops->wakeup(gadget);
}

static inline int usb_gadget_set_selfpowered(struct usb_gadget *gadget)
{
	if (!gadget->ops->set_selfpowered)
		return -EOPNOTSUPP;
	return gadget->ops->set_selfpowered(gadget, 1);
}

static inline int usb_gadget_clear_selfpowered(struct usb_gadget *gadget)
{
	if (!gadget->ops->set_selfpowered)
		return -EOPNOTSUPP;
	return gadget->ops->set_selfpowered(gadget, 0);
}

static inline int usb_gadget_vbus_connect(struct usb_gadget *gadget)
{
	if (!gadget->ops->vbus_session)
		return -EOPNOTSUPP;
	return gadget->ops->vbus_session(gadget, 1);
}

static inline int usb_gadget_vbus_draw(struct usb_gadget *gadget, unsigned mA)
{
	if (!gadget->ops->vbus_draw)
		return -EOPNOTSUPP;
	return gadget->ops->vbus_draw(gadget, mA);
}

static inline int usb_gadget_vbus_disconnect(struct usb_gadget *gadget)
{
	if (!gadget->ops->vbus_session)
		return -EOPNOTSUPP;
	return gadget->ops->vbus_session(gadget, 0);
}

static inline int usb_gadget_connect(struct usb_gadget *gadget)
{
	if (!gadget->ops->pullup)
		return -EOPNOTSUPP;
	return gadget->ops->pullup(gadget, 1);
}

static inline int usb_gadget_disconnect(struct usb_gadget *gadget)
{
	if (!gadget->ops->pullup)
		return -EOPNOTSUPP;
	return gadget->ops->pullup(gadget, 0);
}

void usb_gadget_set_state(struct usb_gadget *gadget,
		enum usb_device_state state);

/* ----------------------------------------------------------------------- */

struct usb_gadget_driver {
	char			*function;
	enum usb_device_speed	speed;
	int			(*bind)(struct usb_gadget *);
	void			(*unbind)(struct usb_gadget *);
	int			(*setup)(struct usb_gadget *,
					const struct usb_ctrlrequest *);
	void			(*disconnect)(struct usb_gadget *);
	void			(*suspend)(struct usb_gadget *);
	void			(*resume)(struct usb_gadget *);
	void			(*reset)(struct usb_gadget *);

	/* FIXME support safe rmmod */
	struct device_driver	driver;
};

/* ----------------------------------------------------------------------- */

int usb_gadget_register_driver(struct usb_gadget_driver *driver);
int usb_gadget_unregister_driver(struct usb_gadget_driver *driver);

extern int usb_gadget_map_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in);
extern void usb_gadget_unmap_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in);

int usb_gadget_probe_driver(struct usb_gadget_driver *driver);

/* ----------------------------------------------------------------------- */

struct usb_gadget *usb_get_gadget_by_name(const char *name);
int usb_gadget_udc_start(struct usb_gadget *gadget,
		struct usb_gadget_driver *driver);
int usb_gadget_udc_stop(struct usb_gadget *gadget);
struct usb_gadget *usb_get_gadget(void);

#endif	/* __LINUX_USB_GADGET_H */
