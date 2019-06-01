// Copyright (c) 2019 Andreas Atteneder, All Rights Reserved.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#ifdef DLL_EXPORT_FLAG
#define CALL_CONV __stdcall
#define DLL_EXPORT __declspec(dllexport) CALL_CONV
#else
#define CALL_CONV
#define DLL_EXPORT
#endif

#include "basisu_transcoder.h"

using namespace basist;

static basist::etc1_global_selector_codebook *g_pGlobal_codebook;
basist::basisu_transcoder transcoder(g_pGlobal_codebook);

#define MAGIC 0xDEADBEE1

class basis_file
{
    int m_magic = 0;
    const uint8_t *m_file;
    size_t byteLength;
    
public:
    basis_file(const uint8_t *buffer, size_t byteLength)
    : m_file(buffer),
    byteLength(byteLength)
    {
        if (!transcoder.validate_header(buffer, byteLength)) {
            m_file = nullptr;
            byteLength = 0;
        }
        
        // Initialized after validation
        m_magic = MAGIC;
    }
    
    void close() {
        assert(m_magic == MAGIC);
        m_file = nullptr;
        byteLength = 0;
    }
    
    uint32_t getHasAlpha() {
        assert(m_magic == MAGIC);
        if (m_magic != MAGIC)
            return 0;
        
        basisu_image_level_info li;
        if (!transcoder.get_image_level_info(m_file, byteLength, li, 0, 0))
            return 0;
        
        return li.m_alpha_flag;
    }
    
    uint32_t getNumImages() {
        assert(m_magic == MAGIC);
        if (m_magic != MAGIC)
            return 0;
        
        return transcoder.get_total_images(m_file, byteLength);
    }
    
    uint32_t getNumLevels(uint32_t image_index) {
        assert(m_magic == MAGIC);
        if (m_magic != MAGIC)
            return 0;
        
        basisu_image_info ii;
        if (!transcoder.get_image_info(m_file, byteLength, ii, image_index))
            return 0;
        
        return ii.m_total_levels;
    }
    
    uint32_t getImageWidth(uint32_t image_index, uint32_t level_index) {
        assert(m_magic == MAGIC);
        if (m_magic != MAGIC)
            return 0;
        
        uint32_t orig_width, orig_height, total_blocks;
        if (!transcoder.get_image_level_desc(m_file, byteLength, image_index, level_index, orig_width, orig_height, total_blocks))
            return 0;
        
        return orig_width;
    }
    
    uint32_t getImageHeight(uint32_t image_index, uint32_t level_index) {
        assert(m_magic == MAGIC);
        if (m_magic != MAGIC)
            return 0;
        
        uint32_t orig_width, orig_height, total_blocks;
        if (!transcoder.get_image_level_desc(m_file, byteLength, image_index, level_index, orig_width, orig_height, total_blocks))
            return 0;
        
        return orig_height;
    }
    
    uint32_t getImageTranscodedSizeInBytes(uint32_t image_index, uint32_t level_index, uint32_t format) {
        assert(m_magic == MAGIC);
        if (m_magic != MAGIC)
            return 0;
        
        if (format >= cTFTotalTextureFormats)
            return 0;
        
        uint32_t bytes_per_block = basis_get_bytes_per_block(static_cast<transcoder_texture_format>(format));
        
        uint32_t orig_width, orig_height, total_blocks;
        if (!transcoder.get_image_level_desc(m_file, byteLength, image_index, level_index, orig_width, orig_height, total_blocks))
            return 0;
        
        return total_blocks * bytes_per_block;
    }
    
    uint32_t startTranscoding() {
        assert(m_magic == MAGIC);
        if (m_magic != MAGIC)
            return 0;
        
        return transcoder.start_transcoding(m_file, byteLength);
    }
    
    uint32_t transcodeImage(void* dst, size_t dst_size, uint32_t image_index, uint32_t level_index, uint32_t format, uint32_t pvrtc_wrap_addressing, uint32_t get_alpha_for_opaque_formats) {
        assert(m_magic == MAGIC);
        if (m_magic != MAGIC)
            return 0;
        
        if (format >= cTFTotalTextureFormats)
            return 0;
        
        uint32_t bytes_per_block = basis_get_bytes_per_block(static_cast<transcoder_texture_format>(format));
        
        uint32_t orig_width, orig_height, total_blocks;
        if (!transcoder.get_image_level_desc(m_file, byteLength, image_index, level_index, orig_width, orig_height, total_blocks))
            return 0;
        
        uint32_t required_size = total_blocks * bytes_per_block;
        
        uint32_t status = transcoder.transcode_image_level(
                                                             m_file, byteLength, image_index, level_index,
                                                             dst, dst_size / bytes_per_block,
                                                             static_cast<basist::transcoder_texture_format>(format),
                                                             (
                                                              (pvrtc_wrap_addressing ? basisu_transcoder::cDecodeFlagsPVRTCWrapAddressing : 0) |
                                                              (get_alpha_for_opaque_formats ? basisu_transcoder::cDecodeFlagsTranscodeAlphaDataToOpaqueFormats : 0)
                                                              ));
        return status;
    }
};

extern "C" {
    
DLL_EXPORT void aa_basis_init()
{
    basisu_transcoder_init();
    
    if (!g_pGlobal_codebook)
        g_pGlobal_codebook = new basist::etc1_global_selector_codebook(g_global_selector_cb_size, g_global_selector_cb);
}

DLL_EXPORT basis_file* aa_create_basis( const uint8_t * data, size_t length ) {
    basis_file* new_basis = new basis_file(data,length);
    return new_basis;
}

DLL_EXPORT void aa_delete_basis( basis_file* basis ) {
    delete basis;
}

DLL_EXPORT bool aa_getHasAlpha( basis_file* basis ) {
    assert(basis!=nullptr);
    return basis->getHasAlpha();
}

DLL_EXPORT uint32_t aa_getNumImages( basis_file* basis ) {
    return basis->getNumImages();
}

DLL_EXPORT uint32_t aa_getNumLevels( basis_file* basis, uint32_t image_index) {
    return basis->getNumLevels(image_index);
}

DLL_EXPORT uint32_t aa_getImageWidth( basis_file* basis, uint32_t image_index, uint32_t level_index) {
    return basis->getImageWidth(image_index,level_index);
}

DLL_EXPORT uint32_t aa_getImageHeight( basis_file* basis, uint32_t image_index, uint32_t level_index) {
    return basis->getImageHeight(image_index,level_index);
}

DLL_EXPORT uint32_t aa_getImageTranscodedSizeInBytes( basis_file* basis, uint32_t image_index, uint32_t level_index, uint32_t format) {
    return basis->getImageTranscodedSizeInBytes(image_index,level_index,format);
}

DLL_EXPORT bool aa_startTranscoding( basis_file* basis ) {
    return basis->startTranscoding();
}

DLL_EXPORT bool aa_transcodeImage( basis_file* basis, void* dst, size_t dst_size, uint32_t image_index, uint32_t level_index, uint32_t format, uint32_t pvrtc_wrap_addressing, uint32_t get_alpha_for_opaque_formats) {
    return basis->transcodeImage(dst,dst_size,image_index,level_index,format,pvrtc_wrap_addressing,get_alpha_for_opaque_formats);
}
}
