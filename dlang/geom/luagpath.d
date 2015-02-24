/**
 * A Lua interface for the D gpath module.
 *
 * Authors: Rowan G. and Peter J.
 * Date: 2015-02-22
 */

module luagpath;

// We cheat to get the C Lua headers by using LuaD.
import luad.all;
import luad.c.lua;
import luad.c.lauxlib;
import std.stdio;
import std.string;
import util.lua_service;
import geom;
import gpath;
import luageom;

immutable string LineMT = "Line"; // Name of Line metatable

Path checkPath(lua_State* L, int index) {
    if ( isObjType(L, index, LineMT) ) {
	return checkObj!(Line, LineMT)(L, index);
    }
    // if all else fails
    return null;
}

extern(C) int opCallPath(T, string MTname)(lua_State* L)
{
    auto path = checkObj!(T, MTname)(L, 1);
    auto t = luaL_checknumber(L, 2);
    auto pt = path(t);
    return pushVector3(L, pt);
}

extern(C) int t0Path(T, string MTname)(lua_State* L)
{
    // Not much error checking here because we assume
    // users are knowing what they are doing if
    // they are messing with the getter/setter functions.
    int narg = lua_gettop(L);
    auto path = checkObj!(T, MTname)(L, 1);
    if ( narg == 1 ) { // This is a getter
	lua_pushnumber(L, path.t0);
	return 1;
    }
    // else: treat as a setter
    path.t0 = luaL_checknumber(L, 2);
    return 0;
}

extern(C) int t1Path(T, string MTname)(lua_State* L)
{
    // Not much error checking here because we assume
    // users are knowing what they are doing if
    // they are messing with the getter/setter functions.
    int narg = lua_gettop(L);
    auto path = checkObj!(T, MTname)(L, 1);
    if ( narg == 1 ) { // This is a getter
	lua_pushnumber(L, path.t1);
	return 1;
    }
    // else: treat as a setter
    path.t1 = luaL_checknumber(L, 2);
    return 0;
}

extern(C) int copyPath(T, string MTname)(lua_State* L)
{
    // Sometimes it's convenient to get a copy of a path.
    auto path = checkObj!(T, MTname)(L, 1);
    pushObj!(T, MTname)(L, path);
    return 1;
}

/* ----------------- Line specific functions --------------- */

/**
 * The Lua constructor for a Line.
 *
 * Example construction in Lua:
 * ---------------------------------
 * a = Vector3:new{}
 * b = Vector3:new{1, 1}
 * ab = Line:new{a, b}
 * ab = Line:new{a, b, t0=0.0, t1=1.0}
 * ---------------------------------
 */
extern(C) int newLine(lua_State* L)
{
    lua_remove(L, 1); // remove first agurment "this"
    int narg = lua_gettop(L);
    if ( narg == 0 || !lua_istable(L, 1) ) {
	string errMsg = `Error in call to Line:new{}.;
A table containing arguments is expected, but no table was found.`;
	luaL_error(L, errMsg.toStringz);
    }
    // Expect Vector3 at position 1.
    lua_rawgeti(L, 1, 1);
    auto a = checkVector3(L, -1);
    if ( a is null ) {
	string errMsg = `Error in call to Line:new{}.
A Vector3 object is expected as the first argument. No valid Vector3 was found.`;
	luaL_error(L, errMsg.toStringz());
    }
    lua_pop(L, 1);
    // Expect Vector3 at position 2.
    lua_rawgeti(L, 1, 2);
    auto b = checkVector3(L, -1);
    if ( b is null ) {
	string errMsg = `Error in call to Line:new{}.
A Vector3 object is expected as the second argument. No valid Vector3 was found.`;
	luaL_error(L, errMsg.toStringz());
    }
    lua_pop(L, 1);
    string errMsgTmplt = `Error in call to Line:new{}.
A valid value for '%s' was not found in list of arguments.
The value, if present, should be a number.`;
    double t0 = getNumberFromTable(L, 1, "t0", false, 0.0, true, format(errMsgTmplt, "t0"));
    double t1 = getNumberFromTable(L, 1, "t1", false, 1.0, true, format(errMsgTmplt, "t1"));
    auto ab = new Line(*a, *b, t0, t1);
    return pushObj!(Line, LineMT)(L, ab);
}

void registerPaths(LuaState lua)
{
    auto L = lua.state;

    // Register the Line object
    luaL_newmetatable(L, LineMT.toStringz);
    
    /* metatable.__index = metatable */
    lua_pushvalue(L, -1); // duplicates the current metatable
    lua_setfield(L, -2, "__index");

    /* Register methods for use. */
    lua_pushcfunction(L, &newLine);
    lua_setfield(L, -2, "new");
    lua_pushcfunction(L, &opCallPath!(Line, LineMT));
    lua_setfield(L, -2, "__call");
    lua_pushcfunction(L, &opCallPath!(Line, LineMT));
    lua_setfield(L, -2, "eval");
    lua_pushcfunction(L, &toStringObj!(Line, LineMT));
    lua_setfield(L, -2, "__tostring");
    lua_pushcfunction(L, &copyPath!(Line, LineMT));
    lua_setfield(L, -2, "copy");
    lua_pushcfunction(L, &t0Path!(Line, LineMT));
    lua_setfield(L, -2, "t0");
    lua_pushcfunction(L, &t1Path!(Line, LineMT));
    lua_setfield(L, -2, "t1");

    lua_setglobal(L, LineMT.toStringz);
}
    





