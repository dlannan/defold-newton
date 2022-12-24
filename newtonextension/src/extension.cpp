// myextension.cpp
// Extension lib defines
#define LIB_NAME "Newton"
#define MODULE_NAME "newton"

// include the Defold SDK
#include <dmsdk/sdk.h>
#include <stdlib.h>
#include <Newton.h>
#include <vector>
#include <map>

#include <dMatrix.h>

NewtonWorld* gWorld = NULL;

std::map<uint64_t, NewtonBody*  > gBodies;
std::map<uint64_t, NewtonCollision*> gColls;
std::map<uint64_t, NewtonMesh* > gMeshes;

std::map<uint64_t, int>   bodyUserData;
std::map<NewtonBody* , int>   bodyCallback;

lua_State *gCbL = NULL;

extern uint64_t GetId();

// External collision 
extern int addCollisionSphere( lua_State * L );
extern int addCollisionPlane( lua_State * L ); 
extern int addCollisionCube( lua_State * L );
extern int addCollisionCone( lua_State * L );
extern int addCollisionCapsule( lua_State * L );
extern int addCollisionCylinder( lua_State * L );
extern int addCollisionChamferCylinder( lua_State * L );
extern int addCollisionConvexHull( lua_State * L );
extern int destroyCollision(lua_State *L);

extern int createMeshFromCollision( lua_State *L );

extern int worldRayCast( lua_State *L );

// External Body
extern int addBody( lua_State *L );
extern int bodyGetMass( lua_State *L );
extern int bodySetMassProperties( lua_State *L );
extern int bodyGetUserData( lua_State *L );
extern int bodySetUserData( lua_State *L );
extern int bodySetLinearDamping( lua_State *L );
extern int bodySetAngularDamping( lua_State *L );
extern int bodySetMassMatrix( lua_State *L );
extern int bodyGetCentreOfMass( lua_State *L );
extern int bodySetForceAndTorqueCallback( lua_State *L );

// utils
int SetTableVector( lua_State *L, dFloat *data, const char *name );

static int Create( lua_State *L )
{
    // Cleanup if this is called again during a run.
    Close(L);

    // Print the library version.
    printf("[Newton] Version %d\n", NewtonWorldGetVersion());
    // Create the Newton world.
    gWorld = NewtonCreate();
    NewtonInvalidateCache(gWorld);
    return 0;
}

static int Update( lua_State *L )
{
    double timestep = luaL_checknumber(L, 1);

    // Callbacks use this state to run in - not sure how good/bad/crazy this is
    gCbL = L;

    NewtonUpdate(gWorld, (float)timestep);

    lua_newtable(L);
    
    for ( const auto &bodyItem : gBodies )
    {        
        NewtonBody *body = bodyItem->second;

        // After update, build the table and set all the pos and quats.
        dFloat rot[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        NewtonBodyGetRotation(body, rot);

        dFloat pos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        NewtonBodyGetPosition(body, pos);

        lua_pushnumber(L, bodyItem->first); 
        lua_newtable(L);
        
        SetTableVector(L, pos, "pos");
        SetTableVector(L, rot, "rot");
        lua_settable(L, -3);
    }

    return 1;
}

static int Close( lua_State *L )
{
    // Clean up.
    for ( const auto &meshItem : gMeshes )
        NewtonMeshDestroy(meshItem->second);
    for ( const auto &collItem : gColls )
        NewtonDestroyCollision(collItem->second);
    for ( const auto &bodyItem : gBodies )
        NewtonDestroyBody(bodyItem->second);

    gColls.clear();
    gBodies.clear();
    gMeshes.clear();

    if(gWorld) NewtonDestroy(gWorld);
    return 0;
}


// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"create", Create}, 
    {"update", Update}, 
    {"close", Close},
    
    {"collision_addplane", addCollisionPlane },
    {"collision_addcube", addCollisionCube },
    {"collision_addsphere", addCollisionSphere },
    {"collision_addcone", addCollisionCone },
    {"collision_addcapsule", addCollisionCapsule },
    {"collision_addcylinder", addCollisionCylinder },
    {"collision_addchamfercylinder", addCollisionChamferCylinder },
    {"collision_addconvexhull", addCollisionConvexHull },
    {"collision_destroy", destroyCollision },

    {"body_add", addBody },
    {"body_getmass", bodyGetMass },
    {"body_setmassproperties", bodySetMassProperties },
    {"body_getuserdata", bodyGetUserData },
    {"body_setuserdata", bodySetUserData },
    {"body_setlineardamping", bodySetLinearDamping },
    {"body_setangulardamping", bodySetAngularDamping },
    {"body_setmassmatrix", bodySetMassMatrix },
    {"body_getcentreofmass", bodyGetCentreOfMass },
    {"body_setforceandtorquecallback", bodySetForceAndTorqueCallback },

    {"createmeshfromcollision", createMeshFromCollision },
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, MODULE_NAME, Module_methods);

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

dmExtension::Result AppInitializeNewtonExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppInitializeNewtonExtension\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeNewtonExtension(dmExtension::Params* params)
{
    // Init Lua
    LuaInit(params->m_L);
    dmLogInfo("Registered %s Extension\n", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeNewtonExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppFinalizeNewtonExtension\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeNewtonExtension(dmExtension::Params* params)
{
    dmLogInfo("FinalizeNewtonExtension\n");
     std::map<uint64_t, NewtonMesh* > it = gMeshes.begin();
    for(; it != gMeshes.end(); ++it)
        NewtonMeshDestroy(it->second);
    gMeshes.clear();
	std::map<uint64_t, MeshCollision *> collit = gColl.begin();
    for(;collit != gColl.end(); ++collit)
        NewtonDestroyCollision(collit->second);
    gColls.clear();
    NewtonDestroy(gWorld);    
    return dmExtension::RESULT_OK;
}

dmExtension::Result OnUpdateNewtonExtension(dmExtension::Params* params)
{
    // dmLogInfo("OnUpdateNewtonExtension\n");
    return dmExtension::RESULT_OK;
}

void OnEventNewtonExtension(dmExtension::Params* params, const dmExtension::Event* event)
{
    switch(event->m_Event)
    {
        case dmExtension::EVENT_ID_ACTIVATEAPP:
            dmLogInfo("OnEventNewtonExtension - EVENT_ID_ACTIVATEAPP\n");
            break;
        case dmExtension::EVENT_ID_DEACTIVATEAPP:
            dmLogInfo("OnEventNewtonExtension - EVENT_ID_DEACTIVATEAPP\n");
            break;
        case dmExtension::EVENT_ID_ICONIFYAPP:
            dmLogInfo("OnEventNewtonExtension - EVENT_ID_ICONIFYAPP\n");
            break;
        case dmExtension::EVENT_ID_DEICONIFYAPP:
            dmLogInfo("OnEventNewtonExtension - EVENT_ID_DEICONIFYAPP\n");
            break;
        default:
            dmLogWarning("OnEventNewtonExtension - Unknown event id\n");
            break;
    }
}

// Defold SDK uses a macro for setting up extension entry points:
//
// DM_DECLARE_EXTENSION(symbol, name, app_init, app_final, init, update, on_event, final)

// NewtonExtension is the C++ symbol that holds all relevant extension data.
// It must match the name field in the `ext.manifest`
DM_DECLARE_EXTENSION(Newton, LIB_NAME, AppInitializeNewtonExtension, AppFinalizeNewtonExtension, InitializeNewtonExtension, OnUpdateNewtonExtension, OnEventNewtonExtension, FinalizeNewtonExtension)
