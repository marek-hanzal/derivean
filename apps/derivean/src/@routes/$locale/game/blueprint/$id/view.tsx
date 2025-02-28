import { createFileRoute, useLoaderData } from "@tanstack/react-router";
import { BlueprintCard } from "~/app/game/BlueprintCard";

export const Route = createFileRoute("/$locale/game/blueprint/$id/view")({
	component() {
		const { entity } = useLoaderData({
			from: "/$locale/game/blueprint/$id",
		});

		return (
			<div className={"w-1/2 mx-auto"}>
				<BlueprintCard entity={entity} />
			</div>
		);
	},
});
