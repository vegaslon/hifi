//
//  ResampleTask.cpp
//  render/src/render
//
//  Various to upsample or downsample textures into framebuffers.
//
//  Created by Olivier Prat on 10/09/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ResampleTask.h"

#include "gpu/Context.h"
#include "gpu/StandardShaderLib.h"

using namespace render;

gpu::PipelinePointer HalfDownsample::_pipeline;

HalfDownsample::HalfDownsample() {

}

void HalfDownsample::configure(const Config& config) {

}

gpu::FramebufferPointer HalfDownsample::getResampledFrameBuffer(const gpu::FramebufferPointer& sourceFramebuffer) {
    auto resampledFramebufferSize = sourceFramebuffer->getSize();

    resampledFramebufferSize.x /= 2U;
    resampledFramebufferSize.y /= 2U;

    if (!_destinationFrameBuffer || resampledFramebufferSize != _destinationFrameBuffer->getSize()) {
        _destinationFrameBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("HalfOutput"));

        auto sampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT);
        auto target = gpu::Texture::createRenderBuffer(sourceFramebuffer->getRenderBuffer(0)->getTexelFormat(), resampledFramebufferSize.x, resampledFramebufferSize.y, gpu::Texture::SINGLE_MIP, sampler);
        _destinationFrameBuffer->setRenderBuffer(0, target);
    }
    return _destinationFrameBuffer;
}

void HalfDownsample::run(const RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceFramebuffer, gpu::FramebufferPointer& resampledFrameBuffer) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    resampledFrameBuffer = getResampledFrameBuffer(sourceFramebuffer);

    if (!_pipeline) {
        auto vs = gpu::StandardShaderLib::getDrawTransformUnitQuadVS();
        auto ps = gpu::StandardShaderLib::getDrawTextureOpaquePS();
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(gpu::State::DepthTest(false, false));
        _pipeline = gpu::Pipeline::create(program, state);
    }

    const auto bufferSize = resampledFrameBuffer->getSize();
    glm::ivec4 viewport{ 0, 0, bufferSize.x, bufferSize.y };

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);

        batch.setFramebuffer(resampledFrameBuffer);

        batch.setViewportTransform(viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.resetViewTransform();
        batch.setPipeline(_pipeline);

        batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(bufferSize, viewport));
        batch.setResourceTexture(0, sourceFramebuffer->getRenderBuffer(0));
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
}
