/** @format */

import { withCycleSchema } from "@derivean/sdk";
import { FilterSchema } from "@use-pico/common";
import { z } from "zod";

export const CycleSchema = withCycleSchema({
	shape: z.object({
		//
	}),
	filter: FilterSchema.merge(z.object({ userId: z.string().optional() })),
});

export type CycleSchema = typeof CycleSchema;
