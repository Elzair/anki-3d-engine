#include <stdio.h>
#include <iostream>
#include <fstream>
#include <typeinfo>
#include "Common.h"

#include "Input.h"
#include "Camera.h"
#include "Math.h"
#include "Renderer.h"
#include "Ui.h"
#include "App.h"
#include "particles.h"
#include "Texture.h"
#include "Mesh.h"
#include "Light.h"
#include "collision.h"
#include "Material.h"
#include "Resource.h"
#include "Scene.h"
#include "Scanner.h"
#include "skybox.h"
#include "map.h"
#include "MeshNode.h"
#include "SkelModelNode.h"
#include "MeshNode.h"
#include "SkelAnim.h"
#include "MeshSkelNodeCtrl.h"
#include "SkelAnimCtrl.h"
#include "SkelNode.h"
#include "LightProps.h"
#include "BulletDebuger.h"
#include "PhyCommon.h"
#include "Parser.h"
#include "ParticleEmitter.h"
#include "PhyCharacter.h"

App* app;

// map (hard coded)
MeshNode* floor__,* sarge,* horse,* crate;
SkelModelNode* imp;
PointLight* point_lights[10];
SpotLight* spot_lights[2];
ParticleEmitter* partEmitter;


// Physics
BulletDebuger debugDrawer;
Vec<btRigidBody*> boxes;

#define ARRAY_SIZE_X 5
#define ARRAY_SIZE_Y 5
#define ARRAY_SIZE_Z 5

#define MAX_PROXIES (ARRAY_SIZE_X*ARRAY_SIZE_Y*ARRAY_SIZE_Z + 1024)

#define SCALING 1.
#define START_POS_X -5
#define START_POS_Y -5
#define START_POS_Z -3


void initPhysics()
{
	btDiscreteDynamicsWorld* dynamicsWorld = app->getScene()->getPhyWorld()->getDynamicsWorld();

	btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(50.),btScalar(50.),btScalar(50.)));

	btTransform groundTransform;
	groundTransform.setIdentity();
	groundTransform.setOrigin(btVector3(0,-50,0));

	//We can also use DemoApplication::localCreateRigidBody, but for clarity it is provided here:
	{
		btScalar mass(0.);

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass,localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,groundShape,localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		//add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);
	}


	{
		//create a few dynamic rigidbodies
		// Re-using the same collision is better for memory usage and performance

		btCollisionShape* colShape = new btBoxShape(btVector3(SCALING*1,SCALING*1,SCALING*1));
		//btCollisionShape* colShape = new btSphereShape(btScalar(1.));

		/// Create Dynamic Objects
		btTransform startTransform;
		startTransform.setIdentity();

		btScalar	mass(1.0);

		btVector3 localInertia(0,0,0);
		colShape->calculateLocalInertia(mass,localInertia);

		float start_x = START_POS_X - ARRAY_SIZE_X/2;
		float start_y = START_POS_Y;
		float start_z = START_POS_Z - ARRAY_SIZE_Z/2;

		btRigidBody* body;
		for (int k=0;k<ARRAY_SIZE_Y;k++)
		{
			for (int i=0;i<ARRAY_SIZE_X;i++)
			{
				for(int j = 0;j<ARRAY_SIZE_Z;j++)
				{
					/*startTransform.setOrigin(SCALING*btVector3(
										btScalar(2.0*i + start_x),
										btScalar(20+2.0*k + start_y),
										btScalar(2.0*j + start_z)));*/


					//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
					MeshNode* crate = new MeshNode;
					crate->init( "models/crate0/crate0.mesh" );
					crate->scaleLspace = 1.11;

					Transform trf( SCALING*Vec3(2.0*i + start_x, 20+2.0*k + start_y, 2.0*j + start_z), Mat3::getIdentity(), 1.0 );
					body = app->getScene()->getPhyWorld()->createNewRigidBody( mass, trf, colShape, crate );

					/*MotionState* myMotionState = new MotionState( startTransform, crate);
					btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,colShape,localInertia);
					//btRigidBody* body = new btRigidBody(rbInfo);
					body = new btRigidBody(rbInfo);

					//if( i=2 ) body->setActivationState(ISLAND_SLEEPING);

					//body->setActivationState(ISLAND_SLEEPING);


					dynamicsWorld->addRigidBody(body);
					//body->setGravity( toBt( Vec3( Util::randRange(-1.0, 1.0), Util::randRange(-1.0, 1.0), Util::randRange(-1.0, 1.0) ) ) );*/
					boxes.push_back( body );
				}
			}
		}
	}


	//dynamicsWorld->setDebugDrawer(&debugDrawer);
}



//=====================================================================================================================================
// init                                                                                                                               =
//=====================================================================================================================================
void init()
{
	PRINT( "Engine initializing..." );

	srand( unsigned(time(NULL)) );
	mathSanityChecks();

	app = new App;
	app->initWindow();
	uint ticks = app->getTicks();

	R::init();
	Ui::init();

	// camera
	Camera* cam = new Camera( R::aspectRatio*toRad(60.0), toRad(60.0), 0.5, 200.0 );
	cam->moveLocalY( 3.0 );
	cam->moveLocalZ( 5.7 );
	cam->moveLocalX( -0.3 );
	app->setActiveCam( cam );

	// lights
	point_lights[0] = new PointLight();
	point_lights[0]->init( "maps/temple/light0.light" );
	point_lights[0]->setLocalTransformation( Vec3( -1.0, 2.4, 1.0 ), Mat3::getIdentity(), 1.0 );
	point_lights[1] = new PointLight();
	point_lights[1]->init( "maps/temple/light1.light" );
	point_lights[1]->setLocalTransformation( Vec3( 2.5, 1.4, 1.0 ), Mat3::getIdentity(), 1.0 );

	spot_lights[0] = new SpotLight();
	spot_lights[0]->init( "maps/temple/light2.light" );
	spot_lights[0]->setLocalTransformation( Vec3( 1.3, 4.3, 3.0 ), Mat3( Euler(toRad(-20), toRad(20), 0.0) ), 1.0 );
	spot_lights[1] = new SpotLight();
	spot_lights[1]->init( "maps/temple/light3.light" );
	spot_lights[1]->setLocalTransformation( Vec3( -2.3, 6.3, 2.9 ), Mat3( Euler(toRad(-70), toRad(-20), 0.0) ), 1.0 );

	// horse
	horse = new MeshNode();
	horse->init( "meshes/horse/horse.mesh" );
	horse->setLocalTransformation( Vec3( -2, 0, 1 ), Mat3( Euler(-M::PI/2, 0.0, 0.0) ), 0.5 );
	
	// sarge
	sarge = new MeshNode();
	sarge->init( "meshes/sphere/sphere16.mesh" );
	//sarge->setLocalTransformation( Vec3( 0, -2.8, 1.0 ), Mat3( Euler(-M::PI/2, 0.0, 0.0) ), 1.1 );
	sarge->setLocalTransformation( Vec3( 0, 2.0, 2.0 ), Mat3::getIdentity(), 0.4 );
	
	// floor
	floor__ = new MeshNode();
	floor__->init( "maps/temple/Cube.019.mesh" );
	floor__->setLocalTransformation( Vec3(0.0, -0.19, 0.0), Mat3( Euler(-M::PI/2, 0.0, 0.0) ), 0.8 );

	// imp	
	imp = new SkelModelNode();
	imp->init( "models/imp/imp.smdl" );
	imp->setLocalTransformation( Vec3( 0.0, 2.11, 0.0 ), Mat3( Euler(-M::PI/2, 0.0, 0.0) ), 0.7 );
	imp->meshNodes[0]->meshSkelCtrl->skelNode->skelAnimCtrl->skelAnim = Rsrc::skelAnims.load( "models/imp/walk.imp.anim" );
	imp->meshNodes[0]->meshSkelCtrl->skelNode->skelAnimCtrl->step = 0.8;

	// particle emitter
	partEmitter = new ParticleEmitter;
	partEmitter->init( NULL );
	partEmitter->translationLspace = Vec3( 3.0, 0.0, 0.0 );

	// crate
	/*crate = new MeshNode;
	crate->init( "models/crate0/crate0.mesh" );
	crate->scaleLspace = 1.0;*/


	//
	//floor_ = new floor_t;
	//floor_->material = Rsrc::materials.load( "materials/default.mtl" );

	const char* skybox_fnames [] = { "textures/env/hellsky4_forward.tga", "textures/env/hellsky4_back.tga", "textures/env/hellsky4_left.tga",
																	 "textures/env/hellsky4_right.tga", "textures/env/hellsky4_up.tga", "textures/env/hellsky4_down.tga" };
	app->getScene()->skybox.load( skybox_fnames );


	initPhysics();

	PRINT( "Engine initialization ends (" << App::getTicks()-ticks << ")" );
}



//=====================================================================================================================================
// main                                                                                                                               =
//=====================================================================================================================================
int main( int /*argc*/, char* /*argv*/[] )
{
	App::printAppInfo();

	init();

	PRINT( "Entering main loop" );
	int ticks = App::getTicks();
	do
	{
		int ticks_ = App::getTicks();
		I::handleEvents();
		R::prepareNextFrame();

		float dist = 0.2;
		float ang = toRad(3.0);
		float scale = 0.01;

		// move the camera
		static Node* mover = app->getActiveCam();

		if( I::keys[ SDLK_1 ] ) mover = app->getActiveCam();
		if( I::keys[ SDLK_2 ] ) mover = point_lights[0];
		if( I::keys[ SDLK_3 ] ) mover = spot_lights[0];
		if( I::keys[ SDLK_4 ] ) mover = point_lights[1];
		if( I::keys[ SDLK_5 ] ) mover = spot_lights[1];
		if( I::keys[ SDLK_6 ] ) mover = partEmitter;
		if( I::keys[ SDLK_m ] == 1 ) I::warpMouse = !I::warpMouse;

		if( I::keys[SDLK_a] ) mover->moveLocalX( -dist );
		if( I::keys[SDLK_d] ) mover->moveLocalX( dist );
		if( I::keys[SDLK_LSHIFT] ) mover->moveLocalY( dist );
		if( I::keys[SDLK_SPACE] ) mover->moveLocalY( -dist );
		if( I::keys[SDLK_w] ) mover->moveLocalZ( -dist );
		if( I::keys[SDLK_s] ) mover->moveLocalZ( dist );
		if( !I::warpMouse )
		{
			if( I::keys[SDLK_UP] ) mover->rotateLocalX( ang );
			if( I::keys[SDLK_DOWN] ) mover->rotateLocalX( -ang );
			if( I::keys[SDLK_LEFT] ) mover->rotateLocalY( ang );
			if( I::keys[SDLK_RIGHT] ) mover->rotateLocalY( -ang );
		}
		else
		{
			float accel = 44.0;
			mover->rotateLocalX( ang * I::mouseVelocity.y * accel );
			mover->rotateLocalY( -ang * I::mouseVelocity.x * accel );
		}
		if( I::keys[SDLK_q] ) mover->rotateLocalZ( ang );
		if( I::keys[SDLK_e] ) mover->rotateLocalZ( -ang );
		if( I::keys[SDLK_PAGEUP] ) mover->scaleLspace += scale ;
		if( I::keys[SDLK_PAGEDOWN] ) mover->scaleLspace -= scale ;

		if( I::keys[SDLK_k] ) app->getActiveCam()->lookAtPoint( point_lights[0]->translationWspace );


		if( I::keys[SDLK_o] == 1 )
		{
			btRigidBody* body = static_cast<btRigidBody*>( boxes[0] );
			//body->getMotionState()->setWorldTransform( toBt( Mat4( Vec3(0.0, 10.0, 0.0), Mat3::getIdentity(), 1.0 ) ) );
			body->setWorldTransform( toBt( Mat4( Vec3(0.0, 10.0, 0.0), Mat3::getIdentity(), 1.0 ) ) );
			//body->clearForces();
			body->forceActivationState( ACTIVE_TAG );
		}

		mover->rotationLspace.reorthogonalize();

		//static_cast<btRigidBody*>(dynamicsWorld->getCollisionObjectArray()[1])->getMotionState()->setWorldTransform( toBt(point_lights[0]->transformationWspace) );
		//dynamicsWorld->getCollisionObjectArray()[3]->setWorldTransform( toBt(point_lights[0]->transformationWspace) );

		app->getScene()->updateAllControllers();
		app->getScene()->updateAllWorldStuff();

		//partEmitter->update();

		app->getScene()->getPhyWorld()->getDynamicsWorld()->stepSimulation( app->timerTick );
		app->getScene()->getPhyWorld()->getDynamicsWorld()->debugDrawWorld();

		R::render( *app->getActiveCam() );

		//map.octree.root->bounding_box.render();

		// print some debug stuff
		Ui::setColor( Vec4(1.0, 1.0, 1.0, 1.0) );
		Ui::setPos( -0.98, 0.95 );
		Ui::setFontWidth( 0.03 );
		Ui::printf( "frame:%d fps:%dms\n", R::framesNum, (App::getTicks()-ticks_) );
		//Ui::print( "Movement keys: arrows,w,a,s,d,q,e,shift,space\nSelect objects: keys 1 to 5\n" );
		Ui::printf( "Mover: Pos(%.2f %.2f %.2f) Angs(%.2f %.2f %.2f)", mover->translationWspace.x, mover->translationWspace.y, mover->translationWspace.z,
								 toDegrees(Euler(mover->rotationWspace).x), toDegrees(Euler(mover->rotationWspace).y), toDegrees(Euler(mover->rotationWspace).z) );

		if( I::keys[SDLK_ESCAPE] ) break;
		if( I::keys[SDLK_F11] ) app->togleFullScreen();
		if( I::keys[SDLK_F12] == 1 ) R::takeScreenshot("gfx/screenshot.jpg");


		/*char str[128];
		sprintf( str, "capt/%06d.jpg", R::framesNum );
		R::takeScreenshot(str);*/

		// std stuff follow
		SDL_GL_SwapBuffers();
		R::printLastError();
		if( 1 )
		{
			//if( R::framesNum == 10 ) R::takeScreenshot("gfx/screenshot.tga");
			app->waitForNextFrame();
		}
		else
			if( R::framesNum == 5000 ) break;
	}while( true );
	PRINT( "Exiting main loop (" << App::getTicks()-ticks << ")" );


	PRINT( "Exiting..." );
	app->quitApp( EXIT_SUCCESS );
	return 0;
}