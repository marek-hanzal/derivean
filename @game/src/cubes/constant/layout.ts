// Where each field of a packed face lies: the word it is in and the bit it starts at. packFaces writes this
// layout and every shader stage reads it through faceNodes.
//   0  plane | lowU << 16            2  sizeV | material << 16 | occlusion << 24
//   1  lowV | sizeU << 16            3  axis | negative << 2 | level << 3 | rise << 7 | cap << 15
export const Face = {
  plane: { word: 0, shift: 0 },
  lowU: { word: 0, shift: 16 },
  lowV: { word: 1, shift: 0 },
  sizeU: { word: 1, shift: 16 },
  sizeV: { word: 2, shift: 0 },
  material: { word: 2, shift: 16 },
  occlusion: { word: 2, shift: 24 },
  axis: { word: 3, shift: 0 },
  negative: { word: 3, shift: 2 },
  level: { word: 3, shift: 3 },
  rise: { word: 3, shift: 7 },
  cap: { word: 3, shift: 15 },
} as const;

// Words a face takes.
export const FaceWords = 4;

// The axis of a face that looks up or down.
export const Upright = 1;
