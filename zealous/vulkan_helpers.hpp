#pragma once

#include "vulkan_context.hpp"

namespace zealous {
    void InitVulkan( VulkanContext& context );
    void DeInitVulkan( VulkanContext& context );

    bool MustUpdateVulkan( VulkanContext& context );
    void UpdateVulkan( VulkanContext& context );
}
