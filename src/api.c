#include "api.h"
#include "util.h"
#include "list.h"
#include "karuiwm.h"
#include "config.h"
#include <string.h>

#define API_BUFLEN 512

static int init_actions(void);
static int init_layouts(void);
static int init_modules(void);
static int init_modules_paths(void);
static void term_actions(void);
static void term_layouts(void);
static void term_modules(void);

void
api_add_action(struct action *a)
{
	if (a == NULL) {
		WARN("attempt to add NULL action to API");
		return;
	}
	LIST_APPEND(&api.actions, a);
	++api.nactions;
}

void
api_add_layout(struct layout *l)
{
	if (l == NULL) {
		WARN("attempt to add NULL layout to API");
		return;
	}
	LIST_APPEND(&api.layouts, l);
	++api.nlayouts;
}

int
api_init(void)
{
	if (init_actions() < 0) {
		ERROR("failed to initialise actions");
		return -1;
	}
	if (init_layouts() < 0) {
		ERROR("failed to initialise layouts");
		return -1;
	}
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
	term_layouts();
	term_actions();
}

static int
init_actions(void)
{
	api.actions = NULL;
	api.nactions = 0;
	return 0;
}

static int
init_layouts(void)
{
	api.layouts = NULL;
	api.nlayouts = 0;
	return 0;
}

static int
init_modules(void)
{
	char modlist[API_BUFLEN];
	char const *modname;
	struct module *mod;
	int unsigned i;

	api.nmodules = 0;
	api.modules = NULL;

	(void) init_modules_paths();
	(void) config_get_string("modules", "core", modlist, API_BUFLEN);

	/* load modules */
	modname = strtok(modlist, ", \t");
	do {
		mod = module_new(modname);
		if (mod == NULL) {
			WARN("could not create module '%s'", modname);
			continue;
		}
		LIST_APPEND(&api.modules, mod);
		++api.nmodules;
	} while ((modname = strtok(NULL, ", \t")) != NULL);
	if (api.nmodules == 0) {
		ERROR("no modules loaded");
		return -1;
	}

	/* initialise modules */
	for (i = 0, mod = api.modules; i < api.nmodules; ++i, mod = mod->next) {
		if (mod->init() < 0) {
			ERROR("module `%s` failed to initialise", mod->name);
			LIST_REMOVE(&api.modules, mod);
			--api.modules;
		}
	}
	return 0;
}

static int
init_modules_paths(void)
{
	char path[API_BUFLEN];
	char *modulepath;
	api.npaths = 0;
	api.paths = NULL;

	modulepath = strdupf("share/%s/modules", karuiwm.env.APPNAME);
	if (config_get_string("modules.path", NULL, path, API_BUFLEN) == 0) {
		++api.npaths;
		api.paths = scalloc(api.npaths, sizeof(char *), "module paths");
		api.paths[0] = strdupf("%s", path);
	}
	if (karuiwm.env.HOME != NULL) {
		++api.npaths;
		api.paths = srealloc(api.paths, api.npaths*sizeof(char *),
		                     "module paths");
		api.paths[api.npaths - 1] = strdupf("%s/.local/%s",
		                                    karuiwm.env.HOME,
		                                    modulepath);
	}
	api.npaths += 2;
	api.paths = srealloc(api.paths, api.npaths*sizeof(char *),
	                     "module paths");
	api.paths[api.npaths - 2] = strdupf("/usr/local/%s", modulepath);
	api.paths[api.npaths - 1] = strdupf("/usr/%s", modulepath);
	sfree(modulepath);

	return 0;
}

static void
term_actions(void)
{
	struct action *a;

	while (api.nactions > 0) {
		a = api.actions;
		LIST_REMOVE(&api.actions, a);
		--api.nactions;
		action_delete(a);
	}
}

static void
term_layouts(void)
{
	struct layout *l;

	while (api.nlayouts > 0) {
		l = api.layouts;
		LIST_REMOVE(&api.layouts, l);
		--api.layouts;
		layout_delete(l);
	}
}

static void
term_modules(void)
{
	int unsigned i;
	struct module *mod;

	/* modules */
	while (api.nmodules > 0) {
		mod = api.modules;
		mod->term();
		LIST_REMOVE(&api.modules, mod);
		--api.nmodules;
		module_delete(mod);
	}

	/* modules' paths */
	for (i = 0; i < api.npaths; ++i)
		sfree(api.paths[i]);
	sfree(api.paths);
}
