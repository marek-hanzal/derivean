/** @format */

import { BuildingTable } from "@derivean/root-ui";
import { BuildingSchema } from "@derivean/utils";
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

export const Route = createFileRoute("/$locale/root/user/$id/building/list")({
	validateSearch: zodValidator(withSourceSearchSchema(BuildingSchema)),
	loaderDeps({ search: { filter, cursor, sort } }) {
		return { filter, cursor, sort };
	},
	async loader({ context: { queryClient, kysely }, deps: { filter, cursor }, params: { id } }) {
		return queryClient.ensureQueryData({
			queryKey: ["Building", "list-count", id, { filter, cursor }],
			async queryFn() {
				return kysely.transaction().execute((tx) => {
					return withListCount({
						select: tx
							.selectFrom("Building as b")
							.innerJoin("Blueprint as bl", "bl.id", "b.blueprintId")
							.select(["b.id", "bl.name", "b.blueprintId"])
							.where("b.userId", "=", id),
						output: z.object({
							id: z.string().min(1),
							name: z.string().min(1),
							blueprintId: z.string().min(1),
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
		const navigate = Route.useNavigate();
		const { tva } = useRouteContext({ from: "__root__" });
		const tv = tva().slots;

		return (
			<div className={tv.base()}>
				<BuildingTable
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
						textTotal: <Tx label={"Number of buildings (label)"} />,
						...navigateOnCursor(navigate),
					}}
					context={{ linkView: LinkTo }}
					blueprintTableContext={{ linkView: LinkTo, linkEditor: LinkTo }}
				/>
			</div>
		);
	},
});
