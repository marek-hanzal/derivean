"use client";

import {createQueryStore}          from "@use-pico/query";
import {BuildingRequirementSchema} from "../schema/BuildingRequirementSchema";

export const BuildingRequirementQueryStore = createQueryStore({
    name: "BuildingRequirementQueryStore",
    schema: BuildingRequirementSchema.query,
});
