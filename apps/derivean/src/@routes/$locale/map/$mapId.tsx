/** @format */

import { Engine } from "@derivean/engine";
import { createGameEventBus } from "@derivean/utils";
import { createFileRoute } from "@tanstack/react-router";
import { zodValidator } from "@tanstack/zod-adapter";
import { z } from "zod";
import { gameConfig } from "~/app/gameConfig";
import { Map } from "~/app/map/Map";

const SearchSchema = z.object({ zoomToId: z.string().optional() });

export const Route = createFileRoute("/$locale/map/$mapId")({
	validateSearch: zodValidator(SearchSchema),
	async loader({ context: { queryClient, kysely, session }, params: { mapId } }) {
		const user = await session();

		return {
			user,
			cycle: await queryClient.ensureQueryData({
				queryKey: ["map", mapId, "cycle"],
				async queryFn() {
					return kysely.transaction().execute(async (tx) => {
						return (
							await tx
								.selectFrom("Cycle as c")
								.select((eb) => eb.fn.count<number>("c.id").as("count"))
								.where("c.userId", "=", user.id)
								.executeTakeFirstOrThrow()
						).count;
					});
				},
			}),
		};
	},
	component() {
		const { mapId } = Route.useParams();
		/**
		 * Basic assumption is the whole page won't re-render, so it's (quite) safe to create the event bus here.
		 */
		const gameEventBus = createGameEventBus();
		const engine = new Engine(mapId);
		engine.foo();

		//console.log(`WASM memory usage is ${wasm.memory.buffer.byteLength} bytes`);

		return (
			<Map
				mapId={mapId}
				gameConfig={gameConfig}
				gameEventBus={gameEventBus}
				zoom={0.1}
			/>
		);
	},
});
