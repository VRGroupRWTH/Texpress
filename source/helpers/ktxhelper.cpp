#pragma once
#include <texpress/helpers/ktxhelper.hpp>
#include <texpress/utility/stringtools.hpp>
#include <string>
#include <ktx.h>
#include <fp16.h>


namespace texpress {
    bool save_ktx(const uint8_t* data_ptr, const char* path, const glm::ivec4& dimensions, gl::GLenum gl_internal_format, uint64_t size, bool as_texture_array, bool save_monolithic) {
        ktxTexture1* texture;
        ktxTextureCreateInfo createInfo;
        KTX_error_code result;

        glm::ivec4 range;
        range.x = std::max(dimensions.x, 1);
        range.y = std::max(dimensions.y, 1);
        range.z = std::max(dimensions.z, 1);
        range.w = std::max(dimensions.w, 1);

        uint32_t channels = gl_channels(gl::GLenum(gl_internal_format));
        uint32_t type_size = gl_compressed(gl::GLenum(gl_internal_format)) ? 1 : size / (range.x * range.y * range.z * range.w * channels);

        // Automatically a monolithic file if time-dim = 1
        bool monolithic = save_monolithic || (range.w == 1);

        createInfo.glInternalformat = (ktx_uint32_t)gl_internal_format;
        createInfo.vkFormat =
            createInfo.baseWidth = range.x;
        createInfo.baseHeight = range.y;
        createInfo.baseDepth = (monolithic) ? range.z * range.w : range.z;
        createInfo.numDimensions = 3;
        createInfo.numLevels = 1;
        createInfo.numLayers = 1;
        createInfo.numFaces = 1;
        createInfo.isArray = KTX_FALSE;
        createInfo.generateMipmaps = KTX_FALSE;

        if (as_texture_array) {
            createInfo.isArray = KTX_TRUE;
            createInfo.baseDepth = 1;
            createInfo.numDimensions = 2;
            createInfo.numLayers = (monolithic) ? range.z * range.w : range.z;
        }

        result = ktxTexture1_Create(&createInfo,
            KTX_TEXTURE_CREATE_ALLOC_STORAGE,
            &texture);

        if (result)
            return false;

        ktxHashList_Construct(&texture->kvDataHead);
        result = ktxHashList_AddKVPair(&texture->kvDataHead, "Dimensions", sizeof(dimensions), &dimensions);

        if (result)
            return false;

        // Each iteration produces a seperate file
        uint64_t iterations = (monolithic) ? 1 : range.w;

        if (as_texture_array) {
            uint64_t slice_elements = size / (range.w * range.z * type_size);

            for (uint64_t i = 0; i < iterations; i++) {
                for (uint32_t z = 0; z < createInfo.numLayers; z++) {
                    ktx_size_t srcSize = slice_elements * type_size;;
                    uint8_t* src = (uint8_t*)data_ptr + i * createInfo.baseDepth * srcSize + z * srcSize;

                    result = ktxTexture_SetImageFromMemory(ktxTexture(texture), 0, z, 0, src, srcSize);

                    if (result)
                        return false;
                }

                std::string filepath = (monolithic) ? str_canonical(path, -1, -1) : str_canonical(path, -1, i);
                ktxTexture_WriteToNamedFile(ktxTexture(texture), filepath.c_str());
            }
        }
        else {
            uint64_t volume_elements = size / (range.w * type_size);
            if (monolithic) {
                volume_elements *= range.w;
            }

            for (uint64_t i = 0; i < iterations; i++) {
                ktx_size_t srcSize = volume_elements * type_size;;
                texture->pData = (uint8_t*)data_ptr + i * createInfo.baseDepth * srcSize;

                std::string filepath = (monolithic) ? str_canonical(path, -1, -1) : str_canonical(path, -1, i);
                ktxTexture_WriteToNamedFile(ktxTexture(texture), filepath.c_str());
            }

            texture->pData = nullptr;
        }

        ktxTexture_Destroy(ktxTexture(texture));

        return true;
    }

    bool save_ktx2(const uint8_t* data_ptr, const char* path, const glm::ivec4& dimensions, gl::GLenum gl_internal_format, uint64_t size, bool as_texture_array, bool save_monolithic) {
        uint32_t channels = gl_channels(gl::GLenum(gl_internal_format));

        glm::ivec4 range;
        range.x = std::max(dimensions.x, 1);
        range.y = std::max(dimensions.y, 1);
        range.z = std::max(dimensions.z, 1);
        range.w = std::max(dimensions.w, 1);

        uint32_t type_size = gl_compressed(gl::GLenum(gl_internal_format)) ? 1 : size / (range.x * range.y * range.z * range.w * channels);

        return save_ktx2(data_ptr, path, dimensions, gl_internal_to_vk(gl_internal_format), channels, type_size, size, as_texture_array, save_monolithic);
    }


    bool save_ktx2(const uint8_t* data_ptr, const char* path, const glm::ivec4& dimensions, VkFormat vk_format, uint32_t channels, uint32_t type_size, uint64_t size, bool as_texture_array, bool save_monolithic) {
        ktxTexture2* texture;
        ktxTextureCreateInfo createInfo;
        KTX_error_code result;

        glm::ivec4 range;
        range.x = std::max(dimensions.x, 1);
        range.y = std::max(dimensions.y, 1);
        range.z = std::max(dimensions.z, 1);
        range.w = std::max(dimensions.w, 1);

        // Automatically a monolithic file if time-dim = 1
        bool monolithic = save_monolithic || (range.w == 1);

        createInfo.vkFormat = vk_format;
        createInfo.baseWidth = range.x;
        createInfo.baseHeight = range.y;
        createInfo.baseDepth = (monolithic) ? range.z * range.w : range.z;
        createInfo.numDimensions = 3;
        createInfo.numLevels = 1;
        createInfo.numLayers = 1;
        createInfo.numFaces = 1;
        createInfo.isArray = KTX_FALSE;
        createInfo.generateMipmaps = KTX_FALSE;

        if (as_texture_array) {
            createInfo.isArray = KTX_TRUE;
            createInfo.baseDepth = 1;
            createInfo.numDimensions = 2;
            createInfo.numLayers = (monolithic) ? range.z * range.w : range.z;
        }

        result = ktxTexture2_Create(&createInfo,
            KTX_TEXTURE_CREATE_ALLOC_STORAGE,
            &texture);

        if (result)
            return false;

        ktxHashList_Construct(&texture->kvDataHead);
        result = ktxHashList_AddKVPair(&texture->kvDataHead, "Dimensions", sizeof(dimensions), &dimensions);

        if (result)
            return false;

        // Each iteration produces a seperate file
        uint64_t iterations = (monolithic) ? 1 : range.w;

        if (as_texture_array) {
            uint64_t slice_elements = size / (range.w * range.z * type_size);

            for (uint64_t i = 0; i < iterations; i++) {
                for (uint32_t z = 0; z < createInfo.numLayers; z++) {
                    ktx_size_t srcSize = slice_elements * type_size;;
                    uint8_t* src = (uint8_t*)data_ptr + i * createInfo.baseDepth * srcSize + z * srcSize;

                    result = ktxTexture_SetImageFromMemory(ktxTexture(texture), 0, z, 0, src, srcSize);

                    if (result)
                        return false;
                }

                std::string filepath = (monolithic) ? str_canonical(path, -1, -1) : str_canonical(path, -1, i);
                ktxTexture_WriteToNamedFile(ktxTexture(texture), filepath.c_str());
            }
        }
        else {
            uint64_t volume_elements = size / (range.w * type_size);
            if (monolithic) {
                volume_elements *= range.w;
            }

            for (uint64_t i = 0; i < iterations; i++) {
                ktx_size_t srcSize = volume_elements * type_size;;
                texture->pData = (uint8_t*)data_ptr + i * createInfo.baseDepth * srcSize;

                std::string filepath = (monolithic) ? str_canonical(path, -1, -1) : str_canonical(path, -1, i);
                ktxTexture_WriteToNamedFile(ktxTexture(texture), filepath.c_str());
            }

            texture->pData = nullptr;
        }

        ktxTexture_Destroy(ktxTexture(texture));

        return true;
    }




    void load_ktx(const char* path, uint8_t* data_ptr, uint8_t& channels, glm::ivec4& dimensions, gl::GLenum& gl_internal, gl::GLenum& gl_format, gl::GLenum& gl_type) {
        ktxTexture* texture;
        KTX_error_code result;

        result = ktxTexture_CreateFromNamedFile(path,
            KTX_TEXTURE_CREATE_ALLOC_STORAGE,
            &ktxTexture(texture));

        if (texture->classId == class_id::ktxTexture1_c) {
            ktxTexture1* texture1;

            result = ktxTexture_CreateFromNamedFile(path,
                KTX_TEXTURE_CREATE_NO_FLAGS,
                &ktxTexture(texture1));

            gl_internal = gl::GLenum(texture1->glInternalformat);
            gl_format = gl::GLenum(texture1->glFormat);
            gl_type = gl::GLenum(texture1->glType);

            ktxTexture_Destroy(ktxTexture(texture1));
        }
        else if (texture->classId == class_id::ktxTexture2_c) {
            ktxTexture2* texture2;

            result = ktxTexture_CreateFromNamedFile(path,
                KTX_TEXTURE_CREATE_NO_FLAGS,
                &ktxTexture(texture2));

            gl_internal = vk_to_gl_internal(VkFormat(texture2->vkFormat));
            gl_format = texpress::gl_format(channels);
            gl_type = texpress::gl_type(gl_internal);

            ktxTexture_Destroy(ktxTexture(texture2));
        }
        else {
            gl_internal = gl::GLenum::GL_ZERO;
            gl_format = gl::GLenum::GL_ZERO;
            gl_type = gl::GLenum::GL_ZERO;
        }

        if (result)
            return;

        dimensions.x = texture->baseWidth;
        dimensions.y = texture->baseHeight;
        dimensions.z = (texture->isArray) ? texture->numLayers : texture->baseDepth;
        channels = gl_channels(gl_internal);
        dimensions.w = 1;

        char* pValue;
        uint32_t valueLen;
        if (KTX_SUCCESS == ktxHashList_FindValue(&texture->kvDataHead,
            "Dimensions",
            &valueLen, (void**)&pValue))
        {
            auto dims = *reinterpret_cast<glm::ivec4*>(pValue);
            dimensions.z = dims.z;
            dimensions.w = dims.w;
        }

        uint64_t bytes = ktxTexture_GetDataSize(ktxTexture(texture));
        uint64_t bytes_slice = bytes / (dimensions.z * dimensions.w);
        // 2D Array
        if (texture->isArray) {
            for (uint64_t z = 0; z < texture->numLayers; z++) {
                ktx_size_t offset;

                result = ktxTexture_GetImageOffset(ktxTexture(texture), 0, z, 0, &offset);
                uint8_t* imageData = ktxTexture_GetData(ktxTexture(texture)) + offset;
                //std::memcpy(data_ptr + offset, imageData, bytes_slice);
                for (uint32_t b = 0; b < bytes_slice; b++) {
                    data_ptr[b + offset] = imageData[b];
                }
            }
        }
        // 3D Texture
        else {
            uint8_t* imageData = ktxTexture_GetData(ktxTexture(texture));
            std::memcpy(data_ptr, imageData, bytes);
        }

        ktxTexture_Destroy(ktxTexture(texture));
    }

    uint64_t ktx_size(const char* path) {
        ktxTexture1* texture;
        KTX_error_code result;

        result = ktxTexture_CreateFromNamedFile(path,
            KTX_TEXTURE_CREATE_NO_FLAGS,
            &ktxTexture(texture));

        if (result) {
            return 0;
        }

        return ktxTexture_GetDataSize(ktxTexture(texture));
    }

    bool save_ktx(const Texture& input, const char* path, bool as_texture_array, bool save_monolithic) {
        return save_ktx(input.data.data(), path, input.dimensions, input.gl_internal, input.bytes(), as_texture_array, save_monolithic);
    }

    bool save_ktx2(const Texture& input, const char* path, bool as_texture_array, bool save_monolithic) {
        return save_ktx2(input.data.data(), path, input.dimensions, input.gl_internal, input.bytes(), as_texture_array, save_monolithic);
    }

    void load_ktx(const char* path, Texture& tex) {
        tex.data.resize(ktx_size(path));
        load_ktx(path, tex.data.data(), tex.channels, tex.dimensions, tex.gl_internal, tex.gl_format, tex.gl_type);
    }
}