//
// Created by starlitsky2010 on 18-6-27.
//

#ifndef FACENN_SNPE_UDL_LAYERS_H
#define FACENN_SNPE_UDL_LAYERS_H

#include <vector>

#include "DlSystem/IUDL.hpp"
#include "DlSystem/UDLContext.hpp"

#include "MyUdlLayers.h"

namespace engine_spne_udl {

/**
 * Parameters for ShapeIndexPatch layer
 */
    struct ShapeIndexPatchParams {
        CommonLayerParams common_params;

        int origin_patch_h;
        int origin_patch_w;
        int origin_h;
        int origin_w;
        //std::vector<uint32_t> weights_dim;
        //std::vector<float> weights_data;
    };

    class UdlShapeIndexPatch final : public zdl::DlSystem::IUDL {
    public:
        UdlShapeIndexPatch(const UdlShapeIndexPatch &) = delete;

        UdlShapeIndexPatch &operator=(const UdlShapeIndexPatch &) = delete;

        /**
         * @brief UDLContext by value but it has move operation
         */
        UdlShapeIndexPatch(zdl::DlSystem::UDLContext context) :
                m_Context(context) {
        }

        /**
         * @brief Setup User's environment.
         *        This is being called by DnnRunTime framework
         *        to let the user opportunity to setup anything
         *        which is needed for running user defined layers
         * @return true on success, false otherwise
         */
        virtual bool setup(void *cookie, size_t insz, const size_t **indim,
                           const size_t *indimsz, size_t outsz, const size_t **outdim,
                           const size_t *outdimsz) override;

        /**
         * Close the instance. Invoked by DnnRunTime to let
         * the user the opportunity to close handels etc...
         */
        virtual void close(void *cookie) noexcept override;

        /**
         * Execute the user defined layer
         * will contain the return value/output tensor
         */
        virtual bool execute(void *cookie, const float **input, float **output)
        override;

    private:

        bool ParseShapeIndexPatchParams(const void *buffer, size_t size,
                                        ShapeIndexPatchParams &params);

        zdl::DlSystem::UDLContext m_Context;
        size_t m_OutSzDim = 0;
        ShapeIndexPatchParams m_Params;
        int origin_patch_h;
        int origin_patch_w;
        int origin_h;
        int origin_w;
        int feat_h;
        int feat_w;
        int feat_patch_h;
        int feat_patch_w;

        int channels;
        int top_size;
        int batchSize;
        int poitNum = 5;

    };

} // ns myudl

#endif //FACENN_SNPE_UDL_LAYERS_H
