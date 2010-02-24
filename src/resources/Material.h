#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "common.h"
#include "gmath.h"
#include "Resource.h"

/// Mesh material resource
class Material: public Resource
{
	//===================================================================================================================================
	// User defined variables                                                                                                           =
	//===================================================================================================================================
	public:
		/// class for user defined material variables that will be passes in to the shader
		class UserDefinedVar
		{
			public:
				enum type_e
				{
					VT_TEXTURE,
					VT_FLOAT,
					VT_VEC2, // not used yet
					VT_VEC3,
					VT_VEC4
				};

				struct Value       // unfortunately we cannot use union because of vec3_t and vec4_t
				{
					Texture* texture;
					float      float_;
					vec2_t     vec2;
					vec3_t     vec3;
					vec4_t     vec4;
					Value(): texture(NULL) {}
				};

				type_e type;
				Value value;
				int uniLoc;
				string name;
		}; // end class UserDefinedVar

		Vec<UserDefinedVar> user_defined_vars;


	//===================================================================================================================================
	// data                                                                                                                             =
	//===================================================================================================================================
	public:
		ShaderProg* shaderProg; ///< The most imortant asspect of materials

		bool blends; ///< The entities with blending are being rendered in blending stage and those without in material stage
		bool refracts;
		int  blendingSfactor;
		int  blendingDfactor;
		bool depthTesting;
		bool wireframe;
		bool castsShadow; ///< Used in shadowmapping passes but not in EarlyZ
		Texture* grassMap; // ToDo remove this

		// vertex attributes
		struct
		{
			int position;
			int tanget;
			int normal;
			int texCoords;

			// for hw skinning
			int vertWeightBonesNum;
			int vertWeightBoneIds;
			int vertWeightWeights;
		} attribLocs;

		// uniforms
		struct
		{
			int skinningRotations;
			int skinningTranslations;
		} uniLocs;

		// for depth passing
		/*struct
		{
			ShaderProg* shaderProg; ///< Depth pass shader program
			Texture* alpha_testing_map;
			
			struct
			{
				int position;
				int texCoords;

				// for hw skinning
				int vertWeightBonesNum;
				int vertWeightBoneIds;
				int vertWeightWeights;
			} attribute_locs;
			
			struct
			{
				int alpha_testing_map;
			} uniLocs;
		} depth;*/

		Material* dp_mtl;

	//===================================================================================================================================
	// funcs                                                                                                                            =
	//===================================================================================================================================
	protected:
		void setToDefault();
		bool additionalInit(); ///< The func is for not polluting load with extra code
		
	public:
		Material() { setToDefault(); }
		void setup();
		bool load( const char* filename );
		void unload();

		bool hasHWSkinning() const { return attribLocs.vertWeightBonesNum != -1; }
		bool hasAlphaTesting() const { return dp_mtl!=NULL && dp_mtl->attribLocs.texCoords!=-1; }
};


#endif