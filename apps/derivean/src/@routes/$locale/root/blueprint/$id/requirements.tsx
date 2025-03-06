/** @format */

import { BlueprintRequirementTable } from "@derivean/root-ui";
import { BlueprintRequirementSchema } from "@derivean/utils";
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
import { withBoolSchema } from "@use-pico/common";
import { z } from "zod";
import { useRootTva } from "~/app/utils/useRootTva";

export const Route = createFileRoute("/$locale/root/blueprint/$id/requirements")({
	validateSearch: zodValidator(withSourceSearchSchema(BlueprintRequirementSchema)),
	loaderDeps({ search: { filter, cursor, sort } }) {
		return { filter, cursor, sort };
	},
	async loader({ context: { queryClient, kysely }, deps: { filter, cursor }, params: { id } }) {
		return queryClient.ensureQueryData({
			queryKey: ["Blueprint_Requirement", "list-count", id, { filter, cursor }],
			async queryFn() {
				return kysely.transaction().execute(async (tx) => {
					return withListCount({
						select: tx
							.selectFrom("Blueprint_Requirement as br")
							.innerJoin("Resource as r", "r.id", "br.resourceId")
							.select(["br.id", "r.name", "br.resourceId", "br.amount", "br.passive"])
							.where("br.blueprintId", "=", id)
							.orderBy("r.name", "asc"),
						query({ select, where }) {
							let $select = select;

							if (where?.id) {
								$select = $select.where("br.id", "=", where.id);
							}
							if (where?.idIn) {
								$select = $select.where("br.id", "in", where.idIn);
							}

							if (where?.resourceId) {
								$select = $select.where("br.resourceId", "=", where.resourceId);
							}

							return $select;
						},
						output: z.object({
							id: z.string().min(1),
							name: z.string().min(1),
							resourceId: z.string().min(1),
							amount: z.number().nonnegative(),
							passive: withBoolSchema(),
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
				<BlueprintRequirementTable
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
					resourceTableContext={{ linkView: LinkTo }}
				/>
			</div>
		);
	},
});
