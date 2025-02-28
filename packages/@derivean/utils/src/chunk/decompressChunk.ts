import { deserialize } from "borsh";
import { decompressSync } from "fflate";
import type { Chunk } from "./Chunk";
import { ChunkBorsh } from "./ChunkBorsh";

export const decompressChunk = (data: Uint8Array): Chunk.Data => {
	return deserialize(ChunkBorsh, decompressSync(data)) as Chunk.Data;
};
