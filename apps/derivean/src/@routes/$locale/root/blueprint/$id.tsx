/** @format */

import { createFileRoute, Outlet, useRouteContext } from "@tanstack/react-router";
import { withFetch } from "@use-pico/client";
import { Kysely, withJsonOutputArraySchema } from "@use-pico/common";
import { z } from "zod";
import { BlueprintIndexMenu } from "~/app/root/BlueprintIndexMenu";
import { BlueprintPreview } from "~/app/root/BlueprintPreview";
import { BlueprintDependencySchema } from "~/app/schema/BlueprintDependencySchema";
import { BlueprintRequirementSchema } from "~/app/schema/BlueprintRequirementSchema";
import { withBlueprintGraph } from "~/app/utils/withBlueprintGraph";

export const Route = createFileRoute("/$locale/root/blueprint/$id")({
	async loader({ context: { queryClient, kysely }, params: { id } }) {
		const entity = await queryClient.ensureQueryData({
			queryKey: ["Blueprint", "fetch", id],
			async queryFn() {
				return kysely.transaction().execute(async (tx) => {
					return withFetch({
						select: tx
							.selectFrom("Blueprint as bl")
							.select([
								"bl.id",
								"bl.name",
								"bl.sort",
								"bl.cycles",
								(eb) =>
									eb
										.selectFrom("Blueprint_Requirement as br")
										.innerJoin("Resource as r", "r.id", "br.resourceId")
										.select((eb) => {
											return Kysely.jsonGroupArray({
												id: eb.ref("br.id"),
												amount: eb.ref("br.amount"),
												passive: eb.ref("br.passive"),
												resourceId: eb.ref("br.resourceId"),
												blueprintId: eb.ref("br.blueprintId"),
												name: eb.ref("r.name"),
											}).as("requirements");
										})
										.whereRef("br.blueprintId", "=", "bl.id")
										.as("requirements"),
								(eb) =>
									eb
										.selectFrom("Blueprint_Dependency as bd")
										.innerJoin("Blueprint as bl2", "bl2.id", "bd.dependencyId")
										.select((eb) => {
											return Kysely.jsonGroupArray({
												id: eb.ref("bd.id"),
												dependencyId: eb.ref("bd.dependencyId"),
												blueprintId: eb.ref("bd.blueprintId"),
												name: eb.ref("bl2.name"),
											}).as("requirements");
										})
										.whereRef("bd.blueprintId", "=", "bl.id")
										.orderBy("bl2.name", "asc")
										.as("dependencies"),
							])
							.where("bl.id", "=", id)
							.orderBy("bl.name", "asc"),
						query({ select, where }) {
							let $select = select;

							if (where?.fulltext) {
								const fulltext = `%${where.fulltext}%`.toLowerCase();
								$select = $select.where((eb) => {
									return eb.or([
										eb("bl.id", "like", fulltext),
										eb("bl.name", "like", fulltext),
										eb(
											"bl.id",
											"in",
											eb
												.selectFrom("Blueprint_Requirement as br")
												.innerJoin("Resource as r", "r.id", "br.resourceId")
												.select("br.blueprintId")
												.where((eb) => {
													return eb.or([eb("r.name", "like", fulltext)]);
												}),
										),
										eb(
											"bl.id",
											"in",
											eb
												.selectFrom("Blueprint_Dependency as bd")
												.innerJoin(
													"Blueprint as b",
													"b.id",
													"bd.dependencyId",
												)
												.select("bd.blueprintId")
												.where((eb) => {
													return eb.or([eb("b.name", "like", fulltext)]);
												}),
										),
									]);
								});
							}

							return $select;
						},
						output: z.object({
							id: z.string().min(1),
							name: z.string().min(1),
							cycles: z.number().nonnegative(),
							sort: z.number().nonnegative(),
							requirements: withJsonOutputArraySchema(
								BlueprintRequirementSchema.entity.merge(
									z.object({ name: z.string().min(1) }),
								),
							),
							dependencies: withJsonOutputArraySchema(
								BlueprintDependencySchema.entity.merge(
									z.object({ name: z.string().min(1) }),
								),
							),
						}),
					});
				});
			},
		});

		return {
			entity,
			graph: await kysely.transaction().execute(async (tx) => {
				return withBlueprintGraph({ tx });
			}),
		};
	},
	component() {
		const { tva } = useRouteContext({ from: "__root__" });
		const { entity } = Route.useLoaderData();
		const tv = tva().slots;

		return (
			<div className={tv.base()}>
				<BlueprintPreview entity={entity} />

				<BlueprintIndexMenu entity={entity} />

				<Outlet />
			</div>
		);
	},
});
