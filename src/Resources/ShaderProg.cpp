#include "ShaderProg.h"
#include "Renderer.h"
#include "ShaderPrePreprocessor.h"
#include "Texture.h"


#define SHADER_ERROR( x ) ERROR( "Shader (" << getRsrcName() << "): " << x )
#define SHADER_WARNING( x ) WARNING( "Shader (" << getRsrcName() << "): " << x )


//=====================================================================================================================================
// createAndCompileShader                                                                                                             =
//=====================================================================================================================================
uint ShaderProg::createAndCompileShader( const char* sourceCode, const char* preproc, int type ) const
{
	uint glId = 0;
	const char* source_strs[2] = {NULL, NULL};

	// create the shader
	glId = glCreateShader( type );

	// attach the source
	source_strs[1] = sourceCode;
	source_strs[0] = preproc;

	// compile
	glShaderSource( glId, 2, source_strs, NULL );
	glCompileShader( glId );

	int success;
	glGetShaderiv( glId, GL_COMPILE_STATUS, &success );

	if( !success )
	{
		// print info log
		int info_len = 0;
		int chars_written = 0;
		char* info_log = NULL;

		glGetShaderiv( glId, GL_INFO_LOG_LENGTH, &info_len );
		info_log = (char*)malloc( (info_len+1)*sizeof(char) );
		glGetShaderInfoLog( glId, info_len, &chars_written, info_log );
		
		const char* shader_type;
		switch( type )
		{
			case GL_VERTEX_SHADER:
				shader_type = "Vertex shader";
				break;
			case GL_FRAGMENT_SHADER:
				shader_type = "Fragment shader";
				break;
			default:
				DEBUG_ERR( 1 ); // Not supported
		}
		SHADER_ERROR( shader_type << " compiler log follows:\n" << info_log );
		
		free( info_log );
		return 0;
	}

	return glId;
}


//=====================================================================================================================================
// link                                                                                                                               =
//=====================================================================================================================================
bool ShaderProg::link()
{
	// link
	glLinkProgram( glId );

	// check if linked correctly
	int success;
	glGetProgramiv( glId, GL_LINK_STATUS, &success );

	if( !success )
	{
		int info_len = 0;
		int chars_written = 0;
		char* info_log_txt = NULL;

		glGetProgramiv( glId, GL_INFO_LOG_LENGTH, &info_len );

		info_log_txt = (char*)malloc( (info_len+1)*sizeof(char) );
		glGetProgramInfoLog( glId, info_len, &chars_written, info_log_txt );
		SHADER_ERROR( "Link log follows:\n" << info_log_txt );
		free( info_log_txt );
		return false;
	}

	return true;
}


//=====================================================================================================================================
// getUniAndAttribVars                                                                                                                =
//=====================================================================================================================================
void ShaderProg::getUniAndAttribVars()
{
	int num;
	char name_[256];
	GLsizei length;
	GLint size;
	GLenum type;


	// attrib locations
	glGetProgramiv( glId, GL_ACTIVE_ATTRIBUTES, &num );
	attribVars.reserve( num );
	for( int i=0; i<num; i++ ) // loop all attributes
	{
		glGetActiveAttrib( glId, i, sizeof(name_)/sizeof(char), &length, &size, &type, name_ );
		name_[ length ] = '\0';

		// check if its FFP location
		int loc = glGetAttribLocation(glId, name_);
		if( loc == -1 ) // if -1 it means that its an FFP var
		{
			//SHADER_WARNING( "You are using FFP vertex attributes (\"" << name_ << "\")" );
			continue;
		}

		attribVars.push_back( Var( loc, name_, type, Var::SVT_ATTRIBUTE ) );
		attribNameToVar[ name_ ] = &attribVars.back();
	}


	// uni locations
	glGetProgramiv( glId, GL_ACTIVE_UNIFORMS, &num );
	uniVars.reserve( num );
	for( int i=0; i<num; i++ ) // loop all uniforms
	{
		glGetActiveUniform( glId, i, sizeof(name_)/sizeof(char), &length, &size, &type, name_ );
		name_[ length ] = '\0';

		// check if its FFP location
		int loc = glGetUniformLocation(glId, name_);
		if( loc == -1 ) // if -1 it means that its an FFP var
		{
			//SHADER_WARNING( "You are using FFP uniforms (\"" << name_ << "\")" );
			continue;
		}

		uniVars.push_back( Var( loc, name_, type, Var::SVT_UNIFORM ) );
		uniNameToVar[ name_ ] = &uniVars.back();
	}
}


//=====================================================================================================================================
// bindCustomAttribLocs                                                                                                               =
//=====================================================================================================================================
bool ShaderProg::bindCustomAttribLocs( const ShaderPrePreprocessor& pars ) const
{
	for( uint i=0; i<pars.getOutput().getAttribLocs().size(); ++i )
	{
		const string& name = pars.getOutput().getAttribLocs()[i].name;
		int loc = pars.getOutput().getAttribLocs()[i].customLoc;
		glBindAttribLocation( glId, loc, name.c_str() );

		// check for error
		GLenum errId = glGetError();
		if( errId != GL_NO_ERROR )
		{
			SHADER_ERROR( "Something went wrong for attrib \"" << name << "\" and location " << loc << " (" << gluErrorString( errId ) << ")" );
			return false;
		}
	}
	return true;
}


//=====================================================================================================================================
// load                                                                                                                               =
//=====================================================================================================================================
bool ShaderProg::load( const char* filename )
{
	if( !customLoad( filename, "" ) ) return false;
	return true;
}


//=====================================================================================================================================
// customLoad                                                                                                                         =
//=====================================================================================================================================
bool ShaderProg::customLoad( const char* filename, const char* extraSource )
{
	if( getRsrcName().length() == 0 )
	{
		name = Util::cutPath( filename );
		path = Util::getPath( filename );
	}

	ShaderPrePreprocessor pars;

	if( !pars.parseFile( filename ) ) return false;

	// 1) create and compile the shaders
	string preprocSource = R::getStdShaderPreprocDefines() + extraSource;
	uint vertGlId = createAndCompileShader( pars.getOutput().getVertShaderSource().c_str(), preprocSource.c_str(), GL_VERTEX_SHADER );
	if( vertGlId == 0 ) return false;

	uint fragGlId = createAndCompileShader( pars.getOutput().getFragShaderSource().c_str(), preprocSource.c_str(), GL_FRAGMENT_SHADER );
	if( fragGlId == 0 ) return false;

	// 2) create program and attach shaders
	glId = glCreateProgram();
	glAttachShader( glId, vertGlId );
	glAttachShader( glId, fragGlId );

	// 3) bind the custom attrib locs
	if( !bindCustomAttribLocs( pars ) ) return false;

	// 5) link
	if( !link() ) return false;
	

	// init the rest
	getUniAndAttribVars();

	return true;
}


//=====================================================================================================================================
// getUniVar                                                                                                                          =
//=====================================================================================================================================
const ShaderProg::Var& ShaderProg::getUniVar( const char* name ) const
{
	NameToVarIterator it = uniNameToVar.find( name );
	if( it == uniNameToVar.end() )
	{
		SHADER_ERROR( "Cannot get uniform loc \"" << name << '\"' );
		return dummyVar;
	}
	return *(it->second);
}


//=====================================================================================================================================
// getAttribVar                                                                                                                       =
//=====================================================================================================================================
const ShaderProg::Var& ShaderProg::getAttribVar( const char* name ) const
{
	NameToVarIterator it = attribNameToVar.find( name );
	if( it == attribNameToVar.end() )
	{
		SHADER_ERROR( "Cannot get attribute loc \"" << name << '\"' );
		return dummyVar;
	}
	return *(it->second);
}


//=====================================================================================================================================
// uniVarExists                                                                                                                       =
//=====================================================================================================================================
bool ShaderProg::uniVarExists( const char* name ) const
{
	NameToVarIterator it = uniNameToVar.find( name );
	return it != uniNameToVar.end();
}


//=====================================================================================================================================
// attribVarExists                                                                                                                    =
//=====================================================================================================================================
bool ShaderProg::attribVarExists( const char* name ) const
{
	NameToVarIterator it = attribNameToVar.find( name );
	return it != attribNameToVar.end();
}


//=====================================================================================================================================
// locTexUnit                                                                                                                         =
//=====================================================================================================================================
void ShaderProg::locTexUnit( int loc, const Texture& tex, uint tex_unit ) const
{
	DEBUG_ERR( loc == -1 );
	DEBUG_ERR( getCurrentProgramGlId() != glId );
	tex.bind( tex_unit );
	glUniform1i( loc, tex_unit );
}

void ShaderProg::locTexUnit( const char* loc, const Texture& tex, uint tex_unit ) const
{
	DEBUG_ERR( getCurrentProgramGlId() != glId );
	tex.bind( tex_unit );
	glUniform1i( getUniVar(loc).getLoc(), tex_unit );
}