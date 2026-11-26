import type { WebGLRenderer } from "three";

// Time the GPU spends between `begin` and `end`, read back a few frames later through the disjoint timer query
// extension. Where the extension is missing the reading stays null, which says so.
export function createGpuTimer(renderer: WebGLRenderer) {
  const gl = renderer.getContext() as WebGL2RenderingContext;
  const extension = gl.getExtension("EXT_disjoint_timer_query_webgl2");
  const waiting: WebGLQuery[] = [];
  let open: WebGLQuery | null = null;
  let milliseconds: number | null = null;

  return {
    get milliseconds() { return milliseconds; },
    begin() {
      if (!extension || open || waiting.length > 8) return;
      open = gl.createQuery();
      if (open) gl.beginQuery(extension.TIME_ELAPSED_EXT, open);
    },
    end() {
      if (!extension || !open) return;
      gl.endQuery(extension.TIME_ELAPSED_EXT);
      waiting.push(open);
      open = null;
      while (waiting.length && gl.getQueryParameter(waiting[0], gl.QUERY_RESULT_AVAILABLE)) {
        const query = waiting.shift()!;
        if (!gl.getParameter(extension.GPU_DISJOINT_EXT)) {
          const taken = gl.getQueryParameter(query, gl.QUERY_RESULT) / 1e6;
          milliseconds = milliseconds === null ? taken : milliseconds + (taken - milliseconds) * 0.1;
        }
        gl.deleteQuery(query);
      }
    },
  };
}

export type GpuTimer = ReturnType<typeof createGpuTimer>;
