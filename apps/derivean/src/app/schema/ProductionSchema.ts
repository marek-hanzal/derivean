/** @format */

import { withProductionSchema } from "@derivean/sdk";
import { FilterSchema } from "@use-pico/common";
import { z } from "zod";

export const ProductionSchema = withProductionSchema({
	shape: z.object({
		//
	}),
	filter: FilterSchema.merge(
		z.object({
			//
		}),
	),
});

export type ProductionSchema = typeof ProductionSchema;
