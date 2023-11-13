"use client";

import {createQueryStore} from "@use-pico/query";
import {KingdomSchema}    from "../schema/KingdomSchema";

export const KingdomQueryStore = createQueryStore({
    schema: KingdomSchema.query,
});
