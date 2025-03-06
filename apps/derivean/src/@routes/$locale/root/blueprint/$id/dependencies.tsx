/** @format */

import { BlueprintDependencyTable } from "@derivean/root-ui";
import { BlueprintDependencySchema } from "@derivean/utils";
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
import { z } from "zod";
import { useRootTva } from "~/app/utils/useRootTva";

export const Route = createFileRoute("/$locale/root/blueprint/$id/dependencies")({
	validateSearch: zodValidator(withSourceSearchSchema(BlueprintDependencySchema)),
	loaderDeps({ search: { filter, cursor, sort } }) {
		return { filter, cursor, sort };
	},
	async loader({ context: { queryClient, kysely }, deps: { filter, cursor }, params: { id } }) {
		return queryClient.ensureQueryData({
			queryKey: ["Blueprint_Dependency", "list-count", id, { filter, cursor }],
			async queryFn() {
				return kysely.transaction().execute(async (tx) => {
					return withListCount({
						select: tx
							.selectFrom("Blueprint_Dependency as bd")
							.innerJoin("Blueprint as bl", "bl.id", "bd.dependencyId")
							.select(["bd.id", "bl.name", "bd.blueprintId", "bd.dependencyId"])
							.where("bd.blueprintId", "=", id)
							.orderBy("bl.name", "asc"),
						query({ select, where }) {
							let $select = select;

							if (where?.id) {
								$select = $select.where("bd.id", "=", where.id);
							}
							if (where?.idIn) {
								$select = $select.where("bd.id", "in", where.idIn);
							}

							if (where?.blueprintId) {
								$select = $select.where("bd.blueprintId", "=", where.blueprintId);
							}

							if (where?.dependencyId) {
								$select = $select.where("bd.dependencyId", "=", where.dependencyId);
							}

							return $select;
						},
						output: z.object({
							id: z.string().min(1),
							name: z.string().min(1),
							blueprintId: z.string().min(1),
							dependencyId: z.string().min(1),
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
				<BlueprintDependencyTable
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
						textTotal: <Tx label={"Number of requirements (label)"} />,
						...navigateOnCursor(navigate),
					}}
					blueprintTableContext={{ linkEditor: LinkTo, linkView: LinkTo }}
				/>
			</div>
		);
	},
});
