#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <GL/glew.h>
#include <GL/gl.h>
#include <limits>
#include "common.h"
#include "Resource.h"


/// Image class. Used in Texture::load
class Image
{
	protected:
		static unsigned char tgaHeaderUncompressed[12];
		static unsigned char tgaHeaderCompressed[12];

		bool loadUncompressedTGA( const char* filename, fstream& fs );
		bool loadCompressedTGA( const char* filename, fstream& fs );
		bool loadPNG( const char* filename );
		bool loadTGA( const char* filename );

	public:
		uint  width;
		uint  height;
		uint  bpp;
		char* data;

		 Image(): data(NULL) {}
		~Image() { unload(); }

		bool load( const char* filename );
		void unload() { if( data ) delete [] data; data=NULL; }
};


/**
 * Texture resource class. It loads or creates an image and then loads it in the GPU. Its an OpenGL container. It supports compressed
 * and uncompressed TGAs and all formats of PNG (PNG loading comes through SDL)
 */
class Texture: public Resource
{
	protected:
		uint   glId; ///< Idendification for OGL. The only class variable
		GLenum type;

	public:
		 Texture(): glId(numeric_limits<uint>::max()), type(GL_TEXTURE_2D) {}
		~Texture() {}

		inline uint getGlId() const { DEBUG_ERR(glId==numeric_limits<uint>::max()); return glId; }
		inline int  getWidth() const { bind(); int i; glGetTexLevelParameteriv( type, 0, GL_TEXTURE_WIDTH, &i ); return i; }
		inline int  getHeight() const { bind(); int i; glGetTexLevelParameteriv( type, 0, GL_TEXTURE_HEIGHT, &i ); return i; }

		bool load( const char* filename );
		void unload();
		bool createEmpty2D( float width, float height, int internal_format, int format, GLenum type_ = GL_FLOAT );
		bool createEmpty2DMSAA( float width, float height, int samples_num, int internal_format );

		void bind( uint unit=0 ) const;

		inline void texParameter( GLenum param_name, GLint value ) const { bind(); glTexParameteri( GL_TEXTURE_2D, param_name, value ); }
};



#endif