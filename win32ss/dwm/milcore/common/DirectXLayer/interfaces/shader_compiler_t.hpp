// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include <cstdint>
#include <windef.h>

#include <memory>
#include <string>

namespace dxlayer
{
    // ~= D3DXMACRO from <d3dxshader.h>
    struct macro
    {
    public:
        const char* definition;
        const char* name;
    };


    // Encapsulates a void* buffer along with the data buffer size
    struct data
    {
        void* buffer;
        int64_t  buffer_size;

        inline data(void* buffer, int64_t buffer_size)
            : buffer(buffer), buffer_size(buffer_size) {}
    };

    // ~= ID3DXBuffer from <d3dx9mesh.h>
    class buffer
    {
    public:
        virtual data get_buffer_data() = 0;
    };


    // Describes basic shader API's that are consumed within 
    // wpfgfx_v0400.dll
    template<dxapi apiset>
    class shader_t
    {
    public:
        // Compile a shader file
        static HRESULT compile(
            const char* src_data,
            const char* entry_point_name,
            const char* shader_profile_target,
            unsigned long flags1, unsigned long flags2,
            std::shared_ptr<buffer>& shader,
            std::shared_ptr<buffer>& err_msgs);

        // Changes an error HRESULT to the more descriptive 
        // WGXERR_SHADER_COMPILE_FAILED if appropriate, and 
        // outputs the compiler errors 
        static HRESULT handle_errors_and_transform_hresult(HRESULT hResult, const std::shared_ptr<buffer>& err_msgs);

        // Returns the name of the highest high-level shader language(HLSL) pixel-shader 
        // profile supported by a given device.
        template<typename ID3DDevice>
        static const char* get_pixel_shader_profile_name(ID3DDevice* pD3DDevice);

        // Returns the name of the highest high-level shader language(HLSL) vertex-shader 
        // profile supported by a given device.
        template<typename ID3DDevice>
        static const char* get_vertex_shader_profile_name(ID3DDevice* pD3DDevice);
    };

}
