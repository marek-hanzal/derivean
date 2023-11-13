"use client";

import {createQueryStore} from "@use-pico/query";
import {EventSchema}      from "../schema/EventSchema";

export const EventQueryStore = createQueryStore({
    schema: EventSchema.query,
});
