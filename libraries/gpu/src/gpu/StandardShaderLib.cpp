//
//  StandardShaderLib.cpp
//  libraries/gpu/src/gpu
//
//  Collection of standard shaders that can be used all over the place
//
//  Created by Sam Gateau on 6/22/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "StandardShaderLib.h"

#include "DrawUnitQuadTexcoord_vert.h"
#include "DrawTransformUnitQuad_vert.h"
#include "DrawTexcoordRectTransformUnitQuad_vert.h"
#include "DrawViewportQuadTransformTexcoord_vert.h"
#include "DrawVertexPosition_vert.h"
#include "DrawTransformVertexPosition_vert.h"

const char DrawNada_frag[] = "void main(void) {}"; // DrawNada is really simple...

#include "DrawWhite_frag.h"
#include "DrawColor_frag.h"
#include "DrawTexture_frag.h"
#include "DrawTextureMirroredX_frag.h"
#include "DrawTextureOpaque_frag.h"
#include "DrawColoredTexture_frag.h"

using namespace gpu;

ShaderPointer StandardShaderLib::_drawUnitQuadTexcoordVS;
ShaderPointer StandardShaderLib::_drawTransformUnitQuadVS;
ShaderPointer StandardShaderLib::_drawTexcoordRectTransformUnitQuadVS;
ShaderPointer StandardShaderLib::_drawViewportQuadTransformTexcoordVS;
ShaderPointer StandardShaderLib::_drawVertexPositionVS;
ShaderPointer StandardShaderLib::_drawTransformVertexPositionVS;
ShaderPointer StandardShaderLib::_drawNadaPS;
ShaderPointer StandardShaderLib::_drawWhitePS;
ShaderPointer StandardShaderLib::_drawColorPS;
ShaderPointer StandardShaderLib::_drawTexturePS;
ShaderPointer StandardShaderLib::_drawTextureMirroredXPS;
ShaderPointer StandardShaderLib::_drawTextureOpaquePS;
ShaderPointer StandardShaderLib::_drawColoredTexturePS;
StandardShaderLib::ProgramMap StandardShaderLib::_programs;

ShaderPointer StandardShaderLib::getProgram(GetShader getVS, GetShader getPS) {

    auto programIt = _programs.find(std::pair<GetShader, GetShader>(getVS, getPS));
    if (programIt != _programs.end()) {
        return (*programIt).second;
    } else {
        auto vs = (getVS)();
        auto ps = (getPS)();
        auto program = gpu::Shader::createProgram(vs, ps);
        if (program) {
            // Program created, let's try to make it
            if (gpu::Shader::makeProgram((*program))) {
                // All good, backup and return that program
                _programs.insert(ProgramMap::value_type(std::pair<GetShader, GetShader>(getVS, getPS), program));
                return program;
            } else {
                // Failed to make the program probably because vs and ps cannot work together?
            }
        } else {
            // Failed to create the program maybe because ps and vs are not true vertex and pixel shaders?
        }
    }
    return ShaderPointer();
}


ShaderPointer StandardShaderLib::getDrawUnitQuadTexcoordVS() {
    if (!_drawUnitQuadTexcoordVS) {
        _drawUnitQuadTexcoordVS = gpu::Shader::createVertex(std::string(DrawUnitQuadTexcoord_vert));
    }
    return _drawUnitQuadTexcoordVS;
}

ShaderPointer StandardShaderLib::getDrawTransformUnitQuadVS() {
    if (!_drawTransformUnitQuadVS) {
        _drawTransformUnitQuadVS = gpu::Shader::createVertex(std::string(DrawTransformUnitQuad_vert));
    }
    return _drawTransformUnitQuadVS;
}

ShaderPointer StandardShaderLib::getDrawTexcoordRectTransformUnitQuadVS() {
    if (!_drawTexcoordRectTransformUnitQuadVS) {
        _drawTexcoordRectTransformUnitQuadVS = gpu::Shader::createVertex(std::string(DrawTexcoordRectTransformUnitQuad_vert));
    }
    return _drawTexcoordRectTransformUnitQuadVS;
}

ShaderPointer StandardShaderLib::getDrawViewportQuadTransformTexcoordVS() {
    if (!_drawViewportQuadTransformTexcoordVS) {
        _drawViewportQuadTransformTexcoordVS = gpu::Shader::createVertex(std::string(DrawViewportQuadTransformTexcoord_vert));
    }
    return _drawViewportQuadTransformTexcoordVS;
}

ShaderPointer StandardShaderLib::getDrawVertexPositionVS() {
    if (!_drawVertexPositionVS) {
        _drawVertexPositionVS = gpu::Shader::createVertex(std::string(DrawVertexPosition_vert));
    }
    return _drawVertexPositionVS;
}

ShaderPointer StandardShaderLib::getDrawTransformVertexPositionVS() {
    if (!_drawTransformVertexPositionVS) {
        _drawTransformVertexPositionVS = gpu::Shader::createVertex(std::string(DrawTransformVertexPosition_vert));
    }
    return _drawTransformVertexPositionVS;
}

ShaderPointer StandardShaderLib::getDrawNadaPS() {
    if (!_drawNadaPS) {
        _drawNadaPS = gpu::Shader::createPixel(std::string(DrawNada_frag));
    }
    return _drawNadaPS;
}

ShaderPointer StandardShaderLib::getDrawWhitePS() {
    if (!_drawWhitePS) {
        _drawWhitePS = gpu::Shader::createPixel(std::string(DrawWhite_frag));
    }
    return _drawWhitePS;
}

ShaderPointer StandardShaderLib::getDrawColorPS() {
    if (!_drawColorPS) {
        _drawColorPS = gpu::Shader::createPixel(std::string(DrawColor_frag));
    }
    return _drawColorPS;
}

ShaderPointer StandardShaderLib::getDrawTexturePS() {
    if (!_drawTexturePS) {
        _drawTexturePS = gpu::Shader::createPixel(std::string(DrawTexture_frag));
    }
    return _drawTexturePS;
}

ShaderPointer StandardShaderLib::getDrawTextureMirroredXPS() {
    if (!_drawTextureMirroredXPS) {
        _drawTextureMirroredXPS = gpu::Shader::createPixel(std::string(DrawTextureMirroredX_frag));
    }
    return _drawTextureMirroredXPS;
}

ShaderPointer StandardShaderLib::getDrawTextureOpaquePS() {
    if (!_drawTextureOpaquePS) {
        _drawTextureOpaquePS = gpu::Shader::createPixel(std::string(DrawTextureOpaque_frag));
    }
    return _drawTextureOpaquePS;
}

ShaderPointer StandardShaderLib::getDrawColoredTexturePS() {
    if (!_drawColoredTexturePS) {
        _drawColoredTexturePS = gpu::Shader::createPixel(std::string(DrawColoredTexture_frag));
    }
    return _drawColoredTexturePS;
}
