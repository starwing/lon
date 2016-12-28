#define LUA_LIB
#ifdef __cplusplus
extern "C" {
#endif
#include "lua.h"
#include "lauxlib.h"
#ifdef __cplusplus
}
#endif

#define LON_IMPLEMENTATION
#define LON_LUA_API
#include "lon.h"

#define LON_DUMPER "lon.Dumper"

/* dumper */

typedef struct lonL_Dumper {
    lon_Dumper D;
    lon_Buffer B;
    lua_State *L;
} lonL_Dumper;

static size_t dump_writer(void *ud, const char *s, size_t len) {
    lonL_Dumper *D = (lonL_Dumper*)ud;
    if (!lon_addlstring(&D->B, s, len))
        luaL_error(D->L, "out of memory");
    return len;
}

static int Ldump_option(lua_State *L) {
    static const char *opts[] = {
        "compat", "indent", "newline", "return",
        "int_hexa", "num_hexa", "num_precision",
        "quote", NULL
    };
    lonL_Dumper *D = (lonL_Dumper*)luaL_checkudata(L, 1, LON_DUMPER);
    int type = lua_type(L, 2);
    if (type == LUA_TSTRING) {
        int flag = luaL_checkoption(L, 2, NULL, opts);
        if (lua_isnoneornil(L, 3)) {
            int value = lon_setdumpopt(&D->D, flag, 0);
            lon_setdumpopt(&D->D, flag, value);
            lua_pushinteger(L, value);
            return 1;
        }
        lon_setdumpopt(&D->D, flag, (int)luaL_checkinteger(L, 3));
    }
    else {
        luaL_checktype(L, 2, LUA_TTABLE);
        lua_pushnil(L);
        while (lua_next(L, 2)) {
            int flag = luaL_checkoption(L, -2, NULL, opts);
            int value = (int)luaL_checkinteger(L, -1);
            lon_setdumpopt(&D->D, flag, value);
            lua_pop(L, 1);
        }
    }
    lua_settop(L, 1); return 1;
}

static int Ldump_new(lua_State *L) {
    lonL_Dumper *D = (lonL_Dumper*)lua_newuserdata(L, sizeof(lonL_Dumper));
    D->L = L;
    luaL_setmetatable(L, LON_DUMPER);
    lon_initdumper(&D->D);
    lon_initbuffer(&D->B, NULL);
    lon_setwriter(&D->D, dump_writer, &D);
    if (!lua_isnoneornil(L, 1)) {
        lua_pushcfunction(L, Ldump_option);
        lua_insert(L, 1);
        lua_call(L, lua_gettop(L)-1, 1);
        return 1;
    }
    return 1;
}

static int Ldump_delete(lua_State *L) {
    lonL_Dumper *D = (lonL_Dumper*)luaL_checkudata(L, 1, LON_DUMPER);
    lon_resetbuffer(&D->B);
    return 0;
}

static int Ldump_dump(lua_State *L) {
    lonL_Dumper *D = (lonL_Dumper*)luaL_checkudata(L, 1, LON_DUMPER);
    int i, top = lua_gettop(L);
    lon_resetbuffer(&D->B);
    lon_dump_begin(&D->D);
    for (i = 1; i <= top; ++i)
        lon_dump_value(&D->D, L, i);
    lon_dump_end(&D->D);
    lua_pushlstring(L, lon_buffer(&D->B), lon_buffsize(&D->B));
    return 1;
}

static void open_dumper(lua_State *L) {
    luaL_Reg libs[] = {
        { "__gc", Ldump_delete },
#define ENTRY(name) { #name, Ldump_##name }
        ENTRY(new),
        ENTRY(delete),
        ENTRY(option),
        ENTRY(dump),
#undef  ENTRY
        { NULL, NULL }
    };
    if (luaL_newmetatable(L, LON_DUMPER)) {
        luaL_setfuncs(L, libs, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
    }
}


/* high level interface */

static size_t buff_writer(void *ud, const char *s, size_t len) {
    luaL_addlstring((luaL_Buffer*)ud, s, len);
    return len;
}

static int Lencode(lua_State *L) {
    int i, top = lua_gettop(L);
    lon_Dumper D;
    luaL_Buffer B;
    luaL_buffinit(L, &B);
    lon_initdumper(&D);
    lon_setwriter(&D, buff_writer, &B);
    lon_dump_begin(&D);
    for (i = 1; i <= top; ++i)
        lon_dump_value(&D, L, i);
    lon_dump_end(&D);
    luaL_pushresult(&B);
    return 1;
}

static int safe_decode(lua_State *L) {
    size_t len;
    const char *s = lua_tolstring(L, 2, &len);
    lon_Loader *loader = (lon_Loader*)lua_touserdata(L, 1);
    lon_setluastate(loader, L);
    if (lon_load_buffer(loader, s, len) == LON_OK)
        return lua_gettop(L)-2;
    return luaL_error(L, "unknown error");
}

static int Ldecode(lua_State *L) {
    lon_Loader loader;
    luaL_checkstring(L, 1);
    lua_settop(L, 1);
    lon_initloader(&loader);
    lua_pushcfunction(L, safe_decode);
    lua_pushlightuserdata(L, &loader);
    lua_pushvalue(L, 1);
    if (lua_pcall(L, 2, LUA_MULTRET, 0) == LUA_OK)
        return lua_gettop(L)-1;
    lon_break(&loader, LON_OK);
    lua_pushnil(L);
    lua_insert(L, -2);
    return 2;
}

LUALIB_API int luaopen_lon(lua_State *L) {
    luaL_Reg libs[] = {
#define ENTRY(name) { #name, L##name }
        ENTRY(encode),
        ENTRY(decode),
#undef  ENTRY
        { NULL, NULL }
    };
    open_dumper(L);
    luaL_newlib(L, libs);
    return 1;
}

/* cc: flags+='-s -O3 -mdll -DLUA_BUILD_AS_DLL'
 * cc: libs+='-llua53' output='lon.dll' */

