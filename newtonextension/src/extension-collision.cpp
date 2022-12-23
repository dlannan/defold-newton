
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

enum ShapeType {

    Shape_Plane           = 1,
    Shape_Cube            ,
    Shape_Sphere          ,
    Shape_Cone            , 
    Shape_Capsule         , 
    Shape_Cylinder        ,
    Shape_ChamferCylinder ,
    Shape_ConvexHull      
}; 

extern int SetTableVector( lua_State *L, dFloat *data, const char *name );
extern void AddTableVertices( lua_State *L, int count, const double *vertices );
extern void AddTableIndices( lua_State *L, int count, int *indices );
extern void AddTableUVs( lua_State *L, int count, const dFloat *uvs );
extern void AddTableNormals( lua_State *L, int count, const dFloat *normals );

 int addCollisionSphere( lua_State * L ) {

    double radii = lua_tonumber(L, 1);
    // Collision shapes: sphere (our ball), and large box (our ground plane).
    NewtonCollision* cs_object = NewtonCreateSphere(gWorld, radii, Shape_Sphere, NULL);
    gColls.push_back( cs_object );
    lua_pushnumber(L, gColls.size()-1);
    return 1;
}

 int addCollisionPlane( lua_State * L ) {

    double width = lua_tonumber(L, 1);
    double depth = lua_tonumber(L, 2);
    // Collision shapes: sphere (our ball), and large box (our ground plane).
    NewtonCollision* cs_object = NewtonCreateBox(gWorld, width, 0.1, depth, Shape_Plane, NULL);
    gColls.push_back( cs_object );
    lua_pushnumber(L, gColls.size() - 1);
    return 1;
}

 int addCollisionCube( lua_State * L ) {

    double sx = lua_tonumber(L, 1);
    double sy = lua_tonumber(L, 2);
    double sz = lua_tonumber(L, 3);
    NewtonCollision* cs_object = NewtonCreateBox(gWorld, sx, sy, sz, Shape_Cube, NULL);
    gColls.push_back( cs_object );
    lua_pushnumber(L, gColls.size() - 1);
    return 1;
}

 int addCollisionCone( lua_State * L ) {

    double radius = lua_tonumber(L, 1);
    double height = lua_tonumber(L, 2);
    NewtonCollision* cs_object = NewtonCreateCone(gWorld, radius, height, Shape_Cone, NULL);
    gColls.push_back( cs_object );
    lua_pushnumber(L, gColls.size() - 1);
    return 1;
}

 int addCollisionCapsule( lua_State * L ) {

    double r0 = lua_tonumber(L, 1);
    double r1 = lua_tonumber(L, 2);
    double height = lua_tonumber(L, 3);
    NewtonCollision* cs_object = NewtonCreateCapsule(gWorld, r0, r1, height, Shape_Capsule, NULL);
    gColls.push_back( cs_object );
    lua_pushnumber(L, gColls.size() - 1);
    return 1;
}

 int addCollisionCylinder( lua_State * L ) {

    double r0 = lua_tonumber(L, 1);
    double r1 = lua_tonumber(L, 2);
    double height = lua_tonumber(L, 3);
    NewtonCollision* cs_object = NewtonCreateCylinder(gWorld, r0, r1, height, Shape_Cylinder, NULL);
    gColls.push_back( cs_object );
    lua_pushnumber(L, gColls.size() - 1);
    return 1;
}

 int addCollisionChamferCylinder( lua_State * L ) {

    double radius = lua_tonumber(L, 1);
    double height = lua_tonumber(L, 2);
    NewtonCollision* cs_object = NewtonCreateChamferCylinder(gWorld, radius, height,Shape_ChamferCylinder, NULL);
    gColls.push_back( cs_object );
    lua_pushnumber(L, gColls.size() - 1);
    return 1;
}

 int addCollisionConvexHull( lua_State * L ) {

    double count = lua_tonumber(L, 1);
    int stride = lua_tonumber(L, 2);
    double tolerance = lua_tonumber(L, 3);
    const float *vertCloud = (float *)lua_topointer(L, 4);
    NewtonCollision* cs_object = NewtonCreateConvexHull(gWorld, count, vertCloud, stride, tolerance, Shape_ConvexHull, NULL);
    gColls.push_back( cs_object );
    lua_pushnumber(L, gColls.size() - 1);
    return 1;
}

 int worldRayCast( lua_State *L ) {

    const dFloat *p0= (dFloat *)lua_topointer(L, 1);
    const dFloat *p1 = (dFloat *)lua_topointer(L, 2);
    NewtonWorldRayFilterCallback filter_cb = *(NewtonWorldRayFilterCallback *)lua_topointer(L, 3);
    void * const userData = (void * const)lua_topointer(L, 4);
    NewtonWorldRayPrefilterCallback prefilter_cb = *(NewtonWorldRayPrefilterCallback *)lua_topointer(L, 5);
        
    NewtonWorldRayCast( gWorld, p0, p1, filter_cb, userData, prefilter_cb, 0);
    return 0;
}

 int destroyCollision(lua_State *L)
{
    int collindex = lua_tonumber(L, 1);
    if(collindex < 0 || collindex > gColls.size()-1) {
        lua_pushnil(L);
        return 1;
    }
    NewtonDestroyCollision(gColls[collindex]);
    gColls.erase(gColls.begin() + collindex);
    lua_pushnumber(L, 1);
    return 1;
}


 int createMeshFromCollision( lua_State *L )
{
    int collindex = lua_tonumber(L, 1);
    if(collindex < 0 || collindex > gColls.size()-1) {
        lua_pushnil(L);
        return 1;
    }

    int mapping = lua_tonumber(L, 2);
    const NewtonCollision *collision = gColls[collindex];

    NewtonMesh *mesh = NewtonMeshCreateFromCollision( collision );
    if(mesh) {
        gMeshes.push_back(mesh);
        lua_pushnumber(L, gMeshes.size() - 1);

        NewtonMeshTriangulate(mesh);
        NewtonMeshCalculateVertexNormals(mesh, 0.3f);

        dMatrix aligmentUV(dGetIdentityMatrix());
        if(mapping == 1)
            NewtonMeshApplySphericalMapping(mesh, 0, &aligmentUV[0][0]);
        else if(mapping == 2)
            NewtonMeshApplyCylindricalMapping(mesh, 0, 0, &aligmentUV[0][0]);
        else
            NewtonMeshApplyBoxMapping(mesh, 0, 0, 0, &aligmentUV[0][0]);
        
        int faceCount = NewtonMeshGetTotalFaceCount (mesh); 
        int indexCount = NewtonMeshGetTotalIndexCount (mesh); 
        int pointCount = NewtonMeshGetPointCount (mesh);
        int vertexStride = NewtonMeshGetVertexStrideInByte(mesh) / sizeof (dFloat);

        // extract vertex data  from the newton mesh		
        dFloat *vertices = new dFloat[3 * indexCount];
        dFloat *normals = new dFloat[3 * indexCount];
        dFloat *uvs = new dFloat[2 * indexCount];

        memset (vertices, 0, 3 * indexCount * sizeof (dFloat));
        memset (normals, 0, 3 * indexCount * sizeof (dFloat));
        memset (uvs, 0, 2 * indexCount * sizeof (dFloat));
       
        int* faceArray = new int [faceCount];
        void** indexArray = new void* [indexCount];
        int* materialIndexArray = new int [faceCount];
        int* remapedIndexArray = new int [indexCount];
                 
        NewtonMeshGetFaces (mesh, faceArray, materialIndexArray, indexArray); 
        NewtonMeshGetUV0Channel(mesh, 2 * sizeof (dFloat), uvs);
        NewtonMeshGetNormalChannel(mesh, 3 * sizeof (dFloat), normals);

        dFloat *mappedNormals = new dFloat[3 * indexCount];
        
        for (int i = 0; i < indexCount; i ++) {

            // int fcount = faceArray[i];
            int index = NewtonMeshGetVertexIndex (mesh, indexArray[i]);            
            //int pindex = NewtonMeshGetPointIndex(mesh, indexArray[i]);

            mappedNormals[i * 3] = normals[index * 3];
            mappedNormals[i * 3+1] = normals[index * 3 + 1];
            mappedNormals[i * 3+2] = normals[index * 3 + 2];
                        
            remapedIndexArray[i] = index;
         }
        
        AddTableIndices(L, indexCount, remapedIndexArray);
        AddTableVertices(L, NewtonMeshGetVertexCount(mesh)*4, NewtonMeshGetVertexArray(mesh));

        AddTableUVs( L, indexCount * 2, uvs );
        AddTableNormals( L, indexCount * 3, mappedNormals );

        // Cleanup
        delete [] faceArray;
        delete [] indexArray;
        delete [] materialIndexArray;
        delete [] remapedIndexArray;

        return 5;
    }
    else {
        lua_pushnil(L);
        return 1;
    }
}