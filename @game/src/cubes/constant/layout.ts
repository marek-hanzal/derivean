// Where each field of a packed face lies: the word it is in, the bit it starts at and how many bits it takes.
// packFaces writes this layout and every shader stage reads it through faceNodes.
//   0  plane | lowU << 16            2  sizeV | material << 16 | occlusion << 24
//   1  lowV | sizeU << 16            3  axis | negative << 2 | level << 3 | rise << 7 | cap << 15
export const Face = {
  plane: { word: 0, shift: 0, bits: 16 },
  lowU: { word: 0, shift: 16, bits: 16 },
  lowV: { word: 1, shift: 0, bits: 16 },
  sizeU: { word: 1, shift: 16, bits: 16 },
  sizeV: { word: 2, shift: 0, bits: 16 },
  material: { word: 2, shift: 16, bits: 8 },
  occlusion: { word: 2, shift: 24, bits: 8 },
  axis: { word: 3, shift: 0, bits: 2 },
  negative: { word: 3, shift: 2, bits: 1 },
  level: { word: 3, shift: 3, bits: 4 },
  rise: { word: 3, shift: 7, bits: 8 },
  cap: { word: 3, shift: 15, bits: 1 },
} as const;

export type FaceField = (typeof Face)[keyof typeof Face];

// The largest number a field holds.
export const fieldMask = ({ bits }: FaceField) => 2 ** bits - 1;

// Words a face takes.
export const FaceWords = 4;

// The axis of a face that looks up or down.
export const Upright = 1;
