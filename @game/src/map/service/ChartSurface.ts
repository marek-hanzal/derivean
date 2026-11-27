/// <reference types="@webgpu/types" />
import type { Chart } from "../../../contract/GameBridge";
import { ChartUniformBytes, ChartWgsl, StretchWgsl } from "../fn/chartShader";
import type { ChartLook } from "../type/ChartLook";
import type { Extent, MapView } from "../type/MapView";

export interface ChartStyle {
  seaLevel: number;
  // Metres between contour lines.
  interval: number;
}

// Where a sheet of rasters lies, its north-west corner and metres per texel, and how it is drawn.
export interface SheetPlace {
  origin: [number, number];
  pitch: number;
  // The chart could have read tiles: ground it only knows from the planet is marked as such.
  markPlanet: boolean;
  look: ChartLook;
}

interface Sheet extends SheetPlace { relief: GPUTexture; climate: GPUTexture; bindings: GPUBindGroup; size: number }

// The largest finite 32-bit float: a stream floor of none (Infinity) as the shader can compare against.
const Widest32 = 3.4028234663852886e38;

// Draws charts on a canvas of its own, with the scene's WebGPU device. A chart's rasters go to the GPU in two
// sheets of four channels (chartShader), and are coloured there: the page's thread only interleaves each pane
// into them as it arrives and writes that part of the sheets, never more than a pane at a time.
// `failed` hears of a shader or pipeline the device refused; the surface then draws nothing.
export function createChartSurface(canvas: HTMLCanvasElement, device: GPUDevice, failed: (error: unknown) => void) {
  const context = canvas.getContext("webgpu");
  if (!context) throw new Error("The map needs a WebGPU canvas");
  const format = navigator.gpu.getPreferredCanvasFormat();
  context.configure({ device, format, alphaMode: "opaque" });
  let broken = false;
  device.pushErrorScope("validation");

  const chartModule = device.createShaderModule({ label: "chart", code: ChartWgsl });
  const sheetLayout = device.createBindGroupLayout({
    entries: [
      { binding: 0, visibility: GPUShaderStage.FRAGMENT, buffer: {} },
      { binding: 1, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: "unfilterable-float" } },
      { binding: 2, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: "unfilterable-float" } },
    ],
  });
  const chartPipeline = (target: GPUTextureFormat) => device.createRenderPipeline({
    label: "chart",
    layout: device.createPipelineLayout({ bindGroupLayouts: [sheetLayout] }),
    vertex: { module: chartModule, entryPoint: "corner" },
    fragment: { module: chartModule, entryPoint: "paint", targets: [{ format: target }] },
  });
  const sharpPipeline = chartPipeline(format);
  const lesserPipeline = chartPipeline("rgba8unorm");
  const stretchModule = device.createShaderModule({ label: "chart stretch", code: StretchWgsl });
  const stretchPipeline = device.createRenderPipeline({
    label: "chart stretch",
    layout: "auto",
    vertex: { module: stretchModule, entryPoint: "corner" },
    fragment: { module: stretchModule, entryPoint: "stretch", targets: [{ format }] },
  });

  void device.popErrorScope().then((error) => {
    if (!error) return;
    broken = true;
    failed(new Error(`Map shader: ${error.message}`));
  });

  const uniforms = device.createBuffer({ size: ChartUniformBytes, usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST });
  const words = new ArrayBuffer(ChartUniformBytes);
  const floats = new Float32Array(words);
  const integers = new Int32Array(words);
  const canvasSize = device.createBuffer({ size: 8, usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST });
  const canvasWords = new Float32Array(2);
  const stretched = device.createSampler({ magFilter: "linear", minFilter: "linear" });

  const raster = (size: number) => device.createTexture({
    size: [size, size], format: "rgba32float", usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST,
  });
  // Textures of size² texels, cleared, and their bindings.
  function rasters(size: number) {
    const [relief, climate] = [raster(size), raster(size)];
    const bindings = device.createBindGroup({
      layout: sheetLayout,
      entries: [{ binding: 0, resource: { buffer: uniforms } }, { binding: 1, resource: relief.createView() }, { binding: 2, resource: climate.createView() }],
    });
    return { relief, climate, bindings };
  }
  function allot(sheet: Sheet, size: number) {
    sheet.relief.destroy();
    sheet.climate.destroy();
    Object.assign(sheet, rasters(size), { size });
  }
  // A sheet that holds nothing yet, with textures of one texel so that it can be bound.
  function sheet(): Sheet {
    return { origin: [0, 0], pitch: 1, size: 0, markPlanet: false, look: { streamFloor: Infinity, relief: 1 }, ...rasters(1) };
  }
  // The sheet on screen, the one being filled for the next chart, and one to spare: coarse and sharp charts
  // take turns, and with a third sheet each finds one of its own size without allocating textures again.
  let front = sheet();
  let back = sheet();
  let spare = sheet();
  let charted = false;
  // A pane's rasters interleaved as the sheets hold them, kept for the next pane of the same size.
  let staging = { size: 0, relief: new Float32Array(0), climate: new Float32Array(0) };

  // A view in motion is drawn into this texture at one pixel per CSS pixel and stretched onto the canvas, which
  // keeps its size: on a dense display that is a fraction of the work, and resizing a canvas costs frames.
  let reduced: { texture: GPUTexture; bindings: GPUBindGroup; width: number; height: number } | undefined;

  return {
    // Starts the next chart: a sheet of size² texels that `put` fills pane by pane, all of it, and `show`
    // brings on screen.
    begin(size: number) {
      if (back.size !== size && spare.size === size) [back, spare] = [spare, back];
      if (back.size !== size) allot(back, size);
    },
    // Writes one pane with its north-west pixel at (column, row) of the sheet begun last.
    put(pane: Chart, column: number, row: number) {
      const texels = pane.size * pane.size;
      if (staging.size !== pane.size) staging = { size: pane.size, relief: new Float32Array(texels * 4), climate: new Float32Array(texels * 4) };
      const { relief, climate } = staging;
      const { ground, discharge, level, waters, water, ice, warmth, cover } = pane;
      for (let texel = 0, at = 0; texel < texels; texel++, at += 4) {
        relief[at] = ground[texel];
        relief[at + 1] = discharge[texel];
        relief[at + 2] = level[texel];
        relief[at + 3] = waters[texel];
        climate[at] = water[texel];
        climate[at + 1] = ice[texel];
        climate[at + 2] = warmth[texel];
        climate[at + 3] = cover[texel];
      }
      const layout = { bytesPerRow: pane.size * 16, rowsPerImage: pane.size };
      device.queue.writeTexture({ texture: back.relief, origin: [column, row] }, relief, layout, [pane.size, pane.size]);
      device.queue.writeTexture({ texture: back.climate, origin: [column, row] }, climate, layout, [pane.size, pane.size]);
    },
    show(place: SheetPlace) {
      Object.assign(back, place);
      [front, back, spare] = [back, spare, front];
      charted = true;
    },
    // Nothing is on screen until the next `show`.
    clear() { charted = false; },

    // Draws the chart on screen for a view. `sharp` fills every pixel of the canvas; otherwise one in each CSS pixel.
    draw(view: MapView, [width, height]: Extent, style: ChartStyle, sharp: boolean) {
      if (broken) return;
      const full = [Math.max(1, Math.round(width * devicePixelRatio)), Math.max(1, Math.round(height * devicePixelRatio))];
      if (canvas.width !== full[0]) canvas.width = full[0];
      if (canvas.height !== full[1]) canvas.height = full[1];
      const lesser = !sharp && devicePixelRatio > 1;
      const [w, h] = lesser ? [Math.max(1, Math.round(width)), Math.max(1, Math.round(height))] : full;
      if (lesser && (reduced?.width !== w || reduced?.height !== h)) {
        reduced?.texture.destroy();
        const texture = device.createTexture({ size: [w, h], format: "rgba8unorm", usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING });
        const bindings = device.createBindGroup({
          layout: stretchPipeline.getBindGroupLayout(0),
          entries: [{ binding: 0, resource: { buffer: canvasSize } }, { binding: 1, resource: texture.createView() }, { binding: 2, resource: stretched }],
        });
        reduced = { texture, bindings, width: w, height: h };
      }
      const ratio = w / width;
      // World metres leave JavaScript numbers here, as offsets from the chart's corner in texels.
      floats[0] = (view.x - width / 2 * view.scale - front.origin[0]) / front.pitch;
      floats[1] = (view.z - height / 2 * view.scale - front.origin[1]) / front.pitch;
      floats[2] = view.scale / ratio / front.pitch;
      integers[3] = charted ? front.size : 0;
      floats[4] = front.pitch;
      floats[5] = style.seaLevel;
      floats[6] = style.interval;
      floats[7] = front.look.relief;
      const floor = Math.min(front.look.streamFloor, Widest32);
      floats[8] = floor;
      floats[9] = Math.log2(floor);
      floats[10] = front.markPlanet ? 1 : 0;
      floats[11] = ratio;
      device.queue.writeBuffer(uniforms, 0, words);

      const encoder = device.createCommandEncoder({ label: "chart" });
      const onScreen = context.getCurrentTexture().createView();
      const chartPass = encoder.beginRenderPass({
        colorAttachments: [{ view: lesser ? reduced!.texture.createView() : onScreen, loadOp: "clear", storeOp: "store", clearValue: [0, 0, 0, 1] }],
      });
      chartPass.setPipeline(lesser ? lesserPipeline : sharpPipeline);
      chartPass.setBindGroup(0, front.bindings);
      chartPass.draw(3);
      chartPass.end();
      if (lesser) {
        canvasWords[0] = full[0];
        canvasWords[1] = full[1];
        device.queue.writeBuffer(canvasSize, 0, canvasWords);
        const stretchPass = encoder.beginRenderPass({ colorAttachments: [{ view: onScreen, loadOp: "clear", storeOp: "store", clearValue: [0, 0, 0, 1] }] });
        stretchPass.setPipeline(stretchPipeline);
        stretchPass.setBindGroup(0, reduced!.bindings);
        stretchPass.draw(3);
        stretchPass.end();
      }
      device.queue.submit([encoder.finish()]);
    },
  };
}

export type ChartSurface = ReturnType<typeof createChartSurface>;
