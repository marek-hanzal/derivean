import { createFileRoute, useLoaderData } from "@tanstack/react-router";
import { BlueprintCard } from "~/app/root/BlueprintCard";
import { withBlueprintGraph } from "~/app/utils/withBlueprintGraph";

export const Route = createFileRoute("/$locale/root/blueprint/$id/view")({
	async loader({ context: { kysely } }) {
		return {
			dependencies: await kysely.transaction().execute(async (tx) => {
				return withBlueprintGraph({ tx });
			}),
		};
	},
	component() {
		const { dependencies } = Route.useLoaderData();
		const { entity } = useLoaderData({
			from: "/$locale/root/blueprint/$id",
		});

		return (
			<div className={"w-1/2 mx-auto"}>
				<BlueprintCard
					dependencies={dependencies}
					entity={entity}
				/>
			</div>
		);
	},
});
