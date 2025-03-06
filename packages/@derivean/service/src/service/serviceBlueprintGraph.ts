/** @format */

import type { WithTransaction } from "@derivean/db";
import { DepGraph } from "dependency-graph";

export namespace serviceBlueprintGraph {
	export type Result = DepGraph<string>;

	export interface Props {
		tx: WithTransaction;
	}
}

export const serviceBlueprintGraph = async ({
	tx,
}: serviceBlueprintGraph.Props): Promise<serviceBlueprintGraph.Result> => {
	const graph = new DepGraph<string>({ circular: false });

	for await (const { id, name } of await tx
		.selectFrom("Blueprint")
		.select(["id", "name"])
		.execute()) {
		graph.addNode(id, name);
	}

	for await (const { blueprintId, dependencyId } of await tx
		.selectFrom("Blueprint_Dependency")
		.select(["blueprintId", "dependencyId"])
		.execute()) {
		graph.addDependency(blueprintId, dependencyId);
	}

	return graph;
};
