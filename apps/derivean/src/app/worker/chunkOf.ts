/** @format */

import { Chunk as WasmChunk } from "@derivean/chunk";
import { withGenerator } from "@derivean/terrain";
import { compressChunk, decompressChunk, type Chunk } from "@derivean/utils";
import { file, write } from "opfs-tools";
import { worker } from "workerpool";
import { gameConfig } from "~/app/gameConfig";

export namespace chunkOf {
	export interface Props {
		id: string;
		mapId: string;
		level: Chunk.View.Level;
		x: number;
		z: number;
	}

	export interface Result {
		hit: boolean;
		chunk: Chunk.Data;
	}
}

export async function chunkOf({ id, mapId, level, x, z }: chunkOf.Props): Promise<chunkOf.Result> {
	const generator = withGenerator({ gameConfig, chunk: new WasmChunk(mapId), level });

	const chunkFile = `/chunk/${mapId}/${id}.bin`;

	return new Promise<chunkOf.Result>((resolve) => {
		file(chunkFile)
			.exists()
			.then((exists) => {
				(exists
					? /**
                     * // new Promise<boolean>((resolve) => {
						// 	resolve(true);
						// })
                     */
						write(chunkFile, compressChunk(generator({ x, z }))).then(() => false)
					: write(chunkFile, compressChunk(generator({ x, z }))).then(() => false)
				).then((hit) => {
					file(chunkFile)
						.arrayBuffer()
						.then((buffer) => {
							resolve({ hit, chunk: decompressChunk(new Uint8Array(buffer)) });
						});
				});
			});
	});
}

worker({ chunkOf });
