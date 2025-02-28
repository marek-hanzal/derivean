/** @format */

import { createFileRoute, redirect } from "@tanstack/react-router";
import { ls } from "@use-pico/client";
import { SessionSchema } from "~/app/schema/SessionSchema";

export const Route = createFileRoute("/$locale/")({
	async beforeLoad({ params: { locale } }) {
		try {
			SessionSchema.parse(ls.get("session"));
			throw redirect({ to: "/$locale/game", params: { locale } });
		} catch (_) {
			throw redirect({ to: `/$locale/public/login`, params: { locale } });
		}
	},
});
