#include "skybox.h"
#include "Resource.h"
#include "Renderer.h"
#include "Math.h"
#include "Camera.h"
#include "Scene.h"
#include "App.h"


static float coords [][4][3] =
{
	// front
	{ { 1,  1, -1}, {-1,  1, -1}, {-1, -1, -1}, { 1, -1, -1} },
	// back
	{ {-1,  1,  1}, { 1,  1,  1}, { 1, -1,  1}, {-1, -1,  1} },
	// left
	{ {-1,  1, -1}, {-1,  1,  1}, {-1, -1,  1}, {-1, -1, -1} },
	// right
	{ { 1,  1,  1}, { 1,  1, -1}, { 1, -1, -1}, { 1, -1,  1} },
	// up
	{ { 1,  1,  1}, {-1,  1,  1}, {-1,  1, -1}, { 1,  1, -1} },
	//
	{ { 1, -1, -1}, {-1, -1, -1}, {-1, -1,  1}, { 1, -1,  1} }
};



/*
=======================================================================================================================================
load                                                                                                                                  =
=======================================================================================================================================
*/
bool Skybox::load( const char* filenames[6] )
{
	for( int i=0; i<6; i++ )
	{
		textures[i] = Rsrc::textures.load( filenames[i] );
	}

	noise = Rsrc::textures.load( "gfx/noise2.tga" );
	noise->texParameter( GL_TEXTURE_WRAP_S, GL_REPEAT );
	noise->texParameter( GL_TEXTURE_WRAP_T, GL_REPEAT );

	shader = Rsrc::shaders.load( "shaders/ms_mp_skybox.glsl" );

	return true;
}


/*
=======================================================================================================================================
render                                                                                                                                =
=======================================================================================================================================
*/
void Skybox::Render( const Mat3& rotation )
{
	//glDisable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );

	glPushMatrix();

	shader->bind();
	glUniform1i( shader->getUniVar("colormap").getLoc(), 0 );
	shader->locTexUnit( shader->getUniVar("noisemap").getLoc(), *noise, 1 );
	glUniform1f( shader->getUniVar("timer").getLoc(), (rotation_ang/(2*PI))*100 );
	glUniform3fv( shader->getUniVar("sceneAmbientCol").getLoc(), 1, &(Vec3( 1.0, 1.0, 1.0 ) / app->getScene()->getAmbientCol())[0] );

	// set the rotation matrix
	Mat3 tmp( rotation );
	tmp.rotateYAxis(rotation_ang);
	R::loadMatrix( Mat4( tmp ) );
	rotation_ang += 0.0001;
	if( rotation_ang >= 2*PI ) rotation_ang = 0.0;



	const float ffac = 0.001; // fault factor. To eliminate the artefacts in the edge of the quads caused by texture filtering
	float uvs [][2] = { {1.0-ffac,1.0-ffac}, {0.0+ffac,1.0-ffac}, {0.0+ffac,0.0+ffac}, {1.0-ffac,0.0+ffac} };

	for( int i=0; i<6; i++ )
	{
		textures[i]->bind(0);
		glBegin( GL_QUADS );
			glTexCoord2fv( uvs[0] );
			glVertex3fv( coords[i][0] );
			glTexCoord2fv( uvs[1] );
			glVertex3fv( coords[i][1] );
			glTexCoord2fv( uvs[2] );
			glVertex3fv( coords[i][2] );
			glTexCoord2fv( uvs[3] );
			glVertex3fv( coords[i][3] );
		glEnd();
	}

	glPopMatrix();
}