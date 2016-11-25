#include <errno.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

static int get_facility(lua_State *const L,
                        const int index,
                        const char *const def)
{
    const char *const lst[] = {
        "auth",
        "authpriv",
        "cron",
        "daemon",
        "ftp",
        "kern",
        "local0",
        "local1",
        "local2",
        "local3",
        "local4",
        "local5",
        "local6",
        "local7",
        "lpr",
        "mail",
        "news",
        "syslog",
        "user",
        "uucp",
        NULL,
    };

    switch (luaL_checkoption(L, index, def, lst)) {
      case  0: return LOG_AUTH;
      case  1: return LOG_AUTHPRIV;
      case  2: return LOG_CRON;
      case  3: return LOG_DAEMON;
      case  4: return LOG_FTP;
      case  5: return LOG_KERN;
      case  6: return LOG_LOCAL0;
      case  7: return LOG_LOCAL1;
      case  8: return LOG_LOCAL2;
      case  9: return LOG_LOCAL3;
      case 10: return LOG_LOCAL4;
      case 11: return LOG_LOCAL5;
      case 12: return LOG_LOCAL6;
      case 13: return LOG_LOCAL7;
      case 14: return LOG_LPR;
      case 15: return LOG_MAIL;
      case 16: return LOG_NEWS;
      case 17: return LOG_SYSLOG;
      case 18: return LOG_USER;
      case 19: return LOG_UUCP;
    }
}

static int free_ident(lua_State *const L) {
    free(lua_touserdata(L, 1));
    return 0;
}

static int openlog_lua(lua_State *const L) {
    const int options = luaL_optinteger(L, 3, 0);
    const int facility = get_facility(L, 4, "user");
    char *ident;

    {
        const char *const raw_ident = lua_tostring(L, 2);

        ident = malloc((sizeof(char)) * (strlen(raw_ident) + 1));
        if (ident == NULL)
            return luaL_error(L, "Out of memory");

        size_t i = 0;

        do {
            ident[i] = raw_ident[i];
        } while (raw_ident[i++] != '\0');

        lua_pushlightuserdata(L, ident);
        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, free_ident);
        lua_setfield(L, -2, "__gc");
        lua_setmetatable(L, -2);
        lua_setfield(L, LUA_REGISTRYINDEX, "syslog_ident");
    }

    openlog(ident, options, facility);

    lua_pushboolean(L, true);
    return 1;
}

static int closelog_lua(lua_State *const L) {
    closelog();

    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, "syslog_ident");

    lua_pushboolean(L, true);
    return 1;
}

static int strerror_lua(lua_State *const L) {
    lua_pushstring(L, strerror(errno));
    return 1;
}

static int get_level(lua_State *const L, const int index) {
    const char *const lst[] = {
        "emerg",
        "alert",
        "crit",
        "err",
        "warning",
        "notice",
        "info",
        "debug",
        NULL,
    };

    switch (luaL_checkoption(L, index, NULL, lst)) {
      case 0: return LOG_EMERG;
      case 1: return LOG_ALERT;
      case 2: return LOG_CRIT;
      case 3: return LOG_ERR;
      case 4: return LOG_WARNING;
      case 5: return LOG_NOTICE;
      case 6: return LOG_INFO;
      case 7: return LOG_DEBUG;
    }
}

static int syslog_lua(lua_State *const L) {
    {
        const bool facility_specified = !lua_isnone(L, 4);

        int priority = get_level(L, 2);

        if (facility_specified)
            priority |= get_facility(L, 3, NULL);

        syslog(priority, "%s", lua_tostring(L, facility_specified? 4:3));
    }

    lua_pushboolean(L, true);
    return 1;
}

static int setlogmask_lua(lua_State *const L) {
    int mask = 0;

#define CHECKLEVEL(str, value) \
    lua_getfield(L, 2, str); \
    if (lua_toboolean(L, -1)) mask |= value; \
    lua_pop(L, 1);

    CHECKLEVEL("emerg",   LOG_EMERG);
    CHECKLEVEL("alert",   LOG_ALERT);
    CHECKLEVEL("crit",    LOG_CRIT);
    CHECKLEVEL("err",     LOG_ERR);
    CHECKLEVEL("warning", LOG_WARNING);
    CHECKLEVEL("notice",  LOG_NOTICE);
    CHECKLEVEL("info",    LOG_INFO);
    CHECKLEVEL("debug",   LOG_DEBUG);

#undef CHECKLEVEL

    setlogmask(mask);
    lua_pushboolean(L, true);
    return 1;
}

int luaopen_syslog(lua_State *const L) {
    lua_createtable(L, 0, 6);

    lua_pushcfunction(L, closelog_lua);   lua_setfield(L, -2, "close");
    lua_pushcfunction(L, openlog_lua);    lua_setfield(L, -2, "open");
    lua_pushcfunction(L, syslog_lua);     lua_setfield(L, -2, "log");
    lua_pushcfunction(L, strerror_lua);   lua_setfield(L, -2, "strerror");
    lua_pushcfunction(L, setlogmask_lua); lua_setfield(L, -2, "setlogmask");

    lua_createtable(L, 0, 6);
    lua_pushinteger(L, LOG_CONS);   lua_setfield(L, -2, "cons");
    lua_pushinteger(L, LOG_NDELAY); lua_setfield(L, -2, "ndelay");
    lua_pushinteger(L, LOG_NOWAIT); lua_setfield(L, -2, "nowait");
    lua_pushinteger(L, LOG_ODELAY); lua_setfield(L, -2, "odelay");
#ifdef LOG_PERROR
    lua_pushinteger(L, LOG_PERROR);
#else
    lua_pushinteger(L, 0);
#endif
    lua_setfield(L, -2, "perror");
    lua_pushinteger(L, LOG_PID);    lua_setfield(L, -2, "pid");
    lua_setfield(L, -2, "option");

    return 1;
}
