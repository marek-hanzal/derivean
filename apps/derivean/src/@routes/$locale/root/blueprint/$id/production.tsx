/** @format */

import { BlueprintProductionTable } from "@derivean/root-ui";
import {
	BlueprintProductionDependencySchema,
	BlueprintProductionRequirementSchema,
	BlueprintProductionResourceSchema,
	BlueprintProductionSchema,
} from "@derivean/utils";
import { createFileRoute } from "@tanstack/react-router";
import { zodValidator } from "@tanstack/zod-adapter";
import {
	LinkTo,
	navigateOnCursor,
	navigateOnFilter,
	navigateOnFulltext,
	navigateOnSelection,
	Tx,
	withListCount,
	withSourceSearchSchema,
} from "@use-pico/client";
import { withJsonOutputArraySchema } from "@use-pico/common";
import { sql } from "kysely";
import { z } from "zod";
import { useRootTva } from "~/app/utils/useRootTva";

export const Route = createFileRoute("/$locale/root/blueprint/$id/production")({
	validateSearch: zodValidator(withSourceSearchSchema(BlueprintProductionSchema)),
	loaderDeps({ search: { filter, cursor, sort } }) {
		return { filter, cursor, sort };
	},
	async loader({ context: { queryClient, kysely }, deps: { filter, cursor }, params: { id } }) {
		return queryClient.ensureQueryData({
			queryKey: ["Blueprint_Production", "list-count", id, { filter, cursor }],
			async queryFn() {
				return kysely.transaction().execute(async (tx) => {
					return withListCount({
						select: tx
							.selectFrom("Blueprint_Production as bp")
							.innerJoin("Blueprint as bl", "bl.id", "bp.blueprintId")
							.innerJoin("Resource as r", "r.id", "bp.resourceId")
							.select([
								"bp.id",
								"r.name",
								"bp.amount",
								"bp.blueprintId",
								"bp.cycles",
								"bp.resourceId",
								(eb) =>
									eb
										.selectFrom("Blueprint_Production_Requirement as bpr")
										.innerJoin("Resource as r", "r.id", "bpr.resourceId")
										.select((eb) => {
											return sql<string>`json_group_array(json_object(
                                                'id', ${eb.ref("bpr.id")},
                                                'amount', ${eb.ref("bpr.amount")},
                                                'passive', ${eb.ref("bpr.passive")},
                                                'blueprintProductionId', ${eb.ref("bpr.blueprintProductionId")},
                                                'resourceId', ${eb.ref("bpr.resourceId")},
                                                'name', ${eb.ref("r.name")}
                                            ))`.as("requirements");
										})
										.whereRef("bpr.blueprintProductionId", "=", "bp.id")
										.as("requirements"),
								(eb) =>
									eb
										.selectFrom("Blueprint_Production_Resource as bpr")
										.innerJoin("Resource as r", "r.id", "bpr.resourceId")
										.select((eb) => {
											return sql<string>`json_group_array(json_object(
                                                'id', ${eb.ref("bpr.id")},
                                                'amount', ${eb.ref("bpr.amount")},
                                                'blueprintProductionId', ${eb.ref("bpr.blueprintProductionId")},
                                                'resourceId', ${eb.ref("bpr.resourceId")},
                                                'name', ${eb.ref("r.name")}
                                            ))`.as("requirements");
										})
										.whereRef("bpr.blueprintProductionId", "=", "bp.id")
										.as("resources"),
								(eb) =>
									eb
										.selectFrom("Blueprint_Production_Dependency as bpd")
										.innerJoin("Blueprint as bl2", "bl2.id", "bpd.blueprintId")
										.select((eb) => {
											return sql<string>`json_group_array(json_object(
                                                'id', ${eb.ref("bpd.id")},
                                                'blueprintProductionId', ${eb.ref("bpd.blueprintProductionId")},
                                                'blueprintId', ${eb.ref("bpd.blueprintId")},
                                                'name', ${eb.ref("bl2.name")}
                                            ))`.as("requirements");
										})
										.whereRef("bpd.blueprintProductionId", "=", "bp.id")
										.as("dependencies"),
							])
							.where("bp.blueprintId", "=", id)
							.orderBy("r.name", "asc"),
						output: z.object({
							id: z.string().min(1),
							name: z.string().min(1),
							blueprintId: z.string().min(1),
							resourceId: z.string().min(1),
							amount: z.number().nonnegative(),
							cycles: z.number().nonnegative(),
							requirements: withJsonOutputArraySchema(
								BlueprintProductionRequirementSchema.entity.merge(
									z.object({ name: z.string().min(1) }),
								),
							),
							resources: withJsonOutputArraySchema(
								BlueprintProductionResourceSchema.entity.merge(
									z.object({ name: z.string().min(1) }),
								),
							),
							dependencies: withJsonOutputArraySchema(
								BlueprintProductionDependencySchema.entity.merge(
									z.object({ name: z.string().min(1) }),
								),
							),
						}),
						filter,
						cursor,
					});
				});
			},
		});
	},
	component() {
		const { data, count } = Route.useLoaderData();
		const { filter, cursor, selection } = Route.useSearch();
		const { id } = Route.useParams();
		const navigate = Route.useNavigate();
		const tva = useRootTva();
		const tv = tva().slots;

		return (
			<div className={tv.base()}>
				<BlueprintProductionTable
					blueprintId={id}
					data={data}
					filter={{ state: { value: filter, set: navigateOnFilter(navigate) } }}
					selection={{
						type: "multi",
						state: { value: selection, set: navigateOnSelection(navigate) },
					}}
					fulltext={{
						value: filter?.fulltext,
						set: navigateOnFulltext(filter?.fulltext, navigate),
					}}
					cursor={{
						count,
						cursor,
						textTotal: <Tx label={"Number of productions (label)"} />,
						...navigateOnCursor(navigate),
					}}
					context={{ linkRequirements: LinkTo }}
					blueprintTableContext={{ linkEditor: LinkTo, linkView: LinkTo }}
					resourceTableContext={{ linkView: LinkTo }}
				/>
			</div>
		);
	},
});
