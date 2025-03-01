/** @format */

import { withBlueprintProductionDependencySchema } from "@derivean/sdk";
import { FilterSchema } from "@use-pico/common";
import { z } from "zod";

export const BlueprintProductionDependencySchema = withBlueprintProductionDependencySchema({
	shape: z.object({ blueprintId: z.string().min(1) }),
	filter: FilterSchema,
});

export type BlueprintProductionDependencySchema = typeof BlueprintProductionDependencySchema;
