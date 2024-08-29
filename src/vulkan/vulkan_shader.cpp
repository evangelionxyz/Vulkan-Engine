// Copyright (c) 2024, Evangelion Manuhutu
#include "vulkan_shader.h"

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <shaderc/shaderc.hpp>

#include <fstream>

#include "vulkan_context.h"
#include "core/assert.h"
#include "core/logger.h"

static const char *get_cached_directory()
{
    return "res/cache/shaders";
}

static void create_cached_directory_if_needed()
{
    const std::string cached_directory = get_cached_directory();
    if (!std::filesystem::exists(cached_directory))
    {
        std::filesystem::create_directories(cached_directory);
    }
}

static shaderc_shader_kind vulkan_shader_to_shaderc_kind(VkShaderStageFlagBits stage)
{
    switch (stage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT: return shaderc_glsl_vertex_shader;
    case VK_SHADER_STAGE_FRAGMENT_BIT: return shaderc_glsl_fragment_shader;
    case VK_SHADER_STAGE_GEOMETRY_BIT: return shaderc_glsl_geometry_shader;
    case VK_SHADER_STAGE_COMPUTE_BIT: return shaderc_glsl_compute_shader;
    default: return static_cast<shaderc_shader_kind>(0);
    }
}

static std::string vulkan_shader_stage_str(VkShaderStageFlagBits stage)
{
    switch (stage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT: return "Vertex";
    case VK_SHADER_STAGE_FRAGMENT_BIT: return "Fragment";
    case VK_SHADER_STAGE_COMPUTE_BIT: return "Compute";
    case VK_SHADER_STAGE_GEOMETRY_BIT: return "Geometry";
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return "Tesselation";
    default: return "Invalid";
    }
}

static std::string vulkan_shader_stage_extension(VkShaderStageFlagBits stage)
{
    switch (stage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT: return ".vert.spv";
    case VK_SHADER_STAGE_FRAGMENT_BIT: return ".frag.spv";
    case VK_SHADER_STAGE_COMPUTE_BIT: return ".comp.spv";
    case VK_SHADER_STAGE_GEOMETRY_BIT: return ".geom.spv";
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return ".tes.spv";
    default: return "Invalid";
    }
}

VulkanShader::VulkanShader(const std::filesystem::path& vertex_shader_path,
                           const std::filesystem::path& fragment_shader_path)
{
    create_cached_directory_if_needed();
    std::string vertex_shader_source = read_file(vertex_shader_path);
    std::string fragment_shader_source = read_file(fragment_shader_path);

    std::unordered_map<VkShaderStageFlagBits, std::vector<u32>> vulkan_shader;
    vulkan_shader[VK_SHADER_STAGE_VERTEX_BIT] = compile_or_get_vulkan_binaries(vertex_shader_source, vertex_shader_path, VK_SHADER_STAGE_VERTEX_BIT);
    vulkan_shader[VK_SHADER_STAGE_FRAGMENT_BIT] = compile_or_get_vulkan_binaries(fragment_shader_source, fragment_shader_path, VK_SHADER_STAGE_FRAGMENT_BIT);

    VulkanContext *vk_context = VulkanContext::get_instance();
    for (auto &[stage, code] : vulkan_shader)
    {
        VkShaderModule shader_module = create_module(code);
        VkPipelineShaderStageCreateInfo shader_create_info = {};
        shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_create_info.stage = stage;
        shader_create_info.module = shader_module;
        shader_create_info.pName = "main";
        vkDestroyShaderModule(vk_context->get_vk_logical_device(), shader_module, vk_context->get_vk_allocator());
    }
}

VulkanShader::~VulkanShader()
{
}

VkShaderModule VulkanShader::create_module(const std::vector<u32>& code) const
{
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size() * sizeof(u32);
    create_info.pCode = code.data();

    VulkanContext *vk_context = VulkanContext::get_instance();
    VkShaderModule module = VK_NULL_HANDLE;
    vkCreateShaderModule(vk_context->get_vk_logical_device(), &create_info, vk_context->get_vk_allocator(), &module);
    ASSERT(module != VK_NULL_HANDLE, "[Vulkan Shader] Could not create shader module");
    return module;
}

std::string VulkanShader::read_file(const std::filesystem::path& file_path) const
{
    std::string result;
    std::ifstream shader_in(file_path.string(), std::ios::binary);
    if (shader_in.is_open())
    {
        shader_in.seekg(0, std::ios::end);
        if (size_t size = shader_in.tellg(); size != -1)
        {
            result.resize(size);
            shader_in.seekg(0, std::ios::beg);
            shader_in.read(&result[0], size);
        }
        else
        {
            LOG_ERROR("[Vulkan Shader] Could not read from file. {0}", file_path.string());
        }

        shader_in.close();
    }
    else
    {
        LOG_ERROR("[Vulkan Shader] Could not open file. {0}", file_path.string());
    }
    return result;
}

std::vector<u32> VulkanShader::compile_or_get_vulkan_binaries(const std::string& shader_source, const std::string& file_path, VkShaderStageFlagBits stage)
{
    std::vector<u32> code;
    shaderc::CompileOptions options;

    std::filesystem::path cached_directory = get_cached_directory();

    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    std::filesystem::path shader_file_path = cached_directory / file_path;
    std::filesystem::path cached_path = cached_directory / (shader_file_path.stem().string() + vulkan_shader_stage_extension(stage));
    std::ifstream shader_file(cached_path, std::ios::binary);
    if (shader_file.is_open())
    {
        shader_file.seekg(0, std::ios::end);
        auto size = shader_file.tellg();
        shader_file.seekg(0, std::ios::beg);
        auto &data = code;
        data.resize(size / sizeof(u32));
        shader_file.read(reinterpret_cast<char*>(data.data()), size);

        shader_file.close();
    }
    else
    {
        shaderc::Compiler compiler;
        shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(shader_source, vulkan_shader_to_shaderc_kind(stage), file_path.c_str());
        bool success = module.GetCompilationStatus() == shaderc_compilation_status_success;
        ASSERT(success, "[Vulkan Shader] Compilation failed {0}", module.GetErrorMessage().c_str());
        code = std::vector<u32>(module.cbegin(), module.cend());

        std::ofstream out_file(cached_path, std::ios::binary);
        if (out_file.is_open())
        {
            auto &data = code;
            out_file.write(reinterpret_cast<char *>(data.data()), data.size() * sizeof(u32));
            out_file.flush();
            out_file.close();
        }
    }

    reflect(stage, code);

    return code;
}

void VulkanShader::reflect(const VkShaderStageFlagBits shader_stage, const std::vector<u32>& code)
{
    spirv_cross::Compiler compiler(code);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    LOG_INFO("[Vulkan Shader] Shader reflect - {0}", vulkan_shader_stage_str(shader_stage));
    LOG_INFO("[Vulkan Shader]    {0} Uniform buffers", resources.uniform_buffers.size());
    LOG_INFO("[Vulkan Shader]    {0} Resources", resources.sampled_images.size());

    if (!resources.uniform_buffers.empty())
    {
        LOG_INFO("[Vulkan Shader] Uniform buffers: ");
        for (const auto& uniform_buffer : resources.uniform_buffers)
        {
            const auto &buffer_type = compiler.get_type(uniform_buffer.base_type_id);
            u32 buffer_size = compiler.get_declared_struct_size(buffer_type);
            u32 binding = compiler.get_decoration(uniform_buffer.id, spv::DecorationBinding);
            size_t member_count = buffer_type.member_types.size();

            LOG_INFO("[Vulkan Shader]    Name = {0}", uniform_buffer.name);
            LOG_INFO("[Vulkan Shader]    Size = {0}", buffer_size);
            LOG_INFO("[Vulkan Shader] Binding = {0}", binding);
            LOG_INFO("[Vulkan Shader] Members = {0}", member_count);
        }
    }
}
