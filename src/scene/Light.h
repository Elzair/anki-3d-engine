#ifndef LIGHT_H
#define LIGHT_H

#include "rsrc/Texture.h"
#include "SceneNode.h"
#include "rsrc/RsrcPtr.h"
#include "rsrc/LightRsrc.h"
#include "VisibilityInfo.h"


/// Light scene node. It can be spot or point
////
/// Explaining the lighting model:
/// @code
/// Final intensity:                If = Ia + Id + Is
/// Ambient intensity:              Ia = Al * Am
/// Ambient intensity of light:     Al
/// Ambient intensity of material:  Am
/// Diffuse intensity:              Id = Dl * Dm * LambertTerm
/// Diffuse intensity of light:     Dl
/// Diffuse intensity of material:  Dm
/// LambertTerm:                    max(Normal dot Light, 0.0)
/// Specular intensity:             Is = Sm * Sl * pow(max(R dot E, 0.0), f)
/// Specular intensity of light:    Sl
/// Specular intensity of material: Sm
/// @endcode
class Light: public SceneNode, public VisibilityInfo
{
	public:
		virtual ~Light();

		static bool classof(const SceneNode* x);

		/// @name Accessors
		/// @{
		const Vec3& getDiffuseColor() const {return diffuseCol;}
		Vec3& getDiffuseColor() {return diffuseCol;}
		void setDiffuseColor(const Vec3& x) {diffuseCol = x;}

		const Vec3& getSpecularColor() const {return specularCol;}
		Vec3& getSpecularColor() {return specularCol;}
		void setSpecularColor(const Vec3& x) {specularCol = x;}

		bool getCastShadow() const {return castsShadowFlag;}
		bool& getCastShadow() {return castsShadowFlag;}
		void setCastShadow(bool x) {castsShadowFlag = x;}
		/// @}

		void init(const char* filename);

	protected:
		RsrcPtr<LightRsrc> lightData;
		Vec3 diffuseCol; ///< Diffuse color
		Vec3 specularCol; ///< Specular color
		bool castsShadowFlag; ///< Casts shadow

		Light(ClassId cid, Scene& scene, ulong flags, SceneNode* parent);
};


inline Light::Light(ClassId cid, Scene& scene, ulong flags, SceneNode* parent)
:	SceneNode(cid, scene, flags, parent)
{}


inline bool Light::classof(const SceneNode* x)
{
	return x->getClassId() == CID_LIGHT ||
		x->getClassId() == CID_POINT_LIGHT ||
		x->getClassId() == CID_SPOT_LIGHT;
}


#endif