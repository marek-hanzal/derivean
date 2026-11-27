import {
  BoxGeometry, DynamicDrawUsage, type Group, InstancedMesh, MeshStandardMaterial,
} from "three/webgpu";
import type { Point } from "../../../contract/GameBridge";

interface ParticleGroup {
  material: MeshStandardMaterial;
  mesh?: InstancedMesh;
  capacity: number;
  count: number;
}

// Particle positions arrive as float32 offsets from an origin the engine sends in doubles; the mesh position
// carries that origin, so the offsets go into the float32 instance matrices as they are. A particle's matrix is a
// scale and a move: an instanced mesh starts every matrix as the identity, so only those six numbers are written,
// and an update sends up only the matrices drawn.
export function createParticleScene(content: Group) {
  const cube = new BoxGeometry(1, 1, 1);
  const groups: ParticleGroup[] = [
    new MeshStandardMaterial({ color: "#60cdda", roughness: 0.24, transparent: true, opacity: 0.77 }),
    new MeshStandardMaterial({ color: "#c3a478", roughness: 0.98 }),
    new MeshStandardMaterial({ color: "#e8f9e9", emissive: "#3d5251", roughness: 0.93 }),
  ].map((material) => ({
    material,
    capacity: 0,
    count: 0,
  }));

  function apply(particles: Float32Array, origin: Point) {
    for (const group of groups) group.count = 0;
    for (let i = 0; i < particles.length; i += 5) groups[particles[i + 4]].count++;

    for (const group of groups) {
      if (group.count > group.capacity) {
        if (group.mesh) {
          content.remove(group.mesh);
          group.mesh.dispose();
        }
        group.capacity = 2 ** Math.ceil(Math.log2(group.count));
        const mesh = new InstancedMesh(cube, group.material, group.capacity);
        mesh.instanceMatrix.setUsage(DynamicDrawUsage);
        mesh.frustumCulled = false;
        mesh.renderOrder = group.material.transparent ? 1 : 0;
        content.add(mesh);
        group.mesh = mesh;
      }
      if (group.mesh) {
        group.mesh.count = group.count;
        group.mesh.visible = group.count > 0;
        group.mesh.position.set(...origin);
      }
      group.count = 0;
    }

    for (let i = 0; i < particles.length; i += 5) {
      const group = groups[particles[i + 4]];
      if (!group.mesh) continue;
      const matrices = group.mesh.instanceMatrix.array as Float32Array;
      const at = group.count++ * 16;
      const scale = particles[i + 3];
      matrices[at] = matrices[at + 5] = matrices[at + 10] = scale;
      matrices[at + 12] = particles[i];
      matrices[at + 13] = particles[i + 1];
      matrices[at + 14] = particles[i + 2];
    }
    for (const group of groups) {
      if (!group.mesh?.count) continue;
      // Matrices past the count are not drawn, so a range left from a frame not yet drawn may go.
      group.mesh.instanceMatrix.clearUpdateRanges();
      group.mesh.instanceMatrix.addUpdateRange(0, group.mesh.count * 16);
      group.mesh.instanceMatrix.needsUpdate = true;
    }
  }

  return {
    apply,
    dispose() {
      for (const { mesh, material } of groups) {
        if (mesh) {
          content.remove(mesh);
          mesh.dispose();
        }
        material.dispose();
      }
      cube.dispose();
    },
  };
}
