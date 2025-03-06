/** @format */

import { createFileRoute } from "@tanstack/react-router";
import { withList } from "@use-pico/client";
import { z } from "zod";
import { BuildingListPanel } from "~/app/game/GameMap2/Building/List/BuildingListPanel";

export const Route = createFileRoute("/$locale/map/$mapId/building/list")({
	async loader({ context: { queryClient, kysely }, params: { mapId } }) {
		return {
			building: await queryClient.ensureQueryData({
				queryKey: ["GameMap", mapId, "building", "list"],
				async queryFn() {
					return kysely.transaction().execute(async (tx) => {
						return withList({
							select: tx
								.selectFrom("Building as b")
								.innerJoin("Blueprint as bl", "bl.id", "b.blueprintId")
								.innerJoin("Plot as p", "p.id", "b.plotId")
								.select(["b.id", "bl.name"])
								.where("p.mapId", "=", mapId)
								.orderBy("bl.name", "asc"),
							output: z.object({ id: z.string().min(1), name: z.string().min(1) }),
						});
					});
				},
			}),
		};
	},
	component() {
		const { building } = Route.useLoaderData();

		return <BuildingListPanel building={building} />;
	},
});
