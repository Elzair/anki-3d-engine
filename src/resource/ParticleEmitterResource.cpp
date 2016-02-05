// Copyright (C) 2009-2016, Panagiotis Christopoulos Charitos.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <anki/resource/ParticleEmitterResource.h>
#include <anki/resource/ResourceManager.h>
#include <anki/resource/Model.h>
#include <anki/util/StringList.h>
#include <anki/misc/Xml.h>
#include <anki/renderer/Ms.h>
#include <anki/renderer/Is.h>
#include <cstring>

namespace anki
{

//==============================================================================
// Misc                                                                        =
//==============================================================================

//==============================================================================
static ANKI_USE_RESULT Error xmlVec3(
	const XmlElement& el_, const CString& str, Vec3& out)
{
	Error err = ErrorCode::NONE;
	XmlElement el;

	err = el_.getChildElementOptional(str, el);
	if(err || !el)
	{
		return err;
	}

	err = el.getVec3(out);
	return err;
}

//==============================================================================
static ANKI_USE_RESULT Error xmlF32(
	const XmlElement& el_, const CString& str, F32& out)
{
	Error err = ErrorCode::NONE;
	XmlElement el;

	err = el_.getChildElementOptional(str, el);
	if(err || !el)
	{
		return err;
	}

	F64 tmp;
	err = el.getF64(tmp);
	if(!err)
	{
		out = tmp;
	}

	return err;
}

//==============================================================================
static ANKI_USE_RESULT Error xmlU32(
	const XmlElement& el_, const CString& str, U32& out)
{
	Error err = ErrorCode::NONE;
	XmlElement el;

	err = el_.getChildElementOptional(str, el);
	if(err || !el)
	{
		return err;
	}

	I64 tmp;
	err = el.getI64(tmp);
	if(!err)
	{
		out = static_cast<U32>(tmp);
	}

	return err;
}

//==============================================================================
// ParticleEmitterProperties                                                   =
//==============================================================================

//==============================================================================
ParticleEmitterProperties& ParticleEmitterProperties::operator=(
	const ParticleEmitterProperties& b)
{
	std::memcpy(this, &b, sizeof(ParticleEmitterProperties));
	return *this;
}

//==============================================================================
void ParticleEmitterProperties::updateFlags()
{
	m_forceEnabled = !isZero(m_particle.m_forceDirection.getLengthSquared());
	m_forceEnabled = m_forceEnabled
		|| !isZero(m_particle.m_forceDirectionDeviation.getLengthSquared());
	m_forceEnabled =
		m_forceEnabled && (m_particle.m_forceMagnitude != 0.0
							  || m_particle.m_forceMagnitudeDeviation != 0.0);

	m_wordGravityEnabled = isZero(m_particle.m_gravity.getLengthSquared());
}

//==============================================================================
// ParticleEmitterResource                                                     =
//==============================================================================

//==============================================================================
ParticleEmitterResource::ParticleEmitterResource(ResourceManager* manager)
	: ResourceObject(manager)
{
}

//==============================================================================
ParticleEmitterResource::~ParticleEmitterResource()
{
}

//==============================================================================
Error ParticleEmitterResource::load(const ResourceFilename& filename)
{
	U32 tmp;

	XmlDocument doc;
	ANKI_CHECK(openFileParseXml(filename, doc));
	XmlElement rel; // Root element
	ANKI_CHECK(doc.getChildElement("particleEmitter", rel));

	// XML load
	//
	ANKI_CHECK(xmlF32(rel, "life", m_particle.m_life));
	ANKI_CHECK(xmlF32(rel, "lifeDeviation", m_particle.m_lifeDeviation));

	ANKI_CHECK(xmlF32(rel, "mass", m_particle.m_mass));
	ANKI_CHECK(xmlF32(rel, "massDeviation", m_particle.m_massDeviation));

	ANKI_CHECK(xmlF32(rel, "size", m_particle.m_size));
	ANKI_CHECK(xmlF32(rel, "sizeDeviation", m_particle.m_sizeDeviation));
	ANKI_CHECK(xmlF32(rel, "sizeAnimation", m_particle.m_sizeAnimation));

	ANKI_CHECK(xmlF32(rel, "alpha", m_particle.m_alpha));
	ANKI_CHECK(xmlF32(rel, "alphaDeviation", m_particle.m_alphaDeviation));

	tmp = m_particle.m_alphaAnimation;
	ANKI_CHECK(xmlU32(rel, "alphaAnimationEnabled", tmp));
	m_particle.m_alphaAnimation = tmp;

	ANKI_CHECK(xmlVec3(rel, "forceDirection", m_particle.m_forceDirection));
	ANKI_CHECK(xmlVec3(
		rel, "forceDirectionDeviation", m_particle.m_forceDirectionDeviation));
	ANKI_CHECK(xmlF32(rel, "forceMagnitude", m_particle.m_forceMagnitude));
	ANKI_CHECK(xmlF32(
		rel, "forceMagnitudeDeviation", m_particle.m_forceMagnitudeDeviation));

	ANKI_CHECK(xmlVec3(rel, "gravity", m_particle.m_gravity));
	ANKI_CHECK(xmlVec3(rel, "gravityDeviation", m_particle.m_gravityDeviation));

	ANKI_CHECK(xmlVec3(rel, "startingPosition", m_particle.m_startingPos));
	ANKI_CHECK(xmlVec3(
		rel, "startingPositionDeviation", m_particle.m_startingPosDeviation));

	ANKI_CHECK(xmlU32(rel, "maxNumberOfParticles", m_maxNumOfParticles));

	ANKI_CHECK(xmlF32(rel, "emissionPeriod", m_emissionPeriod));
	ANKI_CHECK(xmlU32(rel, "particlesPerEmittion", m_particlesPerEmittion));
	tmp = m_usePhysicsEngine;
	ANKI_CHECK(xmlU32(rel, "usePhysicsEngine", tmp));
	m_usePhysicsEngine = tmp;

	XmlElement el;
	CString cstr;
	ANKI_CHECK(rel.getChildElement("material", el));
	ANKI_CHECK(el.getText(cstr));
	ANKI_CHECK(getManager().loadResource(cstr, m_material));

	// sanity checks
	//

	static const char* ERROR = "Particle emmiter: "
							   "Incorrect or missing value %s";

	if(m_particle.m_life <= 0.0)
	{
		ANKI_LOGE(ERROR, "life");
		return ErrorCode::USER_DATA;
	}

	if(m_particle.m_life - m_particle.m_lifeDeviation <= 0.0)
	{
		ANKI_LOGE(ERROR, "lifeDeviation");
		return ErrorCode::USER_DATA;
	}

	if(m_particle.m_size <= 0.0)
	{
		ANKI_LOGE(ERROR, "size");
		return ErrorCode::USER_DATA;
	}

	if(m_maxNumOfParticles < 1)
	{
		ANKI_LOGE(ERROR, "maxNumOfParticles");
		return ErrorCode::USER_DATA;
	}

	if(m_emissionPeriod <= 0.0)
	{
		ANKI_LOGE(ERROR, "emissionPeriod");
		return ErrorCode::USER_DATA;
	}

	if(m_particlesPerEmittion < 1)
	{
		ANKI_LOGE(ERROR, "particlesPerEmission");
		return ErrorCode::USER_DATA;
	}

	// Calc some stuff
	//
	updateFlags();

	// Create ppline
	//
	PipelineInitInfo pinit;

	pinit.m_vertex.m_bindingCount = 1;
	pinit.m_vertex.m_bindings[0].m_stride = VERTEX_SIZE;
	pinit.m_vertex.m_attributeCount = 3;
	pinit.m_vertex.m_attributes[0].m_format =
		PixelFormat(ComponentFormat::R32G32B32, TransformFormat::FLOAT);
	pinit.m_vertex.m_attributes[0].m_offset = 0;
	pinit.m_vertex.m_attributes[0].m_binding = 0;
	pinit.m_vertex.m_attributes[1].m_format =
		PixelFormat(ComponentFormat::R32, TransformFormat::FLOAT);
	pinit.m_vertex.m_attributes[1].m_offset = sizeof(Vec3);
	pinit.m_vertex.m_attributes[1].m_binding = 0;
	pinit.m_vertex.m_attributes[2].m_format =
		PixelFormat(ComponentFormat::R32, TransformFormat::FLOAT);
	pinit.m_vertex.m_attributes[2].m_offset = sizeof(Vec3) + sizeof(F32);
	pinit.m_vertex.m_attributes[2].m_binding = 0;

	pinit.m_depthStencil.m_depthWriteEnabled = false;
	pinit.m_depthStencil.m_format = Ms::DEPTH_RT_PIXEL_FORMAT;

	pinit.m_color.m_attachmentCount = 1;
	pinit.m_color.m_attachments[0].m_format = Is::RT_PIXEL_FORMAT;
	pinit.m_color.m_attachments[0].m_srcBlendMethod = BlendMethod::SRC_ALPHA;
	pinit.m_color.m_attachments[0].m_dstBlendMethod =
		BlendMethod::ONE_MINUS_SRC_ALPHA;

	pinit.m_inputAssembler.m_topology = PrimitiveTopology::POINTS;

	m_lodCount = m_material->getLodCount();
	for(U i = 0; i < m_lodCount; ++i)
	{
		RenderingKey key(Pass::MS_FS, i, false, 1);
		const MaterialVariant& variant = m_material->getVariant(key);

		pinit.m_shaders[U(ShaderType::VERTEX)] =
			variant.getShader(ShaderType::VERTEX);

		pinit.m_shaders[U(ShaderType::FRAGMENT)] =
			variant.getShader(ShaderType::FRAGMENT);

		m_pplines[i] = getManager().getGrManager().newInstance<Pipeline>(pinit);
	}

	return ErrorCode::NONE;
}

} // end namespace anki
