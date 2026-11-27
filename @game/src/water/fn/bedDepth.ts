import { DepthTexture, FloatType, type Material, type Texture, ViewportDepthTextureNode } from "three/webgpu";
import { screenUV } from "three/tsl";

// The scene's depth under a pixel as `material` is drawn, from a copy of the depth buffer made before it draws.
// three (0.187, ViewportTextureNode.getTextureForReference) makes the copy in a clone of the node's texture for
// every render target it is drawn into and keeps the clones until they are disposed, which nothing in three does:
// they are disposed with the material here. The node is used as it is; `.sample()`, `.level()` and the like make
// nodes whose copies bypass this.
export function bedDepth(material: Material) {
  // A float like the depth buffer it is copied from.
  const sceneDepth = new DepthTexture(1, 1, FloatType);
  const node = new ViewportDepthTextureNode(screenUV, null, sceneDepth);
  const copies = new Set<Texture>([sceneDepth]);
  const copyFor = node.getTextureForReference.bind(node);
  node.getTextureForReference = (reference) => {
    const copy = copyFor(reference);
    copies.add(copy);
    return copy;
  };
  material.addEventListener("dispose", () => {
    for (const copy of copies) copy.dispose();
    copies.clear();
  });
  return node;
}
