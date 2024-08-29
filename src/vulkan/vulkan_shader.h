// Copyright (c) 2024, Evangelion Manuhutu
#ifndef VULKAN_SHADER_H
#define VULKAN_SHADER_H

#include "core/types.h"

#include <vulkan/vulkan.h>
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>

struct ShaderProgramSources
{
    std::string VertexSources;
    std::string FragmentSources;
};

class VulkanShader {
public:
    VulkanShader(const std::filesystem::path& vertex_shader_path, const std::filesystem::path& fragment_shader_path);
    ~VulkanShader();

    [[nodiscard]] VkShaderModule create_module(const std::vector<u32>& code) const;
private:
    [[nodiscard]] std::string read_file(const std::filesystem::path& file_path) const;
    static std::vector<u32> compile_or_get_vulkan_binaries(const std::string &shader_source, const std::string &file_path, VkShaderStageFlagBits stage) ;
    static void reflect(VkShaderStageFlagBits shader_stage, const std::vector<u32> &code);
};

#endif //VULKAN_SHADER_H