#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <libusb-1.0/libusb.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#define VERSION "1.0"

#define USB_VID 0x0c45
#define USB_PID 0x7401

#define USBTIMEOUTINMS 1000

static struct libusb_device_handle *usbdevh = NULL;
static int debug = 1;

static void printversion(void) {
	printf("pcsensor2 v" VERSION "\n");
}

static void printusage(void) {
	printversion();
	printf("usage: -h         - this help\n");
	printf("       -v         - print version\n");
	printf("       -q         - silent operation\n");
	printf("       -b [bus]   - set usb bus to search for sensors\n");
	printf("       -d [dev]   - set usb device address to search for sensors,\n");
	printf("                    needs bus given with -b\n");
}

static int init_control_transfer(void) {
	int r;
	uint8_t question1[] = { 0x01, 0x01 };

	r = libusb_control_transfer(usbdevh, 0x21, 0x09, 0x0201, 0x00, question1, sizeof(question1), USBTIMEOUTINMS);
	if (r < 0) {
		fprintf(stderr, "error: init control transfer error\n");
		return -1;
	}

	return 0;
}

static int control_transfer(uint8_t *question, int questionsize) {
	int r;

	r = libusb_control_transfer(usbdevh, 0x21, 0x09, 0x0200, 0x01, question, questionsize, USBTIMEOUTINMS);
	if (r < 0) {
		fprintf(stderr, "error: control transfer error\n");
		return -1;
	}

	return 0;
}

static int interrupt_read(uint8_t *response, int responsesize) {
	int r, transferred;

	r = libusb_interrupt_transfer(usbdevh, 0x82, response, responsesize, &transferred, USBTIMEOUTINMS);
	if (r < 0 || transferred != responsesize) {
		fprintf(stderr, "error: failed to read interrupt response\n");
		return -1;
	}

	return 0;
}

static int readtemp(float *temp) {
	int r;
	uint8_t question1[] = { 0x01, 0x80, 0x33, 0x01, 0x00, 0x00, 0x00, 0x00 };
	uint8_t question2[] = { 0x01, 0x82, 0x77, 0x01, 0x00, 0x00, 0x00, 0x00 };
	uint8_t question3[] = { 0x01, 0x86, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00 };
	uint8_t response[8];

	if (debug)
		printf("sending init control transfer\n");
	r = init_control_transfer();
	if (r < 0)
		return r;

	if (debug)
		printf("sending control transfer question1\n");
	r = control_transfer(question1, sizeof(question1));
	if (r < 0)
		return r;
	if (debug)
		printf("reading interrupt response\n");
	r = interrupt_read(response, sizeof(response));
	if (r < 0)
		return r;

	if (debug)
		printf("sending control transfer question2\n");
	r = control_transfer(question2, sizeof(question2));
	if (r < 0)
		return r;
	if (debug)
		printf("reading interrupt response\n");
	r = interrupt_read(response, sizeof(response));
	if (r < 0)
		return r;

	if (debug)
		printf("sending control transfer question3\n");
	r = control_transfer(question3, sizeof(question3));
	if (r < 0)
		return r;
	if (debug)
		printf("reading interrupt response\n");
	r = interrupt_read(response, sizeof(response));
	if (r < 0)
		return r;
	if (debug)
		printf("reading interrupt response\n");
	r = interrupt_read(response, sizeof(response));
	if (r < 0)
		return r;

	if (debug)
		printf("sending control transfer question1\n");
	r = control_transfer(question1, sizeof(question1));
	if (r < 0)
		return r;
	if (debug)
		printf("reading temperature interrupt response\n");
	r = interrupt_read(response, sizeof(response));
	if (r < 0)
		return r;

	*temp = (response[3] & 0xFF) + (response[2] << 8);
    *temp = *temp * (125.0 / 32000.0);

    return 0;
}

int main(int argc, char **argv) {
	int r, i, usbdevcount;
	char *endptr;
	int cmdline_bus = -1, cmdline_dev = -1;
	libusb_device **usbdevlist = NULL;
	struct libusb_device_descriptor desc;
	libusb_device *probe_usbdev = NULL;
	float temp;

	while ((r = getopt(argc, argv, "vb:d:qh")) != -1) {
		switch (r) {
			case 'v':
				printversion();
				break;
			case 'b':
				errno = 0;
				cmdline_bus = strtol(optarg, &endptr, 10);
				if (*endptr != 0 || errno != 0) {
					fprintf(stderr, "invalid bus number\n");
					return 1;
				}
				break;
			case 'd':
				errno = 0;
				cmdline_dev = strtol(optarg, &endptr, 10);
				if (*endptr != 0 || errno != 0) {
					fprintf(stderr, "invalid device address\n");
					return 1;
				}
				break;
			case 'q':
				debug = 0;
				break;
			case 'h':
				printusage();
				return 0;
				break;
			default:
				if (isprint(optopt))
					fprintf(stderr, "unknown option '-%c'.\n", optopt);
				else
					fprintf(stderr, "unknown option character '-%x'.\n", optopt);
				return 1;
		}
	}

	if (optind < argc || (cmdline_dev && !cmdline_bus)) {
		printusage();
		return 1;
	}

	r = libusb_init(NULL);
	if (r < 0) {
		fprintf(stderr, "error: libusb init error\n");
		return 1;
	}

	libusb_set_debug(NULL, 3);

	if (debug)
		printf("opening device vid=%.4x pid=%.4x...\n", USB_VID, USB_PID);

	usbdevcount = libusb_get_device_list(NULL, &usbdevlist);
	for (i = 0; i < usbdevcount; i++) {
		probe_usbdev = usbdevlist[i];
		r = libusb_get_device_descriptor(probe_usbdev, &desc);
		if (r >= 0 && desc.idVendor == USB_VID && desc.idProduct == USB_PID) {
			if (cmdline_bus >= 0 && libusb_get_bus_number(probe_usbdev) != cmdline_bus)
				continue;
			if (cmdline_dev >= 0 && libusb_get_device_address(probe_usbdev) != cmdline_dev)
				continue;

			r = libusb_open(probe_usbdev, &usbdevh);
			if (r >= 0 && debug)
				printf("device opened\n");
			break;
		}
	}
	if (usbdevlist)
		libusb_free_device_list(usbdevlist, 1);
	if (!usbdevh) {
		fprintf(stderr, "error: can't find device\n");
		libusb_exit(NULL);
		return 1;
	}

    if (libusb_kernel_driver_active(usbdevh, 0))
		libusb_detach_kernel_driver(usbdevh, 0);
    if (libusb_kernel_driver_active(usbdevh, 1))
		libusb_detach_kernel_driver(usbdevh, 1);

	r = libusb_set_configuration(usbdevh, 1);
	if (r < 0) {
		fprintf(stderr, "error: can't set usb configuration\n");
		return 1;
	}

	r = libusb_claim_interface(usbdevh, 0);
	if (r < 0) {
		fprintf(stderr, "error: can't claim interface 0\n");
		libusb_close(usbdevh);
		libusb_exit(NULL);
		return 1;
	}
	r = libusb_claim_interface(usbdevh, 1);
	if (r < 0) {
		fprintf(stderr, "error: can't claim interface 1\n");
		libusb_release_interface(usbdevh, 0);
		libusb_close(usbdevh);
		libusb_exit(NULL);
		return 1;
	}
	if (debug)
		printf("device claimed\n");

	r = readtemp(&temp);
	if (r >= 0) {
		if (debug)
			printf("temperature: %f°C\n", temp);
		else
			printf("%f\n", temp);
	}

	libusb_release_interface(usbdevh, 0);
	libusb_release_interface(usbdevh, 1);
	libusb_close(usbdevh);
	libusb_exit(NULL);

	return 0;
}
