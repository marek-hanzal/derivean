/** @format */

import { createFileRoute, useLoaderData } from "@tanstack/react-router";
import { withList } from "@use-pico/client";
import { z } from "zod";
import { LinkPanel } from "~/app/game/GameMap2/Building/Link/LinkPanel";

export const Route = createFileRoute("/$locale/map/$mapId/building/$buildingId/link")({
	async loader({ context: { queryClient, kysely }, params: { mapId, buildingId } }) {
		return {
			link: await queryClient.ensureQueryData({
				queryKey: ["GameMap", mapId, "building", buildingId, "link"],
				async queryFn() {
					return kysely.transaction().execute(async (tx) => {
						return withList({
							select: tx
								.selectFrom("Building_To_Building as btb")
								.innerJoin("Building as b", "b.id", "btb.linkId")
								.innerJoin("Blueprint as bp", "bp.id", "b.blueprintId")
								.select(["btb.id", "bp.name"])
								.where("btb.buildingId", "=", buildingId)
								.orderBy("bp.name"),
							output: z.object({ id: z.string().min(1), name: z.string().min(1) }),
						});
					});
				},
			}),
		};
	},
	component() {
		const { building } = useLoaderData({ from: "/$locale/map/$mapId/building/$buildingId" });
		const { link } = Route.useLoaderData();

		return (
			<LinkPanel
				link={link}
				building={building}
			/>
		);
	},
});
