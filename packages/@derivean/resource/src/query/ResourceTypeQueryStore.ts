"use client";

import {createQueryStore}   from "@use-pico/query";
import {ResourceTypeSchema} from "../schema/ResourceTypeSchema";

export const ResourceTypeQueryStore = createQueryStore({
    schema: ResourceTypeSchema.query,
});
