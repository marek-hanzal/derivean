"use client";

import {createQueryStore} from "@use-pico/client";
import {BuildingSchema}   from "../schema/BuildingSchema";

export const BuildingQueryStore = createQueryStore({
    name: "BuildingQueryStore",
    schema: BuildingSchema.query,
});
