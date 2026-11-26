import {
  BoxGeometry, EdgesGeometry, Group, LineBasicMaterial, LineSegments, Mesh, MeshBasicMaterial, SphereGeometry,
} from "three/webgpu";
import type { BrushShape, Point, WorldInfo } from "../../../contract/GameBridge";

export function createBrushIndicator(content: Group, world: WorldInfo) {
  const cube = new BoxGeometry(1, 1, 1);
  const edges = new EdgesGeometry(cube);
  const sphere = new SphereGeometry(0.5, 28, 16);
  const lineMaterial = new LineBasicMaterial({ color: "#ffe5a5", transparent: true, opacity: 0.8, depthWrite: false });
  const brushMaterial = new MeshBasicMaterial({
    color: "#ffdb88", transparent: true, opacity: 0.16, depthWrite: false, polygonOffset: true, polygonOffsetFactor: -2,
  });
  const wire = new LineSegments(edges, lineMaterial);
  const cubeBrush = new Mesh(cube, brushMaterial);
  const sphereBrush = new Mesh(sphere, brushMaterial);
  const indicator = new Group();
  indicator.add(wire, cubeBrush, sphereBrush);
  wire.scale.setScalar(world.voxelSize + 0.003);
  indicator.visible = false;
  content.add(indicator);

  return {
    show(point: Point | undefined, shape: BrushShape, radius: number) {
      indicator.visible = point !== undefined;
      if (!point) return;
      indicator.position.set(...point).addScalar(0.5).multiplyScalar(world.voxelSize);
      cubeBrush.visible = shape === "cube";
      sphereBrush.visible = shape === "sphere";
      cubeBrush.scale.setScalar((2 * radius - 1) * world.voxelSize + 0.004);
      sphereBrush.scale.setScalar(2 * radius * world.voxelSize);
    },
    dispose() {
      content.remove(indicator);
      for (const resource of [cube, edges, sphere, lineMaterial, brushMaterial]) resource.dispose();
    },
  };
}
