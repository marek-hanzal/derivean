/** @format */

import { transaction } from "@derivean/db";
import { genId } from "@use-pico/common";

export namespace withConstructionQueue {
	export interface Props {
		userId: string;
		blueprintId: string;
		plotId: string;
		plan: boolean;
		valid: boolean;
	}
}

export const withConstructionQueue = async ({
	userId,
	blueprintId,
	plotId,
	plan,
	valid,
}: withConstructionQueue.Props) => {
	return transaction(async (tx) => {
		const blueprint = await tx
			.selectFrom("Blueprint as b")
			.select(["b.cycles"])
			.where("b.id", "=", blueprintId)
			.executeTakeFirstOrThrow();

		/**
		 * TODO Check if a plot is free, so nothing else is built there (building/road/...)
		 */

		const building = await tx
			.insertInto("Building")
			.values({
				id: genId(),
				userId,
				blueprintId,
				constructionId: (
					await tx
						.insertInto("Construction")
						.values({ id: genId(), userId, cycle: 0, cycles: blueprint.cycles, plan })
						.returning("id")
						.executeTakeFirstOrThrow()
				).id,
				plotId,
				valid,
			})
			.returningAll()
			.executeTakeFirstOrThrow();

		const inventory = await tx
			.selectFrom("Inventory as i")
			.select(["i.amount", "i.limit", "i.resourceId", "i.type"])
			.where(
				"i.id",
				"in",
				tx
					.selectFrom("Blueprint_Inventory as bi")
					.select("bi.inventoryId")
					.where("bi.blueprintId", "=", blueprintId),
			)
			.execute();

		for await (const { amount, limit, resourceId, type } of inventory) {
			await tx
				.insertInto("Building_Inventory")
				.values({
					id: genId(),
					buildingId: building.id,
					inventoryId: (
						await tx
							.insertInto("Inventory")
							.values({ id: genId(), amount, limit, resourceId, type })
							.returning("id")
							.executeTakeFirstOrThrow()
					).id,
				})
				.execute();
		}

		return building;
	});
};
