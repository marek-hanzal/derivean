import { serialize } from "borsh";
import { deflateSync } from "fflate";
import type { Chunk } from "./Chunk";
import { ChunkBorsh } from "./ChunkBorsh";

export const compressChunk = (chunk: Chunk.Data): Uint8Array => {
	return deflateSync(serialize(ChunkBorsh, chunk), {
		level: 9,
	});
};
