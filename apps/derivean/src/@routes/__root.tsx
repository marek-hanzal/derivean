import type { QueryClient } from "@tanstack/react-query";
import { createRootRouteWithContext } from "@tanstack/react-router";
import type { PageCss } from "@use-pico/client";
import type { Kysely } from "kysely";
import type { Database } from "~/app/db/sdk";
import type { SessionSchema } from "~/app/schema/SessionSchema";

export const Route = createRootRouteWithContext<{
	queryClient: QueryClient;
	tva: PageCss;
	session(): Promise<SessionSchema.Type>;
	kysely: Kysely<Database>;
}>()({
	//
});
