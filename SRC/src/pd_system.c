#include "pd_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

static int lua_sys_logToConsole(lua_State *L) {
    const char *msg = luaL_checkstring(L, 1);
    fprintf(stderr, "[playdate] %s\n", msg);
    return 0;
}

static int lua_sys_error(lua_State *L) {
    const char *msg = luaL_checkstring(L, 1);
    fprintf(stderr, "[playdate ERROR] %s\n", msg);
    exit(1);
    return 0;
}

static int lua_sys_getCurrentTimeMilliseconds(lua_State *L) {
    uint64_t now = SDL_GetTicks64();
    lua_pushinteger(L, (lua_Integer)now);
    return 1;
}

/* playdate.wait(ms): pause the update loop (drawing/input keep running) */
static int lua_sys_wait(lua_State *L) {
    lua_Number ms = luaL_checknumber(L, 1);
    if (ms > 0)
        g_pd.wait_until = SDL_GetTicks64() + (uint64_t)ms;
    return 0;
}

static int lua_sys_getSecondsSinceEpoch(lua_State *L) {
    time_t t = time(NULL);
    lua_pushinteger(L, (lua_Integer)t);
    if (lua_gettop(L) >= 1) {
        lua_pushinteger(L, 0);
        return 2;
    }
    return 1;
}

static void push_time_table(lua_State *L, struct tm *tm_info, int msec) {
    lua_newtable(L);
    lua_pushinteger(L, tm_info->tm_year + 1900); lua_setfield(L, -2, "year");
    lua_pushinteger(L, tm_info->tm_mon + 1); lua_setfield(L, -2, "month");
    lua_pushinteger(L, tm_info->tm_mday); lua_setfield(L, -2, "day");
    lua_pushinteger(L, tm_info->tm_wday == 0 ? 7 : tm_info->tm_wday); lua_setfield(L, -2, "weekday");
    lua_pushinteger(L, tm_info->tm_hour); lua_setfield(L, -2, "hour");
    lua_pushinteger(L, tm_info->tm_min); lua_setfield(L, -2, "minute");
    lua_pushinteger(L, tm_info->tm_sec); lua_setfield(L, -2, "second");
    lua_pushinteger(L, msec); lua_setfield(L, -2, "millisecond");
}

static int lua_sys_getTime(lua_State *L) {
    time_t t = time(NULL);
    struct tm tm_info;
    localtime_r(&t, &tm_info);
    push_time_table(L, &tm_info, 0);
    return 1;
}

static int lua_sys_getGMTTime(lua_State *L) {
    time_t t = time(NULL);
    struct tm tm_info;
    gmtime_r(&t, &tm_info);
    push_time_table(L, &tm_info, 0);
    return 1;
}

static int lua_sys_getElapsedTime(lua_State *L) {
    uint64_t now = SDL_GetTicks64();
    float elapsed = (float)(now - g_pd.start_time) / 1000.0f;
    lua_pushnumber(L, elapsed);
    return 1;
}

static int lua_sys_resetElapsedTime(lua_State *L) {
    (void)L;
    g_pd.start_time = SDL_GetTicks64();
    return 0;
}

static int lua_sys_drawFPS(lua_State *L) {
    (void)luaL_checkinteger(L, 1);
    (void)luaL_checkinteger(L, 2);
    return 0;
}

static int lua_sys_setUpdateCallback(lua_State *L) {
    if (lua_isfunction(L, 1)) {
        lua_getglobal(L, "playdate");
        lua_pushvalue(L, 1);
        lua_setfield(L, -2, "update");
        lua_pop(L, 1);
    }
    return 0;
}

static int lua_sys_setPeripheralsEnabled(lua_State *L) {
    (void)luaL_checkinteger(L, 1);
    return 0;
}

static int lua_sys_getAccelerometer(lua_State *L) {
    lua_pushnumber(L, 0);
    lua_pushnumber(L, 0);
    lua_pushnumber(L, 0);
    return 3;
}

static int lua_sys_getFlipped(lua_State *L) {
    lua_pushboolean(L, 0);
    return 1;
}

static int lua_sys_setAutoLockDisabled(lua_State *L) {
    (void)L;
    return 0;
}

static int lua_sys_getReduceFlashing(lua_State *L) {
    lua_pushboolean(L, 0);
    return 1;
}

static int lua_sys_getBatteryPercentage(lua_State *L) {
    lua_pushnumber(L, 100.0f);
    return 1;
}

static int lua_sys_getBatteryVoltage(lua_State *L) {
    lua_pushnumber(L, 4.2f);
    return 1;
}

static int lua_sys_getLanguage(lua_State *L) {
    lua_pushinteger(L, 0);
    return 1;
}

static int lua_sys_getSystemLanguage(lua_State *L) {
    lua_pushinteger(L, 0); /* graphics.font.kLanguageEnglish */
    return 1;
}

#define SYS_STRINGS_KEY "pd.strings"

/* Load <pdx>/en.pds (Playdate compiled strings) into a Lua table.
   Format: 16-byte header ("Playdate STR" + flags, bit 0x80 = zlib body),
   u32 decompressed size, 12 pad bytes, body. Body: u32 count, (count-1)
   u32 offsets, then count key\0value\0 records. */
static void sys_load_strings(lua_State *L) {
    lua_newtable(L);
    char path[1200];
    snprintf(path, sizeof(path), "%s/en.pds", g_pd.pdx_dir ? g_pd.pdx_dir : ".");
    FILE *f = fopen(path, "rb");
    if (!f) goto done;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 32) { fclose(f); goto done; }
    unsigned char *raw = malloc(sz);
    if (fread(raw, 1, sz, f) != (size_t)sz) { free(raw); fclose(f); goto done; }
    fclose(f);
    if (memcmp(raw, "Playdate STR", 12) != 0) { free(raw); goto done; }
    int compressed = raw[15] & 0x80;
    unsigned long body_len = raw[16] | (raw[17] << 8) | (raw[18] << 16) |
                             ((unsigned long)raw[19] << 24);
    unsigned char *body;
    if (compressed) {
        body = malloc(body_len + 1);
        if (uncompress(body, &body_len, raw + 32, sz - 32) != Z_OK) {
            free(body); free(raw); goto done;
        }
    } else {
        body = raw + 16;
        body_len = sz - 16;
    }
    if (body_len >= 4) {
        unsigned long count = body[0] | (body[1] << 8) | (body[2] << 16) |
                              ((unsigned long)body[3] << 24);
        unsigned long pos = 4 + (count > 0 ? (count - 1) * 4 : 0);
        for (unsigned long i = 0; i < count && pos < body_len; i++) {
            const char *key = (const char *)body + pos;
            unsigned long klen = strnlen(key, body_len - pos);
            if (pos + klen + 1 >= body_len) break;
            pos += klen + 1;
            const char *val = (const char *)body + pos;
            unsigned long vlen = strnlen(val, body_len - pos);
            pos += vlen + 1;
            lua_pushlstring(L, key, klen);
            lua_pushlstring(L, val, vlen);
            lua_rawset(L, -3);
        }
    }
    if (compressed) free(body);
    free(raw);
done:
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, SYS_STRINGS_KEY);
}

static int lua_sys_getLocalizedText(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, SYS_STRINGS_KEY);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        sys_load_strings(L);
    }
    lua_getfield(L, -1, key);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushstring(L, key);
    }
    return 1;
}

static int lua_sys_getLaunchArgs(lua_State *L) {
    lua_pushstring(L, "");
    return 1;
}

static int lua_sys_realloc(lua_State *L) {
    (void)L;
    return 0;
}

static int lua_sys_formatString(lua_State *L) {
    (void)L;
    return 0;
}

static int lua_sys_removeAllMenuItems(lua_State *L) {
    (void)L;
    return 0;
}

static int lua_sys_addMenuItem(lua_State *L) {
    (void)luaL_checkstring(L, 1);
    lua_pushnil(L);
    return 1;
}

static int lua_sys_addCheckmarkMenuItem(lua_State *L) {
    (void)luaL_checkstring(L, 1);
    (void)luaL_checkinteger(L, 2);
    lua_pushnil(L);
    return 1;
}

static int lua_sys_addOptionsMenuItem(lua_State *L) {
    (void)luaL_checkstring(L, 1);
    lua_pushnil(L);
    return 1;
}

static int lua_sys_removeMenuItem(lua_State *L) {
    (void)L;
    return 0;
}

static int lua_sys_getMenuItemValue(lua_State *L) {
    (void)L;
    lua_pushinteger(L, 0);
    return 1;
}

static int lua_sys_setMenuItemValue(lua_State *L) {
    (void)L;
    return 0;
}

static int lua_sys_getMenuItemTitle(lua_State *L) {
    (void)L;
    lua_pushstring(L, "");
    return 1;
}

static int lua_sys_setMenuItemTitle(lua_State *L) {
    (void)L;
    return 0;
}

static int lua_sys_setMenuImage(lua_State *L) {
    (void)L;
    return 0;
}

static int lua_sys_getVolume(lua_State *L) {
    lua_pushnumber(L, 0.5f);
    return 1;
}

static int lua_sys_getPowerStatus(lua_State *L) {
    lua_pushinteger(L, 0);
    return 1;
}

static int lua_sys_exitToLauncher(lua_State *L) {
    (void)L;
    fprintf(stderr, "[quit] exitToLauncher ");
    g_pd.running = 0;
    return 0;
}

static int lua_menuitem_method_noop(lua_State *L) {
    (void)L;
    return 0;
}

static int lua_menuitem_getValue(lua_State *L) {
    lua_getfield(L, 1, "_value");
    return 1;
}

static int lua_menuitem_setValue(lua_State *L) {
    lua_pushvalue(L, 2);
    lua_setfield(L, 1, "_value");
    return 0;
}

static int lua_menuitem_getTitle(lua_State *L) {
    lua_getfield(L, 1, "_title");
    return 1;
}

static int lua_menuitem_setTitle(lua_State *L) {
    lua_pushvalue(L, 2);
    lua_setfield(L, 1, "_title");
    return 0;
}

static void push_menu_item(lua_State *L, int title_idx) {
    lua_newtable(L);
    if (title_idx && lua_isstring(L, title_idx)) {
        lua_pushvalue(L, title_idx);
        lua_setfield(L, -2, "_title");
    }
    lua_pushcfunction(L, lua_menuitem_getValue);
    lua_setfield(L, -2, "getValue");
    lua_pushcfunction(L, lua_menuitem_setValue);
    lua_setfield(L, -2, "setValue");
    lua_pushcfunction(L, lua_menuitem_getTitle);
    lua_setfield(L, -2, "getTitle");
    lua_pushcfunction(L, lua_menuitem_setTitle);
    lua_setfield(L, -2, "setTitle");
    lua_pushcfunction(L, lua_menuitem_method_noop);
    lua_setfield(L, -2, "remove");
}

static int lua_sysmenu_addMenuItem(lua_State *L) {
    push_menu_item(L, 2);
    return 1;
}

static int lua_sysmenu_addCheckmarkMenuItem(lua_State *L) {
    push_menu_item(L, 2);
    if (lua_isboolean(L, 3)) {
        lua_pushvalue(L, 3);
        lua_setfield(L, -2, "_value");
    }
    return 1;
}

static int lua_sysmenu_addOptionsMenuItem(lua_State *L) {
    push_menu_item(L, 2);
    return 1;
}

static int lua_sysmenu_getMenuItems(lua_State *L) {
    lua_newtable(L);
    return 1;
}

#define SYS_MENU_KEY "pd.system.menu"

static int lua_sys_getSystemMenu(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, SYS_MENU_KEY);
    if (lua_istable(L, -1)) return 1;
    lua_pop(L, 1);
    lua_newtable(L);
    lua_pushcfunction(L, lua_sysmenu_addMenuItem);
    lua_setfield(L, -2, "addMenuItem");
    lua_pushcfunction(L, lua_sysmenu_addCheckmarkMenuItem);
    lua_setfield(L, -2, "addCheckmarkMenuItem");
    lua_pushcfunction(L, lua_sysmenu_addOptionsMenuItem);
    lua_setfield(L, -2, "addOptionsMenuItem");
    lua_pushcfunction(L, lua_menuitem_method_noop);
    lua_setfield(L, -2, "removeAllMenuItems");
    lua_pushcfunction(L, lua_menuitem_method_noop);
    lua_setfield(L, -2, "removeMenuItem");
    lua_pushcfunction(L, lua_sysmenu_getMenuItems);
    lua_setfield(L, -2, "getMenuItems");
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, SYS_MENU_KEY);
    return 1;
}

static const luaL_Reg sys_funcs[] = {
    {"logToConsole", lua_sys_logToConsole},
    {"error", lua_sys_error},
    {"getCurrentTimeMilliseconds", lua_sys_getCurrentTimeMilliseconds},
    {"wait", lua_sys_wait},
    {"getSecondsSinceEpoch", lua_sys_getSecondsSinceEpoch},
    {"getTime", lua_sys_getTime},
    {"getGMTTime", lua_sys_getGMTTime},
    {"getElapsedTime", lua_sys_getElapsedTime},
    {"resetElapsedTime", lua_sys_resetElapsedTime},
    {"drawFPS", lua_sys_drawFPS},
    {"setUpdateCallback", lua_sys_setUpdateCallback},
    {"setPeripheralsEnabled", lua_sys_setPeripheralsEnabled},
    {"getAccelerometer", lua_sys_getAccelerometer},
    {"getFlipped", lua_sys_getFlipped},
    {"setAutoLockDisabled", lua_sys_setAutoLockDisabled},
    {"getReduceFlashing", lua_sys_getReduceFlashing},
    {"getBatteryPercentage", lua_sys_getBatteryPercentage},
    {"getBatteryVoltage", lua_sys_getBatteryVoltage},
    {"getLanguage", lua_sys_getLanguage},
    {"getSystemLanguage", lua_sys_getSystemLanguage},
    {"getLocalizedText", lua_sys_getLocalizedText},
    {"getLaunchArgs", lua_sys_getLaunchArgs},
    {"realloc", lua_sys_realloc},
    {"formatString", lua_sys_formatString},
    {"removeAllMenuItems", lua_sys_removeAllMenuItems},
    {"addMenuItem", lua_sys_addMenuItem},
    {"addCheckmarkMenuItem", lua_sys_addCheckmarkMenuItem},
    {"addOptionsMenuItem", lua_sys_addOptionsMenuItem},
    {"removeMenuItem", lua_sys_removeMenuItem},
    {"getMenuItemValue", lua_sys_getMenuItemValue},
    {"setMenuItemValue", lua_sys_setMenuItemValue},
    {"getMenuItemTitle", lua_sys_getMenuItemTitle},
    {"setMenuItemTitle", lua_sys_setMenuItemTitle},
    {"setMenuImage", lua_sys_setMenuImage},
    {"getVolume", lua_sys_getVolume},
    {"getPowerStatus", lua_sys_getPowerStatus},
    {"exitToLauncher", lua_sys_exitToLauncher},
    {"getSystemMenu", lua_sys_getSystemMenu},
    {NULL, NULL}
};

void pd_system_register(lua_State *L) {
    lua_getglobal(L, "playdate");
    for (int i = 0; sys_funcs[i].name; i++) {
        lua_pushcfunction(L, sys_funcs[i].func);
        lua_setfield(L, -2, sys_funcs[i].name);
    }
    lua_getfield(L, -1, "graphics");
    if (lua_istable(L, -1)) {
        lua_pushcfunction(L, lua_sys_getLocalizedText);
        lua_setfield(L, -2, "getLocalizedText");
    }
    lua_pop(L, 2);
}
