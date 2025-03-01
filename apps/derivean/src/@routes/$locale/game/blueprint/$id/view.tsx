/** @format */

import { BlueprintCard } from "@derivean/game-ui";
import { createFileRoute, useLoaderData } from "@tanstack/react-router";

export const Route = createFileRoute("/$locale/game/blueprint/$id/view")({
	component() {
		const { entity } = useLoaderData({ from: "/$locale/game/blueprint/$id" });

		return (
			<div className={"w-1/2 mx-auto"}>
				<BlueprintCard entity={entity} />
			</div>
		);
	},
});
