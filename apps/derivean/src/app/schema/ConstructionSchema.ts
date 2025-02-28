import { FilterSchema, withBoolSchema } from "@use-pico/common";
import { z } from "zod";
import { withConstructionSchema } from "~/app/db/sdk";

export const ConstructionSchema = withConstructionSchema({
	shape: z.object({
		plan: withBoolSchema(),
		valid: withBoolSchema(),
	}),
	filter: FilterSchema.merge(
		z.object({
			//
		}),
	),
});

export type ConstructionSchema = typeof ConstructionSchema;
