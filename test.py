import usb.core
import usb.util

import usb.core

# dev = usb.core.find(find_all=1)
# for cfg in dev:
# 	# print(cfg.product)
# 	print('Decimal VendorID=' + str(hex(cfg.idVendor)) + ' & ProductID=' + str(cfg.idProduct) + '\n')
# 	print('Hexadecimal VendorID=' + hex(cfg.idVendor) + ' & ProductID=' + hex(cfg.idProduct) + '\n\n')

# find our device
dev = usb.core.find(idVendor=0x4701, idProduct=0x0290)

# was it found?
if dev is None:
    raise ValueError('Device not found')

# set the active configuration. With no arguments, the first
# configuration will be the active one
dev.set_configuration()

# get an endpoint instance
cfg = dev.get_active_configuration()
intf = cfg[(0,0)]

ep = usb.util.find_descriptor(
    intf,
    # match the first OUT endpoint
    custom_match = \
    lambda e: \
        usb.util.endpoint_direction(e.bEndpointAddress) == \
        usb.util.ENDPOINT_OUT)

assert ep is not None

# write the data
# ep.read()
