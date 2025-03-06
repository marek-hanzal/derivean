/** @format */

import { withBlueprintProductionSchema } from "@derivean/sdk";
import { FilterSchema, withFloatSchema, withIntSchema } from "@use-pico/common";
import { z } from "zod";

export const BlueprintProductionSchema = withBlueprintProductionSchema({
	shape: z.object({ resourceId: z.string().min(1), amount: withFloatSchema(), cycles: withIntSchema() }),
	filter: FilterSchema.merge(z.object({ blueprintId: z.string().optional(), resourceId: z.string().optional() })),
});

export type BlueprintProductionSchema = typeof BlueprintProductionSchema;
