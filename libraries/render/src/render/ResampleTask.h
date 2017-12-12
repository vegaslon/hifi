//
//  ResampleTask.h
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

#ifndef hifi_render_ResampleTask_h
#define hifi_render_ResampleTask_h

#include "Engine.h"

namespace render {

    class HalfDownsample {
    public:
        using Config = JobConfig;
        using JobModel = Job::ModelIO<HalfDownsample, gpu::FramebufferPointer, gpu::FramebufferPointer, Config>;

        HalfDownsample();

        void configure(const Config& config);
        void run(const RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceFramebuffer, gpu::FramebufferPointer& resampledFrameBuffer);

    protected:

        static gpu::PipelinePointer _pipeline;

        gpu::FramebufferPointer _destinationFrameBuffer;

        gpu::FramebufferPointer getResampledFrameBuffer(const gpu::FramebufferPointer& sourceFramebuffer);
    };
}

#endif // hifi_render_ResampleTask_h
