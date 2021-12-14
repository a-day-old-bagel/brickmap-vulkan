#include "cache_keys.h"
#include "hash/hash_utils.h"

namespace rebel_road
{
    namespace vulkan
    {
 
        bool render_pass_key::operator==( const render_pass_key& other ) const
        {
            return info == other.info;
        }

        size_t render_pass_key::hash() const
        {
            size_t seed { 0 };
            hash_combine( seed, info.flags );

            hash_combine( seed, info.attachmentCount );
            for ( int i = 0; i < info.attachmentCount; i++ )
            {
                auto attachment_info = info.pAttachments[i];
                hash_combine( seed, attachment_info.flags );
                hash_combine( seed, attachment_info.format );
                hash_combine( seed, attachment_info.samples );
                hash_combine( seed, attachment_info.loadOp );
                hash_combine( seed, attachment_info.storeOp );
                hash_combine( seed, attachment_info.stencilLoadOp );
                hash_combine( seed, attachment_info.stencilStoreOp );
                hash_combine( seed, attachment_info.initialLayout );
                hash_combine( seed, attachment_info.finalLayout );
            }

            hash_combine( seed, info.subpassCount );
            for ( int i = 0; i < info.subpassCount; i++ )
            {
                auto subpass_info = info.pSubpasses[i];
                hash_combine( seed, subpass_info.flags );
                hash_combine( seed, subpass_info.pipelineBindPoint );

                hash_combine( seed, subpass_info.inputAttachmentCount );
                for ( int j = 0; j < subpass_info.inputAttachmentCount; j++ )
                {
                    auto attachment_ref = subpass_info.pInputAttachments[j];
                    hash_combine( seed, attachment_ref.attachment );
                    hash_combine( seed, attachment_ref.layout );
                }

                hash_combine( seed, subpass_info.colorAttachmentCount );
                for ( int j = 0; j < subpass_info.colorAttachmentCount; j++ )
                {
                    auto attachment_ref = subpass_info.pColorAttachments[j];
                    hash_combine( seed, attachment_ref.attachment );
                    hash_combine( seed, attachment_ref.layout );
                }

                if ( subpass_info.pResolveAttachments )
                {
                    // If we have any resolve attachments, then we must have as many as we have color attachments.
                    for ( int j = 0; j < subpass_info.colorAttachmentCount; j++ )
                    {
                        auto attachment_ref = subpass_info.pResolveAttachments[j];
                        hash_combine( seed, attachment_ref.attachment );
                        hash_combine( seed, attachment_ref.layout );
                    }
                }

                // There is only a single depth attachment.
                if ( subpass_info.pDepthStencilAttachment )
                {
                    auto attachment_ref = subpass_info.pDepthStencilAttachment[0];
                    hash_combine( seed, attachment_ref.attachment );
                    hash_combine( seed, attachment_ref.layout );
                }

                hash_combine( seed, subpass_info.preserveAttachmentCount );
                for ( int j = 0; j < subpass_info.preserveAttachmentCount; j++ )
                {
                    hash_combine( seed, subpass_info.pPreserveAttachments[j] );
                }
            }

            hash_combine( seed, info.dependencyCount );
            for ( int i = 0; i < info.dependencyCount; i++ )
            {
                auto dependency_info = info.pDependencies[i];
                hash_combine( seed, dependency_info.srcSubpass );
                hash_combine( seed, dependency_info.dstSubpass );
                hash_combine( seed, dependency_info.srcStageMask );
                hash_combine( seed, dependency_info.dstStageMask );
                hash_combine( seed, dependency_info.srcAccessMask );
                hash_combine( seed, dependency_info.dstAccessMask );
                hash_combine( seed, dependency_info.dependencyFlags );
            }

            return seed;
        }
   }
}