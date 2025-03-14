/** @format */

import { BlueprintConflictTable } from "@derivean/root-ui";
import { BlueprintDependencySchema } from "@derivean/utils";
import { createFileRoute, useRouteContext } from "@tanstack/react-router";
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
import { z } from "zod";

export const Route = createFileRoute("/$locale/root/blueprint/$id/conflicts")({
	validateSearch: zodValidator(withSourceSearchSchema(BlueprintDependencySchema)),
	loaderDeps({ search: { filter, cursor, sort } }) {
		return { filter, cursor, sort };
	},
	async loader({ context: { queryClient, kysely }, deps: { filter, cursor }, params: { id } }) {
		return queryClient.ensureQueryData({
			queryKey: ["Blueprint_Conflict", "list-count", id, { filter, cursor }],
			async queryFn() {
				return kysely.transaction().execute(async (tx) => {
					return withListCount({
						select: tx
							.selectFrom("Blueprint_Conflict as bc")
							.innerJoin("Blueprint as bl", "bl.id", "bc.conflictId")
							.select(["bc.id", "bl.name", "bc.blueprintId", "bc.conflictId"])
							.where("bc.blueprintId", "=", id)
							.orderBy("bl.name", "asc"),
						query({ select, where }) {
							let $select = select;

							if (where?.id) {
								$select = $select.where("bc.id", "=", where.id);
							}
							if (where?.idIn) {
								$select = $select.where("bc.id", "in", where.idIn);
							}

							return $select;
						},
						output: z.object({
							id: z.string().min(1),
							name: z.string().min(1),
							blueprintId: z.string().min(1),
							conflictId: z.string().min(1),
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
		const { tva } = useRouteContext({ from: "__root__" });
		const tv = tva().slots;

		return (
			<div className={tv.base()}>
				<BlueprintConflictTable
					blueprintTableContext={{ linkEditor: LinkTo, linkView: LinkTo }}
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
						textTotal: <Tx label={"Number of conflicts (label)"} />,
						...navigateOnCursor(navigate),
					}}
					context={{ linkConflict: LinkTo }}
				/>
			</div>
		);
	},
});
