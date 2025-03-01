/** @format */

import { withBlueprintConflictSchema } from "@derivean/sdk";
import { FilterSchema } from "@use-pico/common";
import { z } from "zod";

export const BlueprintConflictSchema = withBlueprintConflictSchema({
	shape: z.object({ conflictId: z.string().min(1) }),
	filter: FilterSchema,
});

export type BlueprintConflictSchema = typeof BlueprintConflictSchema;
