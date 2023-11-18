"use client";

import {createQueryStore}                      from "@use-pico/client";
import {BuildingConstructionRequirementSchema} from "../schema/BuildingConstructionRequirementSchema";

export const BuildingConstructionRequirementQueryStore = createQueryStore({
    name: "BuildingConstructionRequirementQueryStore",
    schema: BuildingConstructionRequirementSchema.query,
});
