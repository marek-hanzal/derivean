/** @format */

import { withMapSchema } from "@derivean/sdk";
import { FilterSchema } from "@use-pico/common";
import { z } from "zod";

export const MapSchema = withMapSchema({ shape: z.object({ name: z.string().min(1) }), filter: FilterSchema });

export type MapSchema = typeof MapSchema;
