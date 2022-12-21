
#include "Newton.h"
#include <dMatrix.h>
#include <dVector.h>
#include <dList.h>
#include <dCustomJoint.h>

extern NewtonWorld* world;

#define TIRE_SHAPE_SIZE                 12

enum VehicleRaycastType
{
	rctWorld = 0,
	rctConvex = 1
};

enum VehicleTireSteer
{
	tsNoSteer = 0,
	tsSteer = 1
};

enum VehicleSteerSide
{
	tsSteerSideA = -1,
	tsSteerSideB = 1
};

enum VehicleTireAccel
{
	tsNoAccel = 0,
	tsAccel = 1
};

enum VehicleTireBrake
{
	tsNoBrake = 0,
	tsBrake = 1
};

enum VehicleTireSide
{
	tsTireSideA = 0,
	tsTireSideB = 1
};

static dFloat VehicleRayCastFilter(const NewtonBody* const body, const NewtonCollision* const collisionHit, const dFloat* const contact, const dFloat* const normal, dLong collisionID, void* const userData, dFloat intersetParam);
static unsigned VehicleRayPrefilterCallback(const NewtonBody* const body, const NewtonCollision* const collision, void* const userData);
//
static void TireJointDeleteCallback(const dCustomJoint* const me);

// dRayCastTireInfo

struct RayCastTireInfo
{
	RayCastTireInfo(const NewtonBody* body)
	{
		m_param = 1.1f;
		m_me = body;
		m_hitBody = NULL;
		m_contactID = 0;
		m_normal = dVector(0.0f, 0.0f, 0.0f, 0.0f);
	}
	dFloat m_param;
	dVector m_normal;
	const NewtonBody* m_me;
	const NewtonBody* m_hitBody;
	int m_contactID;
};

// dRayCastVehicleModel

class dRayCastVehicleModel 
{
//
public:
	// dRayCastTire
	class dRayCastTire : public dCustomJoint
	{
	public:
		dRayCastTire(NewtonBody* const chassisbody, dRayCastVehicleModel* const parentempty)
		: dCustomJoint(2, chassisbody, NULL)
		, m_hitParam(1.1f)
		, m_hitBody(NULL)
		, m_penetration(0.0f)
		, m_contactID(0)
		, m_hitContact(dVector(0.0f))
		, m_hitNormal(dVector(0.0f))
		, m_vehiclemodel(parentempty)
		, m_lateralPin(dVector(0.0f))
		, m_longitudinalPin(dVector(0.0f))
		, m_localAxis(dVector(0.0f))
		, m_brakeForce(0.0f)
		, m_motorForce(0.0f)
		, m_isOnContact(false)
		, m_isOnContactEx(false)
		, m_collision(NULL)
		, m_chassisBody(chassisbody)
		, m_arealpos(0.0f)
		, m_spinAngle(0.0f)
		, m_posit_y(0.0f)
		, m_radius(0.3f)
		, m_width(0.15f)
		, m_springLength(0.2f)
		, m_springConst(200.0f)
		, m_springDamp(10.0f)
		, m_mass(18.0f)
		, m_longitudinalFriction(1.0f)
		, m_lateralFriction(1.0f)
		, m_tireLoad(0.0f)
		, m_suspenssionHardLimit(0.0f)
		, m_chassisSpeed(0.0f)
		, m_tireSpeed(0.0f)
		, m_realvelocity(dVector(0.0f))
		, m_localtireMatrix(dGetIdentityMatrix())
		, m_suspensionMatrix(dGetIdentityMatrix())
		, m_hardPoint(dVector(0.0f))
		, m_tireRenderMatrix(dGetIdentityMatrix())
		, m_frameLocalMatrix(dGetIdentityMatrix())
		, m_angleMatrix(dGetIdentityMatrix())
		, m_TireAxelPosit(dVector(1.0f))
		, m_LocalAxelPosit(dVector(1.0f))
		, m_TireAxelVeloc(dVector(0.0f))
		, m_suspenssionFactor(0.5f)
		, m_suspenssionStep(1.0f / 60.0f)
		, m_tireEntity(NULL)
		, m_tireSide(tsTireSideA)
        , m_steerSide(tsSteerSideB)
		, m_tireSteer(tsNoSteer)
		, m_tireAccel(tsNoAccel)
		, m_brakeMode(tsNoBrake)
		, m_steerAngle(0.0f)
		, m_tireID(-1)
		, m_handForce(0.0f)
		{
		  SetSolverModel(2);
		  m_world = NewtonBodyGetWorld(chassisbody);
		  //
		  m_body0 = chassisbody;
		  m_body1 = NULL;
		  //
		  dMatrix m_matrix;
		  //
	      dVector com(parentempty->m_combackup);
	      // set the chassis matrix at the center of mass
	      //NewtonBodyGetCentreOfMass(m_body0, &com[0]);
	      com.m_w = 1.0f;
	      //
		  NewtonBodyGetMatrix(m_body0, &m_matrix[0][0]);
		  //
		  m_matrix.m_posit += m_matrix.RotateVector(com);
		  // calculate the two local matrix of the pivot point
		  CalculateLocalMatrix(m_matrix, m_localMatrix0, m_localMatrix1);
		  //
		  m_vehiclemodel->m_chassisMatrix = m_matrix /* m_localMatrix0*/;
		};
		//
		virtual ~dRayCastTire()
		{
			if (m_collision) {
				NewtonDestroyCollision(m_collision);
			}
		};
		//
		dFloat ApplySuspenssionLimit()
		{
			dFloat distance;
			distance = (m_springLength - m_posit_y);
			//if (distance > m_springLength)
			if (distance >= m_springLength)
				distance = (m_springLength - m_posit_y) + m_suspenssionHardLimit;
			return distance;
		};
		//
		void ProcessWorldContacts(dFloat const rhitparam, NewtonBody* const rhitBody, dFloat const rpenetration, dLong const rcontactid, dVector const rhitContact, dVector const rhitNormal)
		{
			// Ray world contacts process for reproduce collision with others dynamics bodies.
			// Only experimental, Need a lot of work here to get it right.
			if (m_hitBody) {
				NewtonBodySetSleepState(m_hitBody, 0);
				dFloat ixx, iyy, izz, bmass;
				NewtonBodyGetMass(m_hitBody, &bmass, &ixx, &iyy, &izz);
				if (bmass > 0.0f) {
					//dVector bodyVel = m_realvelocity.Scale(-0.01f);
					//NewtonBodySetVelocity(m_hitBody, &bodyVel[0]);
					//
					dVector uForce(dVector(rhitNormal[0] * -(m_vehiclemodel->m_mass + bmass)*2.0f, rhitNormal[1] * -(m_vehiclemodel->m_mass + bmass)*2.0f, rhitNormal[2] * -(m_vehiclemodel->m_mass + bmass)*2.0f));
					m_vehiclemodel->ApplyForceAndTorque((NewtonBody*)m_hitBody, uForce, dVector(rhitContact[0], rhitContact[1], rhitContact[2], 1.0f));
					//
					//dVector bodyVel = m_realvelocity.Scale(-0.01f);
					//NewtonBodySetVelocity(m_hitBody, &bodyVel[0]);
					//
					//m_posit_y = m_posit_y + info.m_penetration;
				}
				else {
					//printf("a:%.3f b:%.3f c:%.3f \n", rhitNormal[0], rhitNormal[1], rhitNormal[2]);
				}
			}
		};

		void ProcessConvexContacts(NewtonWorldConvexCastReturnInfo const info)
		{
			// Only experimental, Need a lot of work here to get it right.
			if (info.m_hitBody) {
				NewtonBodySetSleepState(info.m_hitBody, 0);
				dFloat ixx, iyy, izz, bmass;
				NewtonBodyGetMass(info.m_hitBody, &bmass, &ixx, &iyy, &izz);
				if (bmass > 0.0f) {
					//dVector bodyVel = m_realvelocity.Scale(-0.01f);
					//NewtonBodySetVelocity(info.m_hitBody, &bodyVel[0]);
					//
					dVector uForce(dVector(info.m_normal[0] * -(m_vehiclemodel->m_mass + bmass)*2.0f, info.m_normal[1] * -(m_vehiclemodel->m_mass + bmass)*2.0f, info.m_normal[2] * -(m_vehiclemodel->m_mass + bmass)*2.0f));
					m_vehiclemodel->ApplyForceAndTorque((NewtonBody*)info.m_hitBody, uForce, dVector(info.m_point[0], info.m_point[1], info.m_point[2], 1.0f));
					//
					//dVector bodyVel = m_realvelocity.Scale(-0.01f);
					//NewtonBodySetVelocity(info.m_hitBody, &bodyVel[0]);
					//
					//m_posit_y = m_posit_y + info.m_penetration;
				}
				else {
					//printf("a:%.3f b:%.3f c:%.3f \n", info.m_normal[0], info.m_normal[1], info.m_normal[2]);
				}
			}
		};
		//
		dMatrix CalculateSuspensionMatrixLocal(dFloat distance)
		{
			dMatrix matrix;
			// calculate the steering angle matrix for the axis of rotation
			matrix.m_front = m_localAxis.Scale(-1.0f);
			matrix.m_up = dVector(0.0f, 1.0f, 0.0f, 0.0f);
			matrix.m_right = dVector(m_localAxis.m_z, 0.0f, -m_localAxis.m_x, 0.0f);
			//
			matrix.m_posit = m_hardPoint + m_localMatrix0.m_up.Scale(distance);
			//
			return matrix;
		}
		//
		dMatrix CalculateTireMatrixAbsolute(dFloat sSide)
		{
			dMatrix am;
			m_angleMatrix = m_angleMatrix * dPitchMatrix((m_spinAngle * sSide) * 60.0f * dDegreeToRad);
			//
			am = m_angleMatrix * CalculateSuspensionMatrixLocal(m_springLength);
			return  am;
		}
		//
		void ApplyBrakes(dFloat bkforce)
		{
			// very simple brake method.
			NewtonUserJointAddLinearRow(m_joint, &m_TireAxelPosit[0], &m_TireAxelPosit[0], &m_longitudinalPin.m_x);
			NewtonUserJointSetRowMaximumFriction(m_joint, bkforce * 1000.0f);
			NewtonUserJointSetRowMinimumFriction(m_joint, -bkforce * 1000.0f);
		}
		//
		dMatrix GetLocalMatrix0()
		{
			return m_localMatrix0;
		};
		//
		void LongitudinalAndLateralFriction(dVector tireposit, dVector lateralpin, dFloat turnfriction, dFloat sidingfriction, dFloat timestep)
		{
			dFloat invMag2,
				frictionCircleMag,
				lateralFrictionForceMag,
				longitudinalFrictionForceMag = 0;
			//
			if (m_tireLoad > 0.0f) {
				frictionCircleMag = m_tireLoad * turnfriction;
				lateralFrictionForceMag = frictionCircleMag;
				//
				longitudinalFrictionForceMag = m_tireLoad * sidingfriction;
				//
				invMag2 = frictionCircleMag / dSqrt(lateralFrictionForceMag * lateralFrictionForceMag + longitudinalFrictionForceMag * longitudinalFrictionForceMag);
				//
				lateralFrictionForceMag = lateralFrictionForceMag * invMag2;
				//
				//
				NewtonUserJointAddLinearRow(m_joint, &tireposit[0], &tireposit[0], &lateralpin[0]);
				NewtonUserJointSetRowMaximumFriction(m_joint, lateralFrictionForceMag);
				NewtonUserJointSetRowMinimumFriction(m_joint, -lateralFrictionForceMag);
			}
			//
		}
		//
		void ProcessPreUpdate(dRayCastVehicleModel* vhModel, dFloat timestep, int threadID)
		{
			NewtonWorldConvexCastReturnInfo info;
			//
			// Initialized empty.
			info.m_contactID = -1;
			info.m_hitBody = NULL;
			info.m_penetration = 0.0f;
			info.m_normal[0] = 0.0f;
			info.m_normal[1] = 0.0f;
			info.m_normal[2] = 0.0f;
			info.m_normal[3] = 0.0f;
			info.m_point[0] = 0.0f;
			info.m_point[1] = 0.0f;
			info.m_point[2] = 0.0f;
			info.m_point[3] = 1.0f;
			//
			m_hitBody = NULL;
			m_isOnContactEx = false;
			//
			dMatrix bodyMatrix;
			NewtonBodyGetMatrix(m_body0, &bodyMatrix[0][0]);
			//
			dMatrix AbsoluteChassisMatrix(bodyMatrix * m_localMatrix0);
			//
			m_frameLocalMatrix = m_localMatrix0;
			//
			if (m_tireSteer == tsSteer) {
				m_localAxis.m_x = dSin((m_steerAngle * (int)m_steerSide) * dDegreeToRad);
				m_localAxis.m_z = dCos((m_steerAngle * (int)m_steerSide) * dDegreeToRad);
			}
			//
			m_posit_y = m_springLength;
			//
			dMatrix CurSuspenssionMatrix = m_suspensionMatrix * AbsoluteChassisMatrix;
			//
			dFloat tiredist = m_springLength;
			//
			m_hitParam = 1.1f;
			//
			if (m_vehiclemodel->m_raytype == rctWorld) {
				tiredist = (m_springLength + m_radius);
				dVector mRayDestination = CurSuspenssionMatrix.TransformVector(m_frameLocalMatrix.m_up.Scale(-tiredist));
				//
				dVector p0(CurSuspenssionMatrix.m_posit);
				dVector p1(mRayDestination);
				//
				NewtonWorldRayCast(world, &p0[0], &p1[0], VehicleRayCastFilter, this, VehicleRayPrefilterCallback, threadID);
			}
			else
				if (m_vehiclemodel->m_raytype == rctConvex) {
					tiredist = (m_springLength);
					dVector mRayDestination = CurSuspenssionMatrix.TransformVector(m_frameLocalMatrix.m_up.Scale(-tiredist));
					//
					if (NewtonWorldConvexCast(world, &CurSuspenssionMatrix[0][0], &mRayDestination[0], m_collision, &m_hitParam, m_vehiclemodel, VehicleRayPrefilterCallback, &info, 1, threadID)) {
						m_hitBody = (NewtonBody*)info.m_hitBody;
						m_hitContact = dVector(info.m_point[0], info.m_point[1], info.m_point[2], info.m_point[3]);
						m_hitNormal = dVector(info.m_normal[0], info.m_normal[1], info.m_normal[2], info.m_normal[3]);
						m_penetration = info.m_penetration;
						m_contactID = info.m_contactID;
						//
					}
				}
			//
			if (m_hitBody) {
				m_isOnContactEx = true;
				m_isOnContact = true;
				//
				dFloat intesectionDist = 0.0f;
				//
				if (m_vehiclemodel->m_raytype == rctWorld)
					intesectionDist = (tiredist * m_hitParam - m_radius);
				else
					intesectionDist = (tiredist * m_hitParam);
				//
				if (intesectionDist < 0.0f) {
					intesectionDist = 0.0f;
				}
				else
				if (intesectionDist > m_springLength) {
					intesectionDist = m_springLength;
				}
				//
				m_posit_y = intesectionDist;
				//
				dVector mChassisVelocity;
				dVector mChassisOmega;
				//
				NewtonBodyGetVelocity(m_body, &mChassisVelocity[0]);
				NewtonBodyGetOmega(m_body, &mChassisOmega[0]);
				//  
				m_TireAxelPosit = AbsoluteChassisMatrix.TransformVector(m_hardPoint - m_frameLocalMatrix.m_up.Scale(m_posit_y));
				m_LocalAxelPosit = m_TireAxelPosit - AbsoluteChassisMatrix.m_posit;
				m_TireAxelVeloc = mChassisVelocity + mChassisOmega.CrossProduct(m_LocalAxelPosit);
				//
				//dVector anorm = AbsoluteChassisMatrix.m_right;
				//
				if (m_posit_y < m_springLength) {
					//
					dVector hitBodyVeloc;
					NewtonBodyGetVelocity(m_hitBody, &hitBodyVeloc[0]);
					//
					dVector relVeloc = m_TireAxelVeloc - hitBodyVeloc;
					//
					m_realvelocity = relVeloc;
					//
					m_tireSpeed = -m_realvelocity.DotProduct3(AbsoluteChassisMatrix.m_up);
					//
					dFloat distance = ApplySuspenssionLimit();
					//
					m_tireLoad = -NewtonCalculateSpringDamperAcceleration(timestep, m_springConst, distance, m_springDamp, m_tireSpeed);
					// The method is a bit wrong here, I need to find a better method for integrate the tire mass.
					// Thid method use the tire mass and interacting on the spring smoothness.
					m_tireLoad = (m_tireLoad * m_vehiclemodel->m_mass * (m_mass / m_radius) * m_suspenssionFactor * m_suspenssionStep);
					//
					dVector SuspensionForce = AbsoluteChassisMatrix.m_up.Scale(m_tireLoad);
					m_vehiclemodel->ApplyForceAndTorque(m_body0, SuspensionForce, m_TireAxelPosit);
					//
					// Only with handbrake
					if (m_handForce > 0.0f) {
						m_motorForce = 0.0f;
					}
					//
					if (m_tireAccel == tsAccel) {
						if (dAbs(m_motorForce) > 0.0f) {
							dVector r_tireForce(AbsoluteChassisMatrix.m_front.Scale(m_motorForce));
							m_vehiclemodel->ApplyForceAndTorque(m_body0, r_tireForce, m_TireAxelPosit);
						}
					}
					//
					// WIP: I'm not sure if it is the right place for process the contacts.
					// I need to do more test, And i'm not so sure about the way to go with contacts vs the force to apply or velocity.
					if (m_vehiclemodel->m_raytype == rctWorld) {
						// No penetration info with the world raycast.
						// I let's it present just in case that later I find a method for introducing a penetration value.
						m_penetration = 0.0f;
						ProcessWorldContacts(m_hitParam, m_hitBody, m_penetration, m_contactID, m_hitContact, m_hitNormal);
					}
					else // With force restitution.
						ProcessConvexContacts(info);
				}
			}
		};
		//

		NewtonBody* m_chassisBody;
		NewtonBody* m_hitBody;
		dFloat m_hitParam;
		dFloat m_penetration;
		dLong m_contactID;
		dVector m_hitContact;
		dVector m_hitNormal;
		NewtonCollision* m_collision;
		//
		dMatrix m_tireRenderMatrix;
		dMatrix m_tireMatrix;
		dMatrix m_localtireMatrix;
		dMatrix m_suspensionMatrix;
		dMatrix m_frameLocalMatrix;
		dVector m_hardPoint;
		dVector m_localAxis;
		dMatrix m_angleMatrix;
		//
		dFloat m_arealpos;
		dFloat m_posit_y;
		dFloat m_radius;
		dFloat m_width;
		dFloat m_springLength;
		dFloat m_springConst;
		dFloat m_springDamp;
		dFloat m_mass;
		dFloat m_longitudinalFriction;
		dFloat m_lateralFriction;
		dFloat m_tireLoad;
		dFloat m_suspenssionHardLimit;
		dFloat m_chassisSpeed;
		dFloat m_tireSpeed;
		dFloat m_suspenssionFactor;
		dFloat m_suspenssionStep;
		dFloat m_spinAngle;
		dVector m_realvelocity;
		dVector m_TireAxelPosit;
		dVector m_LocalAxelPosit;
		dVector m_TireAxelVeloc;
		bool m_isOnContactEx;
		int m_tireID;
		VehicleTireSide m_tireSide;
		VehicleSteerSide m_steerSide;
		VehicleTireSteer m_tireSteer;
		VehicleTireAccel m_tireAccel;
		VehicleTireBrake m_brakeMode;
		dFloat m_steerAngle;
		dFloat m_brakeForce;
		dFloat m_handForce;
		dFloat m_motorForce;
	private:
		//
		dRayCastVehicleModel* m_vehiclemodel;
		dVector m_lateralPin;
		dVector m_longitudinalPin;
		bool m_isOnContact;
	protected:
		virtual void SubmitConstraints(dFloat timestep, int threadIndex)
		{
			dMatrix matrix0;
			dMatrix matrix1;
			dMatrix AbsoluteChassisMatrix;
			//
			NewtonBodySetSleepState(m_body0, 0);
			//
			// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
			CalculateGlobalMatrix(matrix0, matrix1);
			NewtonBodyGetMatrix(m_body0, &AbsoluteChassisMatrix[0][0]);
			//
			m_vehiclemodel->SetChassisMatrix(AbsoluteChassisMatrix /* m_localMatrix0*/);
			//
			dFloat r_angularVelocity = 0.0f;
			//
			if (m_brakeMode == tsNoBrake) 
			  m_brakeForce = 0.0f;
			//
			m_lateralPin = AbsoluteChassisMatrix.RotateVector(m_localAxis);
			m_longitudinalPin = AbsoluteChassisMatrix.m_up.CrossProduct(m_lateralPin);
			//
			if ((m_brakeForce <= 0.0f) && (m_handForce <= 0.0f)) {
				if ((dAbs(m_motorForce) > 0.0f) || m_isOnContact) {
					r_angularVelocity = m_TireAxelVeloc.DotProduct3(m_longitudinalPin);
					m_spinAngle = m_spinAngle - r_angularVelocity * timestep * (dFloat)3.14159265;
				}
			}
			//
			if (m_isOnContact) {
				m_isOnContactEx = true;
				//
				// Need a better implementation for deal about the brake and handbrake...
				if ((m_brakeMode == tsBrake) && (m_brakeForce > 0.0f)) {
					ApplyBrakes(m_brakeForce);
				} else
				if (m_handForce > 0.0f) {
					ApplyBrakes(m_handForce);
				}
				//
				LongitudinalAndLateralFriction(m_TireAxelPosit, m_lateralPin, m_longitudinalFriction, m_lateralFriction, timestep);
				//
				m_isOnContact = false;
			}
		};
		//
		virtual void Deserialize(NewtonDeserializeCallback callback, void* const userData)
		{
			int state;
			callback(userData, &state, sizeof(state));
		};
		//
		virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const
		{
			dCustomJoint::Serialize(callback, userData);
			int state = 0;
			callback(userData, &state, sizeof(state));
		};
		//
	};
	// dRayCastVehicleModel class
	//
	dRayCastVehicleModel( const char* vhfilename, const dMatrix& location, dFloat vhmass)
	: m_readyToRender(false)
	, m_vehicleEntity(NULL)
	//, m_raytype(rctWorld)
	, m_raytype(rctConvex)
	, m_chassisMatrix(location)
	, m_tireCount(0)
	, m_tires()
	, m_mass(vhmass)
	, m_isDefaultCtrl(false)
	, m_debugtire(false)
	, m_combackup(dVector(0.0f))
	{
		//
		m_body = CreateChassisBody(location, vhmass);
	};
	//
	~dRayCastVehicleModel() 
	{
		for (dList<dRayCastVehicleModel::dRayCastTire*>::dListNode* node = m_tires.GetFirst(); node; node = node->GetNext()) {
			dRayCastVehicleModel::dRayCastTire* atire = node->GetInfo();
			delete atire;
		}
		m_tires.RemoveAll();
	};
	//
	void SetCenterChassisMass(const dVector centermass)
	{
		dVector comchassis;
		NewtonBodyGetCentreOfMass(GetBody(), &comchassis[0]);
		comchassis = comchassis + centermass;
		NewtonBodySetCentreOfMass(GetBody(), &comchassis[0]);
	};
	//
	void SetRayCastMode(const VehicleRaycastType raytype)
	{
		m_raytype = raytype;
	};
	//
	NewtonBody* CreateChassisBody(const dMatrix& location, dFloat mass)
	{

		//
		m_vehicleEntity = carEntity;

		NewtonCollision* const shape = m_vehicleEntity->CreateCollisionFromchildren(scene->GetNewton());

		// create a body and call the low level init function
		dMatrix locationMatrix(location);

		NewtonBody* const body = NewtonCreateDynamicBody(scene->GetNewton(), shape, &locationMatrix[0][0]);

		NewtonBodySetUserData(body, m_vehicleEntity);
		//
		//NewtonBodySetTransformCallback(body, DemoEntity::TransformCallback);
		NewtonBodySetForceAndTorqueCallback(body, PhysicsApplyGravityForce);

		dVector aAngularDamping(0.0f);
		NewtonBodySetLinearDamping(body, 0);
		NewtonBodySetAngularDamping(body, &aAngularDamping[0]);

		// set vehicle mass, inertia and center of mass
		NewtonBodySetMassProperties(body, mass, shape);

		dFloat Ixx;
		dFloat Iyy;
		dFloat Izz;
		NewtonBodyGetMass(body, &mass, &Ixx, &Iyy, &Izz);
		Ixx *= 1.5f;
		Iyy *= 1.5f;
		Izz *= 1.5f;

		NewtonBodySetMassMatrix(body, mass, Ixx, Iyy, Izz);
		NewtonDestroyCollision(shape);
		//
		NewtonBodyGetCentreOfMass(body, &m_combackup[0]);
		//
		return body;
	};
	//
	void CalculateTireDimensions(const char* const tireName, dFloat& width, dFloat& radius, NewtonWorld* const world, DemoEntity* const vehEntity) const
	{
		// find the the tire visual mesh 
		DemoEntity* const tirePart = vehEntity->Find(tireName);
		dAssert(tirePart);

		// make a convex hull collision shape to assist in calculation of the tire shape size
		DemoMesh* const tireMesh = (DemoMesh*)tirePart->GetMesh();
		dAssert(tireMesh->IsType(DemoMesh::GetRttiType()));
		//dAssert (tirePart->GetMeshMatrix().TestIdentity());
		const dMatrix& meshMatrix = tirePart->GetMeshMatrix();
		dArray<dVector> temp(tireMesh->m_vertexCount);
		meshMatrix.TransformTriplex(&temp[0].m_x, sizeof(dVector), tireMesh->m_vertex, 3 * sizeof(dFloat), tireMesh->m_vertexCount);
		NewtonCollision* const collision = NewtonCreateConvexHull(world, tireMesh->m_vertexCount, &temp[0].m_x, sizeof(dVector), 0, 0, NULL);

		// get the location of this tire relative to the car chassis
		dMatrix tireMatrix(tirePart->CalculateGlobalMatrix(vehEntity));

		// find the support points that can be used to define the with and high of the tire collision mesh
		dVector extremePoint(0.0f);
		dVector upDir(tireMatrix.UnrotateVector(dVector(0.0f, 1.0f, 0.0f, 0.0f)));
		NewtonCollisionSupportVertex(collision, &upDir[0], &extremePoint[0]);
		radius = dAbs(upDir.DotProduct3(extremePoint));

		dVector widthDir(tireMatrix.UnrotateVector(dVector(0.0f, 0.0f, 1.0f, 0.0f)));
		NewtonCollisionSupportVertex(collision, &widthDir[0], &extremePoint[0]);
		width = widthDir.DotProduct3(extremePoint);

		widthDir = widthDir.Scale(-1.0f);
		NewtonCollisionSupportVertex(collision, &widthDir[0], &extremePoint[0]);
		width += widthDir.DotProduct3(extremePoint);

		// destroy the auxiliary collision
		NewtonDestroyCollision(collision);
	}
	//
	void AddTire(const char* const modelName, VehicleTireSide tireside, VehicleTireSteer tiresteer, VehicleSteerSide steerside, VehicleTireAccel tireaccel, VehicleTireBrake tirebrake,
	dFloat lateralFriction = 1.0f, dFloat longitudinalFriction = 1.0f, dFloat slimitlength = 0.25f, dFloat smass = 14.0f, dFloat spingCont = 200.0f, dFloat sprintDamp = 10.0f)
	{

			dRayCastTire* m_tire = new dRayCastTire(m_body, this);
			// add Tires
			dFloat width;
			dFloat radii;
			CalculateTireDimensions(modelName, width, radii, m_scene->GetNewton(), m_vehicleEntity);
			//
			m_tire->m_springConst = spingCont;
			m_tire->m_springDamp = sprintDamp;
			m_tire->m_steerSide = steerside;
			m_tire->m_tireAccel = tireaccel;
			m_tire->m_tireSteer = tiresteer;
			m_tire->m_brakeMode = tirebrake;
			m_tire->SetUserDestructorCallback(TireJointDeleteCallback);
			m_tire->SetBodiesCollisionState(0);
			//m_tire->SetUserDestructorCallback(NULL);
			m_tire->m_tireSide = tireside;
			// for simplicity, tires are position in global space
			m_tire->m_localtireMatrix = m_tire->m_tireEntity->GetCurrentMatrix();
			m_tire->m_tireMatrix = (m_tire->m_tireEntity->GetCurrentMatrix() * m_chassisMatrix);
			m_tire->m_arealpos = m_tire->m_tireMatrix.m_posit.m_y;
			//
			//m_tires[m_tireCount]->m_localtireMatrix = m_tires[m_tireCount]->m_tireMatrix;
			//m_tire->m_localtireMatrix.m_posit.m_y = 0.0f;
			//
			m_tire->m_localAxis = m_tire->GetLocalMatrix0().UnrotateVector(dVector(0.0f, 0.0f, 1.0f, 0.0f));
			m_tire->m_localAxis.m_w = 0.0f;
			//
			m_tire->m_suspensionMatrix.m_posit = m_tire->m_localtireMatrix.m_posit;
			m_tire->m_hardPoint = m_tire->GetLocalMatrix0().UntransformVector(m_tire->m_localtireMatrix.m_posit);
			//
			m_tire->m_longitudinalFriction = longitudinalFriction;
			m_tire->m_lateralFriction = lateralFriction;
			m_tire->m_mass = smass;
			m_tire->m_width = width * 0.5f;
			m_tire->m_radius = radii;
			m_tire->m_springLength = slimitlength;
			//
			m_tire->m_posit_y = m_tire->m_springLength;
			m_tire->m_suspenssionHardLimit = m_tire->m_radius * 0.5f;
			//
			m_tire->m_tireID = m_tireCount;
			//
			dVector r_shapePoints[TIRE_SHAPE_SIZE * 2];
			//
			for (int i = 0; i < TIRE_SHAPE_SIZE; i++) {
				r_shapePoints[i].m_x = -m_tire->m_width;
				r_shapePoints[i].m_y = m_tire->m_radius * dCos(2.0f * 3.1416 * dFloat(i) / dFloat(TIRE_SHAPE_SIZE));
				r_shapePoints[i].m_z = m_tire->m_radius * dSin(2.0f * 3.1416 * dFloat(i) / dFloat(TIRE_SHAPE_SIZE));
				r_shapePoints[i + TIRE_SHAPE_SIZE].m_x = -r_shapePoints[i].m_x;
				r_shapePoints[i + TIRE_SHAPE_SIZE].m_y = r_shapePoints[i].m_y;
				r_shapePoints[i + TIRE_SHAPE_SIZE].m_z = r_shapePoints[i].m_z;
			}
			//
			m_tire->m_collision = NewtonCreateConvexHull(m_scene->GetNewton(), TIRE_SHAPE_SIZE * 2, &r_shapePoints[0].m_x, sizeof(dVector), 0.0f, 0, NULL);
			//
			m_tires.Append(m_tire);
			m_tireCount++;

	};
	//
	dRayCastTire* GetTire(int tireID) 
	{
		int aID = 0;
		for (dList<dRayCastVehicleModel::dRayCastTire*>::dListNode* node = m_tires.GetFirst(); node; node = node->GetNext()) {
			if (aID == tireID) {
			  return (dRayCastVehicleModel::dRayCastTire*)node->GetInfo();
			}
			aID++;
		}
		return 0;
	};
	//
	dFloat VectorLength(dVector aVec)
	{
		return dSqrt(aVec[0] * aVec[0] + aVec[1] * aVec[1] + aVec[2] * aVec[2]);
	};
	//
	dVector Rel2AbsPoint(NewtonBody* Body, dVector Pointrel)
	{
		dMatrix M;
		dVector P, A;
		NewtonBodyGetMatrix(Body, &M[0][0]);
		P = dVector(Pointrel[0], Pointrel[1], Pointrel[2], 1.0f);
		A = M.TransformVector(P);
		return dVector(A.m_x, A.m_y, A.m_z, 0.0f);
	};
	//
	void AddForceAtPos(NewtonBody* Body, dVector Force, dVector Point)
	{
		dVector R, Torque, com;
		com = m_combackup;
		//NewtonBodyGetCentreOfMass(Body, &com[0]);
		R = Point - Rel2AbsPoint(Body, com);
		Torque = R.CrossProduct(Force);
		NewtonBodyAddForce(Body, &Force[0]);
		NewtonBodyAddTorque(Body, &Torque[0]);
	};
	//
	void AddForceAtRelPos(NewtonBody* Body, dVector Force, dVector Point)
	{
		if (VectorLength(Force) != 0.0f) {
			AddForceAtPos(Body, Force, Rel2AbsPoint(Body, Point));
		}
	};
	//
	void ApplyForceAndTorque(NewtonBody* vBody, dVector vForce, dVector vPoint)
	{
		AddForceAtPos(vBody, vForce, vPoint);
	};
	//
	void SetChassisMatrix(dMatrix vhmatrix) 
	{
		m_chassisMatrix = vhmatrix;
	};
	//
	bool m_readyToRender;
	int m_tireCount;
	bool m_isDefaultCtrl;
	bool m_debugtire;
	dFloat m_mass;
	NewtonBody* m_body;
	dVector m_combackup;
	VehicleRaycastType m_raytype;
	dList<dRayCastVehicleModel::dRayCastTire*> m_tires;
private:
	dMatrix m_chassisMatrix;
};

// RayCast CallBack's

dFloat VehicleRayCastFilter(const NewtonBody* const body, const NewtonCollision* const collisionHit, const dFloat* const contact, const dFloat* const normal, dLong collisionID, void* const userData, dFloat intersetParam)
{
	dRayCastVehicleModel::dRayCastTire* const me = (dRayCastVehicleModel::dRayCastTire*)userData;
	if (intersetParam < me->m_hitParam) {
		me->m_hitParam = intersetParam;
		me->m_hitBody = (NewtonBody*)body;
		me->m_hitContact = dVector(contact[0], contact[1], contact[2], contact[3]);
		me->m_hitNormal = dVector(normal[0], normal[1], normal[2], normal[3]);
	}
	return intersetParam;
}

unsigned VehicleRayPrefilterCallback(const NewtonBody* const body, const NewtonCollision* const collision, void* const userData)
{
	NewtonBody* const me = (NewtonBody*)userData;
	if (me != body)
		return 1;
	else
		return 0;
}

void TireJointDeleteCallback(const dCustomJoint* const me)
{
	// do nothing...
}

dRayCastVehicleModel* viper = NULL;
dRayCastVehicleModel* truck = NULL;
dRayCastVehicleModel* currentVehicle = NULL;
bool trucksteermode = true;
int raymode = 1;

// dRayCastVehicleManager


//
static void UpdateDriverInput(dRayCastVehicleModel* const vehiclemodel, dRayCastVehicleModel::dRayCastTire* vhtire, dFloat timestep)
{	
	vhtire->m_steerAngle = 0.0f;
	vhtire->m_motorForce = 0.0f;
	vhtire->m_brakeForce = 0.0f;
	vhtire->m_handForce = 0.0f;
	//
	// Need a better implementation here about the brake and handbrake...
	if (vehiclemodel == viper) {
		vhtire->m_steerAngle = (dFloat(vehiclemodel->m_scene->GetKeyState('A') * 45.0f) - dFloat(vehiclemodel->m_scene->GetKeyState('D') * 45.0f));
		vhtire->m_motorForce = (dFloat(vehiclemodel->m_scene->GetKeyState('W') * 5000.0f * 120.0f * timestep) - dFloat(vehiclemodel->m_scene->GetKeyState('S') * 4500.0f * 120.0f * timestep));
		
		dFloat brakeval = dFloat(vehiclemodel->m_scene->GetKeyState(' '));
		if (dAbs(brakeval) > 0.0f)
			if ((vhtire->m_tireID == 0) || (vhtire->m_tireID == 1) || (vhtire->m_tireID == 2) || (vhtire->m_tireID == 3)) {
			vhtire->m_handForce = brakeval * 5.5f;
			}
		//
		dFloat brakeval2 = dFloat(vehiclemodel->m_scene->GetKeyState('B'));
		if (dAbs(brakeval2) > 0.0f)
			if ((vhtire->m_tireID == 2) || (vhtire->m_tireID == 3)) {
				vhtire->m_brakeForce = brakeval2 * 7.5f;
			}
			else {
				vhtire->m_brakeForce = 0.0f;
			}
	} else
	if (vehiclemodel == truck) {
		vhtire->m_steerAngle = (dFloat(vehiclemodel->m_scene->GetKeyState('A') * 35.0f) - dFloat(vehiclemodel->m_scene->GetKeyState('D') * 35.0f));
		vhtire->m_motorForce = (dFloat(vehiclemodel->m_scene->GetKeyState('W') * 3500.0f * 120.0f * timestep) - dFloat(vehiclemodel->m_scene->GetKeyState('S') * 2000.0f * 120.0f * timestep));
		dFloat brakeval = dFloat(vehiclemodel->m_scene->GetKeyState(' '));
		if (dAbs(brakeval) > 0.0f)
			if ((vhtire->m_tireID == 0) || (vhtire->m_tireID == 1) || (vhtire->m_tireID == 2) || (vhtire->m_tireID == 3)) {
				vhtire->m_handForce = brakeval * 5.5f;
			}
		//
		dFloat brakeval2 = dFloat(vehiclemodel->m_scene->GetKeyState('B'));
		if (dAbs(brakeval2) > 0.0f)
			if ((vhtire->m_tireID == 2) || (vhtire->m_tireID == 3)) {
				vhtire->m_brakeForce = brakeval2 * 7.5f;
			}
			else {
				vhtire->m_brakeForce = 0.0f;
			}
	}
};
//
virtual void OnPreUpdate(dModelRootNode* const model, dFloat timestep, int threadID) const 
{
	dRayCastVehicleModel* controller = (dRayCastVehicleModel*)model;
	//
	for (dList<dRayCastVehicleModel::dRayCastTire*>::dListNode* node = controller->m_tires.GetFirst(); node; node = node->GetNext()) {
		dRayCastVehicleModel::dRayCastTire* atire = node->GetInfo();
		if (controller->m_isDefaultCtrl)
			UpdateDriverInput(controller, atire, timestep);
		//
		atire->ProcessPreUpdate(controller, timestep, threadID);
	}
};
	//
	virtual void OnPostUpdate(dModelRootNode* const model, dFloat timestep, int threadID) const 
	{
		//dRayCastVehicleModel* controller = (dRayCastVehicleModel*)model;
		//
	};
	//
	virtual void OnUpdateTransform(const dModelNode* const bone, const dMatrix& localMatrix) const 
	{
		dRayCastVehicleModel* aMod = (dRayCastVehicleModel*)bone;
		NewtonBody* const body = bone->GetBody();
		DemoEntity* aUserData = (DemoEntity*)NewtonBodyGetUserData(body);
		aUserData->SetMatrix(*m_scene, dQuaternion(localMatrix), dVector(localMatrix.m_posit));
		//
		int tirecnt = 0;
		for (dList<dRayCastVehicleModel::dRayCastTire*>::dListNode* node = aMod->m_tires.GetFirst(); node; node = node->GetNext()) {
			dRayCastVehicleModel::dRayCastTire* atire = node->GetInfo();
			//
			dMatrix m_AbsoluteMatrix;
			//
			// Adjust the right and left tire mesh side and rotation side.
			if (atire->m_tireSide == tsTireSideA) {
				m_AbsoluteMatrix = (atire->CalculateTireMatrixAbsolute(-1.0f));
			}
			else {
				m_AbsoluteMatrix = (atire->CalculateTireMatrixAbsolute(1.0f) * dYawMatrix(-180.0f * dDegreeToRad));
			}
			//
			dVector atireposit = atire->m_localtireMatrix.m_posit;
			atireposit.m_y -= atire->m_posit_y - atire->m_radius * 0.5f;
			atire->m_tireEntity->SetMatrix(*m_scene, dQuaternion(m_AbsoluteMatrix), dVector(atireposit));
			//
			if (!atire->m_isOnContactEx) {
				// Let's spin the tire on the air with very minimal spin reduction.
				if (atire->m_spinAngle > 0.0f)
					atire->m_spinAngle = atire->m_spinAngle - 0.00001f;
				else
				if (atire->m_spinAngle < 0.0f)
					atire->m_spinAngle = atire->m_spinAngle + 0.00001f;
			}
			else {
				atire->m_spinAngle = 0.0f;
			}
			//
			tirecnt++;
		}
	};
	//
	void SetPlayerController(dRayCastVehicleModel* playerModelCtrl)
	{
		m_player = playerModelCtrl;
	}
	//
	dRayCastVehicleModel* CreateVehicleRayCast(const char* vhfilename, const bool useControl, const dMatrix& location, dFloat vhmass = 1200.0f)
	{
		//
		dRayCastVehicleModel* const controller = new dRayCastVehicleModel(m_scene, vhfilename, location, vhmass);
		//
		controller->m_isDefaultCtrl = useControl;
		// the the model to calculate the local transformation
		controller->SetTransformMode(true);

		// add the model to the manager
		AddRoot(controller);
		
		m_player = controller;

		return controller;
	};
	//
	dRayCastVehicleModel* m_player;
	int m_vehicleID;
};


