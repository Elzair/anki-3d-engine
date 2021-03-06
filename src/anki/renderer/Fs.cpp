// Copyright (C) 2009-2016, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <anki/renderer/Fs.h>
#include <anki/renderer/Renderer.h>
#include <anki/renderer/Ms.h>
#include <anki/renderer/Is.h>
#include <anki/renderer/Sm.h>
#include <anki/renderer/Volumetric.h>
#include <anki/renderer/DepthDownscale.h>
#include <anki/scene/SceneGraph.h>
#include <anki/scene/FrustumComponent.h>

namespace anki
{

Fs::~Fs()
{
	if(m_pplineCache)
	{
		getAllocator().deleteInstance(m_pplineCache);
	}
}

Error Fs::init(const ConfigSet&)
{
	m_width = m_r->getWidth() / FS_FRACTION;
	m_height = m_r->getHeight() / FS_FRACTION;

	// Create RT
	m_r->createRenderTarget(m_width,
		m_height,
		IS_COLOR_ATTACHMENT_PIXEL_FORMAT,
		TextureUsageBit::SAMPLED_FRAGMENT | TextureUsageBit::FRAMEBUFFER_ATTACHMENT_READ_WRITE,
		SamplingFilter::LINEAR,
		1,
		m_rt);

	FramebufferInitInfo fbInit;
	fbInit.m_colorAttachmentCount = 1;
	fbInit.m_colorAttachments[0].m_texture = m_rt;
	fbInit.m_colorAttachments[0].m_loadOperation = AttachmentLoadOperation::CLEAR;
	fbInit.m_colorAttachments[0].m_usageInsideRenderPass = TextureUsageBit::FRAMEBUFFER_ATTACHMENT_READ_WRITE;
	fbInit.m_depthStencilAttachment.m_texture = m_r->getDepthDownscale().m_hd.m_depthRt;
	fbInit.m_depthStencilAttachment.m_loadOperation = AttachmentLoadOperation::LOAD;
	fbInit.m_depthStencilAttachment.m_usageInsideRenderPass =
		TextureUsageBit::SAMPLED_FRAGMENT | TextureUsageBit::FRAMEBUFFER_ATTACHMENT_READ;
	fbInit.m_depthStencilAttachment.m_aspect = DepthStencilAspectMask::DEPTH;
	m_fb = getGrManager().newInstance<Framebuffer>(fbInit);

	// Init the global resources
	{
		ResourceGroupInitInfo init;
		init.m_textures[0].m_texture = m_r->getDepthDownscale().m_hd.m_depthRt;
		init.m_textures[0].m_usage = TextureUsageBit::SAMPLED_FRAGMENT | TextureUsageBit::FRAMEBUFFER_ATTACHMENT_READ;
		init.m_textures[1].m_texture = m_r->getSm().getSpotTextureArray();
		init.m_textures[2].m_texture = m_r->getSm().getOmniTextureArray();

		init.m_uniformBuffers[0].m_uploadedMemory = true;
		init.m_uniformBuffers[0].m_usage = BufferUsageBit::UNIFORM_FRAGMENT | BufferUsageBit::UNIFORM_VERTEX;
		init.m_uniformBuffers[1].m_uploadedMemory = true;
		init.m_uniformBuffers[1].m_usage = BufferUsageBit::UNIFORM_FRAGMENT | BufferUsageBit::UNIFORM_VERTEX;
		init.m_uniformBuffers[2].m_uploadedMemory = true;
		init.m_uniformBuffers[2].m_usage = BufferUsageBit::UNIFORM_FRAGMENT | BufferUsageBit::UNIFORM_VERTEX;

		init.m_storageBuffers[0].m_uploadedMemory = true;
		init.m_storageBuffers[0].m_usage = BufferUsageBit::STORAGE_FRAGMENT_READ | BufferUsageBit::STORAGE_VERTEX_READ;
		init.m_storageBuffers[1].m_uploadedMemory = true;
		init.m_storageBuffers[1].m_usage = BufferUsageBit::STORAGE_FRAGMENT_READ | BufferUsageBit::STORAGE_VERTEX_READ;

		m_globalResources = getGrManager().newInstance<ResourceGroup>(init);
	}

	// Init state
	{
		ColorStateInfo& color = m_state.m_color;
		color.m_attachmentCount = 1;
		color.m_attachments[0].m_format = IS_COLOR_ATTACHMENT_PIXEL_FORMAT;
		color.m_attachments[0].m_srcBlendMethod = BlendMethod::SRC_ALPHA;
		color.m_attachments[0].m_dstBlendMethod = BlendMethod::ONE_MINUS_SRC_ALPHA;

		m_state.m_depthStencil.m_format = MS_DEPTH_ATTACHMENT_PIXEL_FORMAT;
		m_state.m_depthStencil.m_depthWriteEnabled = false;
	}

	m_pplineCache = getAllocator().newInstance<GrObjectCache>(&getGrManager());

	ANKI_CHECK(initVol());

	return ErrorCode::NONE;
}

Error Fs::initVol()
{
	ANKI_CHECK(m_r->createShader("shaders/VolumetricUpscale.frag.glsl",
		m_vol.m_frag,
		"#define SRC_SIZE vec2(float(%u), float(%u))\n",
		m_r->getWidth() / VOLUMETRIC_FRACTION,
		m_r->getHeight() / VOLUMETRIC_FRACTION));

	ColorStateInfo color;
	color.m_attachmentCount = 1;
	color.m_attachments[0].m_format = IS_COLOR_ATTACHMENT_PIXEL_FORMAT;
	color.m_attachments[0].m_srcBlendMethod = BlendMethod::ONE;
	color.m_attachments[0].m_dstBlendMethod = BlendMethod::ONE;
	m_r->createDrawQuadPipeline(m_vol.m_frag->getGrShader(), color, m_vol.m_ppline);

	SamplerInitInfo sinit;
	sinit.m_repeat = false;
	sinit.m_mipmapFilter = SamplingFilter::NEAREST;

	ResourceGroupInitInfo rcinit;
	rcinit.m_textures[0].m_texture = m_r->getDepthDownscale().m_hd.m_depthRt;
	rcinit.m_textures[0].m_sampler = getGrManager().newInstance<Sampler>(sinit);
	rcinit.m_textures[1].m_texture = m_r->getDepthDownscale().m_qd.m_depthRt;
	rcinit.m_textures[1].m_sampler = rcinit.m_textures[0].m_sampler;
	rcinit.m_textures[2].m_texture = m_r->getVolumetric().m_rt;
	rcinit.m_uniformBuffers[0].m_uploadedMemory = true;
	rcinit.m_uniformBuffers[0].m_usage = BufferUsageBit::UNIFORM_FRAGMENT;
	m_vol.m_rc = getGrManager().newInstance<ResourceGroup>(rcinit);

	return ErrorCode::NONE;
}

void Fs::drawVolumetric(RenderingContext& ctx, CommandBufferPtr cmdb)
{
	const Frustum& fr = ctx.m_frustumComponent->getFrustum();

	cmdb->bindPipeline(m_vol.m_ppline);

	TransientMemoryInfo trans;
	Vec4* unis = static_cast<Vec4*>(getGrManager().allocateFrameTransientMemory(
		sizeof(Vec4), BufferUsageBit::UNIFORM_ALL, trans.m_uniformBuffers[0]));
	computeLinearizeDepthOptimal(fr.getNear(), fr.getFar(), unis->x(), unis->y());

	cmdb->bindResourceGroup(m_vol.m_rc, 0, &trans);

	m_r->drawQuad(cmdb);
}

Error Fs::buildCommandBuffers(RenderingContext& ctx, U threadId, U threadCount) const
{
	// Get some stuff
	VisibilityTestResults& vis = ctx.m_frustumComponent->getVisibilityTestResults();

	U problemSize = vis.getCount(VisibilityGroupType::RENDERABLES_FS);
	PtrSize start, end;
	ThreadPoolTask::choseStartEnd(threadId, threadCount, problemSize, start, end);

	if(start == end)
	{
		// Early exit
		return ErrorCode::NONE;
	}

	// Create the command buffer and set some state
	CommandBufferInitInfo cinf;
	cinf.m_flags = CommandBufferFlag::SECOND_LEVEL;
	cinf.m_framebuffer = m_fb;
	CommandBufferPtr cmdb = m_r->getGrManager().newInstance<CommandBuffer>(cinf);
	ctx.m_fs.m_commandBuffers[threadId] = cmdb;

	cmdb->setViewport(0, 0, m_width, m_height);
	cmdb->setPolygonOffset(0.0, 0.0);
	cmdb->bindResourceGroup(m_globalResources, 1, &ctx.m_is.m_dynBufferInfo);

	// Start drawing
	Error err = m_r->getSceneDrawer().drawRange(Pass::MS_FS,
		*ctx.m_frustumComponent,
		cmdb,
		*m_pplineCache,
		m_state,
		vis.getBegin(VisibilityGroupType::RENDERABLES_FS) + start,
		vis.getBegin(VisibilityGroupType::RENDERABLES_FS) + end);

	return err;
}

void Fs::setPreRunBarriers(RenderingContext& ctx)
{
	ctx.m_commandBuffer->setTextureSurfaceBarrier(m_rt,
		TextureUsageBit::NONE,
		TextureUsageBit::FRAMEBUFFER_ATTACHMENT_READ_WRITE,
		TextureSurfaceInfo(0, 0, 0, 0));
}

void Fs::setPostRunBarriers(RenderingContext& ctx)
{
	ctx.m_commandBuffer->setTextureSurfaceBarrier(m_rt,
		TextureUsageBit::FRAMEBUFFER_ATTACHMENT_READ_WRITE,
		TextureUsageBit::SAMPLED_FRAGMENT,
		TextureSurfaceInfo(0, 0, 0, 0));
}

void Fs::run(RenderingContext& ctx)
{
	CommandBufferPtr& cmdb = ctx.m_commandBuffer;
	cmdb->beginRenderPass(m_fb);
	cmdb->setViewport(0, 0, m_width, m_height);
	cmdb->setPolygonOffset(0.0, 0.0);

	for(U i = 0; i < m_r->getThreadPool().getThreadsCount(); ++i)
	{
		if(ctx.m_fs.m_commandBuffers[i].isCreated())
		{
			cmdb->pushSecondLevelCommandBuffer(ctx.m_fs.m_commandBuffers[i]);
		}
	}

	cmdb->endRenderPass();
}

} // end namespace anki
