/** @format */

import { withBlueprintDependencySchema } from "@derivean/sdk";
import { FilterSchema } from "@use-pico/common";
import { z } from "zod";

export const BlueprintDependencySchema = withBlueprintDependencySchema({
	shape: z.object({ dependencyId: z.string().min(1) }),
	filter: FilterSchema.merge(
		z.object({ blueprintId: z.string().optional(), dependencyId: z.string().optional() }),
	),
});

export type BlueprintDependencySchema = typeof BlueprintDependencySchema;
