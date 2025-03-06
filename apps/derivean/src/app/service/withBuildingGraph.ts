/** @format */

import type { WithTransaction } from "@derivean/db";
import Graph from "graphology";

export namespace withBuildingGraph {
	export interface Node {
		type: "building" | "route";
	}

	export interface Edge {
		type: "building-route" | "route-route";
		length: number;
		weight: number;
	}

	export type BuildingGraph = Graph<Node, Edge>;

	export interface Props {
		tx: WithTransaction;
		userId: string;
		mapId: string;
	}
}

export const withBuildingGraph = async ({ tx, userId, mapId }: withBuildingGraph.Props) => {
	const buildings = await tx
		.selectFrom("Building as b")
		.innerJoin("Plot as p", "p.id", "b.plotId")
		.select(["b.id"])
		.where("b.userId", "=", userId)
		.where("p.mapId", "=", mapId)
		.execute();

	const graph = new Graph<withBuildingGraph.Node, withBuildingGraph.Edge>({
		allowSelfLoops: true,
		multi: false,
		type: "undirected",
	});

	return { buildings, graph };
};
