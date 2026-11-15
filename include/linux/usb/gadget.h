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
#include <linux/usb/ch11.h>

struct usb_ep;

/**
 * struct usb_request - describes one i/o request
 * @buf: Buffer used for data.  Always provide this; some controllers
 *	only use PIO, or don't use this for some transfers.
 * @dma: DMA address for bulk/intr endpoints.
 * @length: Length of that data
 * @stream_id: The stream id, when USB3.0 bulk streams are being used
 * @no_interrupt: If true, hints that no completion irq is needed.
 * @zero: If true, when requests are loaded, they must sink a
 *	zero length packet (unless @length is zero) to terminate
 *	the transfer.
 * @short_not_ok: When read, causes a host controller to use
 *	short reads to terminate the transfer.
 * @complete: Completion handler. This requests that the driver
 *	return the completed request to the gadget driver.
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

/* endpoint-specific parts of the api to the usb controller hardware.
 * unlike the urb model, (de)multiplexing is not explicitly modelled,
 * request pickup and completion is basic.
 */
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
 * @name:identifier for the endpoint, such as "ep-a" or "ep1in-bulk"
 * @ops: Function pointers used to access hardware-specific operations.
 * @ep_list:the gadget's ep_list holds all of its endpoints
 * @maxpacket:The maximum packet size used on this endpoint.
 * @maxpacket_limit:The maximum packet size this endpoint can support.
 * @max_streams: The maximum number of streams supported
 *	by this endpoint.
 * @mult: ...
 * @caps: ...
 * @driver_data:for use by the hardware driver
 * @address: UDC address of the endpoint, or zero
 * @claimed: True if this endpoint is claimed by a function.
 * @comp_desc: In case of SuperSpeed support, this is the endpoint
 *	companion descriptor.
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

/**
 * struct usb_gadget_ops - operations provided by a USB Device Controller
 * @get_frame: Returns the current frame number, or -EOPNOTSUPP.
 * @wakeup: Reports a remote wakeup.
 * @set_selfpowered: Sets the device selfpowered feature.
 * @vbus_session: Updates the controller's VBUS session state.
 * @vbus_draw: Updates the controller's VBUS draw state.
 * @pullup: Enables or disables the D+ pullup.
 * @ioctl: Used for device-specific requests.
 * @get_config_params: Passes the hardware's SuperSpeed configuration.
 * @udc_start: Prepares the controller for use with this gadget driver.
 * @udc_stop: Reverses @udc_start.
 * @match_ep: Called to match an endpoint.
 * @udc_set_speed: Called to set the maximum speed.
 * @udc_async_callbacks: Called to enable or disable asynchronous callbacks.
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
	 * @gadget: The gadget to set the speed for.
	 * @speed: The speed to set.
	 */
	void	(*udc_set_speed)(struct usb_gadget *gadget,
				 enum usb_device_speed);

	/**
	 * udc_async_callbacks - Enables or disables asynchronous callbacks.
	 * @gadget: The gadget to configure.
	 * @enable: True to enable, false to disable.
	 */
	void	(*udc_async_callbacks)(struct usb_gadget *gadget, bool enable);
};

/**
 * struct usb_gadget - represents a usb slave device
 * @work: (internal use) Workqueue to be used for sysfs_notify()
 * @ops: Function pointers used to access hardware-specific operations.
 * @ep0: Endpoint zero, used when reading or writing responses to
 *	driver setup() requests
 * @ep_list: List of other endpoints supported by the device.
 * @speed: Speed of current connection to USB host.
 * @max_speed: Maximal speed the UDC can handle.  UDC must support this
 *      and all slower speeds.
 * @state: the state we are now (attached, suspended, configured, etc)
 * @name: Identifies the controller hardware type.  Used in diagnostics
 *	and sometimes configuration.
 * @dev: Driver model state for this abstract device.
 * @out_epnum: last used out ep number
 * @in_epnum: last used in ep number
 * @is_dualspeed: true if the controller supports SuperSpeed or HighSpeed.
 * @is_otg: true if the USB device port uses a Mini-AB jack
 * @is_a_peripheral: true if the controller is in A-Peripheral mode.
 * @b_hnp_enable: OTG device feature flag, indicating that the A-Host
 *	enabled HNP support.
 * @a_hnp_support: OTG device feature flag, indicating that the A-Host
 *	supports HNP.
 * @a_alt_hnp_support: OTG device feature flag, indicating that the A-Host
 *	can use an alternate port for HNP.
 * @quirk_ep0_mps_limit: limit the max packet size of ep0.
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

/* iterates the non-control endpoints; 'tmp' is a struct usb_ep pointer */
#define gadget_for_each_ep(tmp, gadget) \
	list_for_each_entry(tmp, &(gadget)->ep_list, ep_list)

/**
 * usb_ep_align_maybe - returns @len aligned to ep's maxpacketsize if gadget
 *	requires quirk_ep0_mps_limit
 * @gadget: gadget whose maxpacketsize need adjusting
 * @ep: ep whose maxpacketsize need adjusting
 * @len: buffer size's length to align to @ep's maxpacketsize
 *
 * This helper function is specially useful for gadget zero, which support
 * both High Bandwidth and SuperSpeed endpoints.
 */
static inline size_t
usb_ep_align_maybe(struct usb_gadget *g, struct usb_ep *ep, size_t len)
{
	if (g->quirk_ep0_mps_limit)
		return roundup(len, ep->maxpacket);
	else
		return len;
}

/**
 * gadget_is_dualspeed - return true if the hardware handles both speeds
 * @g: controller that might support both high and full speeds
 *
 * This is only for the gadget side.  Offically, the device side is
 * "full speed" or "high speed".  "Dual speed" is an invention of the
 * gadget framework.
 */
static inline int gadget_is_dualspeed(struct usb_gadget *g)
{
	return g->is_dualspeed;
}

/**
 * gadget_is_superspeed() - return true if the hardware handles SuperSpeed
 * @g: controller that might support SuperSpeed
 */
static inline int gadget_is_superspeed(struct usb_gadget *g)
{
	return g->max_speed >= USB_SPEED_SUPER;
}

/**
 * gadget_is_superspeed_plus() - return true if the hardware handles
 * SuperSpeed Plus
 * @g: controller that might support SuperSpeed Plus
 */
static inline int gadget_is_superspeed_plus(struct usb_gadget *g)
{
	return g->max_speed >= USB_SPEED_SUPER_PLUS;
}

/**
 * gadget_is_otg - return true if the hardware understands OTG
 * @g: controller that might support OTG
 */
static inline int gadget_is_otg(struct usb_gadget *g)
{
	return g->is_otg;
}

/**
 * usb_gadget_frame_number - returns the current frame number
 * @gadget: controller that reports the frame number
 *
 * Returns the usb frame number, or 0 if none could be read.
 */
static inline int usb_gadget_frame_number(struct usb_gadget *gadget)
{
	return gadget->ops->get_frame(gadget);
}

/**
 * usb_gadget_wakeup - tries to wake up the host connected to this gadget
 * @gadget: controller used to wake up the host
 *
 * Returns zero on success, else negative error code if the hardware
 * doesn't support such attempts, or it in a state that can't wakeup.
 */
static inline int usb_gadget_wakeup(struct usb_gadget *gadget)
{
	if (!gadget->ops->wakeup)
		return -EOPNOTSUPP;
	return gadget->ops->wakeup(gadget);
}

/**
 * usb_gadget_set_selfpowered - sets the device selfpowered feature.
 * @gadget:the device being declared as self-powered
 *
 * this affects the device status reported by the hardware driver
 * to reflect that it now has a local power supply.
 *
 * returns zero on success, else negative errno.
 */
static inline int usb_gadget_set_selfpowered(struct usb_gadget *gadget)
{
	if (!gadget->ops->set_selfpowered)
		return -EOPNOTSUPP;
	return gadget->ops->set_selfpowered(gadget, 1);
}

/**
 * usb_gadget_clear_selfpowered - clear the device selfpowered feature.
 * @gadget:the device being declared as bus-powered
 *
 * this affects the device status reported by the hardware driver.
 * some hardware may not support bus-powered otg-only devices.
 *
 * returns zero on success, else negative errno.
 */
static inline int usb_gadget_clear_selfpowered(struct usb_gadget *gadget)
{
	if (!gadget->ops->set_selfpowered)
		return -EOPNOTSUPP;
	return gadget->ops->set_selfpowered(gadget, 0);
}

/**
 * usb_gadget_vbus_connect - Notify controller that VBUS is powered
 * @gadget:The device which now has VBUS power.
 *
 * This call is used by a driver for an external transceiver (or GPIO)
 * that detects a VBUS power session starting.
 *
 * returns zero on success, else negative errno.
 */
static inline int usb_gadget_vbus_connect(struct usb_gadget *gadget)
{
	if (!gadget->ops->vbus_session)
		return -EOPNOTSUPP;
	return gadget->ops->vbus_session(gadget, 1);
}

/**
 * usb_gadget_vbus_draw - constrain controller's VBUS power usage
 * @gadget:The device whose VBUS usage is being described
 * @mA:How much current to draw
 *
 * This call is used by gadget drivers during SET_CONFIGURATION calls,
 * reporting how much power the device may consume.
 *
 * returns zero on success, else negative errno.
 */
static inline int usb_gadget_vbus_draw(struct usb_gadget *gadget, unsigned mA)
{
	if (!gadget->ops->vbus_draw)
		return -EOPNOTSUPP;
	return gadget->ops->vbus_draw(gadget, mA);
}

/**
 * usb_gadget_vbus_disconnect - notify controller about VBUS session end
 * @gadget:the device whose VBUS supply is being described
 *
 * This call is used by a driver for an external transceiver (or GPIO)
 * that detects a VBUS power session ending.
 *
 * returns zero on success, else negative errno.
 */
static inline int usb_gadget_vbus_disconnect(struct usb_gadget *gadget)
{
	if (!gadget->ops->vbus_session)
		return -EOPNOTSUPP;
	return gadget->ops->vbus_session(gadget, 0);
}

/**
 * usb_gadget_connect - software-controlled connect to USB host
 * @gadget:the peripheral being connected
 *
 * Enables the D+ (or D-) pullup, which lets the host conclude that a
 * USB device (a usb "gadget") is connected.
 *
 * returns zero on success, else negative errno.
 */
static inline int usb_gadget_connect(struct usb_gadget *gadget)
{
	if (!gadget->ops->pullup)
		return -EOPNOTSUPP;
	return gadget->ops->pullup(gadget, 1);
}

/**
 * usb_gadget_disconnect - software-controlled disconnect from USB host
 * @gadget:the peripheral being disconnected
 *
 * Disables the D+ (or D-) pullup, which lets the host conclude that a
 * USB device (a usb "gadget") is disconnected.
 *
 * returns zero on success, else negative errno.
 */
static inline int usb_gadget_disconnect(struct usb_gadget *gadget)
{
	if (!gadget->ops->pullup)
		return -EOPNOTSUPP;
	return gadget->ops->pullup(gadget, 0);
}

/**
 * usb_gadget_set_state() - update USB-Gadget connection state
 * @gadget: gadget object
 * @state: new state to set
 */
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

/* driver modules register and unregister, as usual.
 * these calls must be made in a context that can sleep.
 */
int usb_gadget_register_driver(struct usb_gadget_driver *driver);
int usb_gadget_unregister_driver(struct usb_gadget_driver *driver);

/* utility to simplify map/unmap of usb_requests to/from DMA */
extern int usb_gadget_map_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in);
extern void usb_gadget_unmap_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in);

/* utility to set the device state */
int usb_gadget_probe_driver(struct usb_gadget_driver *driver);

/* ----------------------------------------------------------------------- */

struct usb_gadget *usb_get_gadget_by_name(const char *name);
int usb_gadget_udc_start(struct usb_gadget *gadget,
		struct usb_gadget_driver *driver);
int usb_gadget_udc_stop(struct usb_gadget *gadget);
struct usb_gadget *usb_get_gadget(void);

#endif	/* __LINUX_USB_GADGET_H */
