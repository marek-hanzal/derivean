import type { Chart } from "../../../contract/GameBridge";
import { FloatRasters, Rasters } from "../constant/chart";
import { FragmentShader, VertexShader } from "../fn/chartShader";
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

interface Sheet extends SheetPlace { textures: WebGLTexture[]; size: number }

const Uniforms = ["uResolution", "uTexelOrigin", "uTexelPerPixel", "uSize", "uPitch", "uSeaLevel", "uInterval", "uRelief", "uStreamFloor",
  "uMarkPlanet", "uPixelRatio"] as const;

// Draws charts on a canvas of its own. A chart's rasters go to the GPU as they are, one single-channel texture
// each, and are coloured there: the page's thread only hands each pane on, never more than a pane at a time.
// This is plain WebGL 2 and not the scene's Three.js renderer because panes are written into parts of float
// textures as they arrive, which a Three.js data texture can only do by uploading itself whole.
export function createChartSurface(canvas: HTMLCanvasElement) {
  const made = canvas.getContext("webgl2", { alpha: false, antialias: false, depth: false, stencil: false });
  if (!made) throw new Error("The map needs WebGL 2");
  const gl = made;

  function compile(kind: number, source: string) {
    const shader = gl.createShader(kind)!;
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) throw new Error(`Map shader: ${gl.getShaderInfoLog(shader)}`);
    return shader;
  }
  const program = gl.createProgram()!;
  gl.attachShader(program, compile(gl.VERTEX_SHADER, VertexShader));
  gl.attachShader(program, compile(gl.FRAGMENT_SHADER, FragmentShader));
  gl.linkProgram(program);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) throw new Error(`Map shader: ${gl.getProgramInfoLog(program)}`);
  gl.useProgram(program);
  Rasters.forEach((name, unit) => gl.uniform1i(gl.getUniformLocation(program, `u_${name}`), unit));
  const uniform = Object.fromEntries(Uniforms.map((name) => [name, gl.getUniformLocation(program, name)])) as Record<typeof Uniforms[number], WebGLUniformLocation | null>;
  gl.pixelStorei(gl.UNPACK_ALIGNMENT, 1);

  const scratch = gl.TEXTURE0 + Rasters.length;
  const float = (unit: number) => unit < FloatRasters;

  function plain(texture: WebGLTexture) {
    gl.activeTexture(scratch);
    gl.bindTexture(gl.TEXTURE_2D, texture);
    for (const filter of [gl.TEXTURE_MIN_FILTER, gl.TEXTURE_MAG_FILTER]) gl.texParameteri(gl.TEXTURE_2D, filter, gl.NEAREST);
    for (const wrap of [gl.TEXTURE_WRAP_S, gl.TEXTURE_WRAP_T]) gl.texParameteri(gl.TEXTURE_2D, wrap, gl.CLAMP_TO_EDGE);
    return texture;
  }
  function sheet(): Sheet {
    return { textures: Rasters.map(() => plain(gl.createTexture()!)), origin: [0, 0], pitch: 1, size: 0, markPlanet: false, look: { streamFloor: Infinity, relief: 1 } };
  }
  // The sheet on screen, the one being filled for the next chart, and one to spare: coarse and sharp charts
  // take turns, and with a third sheet each finds one of its own size without allocating textures again.
  let front = sheet();
  let back = sheet();
  let spare = sheet();
  let charted = false;

  // A view in motion is drawn into this buffer at one pixel per CSS pixel and stretched onto the canvas, which
  // keeps its size: on a dense display that is a fraction of the work, and resizing a canvas costs frames.
  const reduced = { buffer: gl.createFramebuffer()!, texture: plain(gl.createTexture()!), width: 0, height: 0 };
  gl.bindFramebuffer(gl.FRAMEBUFFER, reduced.buffer);
  gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, reduced.texture, 0);
  gl.bindFramebuffer(gl.FRAMEBUFFER, null);

  return {
    // Starts the next chart: a sheet of size² texels that `put` fills pane by pane, all of it, and `show`
    // brings on screen.
    begin(size: number) {
      if (back.size !== size && spare.size === size) [back, spare] = [spare, back];
      if (back.size === size) return;
      back.size = size;
      back.textures.forEach((texture, unit) => {
        gl.activeTexture(scratch);
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.texImage2D(gl.TEXTURE_2D, 0, float(unit) ? gl.R32F : gl.R8, size, size, 0, gl.RED, float(unit) ? gl.FLOAT : gl.UNSIGNED_BYTE, null);
      });
    },
    // Writes one pane with its north-west pixel at (column, row) of the sheet begun last.
    put(pane: Chart, column: number, row: number) {
      back.textures.forEach((texture, unit) => {
        gl.activeTexture(scratch);
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.texSubImage2D(gl.TEXTURE_2D, 0, column, row, pane.size, pane.size, gl.RED, float(unit) ? gl.FLOAT : gl.UNSIGNED_BYTE, pane[Rasters[unit]]);
      });
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
      const full = [Math.max(1, Math.round(width * devicePixelRatio)), Math.max(1, Math.round(height * devicePixelRatio))];
      if (canvas.width !== full[0]) canvas.width = full[0];
      if (canvas.height !== full[1]) canvas.height = full[1];
      const lesser = !sharp && devicePixelRatio > 1;
      const [w, h] = lesser ? [Math.max(1, Math.round(width)), Math.max(1, Math.round(height))] : full;
      if (lesser && (reduced.width !== w || reduced.height !== h)) {
        gl.activeTexture(scratch);
        gl.bindTexture(gl.TEXTURE_2D, reduced.texture);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA8, w, h, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
        [reduced.width, reduced.height] = [w, h];
      }
      gl.bindFramebuffer(gl.FRAMEBUFFER, lesser ? reduced.buffer : null);
      const ratio = w / width;
      gl.viewport(0, 0, w, h);
      front.textures.forEach((texture, unit) => {
        gl.activeTexture(gl.TEXTURE0 + unit);
        gl.bindTexture(gl.TEXTURE_2D, texture);
      });
      // World metres leave JavaScript numbers here, as offsets from the chart's corner in texels.
      gl.uniform2f(uniform.uTexelOrigin, (view.x - width / 2 * view.scale - front.origin[0]) / front.pitch,
        (view.z - height / 2 * view.scale - front.origin[1]) / front.pitch);
      gl.uniform1f(uniform.uTexelPerPixel, view.scale / ratio / front.pitch);
      gl.uniform2f(uniform.uResolution, w, h);
      gl.uniform1i(uniform.uSize, charted ? front.size : 0);
      gl.uniform1f(uniform.uPitch, front.pitch);
      gl.uniform1f(uniform.uSeaLevel, style.seaLevel);
      gl.uniform1f(uniform.uInterval, style.interval);
      gl.uniform1f(uniform.uRelief, front.look.relief);
      gl.uniform1f(uniform.uStreamFloor, front.look.streamFloor);
      gl.uniform1f(uniform.uMarkPlanet, front.markPlanet ? 1 : 0);
      gl.uniform1f(uniform.uPixelRatio, ratio);
      gl.drawArrays(gl.TRIANGLES, 0, 3);
      if (!lesser) return;
      gl.bindFramebuffer(gl.READ_FRAMEBUFFER, reduced.buffer);
      gl.bindFramebuffer(gl.DRAW_FRAMEBUFFER, null);
      gl.blitFramebuffer(0, 0, w, h, 0, 0, full[0], full[1], gl.COLOR_BUFFER_BIT, gl.LINEAR);
    },
  };
}

export type ChartSurface = ReturnType<typeof createChartSurface>;
