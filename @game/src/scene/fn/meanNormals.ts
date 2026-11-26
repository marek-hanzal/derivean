// Mean surface direction around each vertex of a voxel face list, as normalised int8 xyz per face corner.
// Faces add their area-weighted normal to every coarse cell they touch; a staircase of treads and risers thus
// averages to the normal of the slope it approximates. The voxel surface shades with it beyond the range at
// which single voxels can be told apart, where it matches the smooth far terrain.
const CellVoxels = 4;

export function meanNormals(faces: Int16Array) {
  const quads = faces.length / 8;
  const low = [Infinity, Infinity, Infinity];
  const high = [-Infinity, -Infinity, -Infinity];
  for (let q = 0; q < quads; q++) {
    const i = q * 8;
    const axis = faces[i];
    const u = (axis + 1) % 3;
    const v = (axis + 2) % 3;
    low[axis] = Math.min(low[axis], faces[i + 2]);
    high[axis] = Math.max(high[axis], faces[i + 2]);
    low[u] = Math.min(low[u], faces[i + 3]);
    high[u] = Math.max(high[u], faces[i + 3] + faces[i + 5]);
    low[v] = Math.min(low[v], faces[i + 4]);
    high[v] = Math.max(high[v], faces[i + 4] + faces[i + 6]);
  }
  const size = low.map((from, axis) => Math.max(1, Math.ceil((high[axis] - from) / CellVoxels)));
  const cell = (axis: number, voxel: number) => Math.max(0, Math.min(size[axis] - 1, Math.floor((voxel - low[axis]) / CellVoxels)));
  const at = (x: number, y: number, z: number) => ((z * size[1] + y) * size[0] + x) * 3;
  const sums = new Float32Array(size[0] * size[1] * size[2] * 3);
  const index = [0, 0, 0];

  for (let q = 0; q < quads; q++) {
    const i = q * 8;
    const axis = faces[i];
    const u = (axis + 1) % 3;
    const v = (axis + 2) % 3;
    index[axis] = cell(axis, faces[i + 2] - (faces[i + 1] > 0 ? 1 : 0));
    const lastU = cell(u, faces[i + 3] + faces[i + 5] - 1);
    const lastV = cell(v, faces[i + 4] + faces[i + 6] - 1);
    for (let cu = cell(u, faces[i + 3]); cu <= lastU; cu++) {
      const spanU = Math.min(faces[i + 3] + faces[i + 5], low[u] + (cu + 1) * CellVoxels) - Math.max(faces[i + 3], low[u] + cu * CellVoxels);
      for (let cv = cell(v, faces[i + 4]); cv <= lastV; cv++) {
        const spanV = Math.min(faces[i + 4] + faces[i + 6], low[v] + (cv + 1) * CellVoxels) - Math.max(faces[i + 4], low[v] + cv * CellVoxels);
        index[u] = cu;
        index[v] = cv;
        sums[at(index[0], index[1], index[2]) + axis] += faces[i + 1] * spanU * spanV;
      }
    }
  }

  const blurred = new Float32Array(sums.length);
  for (let z = 0; z < size[2]; z++) {
    for (let y = 0; y < size[1]; y++) {
      for (let x = 0; x < size[0]; x++) {
        const out = at(x, y, z);
        for (let nz = Math.max(0, z - 1); nz <= Math.min(size[2] - 1, z + 1); nz++) {
          for (let ny = Math.max(0, y - 1); ny <= Math.min(size[1] - 1, y + 1); ny++) {
            for (let nx = Math.max(0, x - 1); nx <= Math.min(size[0] - 1, x + 1); nx++) {
              const from = at(nx, ny, nz);
              blurred[out] += sums[from];
              blurred[out + 1] += sums[from + 1];
              blurred[out + 2] += sums[from + 2];
            }
          }
        }
      }
    }
  }

  // Corners read the blurred cells trilinearly, so the direction varies smoothly across a face and between faces.
  const normals = new Int8Array(quads * 12);
  const corner = [0, 0, 0];
  const base = [0, 0, 0];
  const share = [0, 0, 0];
  const mean = [0, 0, 0];
  for (let q = 0; q < quads; q++) {
    const i = q * 8;
    const axis = faces[i];
    const u = (axis + 1) % 3;
    const v = (axis + 2) % 3;
    for (let index = 0; index < 4; index++) {
      corner[axis] = faces[i + 2];
      corner[u] = faces[i + 3] + (index === 1 || index === 2 ? faces[i + 5] : 0);
      corner[v] = faces[i + 4] + (index >= 2 ? faces[i + 6] : 0);
      for (let k = 0; k < 3; k++) {
        const place = Math.max(0, Math.min(size[k] - 1, (corner[k] - low[k]) / CellVoxels - 0.5));
        base[k] = Math.min(Math.max(0, size[k] - 2), Math.floor(place));
        share[k] = size[k] > 1 ? place - base[k] : 0;
      }
      mean.fill(0);
      for (let n = 0; n < 8; n++) {
        const dx = n & 1, dy = n >> 1 & 1, dz = n >> 2;
        const weight = (dx ? share[0] : 1 - share[0]) * (dy ? share[1] : 1 - share[1]) * (dz ? share[2] : 1 - share[2]);
        if (weight === 0) continue;
        const from = at(base[0] + dx, base[1] + dy, base[2] + dz);
        for (let k = 0; k < 3; k++) mean[k] += blurred[from + k] * weight;
      }
      const out = (q * 4 + index) * 3;
      const length = Math.hypot(mean[0], mean[1], mean[2]);
      // Opposed faces can cancel (a thin wall); keep the face's own direction there.
      if (length < 0.5) normals[out + axis] = faces[i + 1] * 127;
      else for (let k = 0; k < 3; k++) normals[out + k] = Math.round(mean[k] / length * 127);
    }
  }
  return normals;
}
