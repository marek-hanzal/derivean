"use client";

import {createQueryStore} from "@use-pico/query";
import {EventHeroSchema}  from "../schema/EventHeroSchema";

export const EventHeroQueryStore = createQueryStore({
    name:   "EventHeroQueryStore",
    schema: EventHeroSchema.query,
});
