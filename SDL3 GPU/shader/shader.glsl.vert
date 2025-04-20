
#version 460

layout(set=1,binding=0)uniform UBO{
	mat4 mvp;
};

layout(location=0) in vec3 position;	
layout(location=1) in vec2 texcoord;	
layout(location=2) in vec4 inColor;

layout(location=0) out vec4 color;
layout(location=1) out vec2 outTexcoord;

void main(){
	gl_Position = mvp * vec4(position,1);
	color = inColor;
	outTexcoord = texcoord;
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
		SDL_BindGPUFragmentSamplers(renderPass, 0,&textureSamplerBinding,1);
        SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
        SDL_EndGPURenderPass(renderPass);
        assert(SDL_SubmitGPUCommandBuffer(commandBuffer));