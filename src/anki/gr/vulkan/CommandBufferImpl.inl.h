// Copyright (C) 2009-2016, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <anki/gr/vulkan/CommandBufferImpl.h>
#include <anki/gr/vulkan/GrManagerImpl.h>
#include <anki/gr/vulkan/TextureImpl.h>
#include <anki/gr/Pipeline.h>
#include <anki/gr/vulkan/PipelineImpl.h>
#include <anki/gr/Buffer.h>
#include <anki/gr/vulkan/BufferImpl.h>
#include <anki/gr/OcclusionQuery.h>
#include <anki/gr/vulkan/OcclusionQueryImpl.h>
#include <anki/core/Trace.h>

namespace anki
{

inline void CommandBufferImpl::setViewport(U16 minx, U16 miny, U16 maxx, U16 maxy)
{
	ANKI_ASSERT(minx < maxx && miny < maxy);
	commandCommon();

	if(m_viewport[0] != minx || m_viewport[1] != miny || m_viewport[2] != maxx || m_viewport[3] != maxy)
	{
		VkViewport s;
		s.x = minx;
		s.y = miny;
		s.width = maxx - minx;
		s.height = maxy - miny;
		s.minDepth = 0.0;
		s.maxDepth = 1.0;
		ANKI_CMD(vkCmdSetViewport(m_handle, 0, 1, &s), ANY_OTHER_COMMAND);

		VkRect2D scissor = {};
		scissor.extent.width = maxx - minx;
		scissor.extent.height = maxy - miny;
		scissor.offset.x = minx;
		scissor.offset.y = miny;
		ANKI_CMD(vkCmdSetScissor(m_handle, 0, 1, &scissor), ANY_OTHER_COMMAND);

		m_viewport[0] = minx;
		m_viewport[1] = miny;
		m_viewport[2] = maxx;
		m_viewport[3] = maxy;
	}
	else
	{
		// Skip
	}
}

inline void CommandBufferImpl::setPolygonOffset(F32 factor, F32 units)
{
	commandCommon();

	if(m_polygonOffsetFactor != factor || m_polygonOffsetUnits != units)
	{
		ANKI_CMD(vkCmdSetDepthBias(m_handle, units, 0.0, factor), ANY_OTHER_COMMAND);

		m_polygonOffsetFactor = factor;
		m_polygonOffsetUnits = units;
	}
	else
	{
		// Skip
	}
}

inline void CommandBufferImpl::setStencilCompareMask(FaceSelectionMask face, U32 mask)
{
	commandCommon();

	VkStencilFaceFlags flags = 0;

	if(!!(face & FaceSelectionMask::FRONT) && m_stencilCompareMasks[0] != mask)
	{
		m_stencilCompareMasks[0] = mask;
		flags = VK_STENCIL_FACE_FRONT_BIT;
	}

	if(!!(face & FaceSelectionMask::BACK) && m_stencilCompareMasks[1] != mask)
	{
		m_stencilCompareMasks[1] = mask;
		flags |= VK_STENCIL_FACE_BACK_BIT;
	}

	if(flags)
	{
		ANKI_CMD(vkCmdSetStencilCompareMask(m_handle, flags, mask), ANY_OTHER_COMMAND);
	}
}

inline void CommandBufferImpl::setStencilWriteMask(FaceSelectionMask face, U32 mask)
{
	commandCommon();

	VkStencilFaceFlags flags = 0;

	if(!!(face & FaceSelectionMask::FRONT) && m_stencilWriteMasks[0] != mask)
	{
		m_stencilWriteMasks[0] = mask;
		flags = VK_STENCIL_FACE_FRONT_BIT;
	}

	if(!!(face & FaceSelectionMask::BACK) && m_stencilWriteMasks[1] != mask)
	{
		m_stencilWriteMasks[1] = mask;
		flags |= VK_STENCIL_FACE_BACK_BIT;
	}

	if(flags)
	{
		ANKI_CMD(vkCmdSetStencilWriteMask(m_handle, flags, mask), ANY_OTHER_COMMAND);
	}
}

inline void CommandBufferImpl::setStencilReference(FaceSelectionMask face, U32 ref)
{
	commandCommon();

	VkStencilFaceFlags flags = 0;

	if(!!(face & FaceSelectionMask::FRONT) && m_stencilReferenceMasks[0] != ref)
	{
		m_stencilReferenceMasks[0] = ref;
		flags = VK_STENCIL_FACE_FRONT_BIT;
	}

	if(!!(face & FaceSelectionMask::BACK) && m_stencilReferenceMasks[1] != ref)
	{
		m_stencilWriteMasks[1] = ref;
		flags |= VK_STENCIL_FACE_BACK_BIT;
	}

	if(flags)
	{
		ANKI_CMD(vkCmdSetStencilReference(m_handle, flags, ref), ANY_OTHER_COMMAND);
	}
}

inline void CommandBufferImpl::setImageBarrier(VkPipelineStageFlags srcStage,
	VkAccessFlags srcAccess,
	VkImageLayout prevLayout,
	VkPipelineStageFlags dstStage,
	VkAccessFlags dstAccess,
	VkImageLayout newLayout,
	VkImage img,
	const VkImageSubresourceRange& range)
{
	ANKI_ASSERT(img);
	commandCommon();

	VkImageMemoryBarrier inf = {};
	inf.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	inf.srcAccessMask = srcAccess;
	inf.dstAccessMask = dstAccess;
	inf.oldLayout = prevLayout;
	inf.newLayout = newLayout;
	inf.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	inf.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	inf.image = img;
	inf.subresourceRange = range;

#if ANKI_BATCH_COMMANDS
	flushBatches(CommandBufferCommandType::SET_BARRIER);

	if(m_imgBarriers.getSize() <= m_imgBarrierCount)
	{
		m_imgBarriers.resize(m_alloc, max<U>(2, m_imgBarrierCount * 2));
	}

	m_imgBarriers[m_imgBarrierCount++] = inf;

	m_srcStageMask |= srcStage;
	m_dstStageMask |= dstStage;
#else
	ANKI_CMD(vkCmdPipelineBarrier(m_handle, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &inf), ANY_OTHER_COMMAND);
	ANKI_TRACE_INC_COUNTER(VK_PIPELINE_BARRIERS, 1);
#endif
}

inline void CommandBufferImpl::setImageBarrier(VkPipelineStageFlags srcStage,
	VkAccessFlags srcAccess,
	VkImageLayout prevLayout,
	VkPipelineStageFlags dstStage,
	VkAccessFlags dstAccess,
	VkImageLayout newLayout,
	TexturePtr tex,
	const VkImageSubresourceRange& range)
{
	setImageBarrier(srcStage, srcAccess, prevLayout, dstStage, dstAccess, newLayout, tex->m_impl->m_imageHandle, range);

	m_texList.pushBack(m_alloc, tex);
}

inline void CommandBufferImpl::setTextureBarrierInternal(
	TexturePtr tex, TextureUsageBit prevUsage, TextureUsageBit nextUsage, const VkImageSubresourceRange& range)
{
	const TextureImpl& impl = *tex->m_impl;
	ANKI_ASSERT(impl.usageValid(prevUsage));
	ANKI_ASSERT(impl.usageValid(nextUsage));

	VkPipelineStageFlags srcStage;
	VkAccessFlags srcAccess;
	VkImageLayout oldLayout;
	VkPipelineStageFlags dstStage;
	VkAccessFlags dstAccess;
	VkImageLayout newLayout;
	impl.computeBarrierInfo(prevUsage, nextUsage, range.baseMipLevel, srcStage, srcAccess, dstStage, dstAccess);
	oldLayout = impl.computeLayout(prevUsage, range.baseMipLevel);
	newLayout = impl.computeLayout(nextUsage, range.baseMipLevel);

	setImageBarrier(srcStage, srcAccess, oldLayout, dstStage, dstAccess, newLayout, tex, range);
}

inline void CommandBufferImpl::setTextureSurfaceBarrier(
	TexturePtr tex, TextureUsageBit prevUsage, TextureUsageBit nextUsage, const TextureSurfaceInfo& surf)
{
	if(surf.m_level > 0)
	{
		ANKI_ASSERT(!(nextUsage & TextureUsageBit::GENERATE_MIPMAPS)
			&& "This transition happens inside CommandBufferImpl::generateMipmapsX");
	}

	const TextureImpl& impl = *tex->m_impl;
	impl.checkSurface(surf);

	VkImageSubresourceRange range;
	impl.computeSubResourceRange(surf, impl.m_akAspect, range);
	setTextureBarrierInternal(tex, prevUsage, nextUsage, range);
}

inline void CommandBufferImpl::setTextureVolumeBarrier(
	TexturePtr tex, TextureUsageBit prevUsage, TextureUsageBit nextUsage, const TextureVolumeInfo& vol)
{
	if(vol.m_level > 0)
	{
		ANKI_ASSERT(!(nextUsage & TextureUsageBit::GENERATE_MIPMAPS)
			&& "This transition happens inside CommandBufferImpl::generateMipmaps");
	}

	const TextureImpl& impl = *tex->m_impl;
	impl.checkVolume(vol);

	VkImageSubresourceRange range;
	impl.computeSubResourceRange(vol, impl.m_akAspect, range);
	setTextureBarrierInternal(tex, prevUsage, nextUsage, range);
}

inline void CommandBufferImpl::setBufferBarrier(VkPipelineStageFlags srcStage,
	VkAccessFlags srcAccess,
	VkPipelineStageFlags dstStage,
	VkAccessFlags dstAccess,
	PtrSize offset,
	PtrSize size,
	VkBuffer buff)
{
	ANKI_ASSERT(buff);
	commandCommon();

	VkBufferMemoryBarrier b = {};
	b.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	b.srcAccessMask = srcAccess;
	b.dstAccessMask = dstAccess;
	b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	b.buffer = buff;
	b.offset = offset;
	b.size = size;

#if ANKI_BATCH_COMMANDS
	flushBatches(CommandBufferCommandType::SET_BARRIER);

	if(m_buffBarriers.getSize() <= m_buffBarrierCount)
	{
		m_buffBarriers.resize(m_alloc, max<U>(2, m_buffBarrierCount * 2));
	}

	m_buffBarriers[m_buffBarrierCount++] = b;

	m_srcStageMask |= srcStage;
	m_dstStageMask |= dstStage;
#else
	ANKI_CMD(vkCmdPipelineBarrier(m_handle, srcStage, dstStage, 0, 0, nullptr, 1, &b, 0, nullptr), ANY_OTHER_COMMAND);
	ANKI_TRACE_INC_COUNTER(VK_PIPELINE_BARRIERS, 1);
#endif
}

inline void CommandBufferImpl::setBufferBarrier(
	BufferPtr buff, BufferUsageBit before, BufferUsageBit after, PtrSize offset, PtrSize size)
{
	const BufferImpl& impl = *buff->m_impl;

	VkPipelineStageFlags srcStage;
	VkAccessFlags srcAccess;
	VkPipelineStageFlags dstStage;
	VkAccessFlags dstAccess;
	impl.computeBarrierInfo(before, after, srcStage, srcAccess, dstStage, dstAccess);

	setBufferBarrier(srcStage, srcAccess, dstStage, dstAccess, offset, size, impl.getHandle());

	m_bufferList.pushBack(m_alloc, buff);
}

inline void CommandBufferImpl::drawArrays(U32 count, U32 instanceCount, U32 first, U32 baseInstance)
{
	drawcallCommon();
	ANKI_CMD(vkCmdDraw(m_handle, count, instanceCount, first, baseInstance), ANY_OTHER_COMMAND);
}

inline void CommandBufferImpl::drawElements(
	U32 count, U32 instanceCount, U32 firstIndex, U32 baseVertex, U32 baseInstance)
{
	drawcallCommon();
	ANKI_CMD(vkCmdDrawIndexed(m_handle, count, instanceCount, firstIndex, baseVertex, baseInstance), ANY_OTHER_COMMAND);
}

inline void CommandBufferImpl::drawArraysIndirect(U32 drawCount, PtrSize offset, BufferPtr buff)
{
	drawcallCommon();
	const BufferImpl& impl = *buff->m_impl;
	ANKI_ASSERT(impl.usageValid(BufferUsageBit::INDIRECT));
	ANKI_ASSERT((offset % 4) == 0);
	ANKI_ASSERT((offset + sizeof(DrawArraysIndirectInfo) * drawCount) <= impl.getSize());

	ANKI_CMD(vkCmdDrawIndirect(m_handle, impl.getHandle(), offset, drawCount, sizeof(DrawArraysIndirectInfo)),
		ANY_OTHER_COMMAND);
}

inline void CommandBufferImpl::drawElementsIndirect(U32 drawCount, PtrSize offset, BufferPtr buff)
{
	drawcallCommon();
	const BufferImpl& impl = *buff->m_impl;
	ANKI_ASSERT(impl.usageValid(BufferUsageBit::INDIRECT));
	ANKI_ASSERT((offset % 4) == 0);
	ANKI_ASSERT((offset + sizeof(DrawElementsIndirectInfo) * drawCount) <= impl.getSize());

	ANKI_CMD(vkCmdDrawIndexedIndirect(m_handle, impl.getHandle(), offset, drawCount, sizeof(DrawElementsIndirectInfo)),
		ANY_OTHER_COMMAND);
}

inline void CommandBufferImpl::dispatchCompute(U32 groupCountX, U32 groupCountY, U32 groupCountZ)
{
	commandCommon();
	flushDsetBindings();
	ANKI_CMD(vkCmdDispatch(m_handle, groupCountX, groupCountY, groupCountZ), ANY_OTHER_COMMAND);
}

inline void CommandBufferImpl::resetOcclusionQuery(OcclusionQueryPtr query)
{
	commandCommon();

	VkQueryPool handle = query->m_impl->m_handle.m_pool;
	U32 idx = query->m_impl->m_handle.m_queryIndex;
	ANKI_ASSERT(handle);

#if ANKI_BATCH_COMMANDS
	flushBatches(CommandBufferCommandType::RESET_OCCLUSION_QUERY);

	if(m_queryResetAtoms.getSize() <= m_queryResetAtomCount)
	{
		m_queryResetAtoms.resize(m_alloc, max<U>(2, m_queryResetAtomCount * 2));
	}

	QueryResetAtom atom;
	atom.m_pool = handle;
	atom.m_queryIdx = idx;

	m_queryResetAtoms[m_queryResetAtomCount++] = atom;
#else
	ANKI_CMD(vkCmdResetQueryPool(m_handle, handle, idx, 1), ANY_OTHER_COMMAND);
#endif

	m_queryList.pushBack(m_alloc, query);
}

inline void CommandBufferImpl::beginOcclusionQuery(OcclusionQueryPtr query)
{
	commandCommon();

	VkQueryPool handle = query->m_impl->m_handle.m_pool;
	U32 idx = query->m_impl->m_handle.m_queryIndex;
	ANKI_ASSERT(handle);

	ANKI_CMD(vkCmdBeginQuery(m_handle, handle, idx, 0), ANY_OTHER_COMMAND);

	m_queryList.pushBack(m_alloc, query);
}

inline void CommandBufferImpl::endOcclusionQuery(OcclusionQueryPtr query)
{
	commandCommon();

	VkQueryPool handle = query->m_impl->m_handle.m_pool;
	U32 idx = query->m_impl->m_handle.m_queryIndex;
	ANKI_ASSERT(handle);

	ANKI_CMD(vkCmdEndQuery(m_handle, handle, idx), ANY_OTHER_COMMAND);

	m_queryList.pushBack(m_alloc, query);
}

inline void CommandBufferImpl::clearTextureInternal(
	TexturePtr tex, const ClearValue& clearValue, const VkImageSubresourceRange& range)
{
	commandCommon();

	VkClearColorValue vclear;
	static_assert(sizeof(vclear) == sizeof(clearValue), "See file");
	memcpy(&vclear, &clearValue, sizeof(clearValue));

	const TextureImpl& impl = *tex->m_impl;
	if(impl.m_aspect == VK_IMAGE_ASPECT_COLOR_BIT)
	{
		ANKI_CMD(vkCmdClearColorImage(
					 m_handle, impl.m_imageHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &vclear, 1, &range),
			ANY_OTHER_COMMAND);
	}
	else
	{
		ANKI_ASSERT(0 && "TODO");
	}

	m_texList.pushBack(m_alloc, tex);
}

inline void CommandBufferImpl::clearTextureSurface(
	TexturePtr tex, const TextureSurfaceInfo& surf, const ClearValue& clearValue, DepthStencilAspectMask aspect)
{
	const TextureImpl& impl = *tex->m_impl;
	ANKI_ASSERT(impl.m_type != TextureType::_3D && "Not for 3D");

	VkImageSubresourceRange range;
	impl.computeSubResourceRange(surf, aspect, range);
	clearTextureInternal(tex, clearValue, range);
}

inline void CommandBufferImpl::clearTextureVolume(
	TexturePtr tex, const TextureVolumeInfo& vol, const ClearValue& clearValue, DepthStencilAspectMask aspect)
{
	const TextureImpl& impl = *tex->m_impl;
	ANKI_ASSERT(impl.m_type == TextureType::_3D && "Only for 3D");

	VkImageSubresourceRange range;
	impl.computeSubResourceRange(vol, aspect, range);
	clearTextureInternal(tex, clearValue, range);
}

inline void CommandBufferImpl::uploadBuffer(BufferPtr buff, PtrSize offset, const TransientMemoryToken& token)
{
	commandCommon();
	BufferImpl& impl = *buff->m_impl;

	VkBufferCopy region;
	region.srcOffset = token.m_offset;
	region.dstOffset = offset;
	region.size = token.m_range;

	ANKI_ASSERT(offset + token.m_range <= impl.getSize());

	ANKI_CMD(vkCmdCopyBuffer(m_handle,
				 getGrManagerImpl().getTransientMemoryManager().getBufferHandle(token.m_usage),
				 impl.getHandle(),
				 1,
				 &region),
		ANY_OTHER_COMMAND);

	m_bufferList.pushBack(m_alloc, buff);
}

inline void CommandBufferImpl::pushSecondLevelCommandBuffer(CommandBufferPtr cmdb)
{
	commandCommon();
	ANKI_ASSERT(insideRenderPass());
	ANKI_ASSERT(m_subpassContents == VK_SUBPASS_CONTENTS_MAX_ENUM
		|| m_subpassContents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	ANKI_ASSERT(cmdb->m_impl->m_finalized);

	m_subpassContents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;

	if(ANKI_UNLIKELY(m_rpCommandCount == 0))
	{
		beginRenderPassInternal();
	}

	ANKI_CMD(vkCmdExecuteCommands(m_handle, 1, &cmdb->m_impl->m_handle), ANY_OTHER_COMMAND);

	++m_rpCommandCount;
	m_cmdbList.pushBack(m_alloc, cmdb);
}

inline void CommandBufferImpl::drawcallCommon()
{
	// Preconditions
	commandCommon();
	ANKI_ASSERT(insideRenderPass() || secondLevel());
	ANKI_ASSERT(m_subpassContents == VK_SUBPASS_CONTENTS_MAX_ENUM || m_subpassContents == VK_SUBPASS_CONTENTS_INLINE);
	m_subpassContents = VK_SUBPASS_CONTENTS_INLINE;

	if(ANKI_UNLIKELY(m_rpCommandCount == 0) && !secondLevel())
	{
		beginRenderPassInternal();
	}

	flushDsetBindings();

	++m_rpCommandCount;

	ANKI_TRACE_INC_COUNTER(GR_DRAWCALLS, 1);
}

inline void CommandBufferImpl::commandCommon()
{
	ANKI_ASSERT(Thread::getCurrentThreadId() == m_tid
		&& "Commands must be recorder and flushed by the thread this command buffer was created");

	ANKI_ASSERT(!m_finalized);
	ANKI_ASSERT(m_handle);
	m_empty = false;
}

inline void CommandBufferImpl::flushBatches(CommandBufferCommandType type)
{
	if(type != m_lastCmdType)
	{
		switch(m_lastCmdType)
		{
		case CommandBufferCommandType::SET_BARRIER:
			flushBarriers();
			break;
		case CommandBufferCommandType::RESET_OCCLUSION_QUERY:
			flushQueryResets();
			break;
		case CommandBufferCommandType::WRITE_QUERY_RESULT:
			flushWriteQueryResults();
			break;
		case CommandBufferCommandType::ANY_OTHER_COMMAND:
			break;
		default:
			ANKI_ASSERT(0);
		}

		m_lastCmdType = type;
	}
}

inline void CommandBufferImpl::fillBuffer(BufferPtr buff, PtrSize offset, PtrSize size, U32 value)
{
	commandCommon();
	ANKI_ASSERT(!insideRenderPass());
	const BufferImpl& impl = *buff->m_impl;
	ANKI_ASSERT(impl.usageValid(BufferUsageBit::FILL));

	ANKI_ASSERT(offset < impl.getSize());
	ANKI_ASSERT((offset % 4) == 0 && "Should be multiple of 4");

	size = (size == MAX_PTR_SIZE) ? (impl.getSize() - offset) : size;
	ANKI_ASSERT(offset + size <= impl.getSize());
	ANKI_ASSERT((size % 4) == 0 && "Should be multiple of 4");

	ANKI_CMD(vkCmdFillBuffer(m_handle, impl.getHandle(), offset, size, value), ANY_OTHER_COMMAND);

	m_bufferList.pushBack(m_alloc, buff);
}

inline void CommandBufferImpl::writeOcclusionQueryResultToBuffer(
	OcclusionQueryPtr query, PtrSize offset, BufferPtr buff)
{
	commandCommon();
	ANKI_ASSERT(!insideRenderPass());

	const BufferImpl& impl = *buff->m_impl;
	ANKI_ASSERT(impl.usageValid(BufferUsageBit::QUERY_RESULT));
	ANKI_ASSERT((offset % 4) == 0);
	ANKI_ASSERT((offset + sizeof(U32)) <= impl.getSize());

	const OcclusionQueryImpl& q = *query->m_impl;

#if ANKI_BATCH_COMMANDS
	flushBatches(CommandBufferCommandType::WRITE_QUERY_RESULT);

	if(m_writeQueryAtoms.getSize() <= m_writeQueryAtomCount)
	{
		m_writeQueryAtoms.resize(m_alloc, max<U>(2, m_writeQueryAtomCount * 2));
	}

	WriteQueryAtom atom;
	atom.m_pool = q.m_handle.m_pool;
	atom.m_queryIdx = q.m_handle.m_queryIndex;
	atom.m_buffer = impl.getHandle();
	atom.m_offset = offset;

	m_writeQueryAtoms[m_writeQueryAtomCount++] = atom;
#else
	ANKI_CMD(vkCmdCopyQueryPoolResults(m_handle,
				 q.m_handle.m_pool,
				 q.m_handle.m_queryIndex,
				 1,
				 impl.getHandle(),
				 offset,
				 sizeof(U32),
				 VK_QUERY_RESULT_PARTIAL_BIT),
		ANY_OTHER_COMMAND);
#endif

	m_queryList.pushBack(m_alloc, query);
	m_bufferList.pushBack(m_alloc, buff);
}

inline void CommandBufferImpl::flushDsetBindings()
{
	for(U i = 0; i < MAX_BOUND_RESOURCE_GROUPS; ++i)
	{
		if(m_deferredDsetBindingMask & (1 << i))
		{
			ANKI_ASSERT(m_crntPplineLayout);
			const DeferredDsetBinding& binding = m_deferredDsetBindings[i];

			ANKI_CMD(vkCmdBindDescriptorSets(m_handle,
						 binding.m_bindPoint,
						 m_crntPplineLayout,
						 i,
						 1,
						 &binding.m_dset,
						 binding.m_dynOffsetCount,
						 &binding.m_dynOffsets[0]),
				ANY_OTHER_COMMAND);
#if ANKI_DEBUG
			m_boundDsetsMask |= (1 << i);
#endif
		}
	}

	m_deferredDsetBindingMask = 0;

#if ANKI_DEBUG
	ANKI_ASSERT((m_boundDsetsMask & m_pplineDsetMask) == m_pplineDsetMask);

	for(U i = 0; i < MAX_BOUND_RESOURCE_GROUPS; ++i)
	{
		if(m_pplineDsetMask & (1 << i))
		{
			ANKI_ASSERT(m_pplineDsetLayoutInfos[i] == m_deferredDsetBindings[i].m_dsetLayoutInfo);
		}
	}
#endif
}

inline void CommandBufferImpl::bindPipeline(PipelinePtr ppline)
{
	commandCommon();
	const PipelineImpl& impl = *ppline->m_impl;

	ANKI_CMD(vkCmdBindPipeline(m_handle, impl.m_bindPoint, impl.m_handle), ANY_OTHER_COMMAND);

	m_crntPplineLayout = impl.m_pipelineLayout;
#if ANKI_ASSERTIONS
	m_pplineDsetLayoutInfos = impl.m_descriptorSetLayoutInfos;
	m_pplineDsetMask = impl.m_descriptorSetMask;
#endif

	m_pplineList.pushBack(m_alloc, ppline);
}

} // end namespace anki
