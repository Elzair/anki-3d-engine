#ifndef SKIN_NODE_SKIN_PATCH_NODE_H
#define SKIN_NODE_SKIN_PATCH_NODE_H

#include <boost/array.hpp>
#include "PatchNode.h"
#include "gl/Vbo.h"
#include "gl/Vao.h"


class SkinNode;


/// A fragment of the SkinNode
class SkinPatchNode: public PatchNode
{
	public:
		enum TransformFeedbackVbo
		{
			TFV_POSITIONS,
			TFV_NORMALS,
			TFV_TANGENTS,
			TFV_NUM
		};

		/// See TfHwSkinningGeneric.glsl for the locations
		enum TfShaderProgAttribLoc
		{
			POSITION_LOC,
			NORMAL_LOC,
			TANGENT_LOC,
			VERT_WEIGHT_BONES_NUM_LOC,
			VERT_WEIGHT_BONE_IDS_LOC,
			VERT_WEIGHT_WEIGHTS_LOC
		};

		SkinPatchNode(const ModelPatch& modelPatch, SkinNode* parent);

		/// @name Accessors
		/// @{
		GETTER_R(gl::Vao, tfVao, getTfVao)
		const gl::Vbo& getTfVbo(uint i) const {return tfVbos[i];}
		/// @}

	private:
		/// VBOs that contain the deformed vertex attributes
		boost::array<gl::Vbo, TFV_NUM> tfVbos;
		gl::Vao tfVao; ///< For TF passes
};


#endif
