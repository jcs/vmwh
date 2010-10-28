#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <unistd.h>

#include "vmwh.h"

int debug = 0;

int
main(int argc, char *argv[])
{
	int done_init = 0;
	int was_grabbed = 0;

	debug = 1;

	vmware_check_version();
	vmware_get_mouse_position();
	x11_init();

	for (;;) {
		was_grabbed = mouse_grabbed;
		vmware_get_mouse_position();

		if (mouse_grabbed && (!was_grabbed || !done_init)) {
			/* transitioned from host -> guest */
			char *clip;

			if (debug)
				printf("transitioned from host -> guest\n");

			if (vmware_get_clipboard(&clip)) {
				x11_set_clipboard(clip);
				free(clip);
			}

			x11_set_cursor(host_mouse_x, host_mouse_y);
		}

		else if (!mouse_grabbed && (was_grabbed || !done_init)) {
			/* transitioned from guest -> host */
			char *clip;

			if (debug)
				printf("transitioned from guest -> host\n");

			if (x11_get_clipboard(&clip)) {
				vmware_set_clipboard(clip);
				free(clip);
			}

			// vmware_set_cursor(host_mouse_x, host_mouse_y);
		}

		if (!done_init)
			done_init = 1;

		usleep(500);
	}

	return (0);
}
