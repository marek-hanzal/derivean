/** @format */

import { SessionSchema } from "@derivean/utils";
import { createFileRoute, Outlet, redirect } from "@tanstack/react-router";
import { ls, Toaster } from "@use-pico/client";

export const Route = createFileRoute("/$locale/map")({
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
	component() {
		return (
			<>
				<Toaster position={"top-right"} />
				<Outlet />
			</>
		);
	},
});
