#include "system.h"

#ifdef WITH_LUA
#include <lua.h>
#include <lauxlib.h>

#define _RPMLUA_INTERNAL
#include "rpmio/rpmlua.h"
#include "lib/rpmliblua.h"

static const luaL_Reg luarpmlib_f[] = {
    {NULL, NULL}
};

#ifndef lua_pushglobaltable
#define lua_pushglobaltable(L) lua_pushvalue(L, LUA_GLOBALSINDEX)
#endif

void rpmLuaInit(void)
{
    rpmlua lua = rpmluaGetGlobalState();
    lua_pushglobaltable(lua->L);
#if (LUA_VERSION_NUM < 502) || defined(LUA_COMPAT_MODULE)
    luaL_register(lua->L, "rpm", luarpmlib_f);
#else
    luaL_pushmodule(lua->L, "rpm", 1);
    lua_insert(lua->L, -1);
    luaL_setfuncs(lua->L, luarpmlib_f, 0);
#endif
    return;
}

void rpmLuaFree(void)
{
    rpmlua lua = rpmluaGetGlobalState();
    rpmluaFree(lua);
}

#endif /* WITH_LUA */
