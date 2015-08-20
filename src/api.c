#include "api.h"
#include "xresources.h"
#include "util.h"
#include "list.h"
#include "karuiwm.h"
#include <string.h>

#define MODULE_PATH "share/"APPNAME"/modules"

static int init_modules(void);
static int init_modules_paths(void);
static void term_modules();

int
api_init(void)
{
	if (init_modules() < 0) {
		ERROR("failed to initialise modules");
		return -1;
	}
	return 0;
}

void
api_term(void)
{
	term_modules();
}

static int
init_modules(void)
{
	char modlist[512];
	char const *modname, *delim = ", ";
	struct module *mod;

	init_modules_paths();
	(void) xresources_string("modules", "core", modlist, 512);

	modname = strtok(modlist, delim);
	while (modname != NULL) {
		mod = module_new(modname);
		if (mod == NULL)
			WARN("could not initialise module '%s'", modname);
		else
			LIST_APPEND(api.modules, mod);
		modname = strtok(NULL, delim);
	}
	if (api.modules == NULL) {
		ERROR("no modules loaded");
		return -1;
	}
	return 0;
}

static int
init_modules_paths(void)
{
	char path[128];
	api.npaths = 0;
	api.paths = NULL;

	if (xresources_string("modules.path", NULL, path, 128) < 0) {
		++api.npaths;
		api.paths = scalloc(api.npaths, sizeof(char *), "module path");
		api.paths[0] = strdupf("%s", path);
	}
	api.npaths += 3;
	api.paths = srealloc(api.paths, api.npaths*sizeof(char *), "module paths");
	api.paths[api.npaths - 3] = strdupf("%s/.local/"MODULE_PATH, karuiwm.env.HOME);
	api.paths[api.npaths - 2] = strdupf("/usr/local/"MODULE_PATH);
	api.paths[api.npaths - 1] = strdupf("/usr/"MODULE_PATH);
}

static void
term_modules(void)
{
	int unsigned i;
	struct module *mod;

	/* modules */
	while (api.modules != NULL) {
		mod = api.modules;
		LIST_REMOVE(api.modules, mod);
		module_delete(mod);
	}

	/* modules' paths */
	for (i = 0; i < api.npaths; ++i)
		sfree(api.paths[i]);
	sfree(api.paths);
}
