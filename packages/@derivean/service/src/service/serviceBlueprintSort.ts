/** @format */

import type { WithTransaction } from "@derivean/db";
import { serviceBlueprintGraph } from "./serviceBlueprintGraph";

export namespace serviceBlueprintSort {
	export interface Props {
		tx: WithTransaction;
	}
}

export const serviceBlueprintSort = async ({ tx }: serviceBlueprintSort.Props) => {
	const dependencies = serviceBlueprintGraph({ tx });

	const blueprints = await tx.selectFrom("Blueprint as bl").select(["bl.id"]).execute();

	for await (const blueprint of blueprints) {
		const deps = (await dependencies).dependenciesOf(blueprint.id);

		await tx
			.updateTable("Blueprint")
			.set({ sort: deps.length })
			.where("id", "=", blueprint.id)
			.execute();
	}
};
