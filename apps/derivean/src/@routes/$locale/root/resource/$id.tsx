/** @format */

import { ResourcePreview } from "@derivean/root-ui";
import { createFileRoute, Outlet } from "@tanstack/react-router";
import { LinkTo, withFetch } from "@use-pico/client";
import { z } from "zod";
import { ResourceIndexMenu } from "~/app/root/ResourceIndexMenu";
import { useRootTva } from "~/app/utils/useRootTva";

export const Route = createFileRoute("/$locale/root/resource/$id")({
	async loader({ context: { queryClient, kysely }, params: { id } }) {
		return queryClient.ensureQueryData({
			queryKey: ["Resource", id],
			async queryFn() {
				return kysely.transaction().execute(async (tx) => {
					return {
						entity: await withFetch({
							select: tx
								.selectFrom("Resource as r")
								.select(["r.id", "r.name", "r.image"])
								.where("r.id", "=", id),
							output: z.object({
								id: z.string().min(1),
								name: z.string().min(1),
								image: z.string().nullish(),
							}),
						}),
					};
				});
			},
		});
	},
	component() {
		const tva = useRootTva();
		const { entity } = Route.useLoaderData();

		const tv = tva().slots;

		return (
			<div className={tv.base()}>
				<ResourcePreview
					entity={entity}
					linkList={LinkTo}
				/>

				<ResourceIndexMenu entity={entity} />

				<Outlet />
			</div>
		);
	},
});
