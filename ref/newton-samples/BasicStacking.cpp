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


#include "toolbox_stdafx.h"
#include "SkyBox.h"
#include "DemoEntityManager.h"
#include "DemoInstanceEntity.h"
#include "DemoCamera.h"
#include "DemoMesh.h"
#include "PhysicsUtils.h"

// Dave: my old code in lua script
static void BuildTower(DemoEntityManager* const scene, dFloat spacing, dFloat separspace, dFloat mass, const dVector& origin, dFloat dpt, dFloat hgt, dFloat wdt, dFloat towercount, dFloat towerHigh, dFloat towerboxcount)
{
	dMatrix mplacement1(dGetIdentityMatrix());
	dMatrix mplacement2(dGetIdentityMatrix());
	dFloat spacingbetween = 0.0f;
	dFloat rayon = 0.0f;
	dFloat inpx = 0.0f;
	dFloat inpy = 0.0f;
	dFloat inpz = 0.0f;
	dFloat RollAngle = 0.0f;
	int numBox = -1;
	dVector blockBoxSize(dpt, hgt, wdt);
	//
	// create the shape and visual mesh as a common data to be re used
	NewtonWorld* const world = scene->GetNewton();
	// create a material to control collision with this objects
	int defaultMaterialID;
	defaultMaterialID = NewtonMaterialGetDefaultGroupID(world);
	//
	if (separspace == 0.0f) {
		spacingbetween = 1.0f;
	} else {
		spacingbetween = separspace;
	}
	rayon = spacing * towerboxcount;
	// create the shape and visual mesh as a common data to be re used
	NewtonCollision* const collision = CreateConvexCollision(world, dGetIdentityMatrix(), blockBoxSize, _BOX_PRIMITIVE, defaultMaterialID);
	DemoMesh* const geometry = new DemoMesh("towerbox", scene->GetShaderCache(), collision, "wood_0.tga", "wood_0.tga", "wood_0.tga");
	//
	int n = 0;
	int ct1 = 0;
	int ct2 = 0;
	//
	for (n = 0; (n < (towercount)); n++) {
		for (ct1 = 0; (ct1 < (towerHigh)); ct1++) {
			for (ct2 = 0; (ct2 < (towerboxcount)); ct2++) {
				numBox++;
				if (ct1 % 2) {
					inpx = ((-((n+1) - 1)*spacingbetween) + (-((n+1) - 1) * ((towerboxcount)* wdt))) + (dCos(((360 / towerboxcount) * ct2) * dDegreeToRad) * rayon);
					inpz = dSin(((360 / towerboxcount) * ct2) * dDegreeToRad) * rayon;
					RollAngle = (ct2 * (360 / towerboxcount));
				} else {
					inpx = ((-((n+1) - 1)*spacingbetween) + (-((n+1) - 1) * ((towerboxcount)* wdt))) + (dCos((((360.0f / towerboxcount) * ct2) + (360.0f / towerboxcount / 2)) * dDegreeToRad) * rayon);
					inpz = dSin((((360.0f / towerboxcount) * ct2) + (360.0f / towerboxcount / 2)) * dDegreeToRad) * rayon;
					RollAngle = ((ct2 * (360.0f / towerboxcount)) + (360.0f / towerboxcount / 2));
				}

				inpy = -hgt + ((dpt*(ct1 - 1)) + (dpt - 0.01f));
				mplacement1 = dRollMatrix(-90 * dDegreeToRad);
				mplacement1.m_posit = dVector(origin.m_x + inpx, origin.m_y + inpy, origin.m_z + inpz);
				//
				mplacement2 = dPitchMatrix(RollAngle * dDegreeToRad);
				mplacement1 = mplacement2 * mplacement1;
				CreateSimpleSolid(scene, geometry, mass, mplacement1, collision, defaultMaterialID);	
			}
		}
	}
	// do not forget to release the assets
	geometry->Release();
	NewtonDestroyCollision(collision);
}

static void BuildJenga (DemoEntityManager* const scene, dFloat mass, const dVector& origin, const dVector& size, int count)
{
	// build a standard block stack of 20 * 3 boxes for a total of 60
	NewtonWorld* const world = scene->GetNewton();

	dVector blockBoxSize (0.8f, 0.5f, 0.8f * 3.0f);
	blockBoxSize = blockBoxSize.Scale (1.0f);

	// create the stack
	dMatrix baseMatrix (dGetIdentityMatrix());

	// for the elevation of the floor at the stack position
	baseMatrix.m_posit.m_x = origin.m_x;
	baseMatrix.m_posit.m_z = origin.m_z;

	dFloat startElevation = 100.0f;
	dVector floor (FindFloor (world, dVector (baseMatrix.m_posit.m_x, startElevation, baseMatrix.m_posit.m_z, 0.0f), 2.0f * startElevation));
	baseMatrix.m_posit.m_y = floor.m_y + blockBoxSize.m_y / 2.0f;

	// create a 90 degree rotation matrix
	dMatrix rotMatrix (dYawMatrix (dPi * 0.5f));

	// create a material to control collision with this objects
	int defaultMaterialID;
	defaultMaterialID = NewtonMaterialGetDefaultGroupID (world);

	// separate a bit the block alone the horizontal direction
	dFloat gap = 0.01f;

	// create the shape and visual mesh as a common data to be re used
	NewtonCollision* const collision = CreateConvexCollision (world, dGetIdentityMatrix(), blockBoxSize, _BOX_PRIMITIVE, defaultMaterialID);
	DemoMesh* const geometry = new DemoMesh("box", scene->GetShaderCache(), collision, "wood_0.tga", "wood_0.tga", "wood_0.tga");

	for (int i = 0; i < count; i ++) { 
		dMatrix matrix(baseMatrix);
		matrix.m_posit -= matrix.m_front.Scale (blockBoxSize.m_x - gap); 

		for (int j = 0; j < 3; j ++) { 
			CreateSimpleSolid (scene, geometry, mass, matrix, collision, defaultMaterialID);
			matrix.m_posit += matrix.m_front.Scale (blockBoxSize.m_x + gap);  
		}

		baseMatrix = rotMatrix * baseMatrix;
		baseMatrix.m_posit += matrix.m_up.Scale (blockBoxSize.m_y * 0.99f);
	}

	// do not forget to release the assets	
	geometry->Release(); 
	NewtonDestroyCollision (collision);
}

static void BuildPyramid (DemoEntityManager* const scene, dFloat mass, const dVector& origin, const dVector& size, int count, PrimitiveType type, const dMatrix& shapeMatrix = dGetIdentityMatrix())
{
	dMatrix matrix (dGetIdentityMatrix());
	matrix.m_posit = origin;
	matrix.m_posit.m_w = 1.0f;

	// create the shape and visual mesh as a common data to be re used
	NewtonWorld* const world = scene->GetNewton();

	int defaultMaterialID;
	defaultMaterialID = NewtonMaterialGetDefaultGroupID (world);

	NewtonCollision* const collision = CreateConvexCollision (world, shapeMatrix, size, type, defaultMaterialID);
	DemoMesh* const geometry = new DemoMesh("cylinder_1", scene->GetShaderCache(), collision, "wood_4.tga", "wood_4.tga", "wood_1.tga");

	//	matrix = dRollMatrix(dPi/2.0f);
	dFloat startElevation = 100.0f;
	dVector floor (FindFloor (world, dVector (matrix.m_posit.m_x, startElevation, matrix.m_posit.m_z, 0.0f), 2.0f * startElevation));

	matrix.m_posit.m_y = floor.m_y + size.m_y / 2.0f; 

	// get the dimension from shape itself
	dVector minP(0.0f);
	dVector maxP(0.0f);
	CalculateAABB (collision, dGetIdentityMatrix(), minP, maxP);

	dFloat stepz = maxP.m_z - minP.m_z + 0.03125f;
	dFloat stepy = (maxP.m_y - minP.m_y) - 0.01f;

	dFloat y0 = matrix.m_posit.m_y + stepy / 2.0f;
	dFloat z0 = matrix.m_posit.m_z - stepz * count / 2;

	matrix.m_posit.m_y = y0;

	DemoInstanceEntity* const parentInstance = new DemoInstanceEntity(dGetIdentityMatrix(), NULL);
	scene->Append(parentInstance);
	parentInstance->SetMesh(geometry, dGetIdentityMatrix());

	for (int j = 0; j < count; j ++) {
		matrix.m_posit.m_z = z0;
		for (int i = 0; i < (count - j) ; i ++) {
			//CreateSimpleSolid (scene, geometry, mass, matrix, collision, defaultMaterialID);
			CreateInstancedSolid(scene, parentInstance, mass, matrix, collision, defaultMaterialID);
			matrix.m_posit.m_z += stepz;
		}
		z0 += stepz * 0.5f;
		matrix.m_posit.m_y += stepy;
	}

	// do not forget to release the assets	
	geometry->Release(); 
	NewtonDestroyCollision (collision);
}

static void SphereStack(DemoEntityManager* const scene, dFloat mass, const dVector& origin, const dVector& size, int count)
{
	// build a standard block stack of 20 * 3 boxes for a total of 60
	NewtonWorld* const world = scene->GetNewton();

	dVector blockBoxSize(size);
	blockBoxSize = blockBoxSize.Scale(2.0f);

	// create the stack
	dMatrix baseMatrix(dGetIdentityMatrix());

	// for the elevation of the floor at the stack position
	baseMatrix.m_posit.m_x = origin.m_x;
	baseMatrix.m_posit.m_z = origin.m_z;

	dFloat startElevation = 100.0f;
	dVector floor(FindFloor(world, dVector(baseMatrix.m_posit.m_x, startElevation, baseMatrix.m_posit.m_z, 0.0f), 2.0f * startElevation));
	baseMatrix.m_posit.m_y = floor.m_y + blockBoxSize.m_y * 0.5f;

	// create a material to control collision with this objects
	int defaultMaterialID;
	defaultMaterialID = NewtonMaterialGetDefaultGroupID(world);

	// create the shape and visual mesh as a common data to be re used
	NewtonCollision* const collision = CreateConvexCollision(world, dGetIdentityMatrix(), blockBoxSize, _SPHERE_PRIMITIVE, defaultMaterialID);
	DemoMesh* const geometry = new DemoMesh("sphere", scene->GetShaderCache(), collision, "wood_0.tga", "wood_0.tga", "wood_0.tga");
#if 1
	for (int i = 0; i < count; i++) {
		CreateSimpleSolid(scene, geometry, mass, baseMatrix, collision, defaultMaterialID);
		baseMatrix.m_posit += baseMatrix.m_up.Scale(blockBoxSize.m_x);
	}
#else
	int material = NewtonMaterialGetDefaultGroupID(world);
	NewtonMaterialSetDefaultElasticity(world, material, material, 0.0f);

	dArray<NewtonBody*> array;
	baseMatrix.m_posit.m_y += 1.0f;
	for (int i = 0; i < count; i++) {
		array[i] = CreateSimpleSolid(scene, geometry, mass, baseMatrix, collision, defaultMaterialID);
		baseMatrix.m_posit += baseMatrix.m_up.Scale(blockBoxSize.m_x);
	}

	for (int i = 1; i < count - 1; i++) {
		dMatrix matrix0;
		dMatrix matrix1;
		NewtonBodyGetMatrix(array[i], &matrix0[0][0]);
		NewtonBodyGetMatrix(array[i + 1], &matrix1[0][0]);
		matrix0.m_posit = (matrix0.m_posit + matrix1.m_posit).Scale(0.5f);
		dCustomBallAndSocket* const joint = new  dCustomBallAndSocket(matrix0, array[i], array[i + 1]);
		joint->EnableTwist(true);
		joint->EnableCone(true);
		joint->SetConeLimits(0.0f);
		joint->SetTwistLimits(0.0f, 0.0f);
	}

	dFloat Ixx;
	dFloat Iyy;
	dFloat Izz;
	dFloat mass1;
	dFloat scale = 100.0f;
	NewtonBodyGetMass(array[count - 1], &mass1, &Ixx, &Iyy, &Izz);
	NewtonBodySetMassMatrix(array[count - 1], mass1 * scale, Ixx * scale, Iyy * scale, Izz * scale);
#endif

	//baseMatrix.m_posit += baseMatrix.m_up.Scale(blockBoxSize.m_x * 4.0f);
	DemoMesh* const geometry1 = new DemoMesh("sphere", scene->GetShaderCache(), collision, "wood_1.tga", "wood_1.tga", "wood_1.tga");
//	CreateSimpleSolid(scene, geometry1, mass * 1000.0f, baseMatrix, collision, defaultMaterialID);

	// do not forget to release the assets	
	geometry1->Release();
	geometry->Release();
	NewtonDestroyCollision(collision);
}

static void CapsuleStack(DemoEntityManager* const scene, dFloat mass, const dVector& origin, const dVector& size, int count)
{
	// build a standard block stack of 20 * 3 boxes for a total of 60
	NewtonWorld* const world = scene->GetNewton();

	dVector blockBoxSize(size);

	// create the stack
	dMatrix baseMatrix(dGetIdentityMatrix());

	// for the elevation of the floor at the stack position
	baseMatrix.m_posit.m_x = origin.m_x;
	baseMatrix.m_posit.m_z = origin.m_z;

	dFloat startElevation = 100.0f;
	dVector floor(FindFloor(world, dVector(baseMatrix.m_posit.m_x, startElevation, baseMatrix.m_posit.m_z, 0.0f), 2.0f * startElevation));
	baseMatrix.m_posit.m_y = floor.m_y + blockBoxSize.m_y * 0.5f;

	// create a material to control collision with this objects
	int defaultMaterialID;
	defaultMaterialID = NewtonMaterialGetDefaultGroupID(world);

	// create the shape and visual mesh as a common data to be re used
	NewtonCollision* const collision = CreateConvexCollision(world, dGetIdentityMatrix(), blockBoxSize, _CAPSULE_PRIMITIVE, defaultMaterialID);

	dMatrix uvMatrix (dPitchMatrix(dPi));
	DemoMesh* const geometry = new DemoMesh("sphere", scene->GetShaderCache(), collision, "smilli.tga", "smilli.tga", "smilli.tga", 1.0f, uvMatrix);

	dFloat vertialStep = blockBoxSize.m_x;
	dFloat horizontalStep = blockBoxSize.m_y * 0.8f;
	dMatrix matrix0(dGetIdentityMatrix());
	matrix0.m_posit = origin;
	matrix0.m_posit.m_y += vertialStep * 0.5f;
	matrix0.m_posit.m_w = 1.0f;

	dMatrix matrix1(matrix0);
	matrix1.m_posit.m_z += horizontalStep;

	dMatrix matrix2(dYawMatrix(dPi * 0.5f) * matrix0);
	matrix2.m_posit.m_x += horizontalStep * 0.5f;
	matrix2.m_posit.m_z += horizontalStep * 0.5f;
	matrix2.m_posit.m_y += vertialStep;

	dMatrix matrix3(matrix2);
	matrix3.m_posit.m_x -= horizontalStep;

#if 0
	int bodyCount = 0;
	NewtonBody* bodyArray[256];
	for (int i = 0; i < count/2; i++) {
		bodyArray[bodyCount ++] = CreateSimpleSolid(scene, geometry, mass, matrix0, collision, defaultMaterialID);
		bodyArray[bodyCount ++] = CreateSimpleSolid(scene, geometry, mass, matrix1, collision, defaultMaterialID);
		bodyArray[bodyCount ++] = CreateSimpleSolid(scene, geometry, mass, matrix2, collision, defaultMaterialID);
		bodyArray[bodyCount ++] = CreateSimpleSolid(scene, geometry, mass, matrix3, collision, defaultMaterialID);

		matrix0.m_posit.m_y += vertialStep * 2.0f;
		matrix1.m_posit.m_y += vertialStep * 2.0f;
		matrix2.m_posit.m_y += vertialStep * 2.0f;
		matrix3.m_posit.m_y += vertialStep * 2.0f;
	}

	for (int i = 0; i < bodyCount; i++) {
		NewtonBody* const body = bodyArray[i];
		NewtonBodySetGyroscopicTorque(body, 1);
	}

	// hack the inertia (yes I know it is a cheat, but this can be fix with more iterations as well)
	dFloat inertiaScale = 3.0f;
	for (int i = 0; i < bodyCount; i++) {
		dFloat m;
		dFloat Ixx;
		dFloat Iyy;
		dFloat Izz;
		NewtonBodyGetMass(bodyArray[i], &m, &Ixx, &Iyy, &Izz);
		Ixx *= inertiaScale;
		Iyy *= inertiaScale;
		Izz *= inertiaScale;
		NewtonBodySetMassMatrix(bodyArray[i], m, Ixx, Iyy, Izz);
	}
#else

	matrix0.m_posit.m_y += 0.0f;
	matrix0 = dYawMatrix(30.0f * dDegreeToRad) * dRollMatrix(30.0f * dDegreeToRad) * matrix0;
	NewtonBody* body = CreateSimpleSolid(scene, geometry, mass, matrix0, collision, defaultMaterialID);
	dVector damp(0.0f);
	NewtonBodySetLinearDamping(body, damp.m_w);
	NewtonBodySetAngularDamping(body, &damp.m_x);
	NewtonBodySetMassMatrix(body, 10.0f, 1.11163867f, 2.90651083f, 2.90651083f);
	NewtonBodySetGyroscopicTorque(body, 1);

	dVector omega(matrix0.m_front.Scale(20.0f));
	NewtonBodySetOmega(body, &omega.m_x);
#endif

	// do not forget to release the assets	
	geometry->Release();
	NewtonDestroyCollision(collision);
}

static void BoxStack(DemoEntityManager* const scene, dFloat mass, const dVector& origin, const dVector& size, int count)
{
	// build a standard block stack of 20 * 3 boxes for a total of 60
	NewtonWorld* const world = scene->GetNewton();

	dVector blockBoxSize(size);
	blockBoxSize = blockBoxSize.Scale(2.0f);

	// create the stack
	dMatrix baseMatrix(dGetIdentityMatrix());

	// for the elevation of the floor at the stack position
	baseMatrix.m_posit.m_x = origin.m_x;
	baseMatrix.m_posit.m_z = origin.m_z;

	dFloat startElevation = 100.0f;
	dVector floor(FindFloor(world, dVector(baseMatrix.m_posit.m_x, startElevation, baseMatrix.m_posit.m_z, 0.0f), 2.0f * startElevation));
	baseMatrix.m_posit.m_y = floor.m_y + blockBoxSize.m_y * 0.5f;

	// create a material to control collision with this objects
	int defaultMaterialID;
	defaultMaterialID = NewtonMaterialGetDefaultGroupID(world);

	// create the shape and visual mesh as a common data to be re used
	NewtonCollision* const collision = CreateConvexCollision(world, dGetIdentityMatrix(), blockBoxSize, _BOX_PRIMITIVE, defaultMaterialID);
	DemoMesh* const geometry = new DemoMesh("sphere", scene->GetShaderCache(), collision, "wood_0.tga", "wood_0.tga", "wood_0.tga");

	for (int i = 0; i < count; i++) {
		CreateSimpleSolid(scene, geometry, mass, baseMatrix, collision, defaultMaterialID);
		baseMatrix = dYawMatrix (20.0f * dDegreeToRad) * baseMatrix;
		baseMatrix.m_posit += baseMatrix.m_up.Scale(blockBoxSize.m_x);
	}


	// do not forget to release the assets	
	geometry->Release();
	NewtonDestroyCollision(collision);
}

void BasicBoxStacks (DemoEntityManager* const scene)
{
	// load the skybox
	scene->CreateSkyBox();

	CreateLevelMesh (scene, "flatPlane.ngd", 1);
	//AddFloorBox(scene, dVector (0.0f, 0.0f, 0.0f, 0.0f), dVector (100.0f, 1.0f, 100.0f, 0.0f));

	// tower is very hard to stabilize, needs more iteration 
	//BuildTower(scene, 0.25f, 0.0f, 1.0f, dVector(-2.0f, 0.65f, 25.0f), 0.35f, 0.5f, 1.0f, 1, 80, 12);
	//BuildTower(scene, 0.25f, 0.0f, 1.0f, dVector(-2.0f, 0.65f, 25.0f), 0.35f, 0.5f, 1.0f, 1, 30, 12);


	int high = 20;
high = 30;
//high = 40;
//high = 50;
//high = 100;
//high = 12;
//high = 10;
	for (int i = 0; i < 1; i ++) {
		//BuildPyramid (scene, 10.0f, dVector(  0.0f + i * 4.0f, 0.0f, 0.0f, 0.0f), dVector (0.5f, 0.25f, 0.8f, 0.0f), high, _BOX_PRIMITIVE);
		//BuildPyramid (scene, 10.0f, dVector( 10.0f + i * 4.0f, 0.0f, 0.0f, 0.0f), dVector (0.75f, 0.35f, 0.75f, 0.0f), high, _CYLINDER_PRIMITIVE, dRollMatrix(0.5f * dPi));
		//BuildPyramid (scene, 10.0f, dVector( 20.0f + i * 4.0f, 0.0f, 0.0f, 0.0f), dVector (0.5f, 0.35f, 0.8f, 0.0f), high, _CYLINDER_PRIMITIVE, dRollMatrix(0.5f * dPi));
		//BuildPyramid (scene, 10.0f, dVector( 30.0f + i * 4.0f, 0.0f, 0.0f, 0.0f), dVector (0.5f, 0.25f, 0.8f, 0.0f), high, _REGULAR_CONVEX_HULL_PRIMITIVE, dRollMatrix(0.5f * dPi));
		////BuildPyramid (scene, 10.0f, dVector( 40.0f + i * 4.0f, 0.0f, 0.0f, 0.0f), dVector (0.5f, 0.35f, 0.8f, 0.0), high, _CHAMFER_CYLINDER_PRIMITIVE, dRollMatrix(0.5f * dPi));
	}

	high = 20;
	for (int i = 0; i < 1; i ++) {
		for (int j = 0; j < 1; j ++) {
			//BuildJenga (scene, 5.0f, dVector(-5.0f + j * 8, 0.0f, 12.0f + i * 8, 0.0f), dVector (0.5f, 0.25f, 0.8f, 0.0), high);
		}
	}

	high = 20;
	high = 1;
	for (int i = 0; i < 1; i ++) {
		for (int j = 0; j < 1; j ++) {
			//SphereStack(scene, 1.0f, dVector(-5.0f + j * 8, 0.0f, -6.0f + i * 8, 0.0f), dVector (0.5f, 0.5f, 0.5f, 0.0f), high);
			//CapsuleStack (scene, 1.0f, dVector(-5.0f + j * 8, 0.0f, -14.0f + i * 8, 0.0f), dVector (0.8f, 4.0f, 0.8f, 0.0f), high);
			CapsuleStack(scene, 1.0f, dVector(-5.0f + j * 8, 0.0f, -14.0f + i * 8, 0.0f), dVector(1.0f, 1.0f, 1.0f, 0.0f), high);
			//BoxStack(scene, 1.0f, dVector(-5.5f + j * 8, 0.0f, 6.0f + i * 8, 0.0f), dVector (0.5f, 0.5f, 0.5f, 0.0f), high);
		}
	}


	// place camera into position
	dQuaternion rot;
	dVector origin(-40.0f, 10.0f, 0.0f, 0.0f);
	scene->SetCameraMatrix(rot, origin);
}


