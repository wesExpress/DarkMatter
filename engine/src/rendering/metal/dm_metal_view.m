#include "dm_metal_view.h"

#ifdef DM_METAL

#include "core/dm_logger.h"

demo_vertex test_vertices[] = {
    { { 0,    0.5, 0, 1}, {1,0,0,1} },
    { {-0.5, -0.5, 0, 1}, {0,1,0,1} },
    { { 0.5, -0.5, 0, 1}, {0,0,1,1} },
};

@interface dm_metal_view ()
@property (nonatomic, strong) id<MTLDevice> metal_device;
@property (nonatomic, strong) CAMetalLayer* metal_layer;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, strong) id<MTLRenderPipelineState> metal_pipeline;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@end

@implementation dm_metal_view

@synthesize metal_device=metal_device;

+ (id) layerClass
{
    return [CAMetalLayer class];
}

- (id) initWithView: (dm_content_view*)view_in
{
    self = [super init];

    if(self)
    {
        [self makeDevice];

        NSRect frame = [view_in getWindowFrame];
        [self setFrame:frame];
        [self.metal_layer setFrame:frame];

        // content view is the main view for our NSWindow
        // must add our view to the subviews
        [view_in addSubview:self];

        // must set the content view's layer to our metal layer
        [view_in setWantsLayer:YES];
        [view_in setLayer:self.metal_layer];

        // test buffers
        [self makeBuffers];

        // pipeline
        [self makePipeline];
    }

    return self;
}

- (void) makeDevice
{
    metal_device = MTLCreateSystemDefaultDevice();

    self.wantsLayer = true;
    self.metal_layer = [CAMetalLayer layer];
    
    self.metal_layer.device = metal_device;
    self.metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
}

- (void) makeBuffers
{
    self.vertexBuffer = [metal_device newBufferWithBytes:test_vertices
                                      length:sizeof(test_vertices)
                                      options:MTLResourceOptionCPUCacheModeDefault];
}

- (void) makePipeline
{
    //id<MTLLibrary> library = [metal_device newDefaultLibrary];
    id<MTLLibrary> library = [metal_device newLibraryWithFile:@"shaders/metal/shaders.metallib" error:NULL];

    id<MTLFunction> vertexFunc = [library newFunctionWithName:@"vertex_main"];
    id<MTLFunction> fragFunc = [library newFunctionWithName:@"fragment_main"];

    MTLRenderPipelineDescriptor* pipe_desc = [MTLRenderPipelineDescriptor new];
    pipe_desc.vertexFunction = vertexFunc;
    pipe_desc.fragmentFunction = fragFunc;
    pipe_desc.colorAttachments[0].pixelFormat = self.metal_layer.pixelFormat;

    self.metal_pipeline = [self.metal_device newRenderPipelineStateWithDescriptor:pipe_desc error:NULL];

    self.commandQueue = [self.metal_device newCommandQueue];
}

- (void) redrawWithColor:(dm_vec4)color
{
    id<CAMetalDrawable> drawable = [self.metal_layer nextDrawable];

    if(drawable)
    {
        id<MTLTexture> texture = drawable.texture;

        MTLRenderPassDescriptor* passDescriptor = [MTLRenderPassDescriptor new];
        passDescriptor.colorAttachments[0].texture = texture;
        passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(color.x, color.y, color.z, color.w);

        id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];

        id <MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

        [commandEncoder setRenderPipelineState:self.metal_pipeline];
        [commandEncoder setVertexBuffer:self.vertexBuffer offset:0 atIndex:0];
        [commandEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [commandEncoder endEncoding];

        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
    }
    else
    {
        DM_LOG_ERROR("Drawable was null");
    }
}

@end

#endif