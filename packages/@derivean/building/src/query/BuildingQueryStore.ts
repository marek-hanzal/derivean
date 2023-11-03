"use client";

import {createQueryStore}    from "@use-pico/query";
import {BuildingQuerySchema} from "../schema/BuildingQuerySchema";

export const BuildingQueryStore = createQueryStore({
    schema: BuildingQuerySchema,
});
