#pragma once

#include <avs/core.hpp>
#include <avs/runtime/framebuffers.h>

namespace avs::runtime {

// Converts the runtime-managed framebuffer set into the lightweight views
// consumed by core effects. The returned structure keeps raw pointers into the
// runtime storage and must be refreshed via \c refreshFrameBuffers whenever the
// runtime swaps buffers (e.g. after beginFrame()).
FrameBuffers makeFrameBuffers(Framebuffers& buffers);

// Updates an existing \c FrameBuffers view to match the runtime buffers.
void refreshFrameBuffers(Framebuffers& buffers, FrameBuffers& views);

// Helper that copies the contents of a runtime view into a core framebuffer
// view. Both views must reference compatible dimensions.
void copyFrame(const FrameView& src, const FrameBufferView& dst);

// Helper that copies the contents of a core framebuffer view into a runtime
// framebuffer view.
void copyFrame(const FrameBufferView& src, const FrameView& dst);

}  // namespace avs::runtime
