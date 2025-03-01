/** @format */

import { withConstructionSchema } from "@derivean/sdk";
import { FilterSchema, withBoolSchema } from "@use-pico/common";
import { z } from "zod";

export const ConstructionSchema = withConstructionSchema({
	shape: z.object({ plan: withBoolSchema(), valid: withBoolSchema() }),
	filter: FilterSchema.merge(
		z.object({
			//
		}),
	),
});

export type ConstructionSchema = typeof ConstructionSchema;
