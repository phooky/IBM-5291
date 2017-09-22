To build and install device tree overlay:

	dtc -O dtb -I dts -o /lib/firmware/IBM-TILCDC-00A0.dtbo -b 0 -@ IBM-TILCDC-00A0.dts

To load the overlay:

	echo "IBM-TILCDC" >> /sys/devices/platform/bone_capemgr/slots


