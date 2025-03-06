/** @format */

import type { GameDatabase } from "@derivean/db";
import type { SessionSchema } from "@derivean/utils";
import type { QueryClient } from "@tanstack/react-query";
import { createRootRouteWithContext } from "@tanstack/react-router";
import type { PageCss } from "@use-pico/client";

export const Route = createRootRouteWithContext<{
	queryClient: QueryClient;
	tva: PageCss;
	session(): Promise<SessionSchema.Type>;
	kysely: GameDatabase;
}>()({
	//
});
