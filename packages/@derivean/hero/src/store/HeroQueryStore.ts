"use client";

import {createQueryStore} from "@use-pico/client";
import {HeroSchema}       from "../schema/HeroSchema";

export const HeroQueryStore = createQueryStore({
    name:   "HeroQueryStore",
    schema: HeroSchema.query,
});
export const {
    Provider: HeroQueryProvider,
} = HeroQueryStore;
