
#ifndef UDL_MY_UDL_LAYERS_HPP
#define UDL_MY_UDL_LAYERS_HPP

#include <vector>

namespace engine_spne_udl {

enum LayerType {
	SHAPE_INDEX_PATCH_LAYER = 1,
    MY_OTHER_LAYER      = 100
};

struct CommonLayerParams {
    LayerType type;
};

/**
 * Parse the common layer parameters from buffer
 * Returns true on success, false on parse error
 */
static bool ParseCommonLayerParams(const void* buffer, size_t size, CommonLayerParams& params) {
    if(!buffer) return false;
    if(size < sizeof(CommonLayerParams)) return false;
    params = *reinterpret_cast<const CommonLayerParams*>(buffer);
    return true;
}

} // ns myudl

#endif // UDL_MY_UDL_LAYERS_HPP
