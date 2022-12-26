
/* Copyright (c) <2003-2019> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#include "extension.h"



// static MyVehicleManager* gManager = NULL;

// std::map<uint32_t, dNewtonCollisionCompound *> gVehicles;

// void VehiclesInit() {
//     // create a vehicle manager
//     gManager = new MyVehicleManager(gWorld);
// 	gVehicles.clear();
// }

// void VehiclesShutdown() {
// 	gVehicle.clear();
//     delete gManager;
// }

// int VehicleAdd( lua_State *L ) {

//     dNewtonCollisionCompound *shape  = gManager->MakeChassisShape (1);
// 	int index = GetId();
//     gVehicles[index] = shape;
//     lua_pushnumber(L, index);
//     return 1;
// }

// int VehicleRemove( lua_State *L ) {

//     int index = lua_tonumber(L, 1);
// 	std::map<uint32_t, dNewtonCollisionCompound *> it = gVehicles.find(index);
//     if(it != gVehicles.end())
// 	{
//         NewtonDestroyCollision(it->first);    
//         lua_pushnumber(L, 1);
//     } else {
//         printf("[Newton] Vehicle index %d not found.\n", index);
//         lua_pushnil(L);
//     }
//     return 1;
// }