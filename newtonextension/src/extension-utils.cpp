
// include the Defold SDK
#include <dmsdk/sdk.h>
#include <stdlib.h>
#include <Newton.h>
#include <vector>
#include <map>

#include <dMatrix.h>

extern NewtonWorld* gWorld;

extern std::vector<NewtonBody*  > gBodies;
extern std::vector<NewtonCollision*> gColls;
extern std::vector<NewtonMesh* >gMeshes;


extern std::map<int, int>   bodyUserData;
extern std::map<NewtonBody* , int>   bodyCallback;

extern lua_State *gCbL;

 int SetTableVector( lua_State *L, dFloat *data, const char *name )
{
    lua_pushstring(L, name); 
    lua_newtable(L);

    lua_pushstring(L, "x"); 
    lua_pushnumber(L, data[0]);
    lua_settable(L, -3);
    lua_pushstring(L, "y"); 
    lua_pushnumber(L, data[1]);
    lua_settable(L, -3);
    lua_pushstring(L, "z"); 
    lua_pushnumber(L, data[2]);
    lua_settable(L, -3);
    lua_pushstring(L, "w"); 
    lua_pushnumber(L, data[3]);
    lua_settable(L, -3);

    lua_settable(L, -3);
   
    return 0;
}

 void AddTableIndices( lua_State *L, int count, int *indices )
{
    lua_newtable(L);
    int * idxptr = (int *)indices;
    for (int i=1; i<=count; ++i) {
        lua_pushnumber(L, i); 
        lua_pushnumber(L, *idxptr++);
        lua_rawset(L, -3);
    }
}

 void AddTableVertices( lua_State *L, int count, const double *vertices )
{
    lua_newtable(L);
    double * vertptr = (double *)vertices;
    int ctr = 1;
    int step = 4;
    for (int i=0; i<count; i += step) {
        lua_pushnumber(L, ctr++); 
        lua_pushnumber(L, *vertptr++);
        lua_rawset(L, -3);
        lua_pushnumber(L, ctr++); 
        lua_pushnumber(L, *vertptr++);
        lua_rawset(L, -3);
        lua_pushnumber(L, ctr++); 
        lua_pushnumber(L, *vertptr++);
        lua_rawset(L, -3);
        vertptr++;
    }
}


 void AddTableUVs( lua_State *L, int count, const dFloat *uvs )
{
    lua_newtable(L);
    dFloat * uvsptr = (dFloat *)uvs;
    for (int i=1; i<=count; ++i) {
        lua_pushnumber(L, i); 
        lua_pushnumber(L, *uvsptr++);
        lua_rawset(L, -3);
    }
}

 void AddTableNormals( lua_State *L, int count, const dFloat *normals )
{
    lua_newtable(L);
    dFloat * normptr = (dFloat *)normals;
    for (int i=1; i<=count; ++i) {
        lua_pushnumber(L, i); 
        lua_pushnumber(L, *normptr++);
        lua_rawset(L, -3);
    }
}
