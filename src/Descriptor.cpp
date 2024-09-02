#include "Descriptor.h"
#include "VulkanHelper.h"

namespace vkutil {
  DescriptorLayoutBuilder DescriptorLayoutBuilder::add_binding(uint32_t binding, VkDescriptorType type) {
    bindings.push_back({
      .binding = binding,
      .descriptorType = type,
      .descriptorCount = 1,
    });

    return *this;
  }

  void DescriptorLayoutBuilder::clear() {
    bindings.clear();
  }

  VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext,
                                                       VkDescriptorSetLayoutCreateFlags flags) {
    for (auto & binding : bindings) {
      binding.stageFlags |= shaderStages; // TODO: Understand this
    }

    VkDescriptorSetLayoutCreateInfo info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext = pNext;

    info.pBindings = bindings.data();
    info.bindingCount = (uint32_t)bindings.size();
    info.flags = flags;

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

    return set;
  }


  void DescriptorAllocator::init_pool(VkDevice device, uint32_t maxSets, std::span<DescriptorAllocator::PoolSizeRatio> poolRatios) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios) {
      poolSizes.push_back(VkDescriptorPoolSize{
        .type = ratio.type,
        .descriptorCount = uint32_t(ratio.ratio * maxSets)
      });
    }

    VkDescriptorPoolCreateInfo pool_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.flags = 0;
    pool_info.maxSets = maxSets;
    pool_info.poolSizeCount = (uint32_t)poolSizes.size();
    pool_info.pPoolSizes = poolSizes.data();

    vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);
  }

  void DescriptorAllocator::clear_descriptors(VkDevice device) {
    vkResetDescriptorPool(device, pool, 0);
  }

  void DescriptorAllocator::destroy_pool(VkDevice device) {
    vkDestroyDescriptorPool(device, pool, nullptr);
  }

  VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout) {
    VkDescriptorSetAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &layout
    };

    VkDescriptorSet descriptorSet;
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
    return descriptorSet;
  }
}