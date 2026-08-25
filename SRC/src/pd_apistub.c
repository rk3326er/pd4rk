/* Installs no-op fallbacks for every documented Playdate Lua API that the
 * runtime does not implement natively. This converts hard crashes
 * ("field 'x' is not callable") into silently missing features.
 *
 * Existing implementations are never overwritten: a stub is only installed
 * when the slot is nil. Namespaces are created as needed.
 */
#include "pd_runtime.h"

#include <stdio.h>
#include <string.h>

#include "pd_apistubs.h"

static int pd_api_noop(lua_State *L) {
    (void)L;
    return 0;
}

/* Like pd_api_noop but logs the first call per stub (PD_STUB_LOG=1). */
static int pd_api_noop_logged(lua_State *L) {
    if (lua_toboolean(L, lua_upvalueindex(2)) == 0) {
        fprintf(stderr, "[stub called] %s\n", lua_tostring(L, lua_upvalueindex(1)));
        lua_pushboolean(L, 1);
        lua_replace(L, lua_upvalueindex(2));
    }
    return 0;
}

/* Walk a dotted/colon path from _G, creating intermediate tables.
 * Returns 1 with the parent container on the stack top and writes the
 * final key name into `leaf`; returns 0 if an intermediate exists but is
 * not indexable (never stomp on non-table values). */
static int resolve_parent(lua_State *L, const char *path, char *leaf, size_t leaf_sz) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", path);

    /* method separator ':' behaves like '.' for our flat class tables */
    for (char *p = buf; *p; p++)
        if (*p == ':') *p = '.';

    lua_pushglobaltable(L);
    char *save = NULL;
    char *tok = strtok_r(buf, ".", &save);
    while (tok) {
        char *next = strtok_r(NULL, ".", &save);
        if (!next) {
            snprintf(leaf, leaf_sz, "%s", tok);
            return 1; /* parent on stack */
        }
        lua_getfield(L, -1, tok);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, tok);
        } else if (!lua_istable(L, -1) && !lua_isuserdata(L, -1)) {
            lua_pop(L, 2);
            return 0;
        }
        lua_remove(L, -2);
        tok = next;
    }
    lua_pop(L, 1);
    return 0;
}

void pd_install_api_stubs(lua_State *L) {
    int installed = 0;
    for (int i = 0; pd_api_stub_paths[i]; i++) {
        char leaf[128];
        if (!resolve_parent(L, pd_api_stub_paths[i], leaf, sizeof(leaf)))
            continue;
        if (!lua_istable(L, -1)) { /* can't setfield on userdata parents */
            lua_pop(L, 1);
            continue;
        }
        lua_getfield(L, -1, leaf);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            if (getenv("PD_STUB_LOG")) {
                lua_pushstring(L, pd_api_stub_paths[i]);
                lua_pushboolean(L, 0);
                lua_pushcclosure(L, pd_api_noop_logged, 2);
            } else {
                lua_pushcfunction(L, pd_api_noop);
            }
            lua_setfield(L, -2, leaf);
            installed++;
        } else {
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    if (getenv("PD_TRACE"))
        fprintf(stderr, "[stubs] installed %d API fallbacks\n", installed);

    /* Known pdex.bin C-extension libs some hybrid games require at load
       time. Stubbing them lets the Lua side run (minus the effect the
       extension provided). starlib: b360's starfield renderer. */
    luaL_dostring(L,
        "if starlib == nil then\n"
        "  local function stubnew(...)\n"
        "    return setmetatable({}, { __index = function()\n"
        "      return function(self) return self end\n"
        "    end })\n"
        "  end\n"
        "  starlib = { new = stubnew, starfield = { new = stubnew } }\n"
        "end\n"
        /* playdate.scoreboards: network-backed; report failure via the
           async callback like the SDK does when offline */
        "if playdate.scoreboards == nil then\n"
        "  local function offline(...)\n"
        "    for i = 1, select('#', ...) do\n"
        "      local cb = select(i, ...)\n"
        "      if type(cb) == 'function' then cb(nil, 'network unavailable') end\n"
        "    end\n"
        "  end\n"
        "  playdate.scoreboards = setmetatable({}, {\n"
        "    __index = function() return offline end })\n"
        "end\n");
}
