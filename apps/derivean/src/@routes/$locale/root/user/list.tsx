/** @format */

import { UserTable } from "@derivean/root-ui";
import { UserSchema } from "@derivean/utils";
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

export const Route = createFileRoute("/$locale/root/user/list")({
	validateSearch: zodValidator(withSourceSearchSchema(UserSchema)),
	loaderDeps({ search: { filter, cursor } }) {
		return { filter, cursor };
	},
	async loader({ context: { queryClient, kysely }, deps: { filter, cursor } }) {
		return queryClient.ensureQueryData({
			queryKey: ["User", "list-count", { filter, cursor }],
			async queryFn() {
				return kysely.transaction().execute((tx) => {
					return withListCount({
						select: tx.selectFrom("User as u").select(["u.id", "u.name", "u.login"]),
						output: z.object({
							id: z.string().min(1),
							name: z.string().min(1),
							login: z.string().min(1),
						}),
						filter,
						cursor,
					});
				});
			},
		});
	},
	component: () => {
		const { data, count } = Route.useLoaderData();
		const { filter, cursor, selection } = Route.useSearch();
		const navigate = Route.useNavigate();
		const { tva } = useRouteContext({ from: "__root__" });
		const tv = tva().slots;

		return (
			<div className={tv.base()}>
				<UserTable
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
						textTotal: <Tx label={"Number of users (label)"} />,
						...navigateOnCursor(navigate),
					}}
					context={{ linkView: LinkTo }}
				/>
			</div>
		);
	},
});
