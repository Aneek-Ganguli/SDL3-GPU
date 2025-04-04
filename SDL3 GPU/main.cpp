﻿#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

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
		std::cout << "Failed to initilize SDL. Error: " << SDL_GetError() << std::endl;
	}

	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);

	SDL_Window* window = NULL;
	window = SDL_CreateWindow("SDL3 GPU", 800, 600, 0);
	if (!window) {
		std::cout << "Failed to initilize window. Error: " << SDL_GetError() << std::endl;
	}

	SDL_GPUDevice* device = NULL;
	device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV,true,NULL);
	if (!device) {
		std::cout << "Failed to initilize gpu. Error: " << SDL_GetError() << std::endl;
	}

	if (!SDL_ClaimWindowForGPUDevice(device, window)) {
		std::cout << "Failed to claim gpu. Error: " << SDL_GetError() << std::endl;
	}

	SDL_GPUColorTargetInfo colorInfo = {};

	

	SDL_GPUShader* vertexShader = load_shader(device,"../../../../SDL3 GPU/shader/shader.spv.vert", SDL_GPU_SHADERSTAGE_VERTEX,NULL,NULL,NULL,NULL);
	SDL_GPUShader* fragmentShader = load_shader(device,"../../../../SDL3 GPU/shader/shader.spv.vert", SDL_GPU_SHADERSTAGE_FRAGMENT,NULL,NULL,NULL,NULL);

	SDL_GPUColorTargetBlendState blendState = {};
	blendState.enable_blend = false;

	SDL_GPUColorTargetDescription color_target_descriptions;
	color_target_descriptions.format = SDL_GetGPUSwapchainTextureFormat(device, window);
	color_target_descriptions.blend_state = blendState;

	SDL_GPUGraphicsPipelineTargetInfo target_info = {};
	target_info.num_color_targets = 1;
	target_info.color_target_descriptions = &color_target_descriptions;

	SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.vertex_shader = vertexShader;
	pipelineInfo.fragment_shader = fragmentShader;
	pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pipelineInfo.target_info = target_info;

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device,&pipelineInfo);

	SDL_Event event;
	bool running = true;
	while (running) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
		}
		SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUTexture* texture;
		SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &texture, NULL, NULL);


		SDL_GPUColorTargetInfo colorInfo = {};
		colorInfo.texture = texture;
		colorInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		colorInfo.clear_color = { 1.0f, 1.0f, 1.0f, 1.0f };
		colorInfo.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorInfo, 1, NULL);

		SDL_BindGPUGraphicsPipeline(renderPass,pipeline);
		SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

		SDL_EndGPURenderPass(renderPass);
		SDL_SubmitGPUCommandBuffer(commandBuffer);
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
