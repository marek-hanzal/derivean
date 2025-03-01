/** @format */

import { withBlueprintRequirementSchema } from "@derivean/sdk";
import { FilterSchema, withFloatSchema } from "@use-pico/common";
import { z } from "zod";

export const BlueprintRequirementSchema = withBlueprintRequirementSchema({
	shape: z.object({ resourceId: z.string().min(1), amount: withFloatSchema(), passive: z.boolean() }),
	filter: FilterSchema.merge(z.object({ blueprintId: z.string().optional(), resourceId: z.string().optional() })),
});

export type BlueprintRequirementSchema = typeof BlueprintRequirementSchema;
