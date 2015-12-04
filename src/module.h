#ifndef _KARUIWM_MODULE_H
#define _KARUIWM_MODULE_H

struct module {
	/* list.h */
	struct module *prev, *next;

	/* module data */
	char *name;

	/* shared library handler */
	char *so_path;
	void *so_handler;

	/* callback functions */
	int (*init)(void);
	void (*term)(void);
};

void module_delete(struct module *mod);
struct module *module_new(char const *name);

#endif /* ndef _KARUIWM_MODULE_H */
