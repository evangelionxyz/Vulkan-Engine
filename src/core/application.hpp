// Copyright (c) 2024, Evangelion Manuhutu
#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "window.hpp"

#include <memory>
#include <vector>
#include <glm/glm.hpp>

class VulkanContext;
class CommandBuffer;
class GraphicsPipeline;
class VertexBuffer;
class Shader;

class Application {
public:
    Application(i32 argc, char **argv);
    ~Application();

    void run();

private:
    void create_graphics_pipeline();
    void record_frame(uint32_t frame_index);

    void imgui_init();
    void imgui_begin();
    void imgui_end();
    void imgui_shutdown() const;

    Ref<GraphicsPipeline> m_Pipeline;
    Ref<VertexBuffer> m_VertexBuffer;
    std::vector<VkDescriptorSetLayout> m_DescLayouts;
    Ref<CommandBuffer> m_CommandBuffer;
    Scope<Window> m_Window;
    VulkanContext *m_Vk;
    glm::vec4 m_ClearColor = glm::vec4(1.0f);
};

#endif //APPLICATION_H
