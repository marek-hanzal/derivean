"use client";

import {createQueryStore} from "@use-pico/query";
import {BuildingSchema}   from "../schema/BuildingSchema";

export const BuildingQueryStore = createQueryStore({
    schema: BuildingSchema.query,
});
