#include "module.h"
#include "util.h"
#include "api.h"
#include <stdbool.h>
#include <stdio.h>
#include <dlfcn.h>

static bool file_exists(char const *path);
static void *find_shared(char const *name);
static void *load_shared(char const *path);
static void *load_shared_function(void *so_handler, char const *fname);

static bool
file_exists(char const *path)
{
	FILE *f;

	f = fopen(path, "r");
	if (f == NULL) {
		return false;
	} else {
		fclose(f);
		return true;
	}
}

static void *
find_shared(char const *name)
{
	int unsigned i;
	char so_path[512]; /* FIXME magic number */
	void *so_handler;

	for (i = 0; i < api.npaths; ++i) {
		if (snprintf(so_path, 512, "%s/%s.so", /* FIXME magic number */
		             api.paths[i], name) < 0) {
			WARN("snprintf() < 0");
			return NULL;
		}
		DEBUG("checking for shared object at %s", so_path);
		so_handler = load_shared(so_path);
		if (so_handler != NULL)
			return so_handler;
	}
	return NULL;
}

static void *
load_shared_function(void *so_handler, char const *fname)
{
	void *fptr;
	char *dl_error;

	fptr = dlsym(so_handler, fname);
	dl_error = dlerror();
	if (dl_error != NULL) {
		WARN("%s", dl_error);
		return NULL;
	}
	return fptr;
}

static void *
load_shared(char const *path)
{
	void *so_handler;
	char *dl_error;

	if (!file_exists(path))
		return NULL;
	so_handler = dlopen(path, RTLD_NOW);
	dl_error = dlerror();
	if (dl_error != NULL) {
		WARN("%s", dl_error);
		return NULL;
	}
	return so_handler;
}

void
module_delete(struct module *mod)
{
	if (mod->term != NULL)
		mod->term(mod);
	dlclose(mod->so_handler);
	sfree(mod->name);
	sfree(mod->so_path);
	sfree(mod);
}

struct module *
module_new(char const *name)
{
	struct module *mod;
	void *so_handler;
	int (*init)(struct module *mod);

	so_handler = find_shared(name);
	if (so_handler == NULL) {
		ERROR("%s: no such module", name);
		return NULL;
	}

	*(void **) &init = load_shared_function(so_handler, "init");
	if (init == NULL) {
		ERROR("%s: does not contain an init function", name);
		return NULL;
	}

	mod = smalloc(sizeof(struct module), "module");
	mod->name = strdupf(name);
	mod->so_handler = so_handler;
	mod->init = init;

	return mod;
}
