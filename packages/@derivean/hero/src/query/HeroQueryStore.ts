"use client";

import {createQueryStore} from "@use-pico/query";
import {HeroSchema}       from "../schema/HeroSchema";

export const HeroQueryStore = createQueryStore({
    name:   "HeroQueryStore",
    schema: HeroSchema.query,
});
