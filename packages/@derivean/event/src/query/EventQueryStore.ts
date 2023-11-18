"use client";

import {createQueryStore} from "@use-pico/client";
import {EventSchema}      from "../schema/EventSchema";

export const EventQueryStore = createQueryStore({
    name: "EventQueryStore",
    schema: EventSchema.query,
});
