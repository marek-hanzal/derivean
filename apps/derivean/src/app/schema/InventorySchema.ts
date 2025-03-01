/** @format */

import { withInventorySchema } from "@derivean/sdk";
import { FilterSchema, withFloatSchema } from "@use-pico/common";
import { z } from "zod";

export const InventorySchema = withInventorySchema({
	shape: z.object({
		resourceId: z.string().min(1),
		amount: withFloatSchema(),
		limit: withFloatSchema(),
		type: z.enum(["storage", "construction"]),
	}),
	filter: FilterSchema.merge(
		z.object({ resourceId: z.string().optional(), type: z.enum(["storage", "construction"]).optional() }),
	),
});

export type InventorySchema = typeof InventorySchema;
