#include <iostream>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdio.h>

struct UBO {
    glm::mat4 mvp;
};

struct Vec3 {
    float x, y, z;
};

struct VertexData {
    Vec3 position;
    SDL_FColor color;
};

SDL_GPUShader* load_shader(
    SDL_GPUDevice* device,
    const char* filename,
    SDL_GPUShaderStage stage,
    Uint32 sampler_count,
    Uint32 uniform_buffer_count,
    Uint32 storage_buffer_count,
    Uint32 storage_texture_count) {

    if (!SDL_GetPathInfo(filename, NULL)) {
        fprintf(stdout, "File (%s) does not exist.\n", filename);
        return NULL;
    }

    const char* entrypoint;
    SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    }

    size_t code_size;
    void* code = SDL_LoadFile(filename, &code_size);
    if (code == NULL) {
        fprintf(stderr, "ERROR: SDL_LoadFile(%s) failed: %s\n", filename, SDL_GetError());
        return NULL;
    }

    SDL_GPUShaderCreateInfo shader_info = {};
    shader_info.code = (const Uint8*)code;
    shader_info.code_size = code_size;
    shader_info.entrypoint = entrypoint;
    shader_info.format = format;
    shader_info.stage = stage;
    shader_info.num_samplers = sampler_count;
    shader_info.num_uniform_buffers = uniform_buffer_count;
    shader_info.num_storage_buffers = storage_buffer_count;
    shader_info.num_storage_textures = storage_texture_count;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shader_info);

    if (shader == NULL) {
        fprintf(stderr, "ERROR: SDL_CreateGPUShader failed: %s\n", SDL_GetError());
        SDL_free(code);
        return NULL;
    }
    SDL_free(code);
    return shader;
}

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "Failed to initialize SDL. Error: " << SDL_GetError() << std::endl;
    }

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);

    SDL_Window* window = NULL;
    window = SDL_CreateWindow("SDL3 GPU", 800, 600, 0);
    if (!window) {
        std::cout << "Failed to initialize window. Error: " << SDL_GetError() << std::endl;
    }

    SDL_GPUDevice* device = NULL;
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (!device) {
        std::cout << "Failed to initialize GPU. Error: " << SDL_GetError() << std::endl;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        std::cout << "Failed to claim GPU. Error: " << SDL_GetError() << std::endl;
    }

    

    SDL_GPUColorTargetInfo colorInfo = {};

    SDL_GPUShader* vertexShader = load_shader(device, "../../../../SDL3 GPU/shader/shader.spv.vert", SDL_GPU_SHADERSTAGE_VERTEX, NULL, 1, NULL, NULL);
    SDL_GPUShader* fragmentShader = load_shader(device, "../../../../SDL3 GPU/shader/shader.spv.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, NULL, NULL, NULL, NULL);

    SDL_GPUColorTargetBlendState blendState = {};
    blendState.enable_blend = false;

    SDL_GPUColorTargetDescription color_target_descriptions;
    color_target_descriptions.format = SDL_GetGPUSwapchainTextureFormat(device, window);
    color_target_descriptions.blend_state = blendState;

    SDL_GPUGraphicsPipelineTargetInfo target_info = {};
    target_info.num_color_targets = 1;
    target_info.color_target_descriptions = &color_target_descriptions;

    VertexData vertices[] = {
        {-0.5f,  0.5f, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f}}, // Triangle 1
        { 0.5f,  0.5f, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f}},
        {-0.5f, -0.5f, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
        { 0.5f, -0.5f, 0.0f, {1.0f, 1.0f, 0.0f, 1.0f}}, // Triangle 2
    };

    Uint32 indices[] = {
        0, 1, 2,
        2, 1, 3
    };

    // Create and upload the vertex buffer
    SDL_GPUBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBufferInfo.size = sizeof(vertices);
    SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(device, &vertexBufferInfo);
    if (!vertexBuffer) {
        std::cout << "Failed to create vertex buffer. Error: " << SDL_GetError() << std::endl;
    }

    // Create and upload the index buffer
    SDL_GPUBufferCreateInfo indexBufferInfo = {};
    indexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    indexBufferInfo.size = sizeof(indices);
    SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(device, &indexBufferInfo);
    if (!indexBuffer) {
        std::cout << "Failed to create index buffer. Error: " << SDL_GetError() << std::endl;
    }

    // Upload vertex and index data
    SDL_GPUTransferBufferCreateInfo transferBufferInfo = {};
    transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferBufferInfo.size = sizeof(vertices) + sizeof(indices);
    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferInfo);

    void* transferMem = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    memcpy(transferMem, vertices, sizeof(vertices));
    memcpy(static_cast<char*>(transferMem) + sizeof(vertices), indices, sizeof(indices));
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    SDL_GPUCommandBuffer* copyCommandBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(copyCommandBuffer);

    SDL_GPUTransferBufferLocation vertexTransferLocation = {};
    vertexTransferLocation.transfer_buffer = transferBuffer;
    vertexTransferLocation.offset = 0;

    SDL_GPUBufferRegion vertexBufferRegion = {};
    vertexBufferRegion.buffer = vertexBuffer;
    vertexBufferRegion.size = sizeof(vertices);

    SDL_UploadToGPUBuffer(copyPass, &vertexTransferLocation, &vertexBufferRegion, false);

    SDL_GPUTransferBufferLocation indexTransferLocation = {};
    indexTransferLocation.transfer_buffer = transferBuffer;
    indexTransferLocation.offset = sizeof(vertices);

    SDL_GPUBufferRegion indexBufferRegion = {};
    indexBufferRegion.buffer = indexBuffer;
    indexBufferRegion.size = sizeof(indices);

    SDL_UploadToGPUBuffer(copyPass, &indexTransferLocation, &indexBufferRegion, false);

    SDL_EndGPUCopyPass(copyPass);
    assert(SDL_SubmitGPUCommandBuffer(copyCommandBuffer));

    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    // Vertex input state
    SDL_GPUVertexBufferDescription vertexBufferDescription = {};
    vertexBufferDescription.slot = 0;
    vertexBufferDescription.pitch = sizeof(VertexData);
    vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute vertexAttributes[2] = {};
    vertexAttributes[0].location = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[0].offset = offsetof(VertexData, position);
    vertexAttributes[1].location = 1;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertexAttributes[1].offset = offsetof(VertexData, color);

    SDL_GPUVertexInputState vertexInputState = {};
    vertexInputState.num_vertex_buffers = 1;
    vertexInputState.vertex_buffer_descriptions = &vertexBufferDescription;
    vertexInputState.num_vertex_attributes = 2;
    vertexInputState.vertex_attributes = vertexAttributes;

    // Pipeline creation
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.target_info = target_info;
    pipelineInfo.vertex_input_state = vertexInputState;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (!pipeline) {
        std::cout << "Failed to create pipeline. Error: " << SDL_GetError() << std::endl;
    }

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    const float rotationSpeed = glm::radians(90.0f);
    float rotation = 0.0f;

    glm::mat4 Projection = glm::perspective(70.0f, (float)width / height, 0.000000f, 10000.0f);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.0f, 0.0f, -10.0f)) * glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 1.0f, -0.0f));

    SDL_GPUBufferBinding vertexBufferBinding = {};
    vertexBufferBinding.buffer = vertexBuffer;
    vertexBufferBinding.offset = 0;

    SDL_GPUBufferBinding indexBufferBinding = {};
    indexBufferBinding.buffer = indexBuffer;

    Uint64 lastTime = SDL_GetPerformanceCounter();
    SDL_Event event;
    bool running = true;
    while (running) {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float deltaTime = (float)(currentTime - lastTime) / SDL_GetPerformanceFrequency();
        lastTime = currentTime;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        UBO ubo = { Projection * model };
        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUTexture* texture;
        SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &texture, NULL, NULL);

        SDL_GPUColorTargetInfo colorInfo = {};
        colorInfo.texture = texture;
        colorInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorInfo.clear_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        colorInfo.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorInfo, 1, NULL);
        rotation += rotationSpeed * deltaTime;
        model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.0f, 0.0f, -10.0f)) * glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 1.0f, -0.0f));

        SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);
        SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &ubo, sizeof(ubo));
        SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
        SDL_EndGPURenderPass(renderPass);
        assert(SDL_SubmitGPUCommandBuffer(commandBuffer));
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

//Swapchan texture is all big texture that has all textures on screen
//Swapchain texture which is just a special texture that represents a window's contents
//1.aquire command buffer
//2.wait aquire swapchain texture
//3.begain render pass
// render - bnd everything
//4.end renderpass
//5.submit command buffer


//1.	Define Vertex Structure: Define a structure that represents your vertex data, including positions, colors, normals, etc.
//2.	Create Vertex Buffer : Create a GPU buffer to store your vertex data.
//3.	Upload Vertex Data : Upload your vertex data to the GPU buffer.
//		SDL_GPUTransferBufferCreateInfo transferBufferInfo = {};
//transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
//transferBufferInfo.size = sizeof(vertices);
//SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferInfo);
//
//void* mappedMemory = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
//memcpy(mappedMemory, vertices, sizeof(vertices));
//SDL_UnmapGPUTransferBuffer(device, transferBuffer);
//
//SDL_GPUCommandBuffer* copyCommandBuffer = SDL_AcquireGPUCommandBuffer(device);
//SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(copyCommandBuffer);
//
//SDL_GPUTransferBufferLocation transferBufferLocation = {};
//transferBufferLocation.transfer_buffer = transferBuffer;
//
//SDL_GPUBufferRegion bufferRegion = {};
//bufferRegion.buffer = vertexBuffer;
//bufferRegion.size = sizeof(vertices);
//
//SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion, false);
//SDL_EndGPUCopyPass(copyPass);
//SDL_SubmitGPUCommandBuffer(copyCommandBuffer);
//4.	Describe Vertex Buffer : Define the layout of your vertex buffer, specifying the attributes and their formats.
//5.	Bind Vertex Buffer : Bind the vertex buffer to the pipeline before drawing.

//textures
// load texture
// 