#ifndef _RENDERER_HPP_
#define _RENDERER_HPP_

#include "Common.h"
#include "Math.h"
#include "Fbo.h"
#include "Texture.h"
#include "ShaderProg.h"

class Camera;
class PointLight;
class SpotLight;
class RendererInitializer;


/**
 * The renderer class
 *
 * It is a class and not a namespace because we may need external renderers for security cameras for example
 */
class Renderer
{
	//===================================================================================================================================
	// The rendering stages                                                                                                             =
	//===================================================================================================================================
	public:
		/**
		 * Rendering stage
		 */
		class RenderingStage
		{
			protected:
				Renderer& r;

			public:
				RenderingStage( Renderer& r_ ): r(r_) {}
		};


		/**
		 * Material stage
		 */
		class Ms: private RenderingStage
		{
			friend class Renderer;

			private:
				Fbo fbo;

				void init();
				void run();

			public:
				Texture normalFai;
				Texture diffuseFai;
				Texture specularFai;
				Texture depthFai;

				Ms( Renderer& r_ ): RenderingStage( r_ ) {}
		}; // end Ms

		/**
		 * Illumination stage
		 */
		class Is: private RenderingStage
		{
			friend class Renderer;

			public:
				/**
				 * Shadowmapping pass
				 */
				class Sm: private RenderingStage
				{
					friend class Is;
					friend class Renderer;

					PROPERTY_R( bool, enabled, isEnabled ) ///< @ref PROPERTY_R : If false thene there is no shadowmapping at all
					PROPERTY_R( bool, pcfEnabled, isPcfEnabled ) ///< @ref PROPERTY_R : Enable Percentage Closer Filtering
					PROPERTY_R( bool, bilinearEnabled, isBilinearEnabled ) ///< @ref PROPERTY_R : Enable bilinear filtering in shadowMap. Better quality
					PROPERTY_R( int, resolution, getResolution ) ///< Shadowmap resolution. The higher the more quality the shadows are

					private:
						Fbo fbo; ///< Illumination stage shadowmapping FBO
						Texture shadowMap;

						void init();

						/**
						 * Render the scene only with depth and store the result in the shadowMap
						 * @param cam The light camera
						 */
						void run( const Camera& cam );

					public:
						Sm( Renderer& r_ ): RenderingStage( r_ ) {}
				}; // end Sm

			private:
				Fbo fbo;
				uint stencilRb; ///< Illumination stage stencil buffer
				// shader stuff
				/// Illumination stage ambient pass shader program
				class AmbientShaderProg: public ShaderProg
				{
					public:
						struct
						{
							const ShaderProg::UniVar* ambientCol;
							const ShaderProg::UniVar* sceneColMap;
						} uniVars;
				};
				/// Illumination stage light pass shader program
				class LightShaderProg: public ShaderProg
				{
					public:
						struct
						{
							const ShaderProg::UniVar* msNormalFai;
							const ShaderProg::UniVar* msDiffuseFai;
							const ShaderProg::UniVar* msSpecularFai;
							const ShaderProg::UniVar* msDepthFai;
							const ShaderProg::UniVar* planes;
							const ShaderProg::UniVar* lightPos;
							const ShaderProg::UniVar* lightInvRadius;
							const ShaderProg::UniVar* lightDiffuseCol;
							const ShaderProg::UniVar* lightSpecularCol;
							const ShaderProg::UniVar* lightTex;
							const ShaderProg::UniVar* texProjectionMat;
							const ShaderProg::UniVar* shadowMap;
						} uniVars;
				};
				AmbientShaderProg ambientPassSProg; ///< Illumination stage ambient pass shader program
				LightShaderProg pointLightSProg; ///< Illumination stage point light shader program
				LightShaderProg spotLightNoShadowSProg; ///< Illumination stage spot light w/o shadow shader program
				LightShaderProg spotLightShadowSProg; ///< Illumination stage spot light w/ shadow shader program
				// other
				Vec3 viewVectors[4];
				Vec2 planes;
				static float sMOUvSCoords []; ///< Illumination stage stencil masking optimizations UV sphere vertex coords
				static uint  sMOUvSVboId; ///< Illumination stage stencil masking optimizations UV sphere VBO id

				static void initSMOUvS(); ///< Init the illumination stage stencil masking optimizations uv sphere (eg create the @ref sMOUvSVboId VBO)
				void renderSMOUvS( const PointLight& light ); ///< Render the illumination stage stencil masking optimizations uv sphere
				void calcViewVector(); ///< Calc the view vector that we will use inside the shader to calculate the frag pos in view space
				void calcPlanes(); ///< Calc the planes that we will use inside the shader to calculate the frag pos in view space
				void setStencilMask( const PointLight& light );
				void setStencilMask( const SpotLight& light );
				void ambientPass( const Vec3& color );
				void pointLightPass( const PointLight& light );
				void spotLightPass( const SpotLight& light );
				void initFbo();
				void init();
				void run();

			public:
				Texture fai;
				Sm sm;

				Is( Renderer& r_ ): RenderingStage( r_ ), sm(r) {}
		}; // end Is

		/**
		 * Post-processing stage
		 *
		 * This stage is divided into 2 two parts. The first happens before blending stage and the second after.
		 */
		class Pps: private RenderingStage
		{
			friend class Renderer;

			PROPERTY_R( bool, enabled, isEnabled )
			PROPERTY_R( float, renderingQuality, getRenderingQuality )

			public:
				/**
				 * High dynamic range lighting pass
				 */
				class Hdr: private RenderingStage
				{
					friend class Pps;
					friend class Renderer;

					PROPERTY_R( bool, enabled, isEnabled )
					PROPERTY_R( float, renderingQuality, getRenderingQuality )

					private:
						Fbo pass0Fbo, pass1Fbo, pass2Fbo;
						ShaderProg pass0SProg, pass1SProg, pass2SProg;
						struct
						{
							struct
							{
								int fai;
							} pass0SProg;
							struct
							{
								int fai;
							} pass1SProg;
							struct
							{
								int fai;
							} pass2SProg;
						} uniLocs;

						void initFbos( Fbo& fbo, Texture& fai, int internalFormat );
						void init();
						void run();

					public:
						Texture pass0Fai; ///< Vertical blur pass FAI
						Texture pass1Fai; ///< pass0Fai with the horizontal blur FAI
						Texture fai; ///< The final FAI

						Hdr( Renderer& r_ ): RenderingStage(r_) {}
				}; // end Hrd

				/**
				 * Screen space ambient occlusion pass
				 *
				 * Three passes:
				 * - Calc ssao factor
				 * - Blur vertically
				 * - Blur horizontally
				 */
				class Ssao: private RenderingStage
				{
					friend class Pps;
					friend class Renderer;

					PROPERTY_R( bool, enabled, isEnabled )
					PROPERTY_R( float, renderingQuality, getRenderingQuality )
					PROPERTY_R( float, bluringQuality, getBluringQuality )

					private:
						Fbo pass0Fbo, pass1Fbo, pass2Fbo;
						uint width, height, bwidth, bheight;
						Texture* noiseMap;
						struct
						{
							struct
							{
								int camerarange, msDepthFai, noiseMap, msNormalFai;
							} pass0SProg;
							struct
							{
								int fai;
							} pass1SProg;
							struct
							{
								int fai;
							} pass2SProg;
						} uniLocs;

						void initBlurFbo( Fbo& fbo, Texture& fai );
						void init();
						void run();

					public:
						Texture pass0Fai, pass1Fai, fai /** The final FAI */;
						ShaderProg ssaoSProg, blurSProg, blurSProg2;

						Ssao( Renderer& r_ ): RenderingStage(r_) {}
				}; // end Ssao

				private:
					class PpsShaderProg: public ShaderProg
					{
						public:
							struct
							{
								int isFai, ppsSsaoFai, msNormalFai, hdrFai, lscattFai;
							} uniLocs;
					};
					PpsShaderProg sProg;
					Fbo fbo;

					void init();
					void run();

				public:
					Texture fai;
					Hdr hdr;
					Ssao ssao;

					Pps( Renderer& r_ ): RenderingStage(r_), hdr(r_), ssao(r_) {}
		}; // end Pps

		/**
		 * Debugging stage
		 */
		class Dbg: public RenderingStage
		{
			friend class Renderer;

			PROPERTY_R( bool, enabled, isEnabled )
			PROPERTY_RW( bool, showAxisEnabled, setShowAxis, isShowAxisEnabled )
			PROPERTY_RW( bool, showLightsEnabled, setShowLights, isShowLightsEnabled )
			PROPERTY_RW( bool, showSkeletonsEnabled, setShowSkeletons, isShowSkeletonsEnabled )
			PROPERTY_RW( bool, showCamerasEnabled, setShowCameras, isShowCamerasEnabled )

			private:
				Fbo fbo;
				ShaderProg sProg; /// @todo move Dbg to GL 3

				void init();
				void run();

			public:
				Dbg( Renderer& r_ );
				static void renderGrid();
				static void renderSphere( float radius, int complexity );
				static void renderCube( bool cols, float size );
		}; // end Dbg


	//===================================================================================================================================
	//                                                                                                                                  =
	//===================================================================================================================================
	PROPERTY_R( uint, width, getWidth ) ///< Width of the rendering. Dont confuse with the window width
	PROPERTY_R( uint, height, getHeight ) ///< Height of the rendering. Dont confuse with the window width
	PROPERTY_R( uint, framesNum, getFramesNum )
	PROPERTY_R( float, aspectRatio, getAspectRatio )

	protected:
		// the rest
		Camera* cam; ///< Current camera
		static float quadVertCoords [][2];

		static void drawQuad( int vertCoordsUniLoc );

	public:
		// the stages as data members
		Ms ms; ///< Material rendering stage
		Is is; ///< Illumination rendering stage
		Pps pps; ///< Postprocessing rendering stage
		Dbg dbg; ///< Debugging rendering stage

		// texture stuff
		static bool textureCompression; ///< Used in Texture::load to enable texture compression. Decreases video memory usage
		static int  maxTextureUnits; ///< Used in Texture::bind so we wont bind in a nonexistent texture unit. Readonly
		static bool mipmapping; ///< Used in Texture::load. Enables mipmapping increases video memory usage
		static int  maxAnisotropy; ///< Max texture anisotropy. Used in Texture::load
		// matrices & viewing
		Mat4 modelViewMat; ///< This changes once for every mesh rendering
		Mat4 projectionMat; ///< This changes once every frame
		Mat4 modelViewProjectionMat; ///< This changes just like @ref modelViewMat
		Mat3 normalMat; ///< The rotation part of modelViewMat

		Renderer();

		/**
		 * Init the renderer given an initialization class
		 * @param initializer The initializer class
		 */
		void init( const RendererInitializer& initializer );

		/**
		 * This function does all the rendering stages and produces a final FAI
		 * @param cam The camera from where the rendering will be done
		 */
		void render( Camera& cam );

		/**
		 * My version of gluUnproject
		 * @param windowCoords Window screen coords
		 * @param modelViewMat The modelview matrix
		 * @param projectionMat The projection matrix
		 * @param view The view vector
		 * @return The unprojected coords coords
		 */
		static Vec3 unproject( const Vec3& windowCoords, const Mat4& modelViewMat, const Mat4& projectionMat, const int view[4] );

		/**
		 *
		 * @param left
		 * @param right
		 * @param bottom
		 * @param top
		 * @param near
		 * @param far
		 * @return
		 */
		static Mat4 ortho( float left, float right, float bottom, float top, float near, float far );

		/**
		 * Get last OpenGL error string
		 * @return An error string or NULL if not error
		 */
		static const uchar* getLastError();

		/**
		 * Print last OpenGL error or nothing if there is no error
		 */
		static void printLastError();

		static void setProjectionMatrix( const Camera& cam );
		static void setViewMatrix( const Camera& cam );
		static void noShaders() { ShaderProg::unbind(); } ///< unbind shaders @todo remove this. From now on the will be only shaders
		static void setProjectionViewMatrices( const Camera& cam ) { setProjectionMatrix(cam); setViewMatrix(cam); }
		static void setViewport( uint x, uint y, uint w, uint h ) { glViewport(x,y,w,h); }
		static void multMatrix( const Mat4& m4 ) { glMultMatrixf( &(m4.getTransposed())(0,0) ); } ///< OpenGL wrapper
		static void multMatrix( const Transform& trf ) { glMultMatrixf( &(Mat4(trf).getTransposed())(0,0) ); } ///< OpenGL wrapper
		static void loadMatrix( const Mat4& m4 ) { glLoadMatrixf( &(m4.getTransposed())(0,0) ); } ///< OpenGL wrapper
		static void loadMatrix( const Transform& trf ) { glLoadMatrixf( &(Mat4(trf).getTransposed())(0,0) ); } ///< OpenGL wrapper
};

#endif