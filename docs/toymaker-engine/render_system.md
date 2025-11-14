# ToyMaker: Render System

## What is it?

The ToyMaker::RenderSystem is responsible for building and maintaining an application's rendering pipeline, and updating a target texture over the course of an application's execution.

Each pipeline managed by the render system is subdivided into ToyMaker::RenderStage objects.  Each render stage represents a step in the pipeline with its own resources, including texture attachments, mesh attachments, material attachments, framebuffer, and shader program.  It is either a subclass of ToyMaker::BaseRenderStage or ToyMaker::BaseOffscreenRenderStage, where the former renders to the screen framebuffer directly, while the latter renders to its own framebuffer.

Common to all pipelines is the following stage:

- ToyMaker::ResizeRenderStage -- Responsible for scaling a texture from its render dimensions (used by its pipeline) to its target dimensions (requested by the viewport controlling the render system).

At present, rendering pipelines cannot be configured, only selected from built-in options (ToyMaker::ViewportNode::RenderConfiguration::RenderType). The render system currently supports two pipelines:

### The 3D rendering pipeline

This pipeline is a deferred shading 3D pipeline comprised of the following render stages:

- ToyMaker::GeometryRenderStage -- Converts input mesh-material pairs into albedo, specular, normal, and position textures.  These textures are collectively called geometry textures.

- ToyMaker::LightingRenderStage -- Uses geometry textures and light data to produce a texture of the lit scene.

- ToyMaker::SkyboxRenderStage -- Adds a cubemap background to the lit scene.

- ToyMaker::TonemappingRenderStage -- Applies gamma correction and camera exposure post-processing effects to the lit scene.

### The addition rendering pipeline

This pipeline has only one render stage, and works with its viewport to combine textures from its child viewports.

- ToyMaker::AdditionRenderStage -- Takes textures attached to it as `"textureAddend_0"`, `"textureAddend_1"`, `"textureAddend_2"`, and so on, and blends them together into a single output texture.

## Important API

- ToyMaker::ViewportNode -- A developer's primary interface to the render system.  Exposes methods to configure the frequency of rendering updates, render target texture dimensions, skybox texture, and so on.

- ToyMaker::BaseOffscreenRenderStage and ToyMaker::BaseRenderStage -- Base classes for all render stages, including (in the future) custom developer-defined ones.

- ToyMaker::CameraProperties -- In association with a viewport: determines how much of the scene, and with what projection, is rendered by the render pipeline.

## Why does it exist?

If a tree were to fall in a forest, and there was no one to see it, then did it really fall?

The render system in its present state is a placeholder for a more robust and configurable render system later on.  The idea being that users define their render stages (as code, including their material properties and attribute layouts) and rendering pipelines (as configuration files or programmatically), and rely on the render system to run the pipeline when required.
