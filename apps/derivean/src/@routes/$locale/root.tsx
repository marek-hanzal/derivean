/** @format */

import { GameIcon } from "@derivean/ui";
import { SessionSchema } from "@derivean/utils";
import { createFileRoute, redirect, useLoaderData, useParams } from "@tanstack/react-router";
import { AppLayout, LinkTo, LogoutIcon, ls, withList } from "@use-pico/client";
import { z } from "zod";
import { Logo } from "~/app/logo/Logo";
import { RootMenu } from "~/app/root/RootMenu";

export const Route = createFileRoute("/$locale/root")({
	async beforeLoad({ context, params: { locale } }) {
		return {
			...context,
			async session() {
				try {
					return SessionSchema.parse(ls.get("session"));
				} catch (_) {
					throw redirect({ to: `/$locale/public/login`, params: { locale } });
				}
			},
		};
	},
	async loader({ context: { queryClient, kysely, session } }) {
		return {
			session: await session(),
			image: await queryClient.ensureQueryData({
				queryKey: ["GameMap", "image"],
				async queryFn() {
					return kysely.transaction().execute(async (tx) => {
						return withList({
							select: tx
								.selectFrom("Blueprint as b")
								.select(["b.id", "b.image"])
								.where("b.image", "is not", null)
								.union(
									tx
										.selectFrom("Resource as r")
										.select(["r.id", "r.image"])
										.where("r.image", "is not", null),
								),
							output: z.object({ id: z.string().min(1), image: z.string() }),
						});
					});
				},
			}),
		};
	},

	component: () => {
		const { locale } = useParams({ from: "/$locale" });
		const { session, image } = useLoaderData({ from: "/$locale/root" });

		return (
			<>
				<style>
					{image
						.filter(({ image }) => image.length > 0)
						.map((bg) => {
							return `
	                    .bg-${bg.id} {
	                        background-image: url(${bg.image});
	                        background-size: cover;
	                        background-repeat: no-repeat;
	                        background-position: center;
	                    }
	                `;
						})}
				</style>
				<AppLayout
					logo={
						<LinkTo
							to={"/$locale/root"}
							params={{ locale }}
						>
							<Logo />
						</LinkTo>
					}
					menu={<RootMenu />}
					actions={
						<>
							{session.name}
							<LinkTo
								icon={GameIcon}
								to={"/$locale/game"}
								params={{ locale }}
							/>
							<LinkTo
								icon={LogoutIcon}
								to={"/$locale/public/logout"}
								params={{ locale }}
								preload={false}
							/>
						</>
					}
				/>
			</>
		);
	},
});
