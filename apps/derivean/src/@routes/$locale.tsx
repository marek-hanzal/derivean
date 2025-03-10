/** @format */

import { bootstrap } from "@derivean/db";
import { createFileRoute } from "@tanstack/react-router";
import { translator } from "@use-pico/common";

export const Route = createFileRoute("/$locale")({
	loader: async ({ params: { locale } }) => {
		await bootstrap();
		try {
			translator.push((await import(`../translation/${locale}.yaml`)).default);
		} catch (e) {
			console.error(e);
		}
	},
});
