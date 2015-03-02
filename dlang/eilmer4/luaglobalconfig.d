// luaglobalconfig.d
// Lua access to the GlobalConfig class data, for use in the preparation script.
//
// Peter J. and Rowan G.
// 2015-03-02: First code adapted from the other lua wrapper modules.

module luaglobalconfig;

// We cheat to get the C Lua headers by using LuaD.
import luad.all;
import luad.c.lua;
import luad.c.lauxlib;
import std.stdio;
import std.string;
import std.conv;
import util.lua_service;

import gas;
import globalconfig;

// -------------------------------------------------------------------------------
// Set GlobalConfig fields from a table.

extern(C) int configSetFromTable(lua_State* L)
{
    if (!lua_istable(L, 1)) return 0; // nothing to do
    //
    // Look for fields that may be present.
    lua_getfield(L, 1, "title");
    if (!lua_isnil(L, -1)) GlobalConfig.title = to!string(luaL_checkstring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, 1, "dimensions");
    if (!lua_isnil(L, -1)) GlobalConfig.dimensions = luaL_checkint(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, 1, "axisymmetric");
    if (!lua_isnil(L, -1)) GlobalConfig.axisymmetric = to!bool(lua_toboolean(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, 1, "max_time");
    if (!lua_isnil(L, -1)) GlobalConfig.max_time = to!double(luaL_checknumber(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, 1, "dt_init");
    if (!lua_isnil(L, -1)) GlobalConfig.dt_init = to!double(luaL_checknumber(L, -1));
    lua_pop(L, 1);
    //
    return 0;
} // end configSetFromTable()

// Get GlobalConfig fields by their string name.
extern(C) int configGet(lua_State* L)
{
    string fieldName = to!string(luaL_checkstring(L, 1));

    switch (fieldName) {
    case "title":
	lua_pushstring(L, GlobalConfig.title.toStringz);
	break;
    case "dimensions":
	lua_pushnumber(L, GlobalConfig.dimensions);
	break;
    case "axisymmetric":
	lua_pushboolean(L, GlobalConfig.axisymmetric);
	break;
    case "max_time":
	lua_pushnumber(L, GlobalConfig.max_time);
	break;
    case "dt_init":
	lua_pushnumber(L, GlobalConfig.dt_init);
	break;
    default:
	lua_pushnil(L);
    }
    return 1;
} // end configGet()

// Interact via __call, __index and __newindex

extern(C) int configSetWithCall(lua_State* L)
{
    // Arguments to __call are: table then call arguments
    // So remove table and delegate to configSetFromTable
    lua_remove(L, 1);
    return configSetFromTable(L);
}

extern(C) int configSetFromValue(lua_State *L)
{
    // Argumnets to __newindex are: table, key, value
    // We aren't interested in the table because we have
    // the GlobalConfig object to use.
    // Let's put the key and value into a table with one entry
    // and delegate to configSetFromTable.
    lua_newtable(L);
    lua_pushvalue(L, 3);
    lua_setfield(L, -2, luaL_checkstring(L, 2));
    // Now set table to position 1 in stack for use in call to configSetFromTable.
    lua_replace(L, 1);
    return configSetFromTable(L);
}

extern(C) int configGetByKey(lua_State* L)
{
    // Arguments to __index are: table, key
    // Just remove table and delegate to configGet.
    lua_remove(L, 1);
    return configGet(L);
} 

//------------------------------------------------------------------------
// Functions related to the managed gas model.

extern(C) int setGasModel(lua_State* L)
{
    string fname = to!string(luaL_checkstring(L, 1));
    GlobalConfig.gasModelFile = fname;
    GlobalConfig.gmodel = init_gas_model(fname);
    lua_pushinteger(L, GlobalConfig.gmodel.n_species);
    lua_pushinteger(L, GlobalConfig.gmodel.n_modes);
    return 2;
    
}

extern(C) int get_nspecies(lua_State* L)
{
    lua_pushinteger(L, GlobalConfig.gmodel.n_species);
    return 1;
}

extern(C) int get_nmodes(lua_State* L)
{
    lua_pushinteger(L, GlobalConfig.gmodel.n_modes);
    return 1;
}

extern(C) int species_name(lua_State* L)
{
    int i = to!int(luaL_checkinteger(L, 1));
    lua_pushstring(L, GlobalConfig.gmodel.species_name(i).toStringz);
    return 1;
}

//-----------------------------------------------------------------------
// Call the following function from the main program to get the
// functions appearing in the Lua interpreter.

void registerGlobalConfig(LuaState lua)
{
    auto L = lua.state;

    // Register global functions for setting configuration.
    lua_pushcfunction(L, &configSetFromTable);
    lua_setglobal(L, "configSet");
    lua_pushcfunction(L, &configGet);
    lua_setglobal(L, "configGet");

    // Make a 'config' table available
    // First, set its metatable so that the metamethods
    // of __call, __index, and __newindex can do their jobs.
    luaL_newmetatable(L, "config_mt");
    lua_pushcfunction(L, &configSetWithCall);
    lua_setfield(L, -2, "__call");
    lua_pushcfunction(L, &configSetFromValue);
    lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, &configGetByKey);
    lua_setfield(L, -2, "__index");
    lua_setglobal(L, "config_mt");
    // Second make a globally available table called 'config'
    lua_newtable(L);
    luaL_getmetatable(L, "config_mt");
    lua_setmetatable(L, -2);
    lua_setglobal(L, "config");

    // Register other global functions related to the managed gas model.
    lua_pushcfunction(L, &setGasModel);
    lua_setglobal(L, "setGasModel");
    lua_pushcfunction(L, &get_nspecies);
    lua_setglobal(L, "get_nspecies");
    lua_pushcfunction(L, &get_nmodes);
    lua_setglobal(L, "get_nmodes");
    lua_pushcfunction(L, &species_name);
    lua_setglobal(L, "species_name");
} // end registerGlobalConfig()
