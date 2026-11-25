import {
  BoxGeometry, DynamicDrawUsage, InstancedMesh, Matrix4, MeshStandardMaterial, type Scene,
} from "three";

interface ParticleGroup {
  material: MeshStandardMaterial;
  mesh?: InstancedMesh;
  capacity: number;
  count: number;
}

export function createParticleScene(scene: Scene) {
  const cube = new BoxGeometry(1, 1, 1);
  const matrix = new Matrix4();
  const groups: ParticleGroup[] = [
    new MeshStandardMaterial({ color: "#60cdda", roughness: 0.24, transparent: true, opacity: 0.77 }),
    new MeshStandardMaterial({ color: "#c3a478", roughness: 0.98 }),
    new MeshStandardMaterial({ color: "#e8f9e9", emissive: "#3d5251", roughness: 0.93 }),
  ].map((material) => ({
    material,
    capacity: 0,
    count: 0,
  }));

  function apply(particles: Float32Array) {
    for (const group of groups) group.count = 0;
    for (let i = 0; i < particles.length; i += 5) groups[particles[i + 4]].count++;

    for (const group of groups) {
      if (group.count > group.capacity) {
        if (group.mesh) {
          scene.remove(group.mesh);
          group.mesh.dispose();
        }
        group.capacity = 2 ** Math.ceil(Math.log2(group.count));
        const mesh = new InstancedMesh(cube, group.material, group.capacity);
        mesh.instanceMatrix.setUsage(DynamicDrawUsage);
        mesh.frustumCulled = false;
        mesh.renderOrder = group.material.transparent ? 1 : 0;
        scene.add(mesh);
        group.mesh = mesh;
      }
      if (group.mesh) {
        group.mesh.count = group.count;
        group.mesh.visible = group.count > 0;
      }
      group.count = 0;
    }

    for (let i = 0; i < particles.length; i += 5) {
      const group = groups[particles[i + 4]];
      const scale = particles[i + 3];
      matrix.makeScale(scale, scale, scale).setPosition(particles[i], particles[i + 1], particles[i + 2]);
      group.mesh?.setMatrixAt(group.count++, matrix);
    }
    for (const group of groups) {
      if (group.mesh?.count) group.mesh.instanceMatrix.needsUpdate = true;
    }
  }

  return { apply };
}
