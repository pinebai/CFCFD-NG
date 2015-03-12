/**
 * A Lua interface for the D sgrid (StructuredGrid) module.
 *
 * Authors: Rowan G. and Peter J.
 * Date: 2015-02-26
 */

module luasgrid;

// We cheat to get the C Lua headers by using LuaD.
import luad.all;
import luad.c.lua;
import luad.c.lauxlib;
import std.stdio;
import std.string;
import std.conv;
import util.lua_service;
import univariatefunctions;
import geom;
import gpath;
import surface;
import sgrid;
import luageom;
import luasurface;

// Name of metatables
immutable string StructuredGrid2DMT = "StructuredGrid2D";
immutable string StructuredGrid3DMT = "StructuredGrid3D";

StructuredGrid checkStructuredGrid(lua_State* L, int index) {
    if ( isObjType(L, index, StructuredGrid2DMT) ) {
	return checkObj!(StructuredGrid, StructuredGrid2DMT)(L, index);
    }
    if ( isObjType(L, index, StructuredGrid3DMT) ) {
	return checkObj!(StructuredGrid, StructuredGrid3DMT)(L, index);
    }
    // if all else fails
    return null;
}

extern(C) int copyStructuredGrid(T, string MTname)(lua_State* L)
{
    // Sometimes it's convenient to get a copy of a function.
    auto grid = checkObj!(T, MTname)(L, 1);
    pushObj!(T, MTname)(L, grid);
    return 1;
}

extern(C) int get_niv(T, string MTname)(lua_State* L)
{
    int narg = lua_gettop(L); // assume narg == 1; This is a getter
    auto grid = checkObj!(T, MTname)(L, 1);
    lua_pushnumber(L, grid.niv);
    return 1;
}

extern(C) int get_njv(T, string MTname)(lua_State* L)
{
    int narg = lua_gettop(L); // assume narg == 1; This is a getter
    auto grid = checkObj!(T, MTname)(L, 1);
    lua_pushnumber(L, grid.njv);
    return 1;
}

extern(C) int get_nkv(T, string MTname)(lua_State* L)
{
    int narg = lua_gettop(L); // assume narg == 1; This is a getter
    auto grid = checkObj!(T, MTname)(L, 1);
    lua_pushnumber(L, grid.nkv);
    return 1;
}

extern(C) int get_vtx(T, string MTname)(lua_State* L)
{
    int narg = lua_gettop(L);
    auto grid = checkObj!(T, MTname)(L, 1);
    size_t i = to!size_t(luaL_checkint(L, 2)); // Note that we expect 0 <= i < niv
    size_t j = to!size_t(luaL_checkint(L, 3));
    size_t k;
    if (narg > 3) { 
	k = to!size_t(luaL_checkint(L, 4));
    } else {
	k = 0; // Assume 2D grid
    }
    Vector3 vtx = grid[i,j,k];
    return pushVector3(L, vtx);
}

extern(C) int write_to_text_file(T, string MTname)(lua_State* L)
{
    int narg = lua_gettop(L); // assume narg == 2;
    auto grid = checkObj!(T, MTname)(L, 1);
    auto fileName = to!string(luaL_checkstring(L, 2));
    grid.write_to_text_file(fileName, false);
    return 0;
}

/**
 * The Lua constructor for a StructuredGrid.
 *
 * Example construction in Lua:
 * grid = StructuredGrid2D:new{surf=someParametricSurface, niv=10, njv=10,
 *                             clusterf={cf_north, cf_east, cf_south, cf_west},
 *                             label="something"}
 * [TODO] 2D from ParametricVolume
 */
extern(C) int newStructuredGrid2D(lua_State* L)
{
    lua_remove(L, 1); // remove first agurment "this"
    int narg = lua_gettop(L);
    if ( narg == 0 || !lua_istable(L, 1) ) {
	string errMsg = `Error in call to StructuredGrid:new{}.;
A table containing arguments is expected, but no table was found.`;
	luaL_error(L, errMsg.toStringz);
    }
    lua_getfield(L, 1, "surf".toStringz);
    if ( lua_isnil(L, -1) ) {
	string errMsg = "Error in call to StructuredGrid:new{}. surf not found.";
	luaL_error(L, errMsg.toStringz);
    }
    ParametricSurface surf = checkSurface(L, -1);
    if (!surf) {
	string errMsg = "Error in call to StructuredGrid:new{}. surf not a ParametricSurface.";
	luaL_error(L, errMsg.toStringz);
    }
    string errMsgTmplt = `Error in call to StructuredGrid:new{}.
A valid value for '%s' was not found in list of arguments.
The value, if present, should be a number.`;
    int niv = getIntegerFromTable(L, 1, "niv", true, 0, true, format(errMsgTmplt, "niv"));
    int njv = getIntegerFromTable(L, 1, "njv", true, 0, true, format(errMsgTmplt, "njv"));
    // [TODO] include clustering functions.
    UnivariateFunction[] cfList = [new LinearFunction(), new LinearFunction(),
				   new LinearFunction(), new LinearFunction()];
    auto grid = new StructuredGrid(surf, niv, njv, cfList);
    return pushObj!(StructuredGrid, StructuredGrid2DMT)(L, grid);
}

extern(C) int importGridproGrid(lua_State *L)
{
    int narg = lua_gettop(L);
    if ( narg == 0 ) {
	string errMsg = `Error in call to importGridproGrid().
At least one argument is required: the name of the Gridpro file.`;
	luaL_error(L, errMsg.toStringz);
    }
    auto fname = to!string(luaL_checkstring(L, 1));
    double scale = 1.0;
    if ( narg >= 2 ) {
	scale = luaL_checknumber(L, 2);
    }
    auto sgrids = import_gridpro_grid(fname, scale);
    lua_newtable(L);
    foreach ( int i, grid; sgrids ) {
	pushObj!(StructuredGrid, StructuredGrid2DMT)(L, grid);
	lua_rawseti(L, -2, i+1);
    }
    return 1;
}

void registerStructuredGrid(LuaState lua)
{
    auto L = lua.state;

    // Register the StructuredGrid2D object
    luaL_newmetatable(L, StructuredGrid2DMT.toStringz);
    
    /* metatable.__index = metatable */
    lua_pushvalue(L, -1); // duplicates the current metatable
    lua_setfield(L, -2, "__index");

    /* Register methods for use. */
    lua_pushcfunction(L, &newStructuredGrid2D);
    lua_setfield(L, -2, "new");
    lua_pushcfunction(L, &toStringObj!(StructuredGrid, StructuredGrid2DMT));
    lua_setfield(L, -2, "__tostring");
    lua_pushcfunction(L, &copyStructuredGrid!(StructuredGrid, StructuredGrid2DMT));
    lua_setfield(L, -2, "copy");
    lua_pushcfunction(L, &get_niv!(StructuredGrid, StructuredGrid2DMT));
    lua_setfield(L, -2, "get_niv");
    lua_pushcfunction(L, &get_njv!(StructuredGrid, StructuredGrid2DMT));
    lua_setfield(L, -2, "get_njv");
    lua_pushcfunction(L, &get_nkv!(StructuredGrid, StructuredGrid2DMT));
    lua_setfield(L, -2, "get_nkv");
    lua_pushcfunction(L, &get_vtx!(StructuredGrid, StructuredGrid2DMT));
    lua_setfield(L, -2, "get_vtx");
    lua_pushcfunction(L, &write_to_text_file!(StructuredGrid, StructuredGrid2DMT));
    lua_setfield(L, -2, "write_to_text_file");

    lua_setglobal(L, StructuredGrid2DMT.toStringz);

    // Global functions available for use
    lua_pushcfunction(L, &importGridproGrid);
    lua_setglobal(L, "importGridproGrid");

} // end registerStructuredGrid()
    





