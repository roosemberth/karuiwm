#ifndef _KARUIWM_MODULE_H
#define _KARUIWM_MODULE_H

#define MODULE_NAME_BUFSIZE 128

struct module {
	/* callback functions */
	int (*init)(struct module *mod);
	void (*term)(struct module *mod);

	/* module data */
	char name[MODULE_NAME_BUFSIZE];

	/* shared library handler */
	void *so_handler;
};

#endif /* ndef _KARUIWM_MODULE_H */
